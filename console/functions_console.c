#include "functions_console.h"


char* get_timestamp() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char *buffer=malloc(256*(sizeof(char)));  // Enough space for "YYYY-MM-DD HH:MM:SS\0"
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

void print_command(const int fd,const char *source_dir,const char *target_dir, const char *command,const char *time){
    // print the command to console log
    char msg[256];
    if(strcmp(command,"add")== 0)
        snprintf(msg,sizeof(msg), "[%s] Command %s %s -> %s\n",time,command,source_dir,target_dir);

    else if(strcmp(command,"shutdown") == 0)
        snprintf(msg,sizeof(msg), "[%s] Command %s \n",time,command);

    else
        snprintf(msg,sizeof(msg), "[%s] Command %s %s\n",time,command,source_dir);

    if (write(fd, msg, strlen(msg)) < 0) {
        perror("Error writing to log file");
    }
}

void print_manager_response(const int fd, const char *msg){
    
    if (msg == NULL || msg[0] == '\0') {
        return; 
    }

    char temp[1024];
    snprintf(temp, sizeof(temp), "%s\n", msg);
    if (write(fd, temp, strlen(temp)) < 0) {
        perror("Error writing to log file");
    }
    printf("%s\n",msg);

}

void read_from_manager(char *response, const int fd_out, const int console_file, bool *flag, bool *last_flag){
    ssize_t n;
    while ((n = read(fd_out, response, 1024 - 1)) > 0) {
        response[n] = '\0';
        char *line = strtok(response, "\n");
        while (line != NULL) {
            if (strstr(line, "Manager shutdown complete") != NULL) {
                print_manager_response(console_file, line);
                
                // it shutdowns now
                *last_flag = true;
                return;
            }
            if (strstr(line, "Processing remaining queued tasks") != NULL) {
                print_manager_response(console_file, line);

                // there are still active worker so prepare to shut down when they finish
                *flag = true;
            }
            else
                print_manager_response(console_file, line);
            line = strtok(NULL, "\n");
        }
    }
}