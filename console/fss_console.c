#include "../lib/ADTQueue.h" 
#include "functions_console.h"
#define BUFSIZE 256

// they are global vairables so they can be used in the signal handler so we don't have important leaks all the other leaks will be just messages from the fifo 
int fd_in, fd_out, console_file;

// siugnal handler to not have leaks when terminated with ^C
void forced_close(int signum ){
    (void)signum;
    write(1,"\nThe console is forced to terminate!\n",strlen("\nThe console is forced to terminate!\n"));
    write(fd_in,"forced_shutdown",strlen("forced_shutdown"));
    close(fd_in);
    close(fd_out);
    close(console_file);
    // Exit normally
    exit(0);
}

int main(int argc, char **argv){

    // it terminates without leaks even when the ^C  is used 
    static struct sigaction act;
    act.sa_handler=forced_close;
    sigfillset(&(act.sa_mask));
    sigaction(SIGINT,&act,NULL);


    // check flags
    if(argc < 3){
        printf("Use the flags correctly!\n");
        return -1;
    }
    if(strcmp(argv[1],"-l") != 0){
        printf("Use -l correctly!\n");
        return -1;
    }


    // open the fss_out in which u will read from the manager
    if((fd_out = open("fss_out", O_RDONLY | O_NONBLOCK )) < 0){
        perror("fss_out open error from fss_console");
        exit(1);
    }

    // open the fss_in in which u will write to the manager
    if((fd_in = open("fss_in", O_WRONLY)) < 0){
        perror("fss_in open error from fss_console");
        exit(1);
    }

    // open the console log-file if it doesn't exist it will create it
    if((console_file = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC,0666)) < 0){
        perror("Error in opening the console-logfile");
        exit(1);
    }

    // cmd has the user's command, arg1 is the source dir and the arg2 is the target's dir if it exists
    char *cmd = malloc(BUFSIZE);
    if (cmd == NULL) {
        perror("Allocation failed for cmd");
        exit(1);
    }
    char *arg1 = malloc(BUFSIZE);
    if (arg1 == NULL) {
        perror("Allocation failed for arg1");
        exit(1);
    }
    char *arg2 = malloc(BUFSIZE);
    if (arg2 == NULL) {
        perror("Allocation failed for arg2");
        exit(1);
    }
    char *message = malloc(1024);
    if (message == NULL) {
        perror("Allocation failed for message");
        exit(1);
    }
    char *output = malloc(1024);
    if (output == NULL) {
        perror("Allocation failed for output");
        exit(1);
    }
    char *response = malloc(1024);
    if (response == NULL) {
        perror("Allocation failed for response");
        exit(1);
    }
    
    bool flag = false; // prepare to shutdown
    bool last_flag = false; //when this is us the next action is shutdown no wait etc
    while(!flag){

        read_from_manager(response,fd_out,console_file,&flag,&last_flag);
        // if flag is true we prepare for shutdown
        if(flag)
            break;

        printf("> ");
        // clear the buffer of message on each loop
        
        // read input from the user and make some checks in order to be sure that the given command is acceptable
        scanf("%s",cmd);

        // cmd add 
        if(strcmp(cmd,"add") == 0){
            scanf("%s",arg1);
            scanf("%s",arg2);
            // the message is the buffer that with the command which will  be passed to the manager
            snprintf(message, BUFSIZE, "%s %s %s", cmd, arg1, arg2);

            char *time = get_timestamp();
            //print in the console log file
            print_command(console_file,arg1,arg2,cmd,time);

            free(time);
        }       
        
        // cmd cancel
        else if(strcmp(cmd,"cancel") == 0){
            scanf("%s",arg1);
            // the message is the buffer that with the command which will  be passed to the manager
            snprintf(message, BUFSIZE, "%s %s", cmd, arg1);

            char *time = get_timestamp();
            //print in the console log file
            print_command(console_file,arg1,arg2,cmd,time);
                        
            free(time);
        }

        // cmd status
        else if(strcmp(cmd,"status") == 0){
            scanf("%s",arg1);
            // the message is the buffer that with the command which will  be passed to the manager
            snprintf(message, BUFSIZE, "%s %s", cmd, arg1);

            char *time = get_timestamp();
            //print in the console log file
            print_command(console_file,arg1,arg2,cmd,time);
                                    
            free(time);
        }

        // cmd sync
        else if (strcmp(cmd,"sync") == 0 ){
            scanf("%s",arg1);
            // the message is the buffer that with the command which will  be passed to the manager
            snprintf(message, BUFSIZE, "%s %s", cmd, arg1);

            char *time = get_timestamp();
            //print in the console log file
            print_command(console_file,arg1,arg2,cmd,time);
                                    
            free(time);
        }

        // cmd shutdown in order to close the console
        else if(strcmp(cmd,"shutdown") == 0){
            // the message is the buffer that with the command which will  be passed to the manager
            snprintf(message, BUFSIZE, "%s",cmd);
            
            char *time = get_timestamp();
            //print in the console log file
            print_command(console_file,arg1,arg2,cmd,time);
                                    
            free(time);   
        }
        
        // if the commands is not one of the previous then inform the user that the given command is not acceptable and close manager as well 
        // before closing the manager send him that sth forced the console to shutdown
        else{
            write(fd_in,"forced_shutdown",strlen("forced_shutdown") + 1);
            close(fd_in);
            close(fd_out);
            close(console_file);            
            fprintf(stderr,"Give correct commands\n"); 
            last_flag = 1;
            break;
        }

        // through the fss_in send the user's input in the manager
        if (write(fd_in, message, strlen(message)) < 0) {
            if (errno == EPIPE) {
                printf("Manager closed the pipe. Exiting console.\n");
                break;
            }
            perror("write to fss_in");
        }
        // do not waste resources
        usleep(50000); 
        memset(message, 0, 1024);
        memset(response, 0, 1024);
    }
    // wait for the manager to get the message that all the workers have finished
    while(!last_flag)
        read_from_manager(response,fd_out,console_file,&flag,&last_flag);
    
        
free(cmd);
free(arg1);
free(arg2);
free(message);
free(output);
free(response);
close(console_file);
close(fd_in);
close(fd_out);
}