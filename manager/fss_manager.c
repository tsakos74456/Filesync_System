#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include "../lib/ADTQueue.h" 
#include "functions_manager.h"
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "../lib/ADTList.h"
#define PERMS 0644
#define BUFSIZE 256
#include <sys/wait.h>
#include <signal.h>
#include <sys/inotify.h>

volatile sig_atomic_t child_finished = 0;

// erase the flag that a wokrer is finished so I handle it afterwards
void handler_sigchld(int signum) {
    (void)signum;
    child_finished = 1;
}


int main(int argc, char **argv){
    static struct sigaction act;
    act.sa_handler = handler_sigchld;
    sigfillset(&(act.sa_mask));
    sigaction(SIGCHLD, &act, NULL);


    // check if the named pipes with the name fss_in and fss_out already exist
    // if they exist delete them
    if(check_exist_fifo("fss_in"))
        unlink("fss_in");
    if(check_exist_fifo("fss_out"))
        unlink("fss_out");
    
    int worker_limit;
    
    // check flags and the num of worker limit 
    if (!check_flags_manager (argc, argv))
        return -1;

    // check if worker limit is given and if the number is valid 
    if(argc == 7 && strcmp(argv[5],"-n") == 0 && atoi(argv[6]) > 0){
        worker_limit = atoi(argv[6]);
    }

    // if the worker limit is not valid use the default value which is equal to 5
    else
        worker_limit = 5;

    // create and open the named pipes
    int fss_in,fss_out;
    if(mkfifo("fss_in",0666) == -1 ){
        if ( errno != EEXIST ) {
            perror ("receiver:mkfifo fss_in");
            exit(6);
        }
    }
    if ((fss_in = open("fss_in", O_RDONLY | O_NONBLOCK)) < 0) {
        perror ("fss_in open problem");
        exit(3);
    }

    if(mkfifo("fss_out",0666) == -1 ){
        if ( errno != EEXIST ) {
            perror ("receiver:mkfifo fss_out");
            exit(6);
        }  
    } 
    if ((fss_out = open ("fss_out", O_WRONLY)) < 0) {
        perror ("fss_out open problem");
        exit(3);
    }
    

    // open the configuration file from which manager reads the pairs 
    // open the manager log in which the manager will write the actions that he does
    int conf,man;
    char *manager_logfile = argv[2];
    char *config_file = argv[4];

    if((man = open(manager_logfile, O_WRONLY | O_CREAT | O_TRUNC, PERMS)) == -1){
        perror("Failed to open the manager file");
        return -1;
    }

    if((conf = open(config_file, O_RDONLY)) == -1){
        perror("Failed to open the config file");
        return -1;
    }
    
    char *buffer=malloc(BUFSIZE);
    if (buffer == NULL) {
        perror("Allocation failed for buffer");
        exit(1);
    }
    char ch;
    int pos = 0;

    // file descriptor for inotify without blocking on reading
    int inotify_fd = inotify_init1(IN_NONBLOCK);
    if (inotify_fd < 0) {
        perror("inotify_init");
        exit(1);
    }

    Sync_info *info;
    
    // queue in which we keep the sync info (pairs of dir) in order to be done afterwards when a worker is available
    Queue worker_queue = queue_create(NULL);    
    
    // here we keep all the pairs regardless they are active or not
    List pairs_list = list_create(destroy_sync_info);
    
    // list in which we keep track of the active workers
    List worker_list = list_create(free);

    // read the pairs source->target from the config file and add them in  the list 
    while(read(conf,&ch,1) >0){
        if(pos < BUFSIZE - 1)
            buffer[pos++]=ch;
        
        // read the source dir
        if(ch == ' '){
            buffer[pos - 1] = '\0';
            // initialize info and allocate memory
            info = malloc(sizeof(Sync_info));
            if(info == NULL)
                perror("Fail to allocate memory for info");
            info->synced = 0;
            info->status = NULL;     
            info->error_count = 0;  
            info->active = false;   
            info->last_sync_time = NULL; 
            info->now_syncing = true;
            info->initial = true;
            info->source_dir = strdup(buffer);
            info->operation_for_queue = strdup("FULL");
            info->filename_for_queue = strdup("ALL");
            if (info->source_dir == NULL) {
                perror("Failed to duplicate source_dir");
                free(info);  
                exit(1);
            }
            pos = 0;
        }

        // read the target dir
        else if(ch == '\n'){
            buffer[pos - 1] = '\0';
            info->target_dir = strdup(buffer);
            if (info->target_dir == NULL) {
                perror("Failed to duplicate target_dir");
                free(info);  
                exit(1);
            }
            pos = 0;
            // check if the dir is already synced if not add it to the list 
            if(!check_existance_pair(info->source_dir,pairs_list)){
                list_insert_next(pairs_list,list_first(pairs_list),info);
                queue_insert_back(worker_queue,info);
                
            }
            // otherwise print the according message
            else{
                char *time = get_timestamp();
                printf("[%s] Already in queue: %s\n", time, info->source_dir);
                free(info->operation_for_queue);
                free(info->filename_for_queue);
                free(info->source_dir);
                free(info->target_dir);
                free(info);
                free(time);
            }
        }
    }

    // when we finish reading the config_file and we have insert all the pairs in the list start to sync the pairs
    initial_full_sync(worker_queue, worker_limit, worker_list,man,inotify_fd);

    char *msg_console = malloc(BUFSIZE);
    if (msg_console == NULL) {
        perror("Allocation failed for msg_console\n");
        exit(1);
    }
    ssize_t bytes;
    bool flag = false; // flag for shutdown
    
    char *inotify_msg = malloc(4096); // max size of inotify event is 256 + 16 so many messages of inotify can be stroed in one read if it is needed
    if (inotify_msg == NULL)
        perror("Failed memory allocation for inotify_msg");
    const struct inotify_event *current_event;
    while(!flag){
        // handler is activated so a child is finished
        if(child_finished){
            child_finished  = 0;
            pid_t pid;
            ListNode prev = NULL;

            while((pid = waitpid(-1,NULL,WNOHANG)) > 0){
                for(ListNode node = list_first(worker_list) ; node != LIST_EOF ; node = list_next(worker_list,node)){
                    
                    Worker_info *work = list_node_value(worker_list,node);
                    if(work->worker_pid == pid){
        
                        char buffer[10001];
                        ssize_t bytes;
                        while((bytes= read(work->fd_pipe,buffer,10000)) > 0){
                            buffer[bytes] = '\0';
                        }
                        update_pair(work->info,buffer,fss_out,man,pid,work->operation);
                        free(work->operation);
                        close(work->fd_pipe);
                        node = list_next(worker_list, node); // iteration to the next node, otherwise the list will "break"
                        
                        list_remove_next(worker_list, prev);
                        break;
                    }
                    // keep track of the prev node so we can remove the node which we want when we need it 
                    prev = node;
                    
                }
            }
        }
        // reads input from console
        while ((bytes = read(fss_in, msg_console, BUFSIZE)) > 0) {
            msg_console[bytes] = '\0';

            if(strncmp(msg_console, "add",3) == 0){
                // we read the source dir and target dir from console i allocate 4096 bytes as in linux the longest path is 4096 and in windows is much smaller  
                int i = 4;
                char *source_dir = malloc(4096);
                
                if (source_dir == NULL) {
                    perror("Allocation failed for source_dir from console\n");
                    exit(1);
                }
                char *target_dir = malloc(4097);
                if (target_dir == NULL) {
                    perror("Allocation failed for target_dir from console\n");
                    exit(1);
                }
                int j = 0;
                while (msg_console[i] != ' ')
                    source_dir[j++] = msg_console[i++];
                source_dir[j] ='\0';
                i++;
                j = 0;

                while (msg_console[i] != '\0' && msg_console[i] != ' ')
                    target_dir[j++] = msg_console[i++];

                target_dir[j] ='\0';
                // check if the pair already exists
                if(!check_existance_pair(source_dir,pairs_list)){
                    add_dir(source_dir, target_dir, pairs_list, worker_queue, worker_list, worker_limit,fss_out,man,inotify_fd);
                    
                }
                else{
                    char *response = malloc(1024);
                    char *time = get_timestamp();
                    snprintf(response,1024,"[%s] Already in queue: %s\n", time, source_dir);
                    printf("%s",response);
                    write_to_console(response,fss_out);
                    free(time);
                    free(response);
                }
                free(source_dir);
                free(target_dir);
            }

            else if(strncmp(msg_console, "sync",4) == 0){
                // we read the source dir from console i allocate 4096 bytes as in linux the longest path is 4096 and in windows is much smaller  
                int i = 5;
                char *dir = malloc(4096);
                if (dir == NULL) {
                    perror("Allocation failed for source_dir from console\n");
                    exit(1);
                }

                int j = 0;
                while (msg_console[i] != '\0' && msg_console[i] != ' ' )
                    dir[j++] = msg_console[i++];
                dir[j] ='\0';
                console_sync(pairs_list,dir,fss_out,worker_limit,worker_list,man,inotify_fd,worker_queue);
                free(dir);
            }

            else if(strncmp(msg_console, "status",6) == 0){
                // we read the source dir from console i allocate 4096 bytes as in linux the longest path is 4096 and in windows is much smaller  
                int i = 7;
                char *dir = malloc(4096);
                
                if (dir == NULL) {
                    perror("Allocation failed for source_dir from console\n");
                    exit(1);
                }

                int j = 0;
                while (msg_console[i] != '\0' && msg_console[i] != ' ' )
                    dir[j++] = msg_console[i++];
                dir[j] ='\0';
                status(pairs_list,dir,fss_out);
                free(dir);
            }

            else if(strncmp(msg_console, "cancel",6) == 0){
                int i = 7;
                char *dir = malloc(4096);   
                if (dir == NULL) {
                    perror("Allocation failed for source_dir from console\n");
                    exit(1);
                }

                int j = 0;
                while (msg_console[i] != '\0' && msg_console[i] != ' ' )
                    dir[j++] = msg_console[i++];
                dir[j] ='\0';
                cancel_dir(pairs_list,dir,fss_out,man,inotify_fd);
                free(dir);
                
            }
            else if(strcmp(msg_console,"shutdown") == 0){
                // activate stop flag for shutdown
                flag = 1;
                // print the correct shutdown messages
                char *time = get_timestamp();
                char *response = malloc(256);
                if(response == NULL)
                    perror("Failed memory allocation for response!\n");
                snprintf(response,256,"[%s] Shutting down manager...\n",time);
                printf("%s",response);
                write_to_console(response,fss_out);
                free(time);
                time = get_timestamp();
                snprintf(response,256,"[%s] Waiting for all active workers to finish.\n",time);
                printf("%s",response);
                write_to_console(response,fss_out);
                free(time);
                time = get_timestamp();
                snprintf(response,256,"[%s] Processing remaining queued tasks.\n",time);
                printf("%s",response);
                write_to_console(response,fss_out);
                free(time);
                // wait for the worker to finish and sleep so as we wait we don't waste PC's resources
                while(list_size(worker_list) != 0){
                    if(child_finished){
                        child_finished  = 0;
                        pid_t pid;
                        ListNode prev = NULL;
                        while((pid = waitpid(-1,NULL,WNOHANG)) > 0){
                            for(ListNode node = list_first(worker_list) ; node != LIST_EOF ; node = list_next(worker_list,node)){
                                
                                Worker_info *work = list_node_value(worker_list,node);
                                if(work->worker_pid == pid){
                    
                                    char buffer[10001];
                                    ssize_t bytes;
                                    while((bytes= read(work->fd_pipe,buffer,10000)) > 0){
                                        buffer[bytes] = '\0';
                                    }
                                    update_pair(work->info,buffer,fss_out,man,pid,work->operation);
                                    free(work->operation);
                                    close(work->fd_pipe);
                                    node = list_next(worker_list, node); // iteration to the next node, otherwise the list will "break"
                                    
                                    list_remove_next(worker_list, prev);
                                    break;
                                }
                                prev = node;
                                
                            }
                        }
                    }
                    while(queue_size(worker_queue) != 0 && worker_limit > list_size(worker_list)){
                        Sync_info *info = queue_front(worker_queue);
                        call_worker_from_pending_queue(info,worker_list,inotify_fd,man,fss_out);
                        queue_remove_front(worker_queue);
                    }
                    sleep(1);
                }
                time = get_timestamp();
                snprintf(response,256,"[%s] Manager shutdown complete.\n",time);
                write_to_console(response,fss_out);
                printf("%s",response);
                free(response);
                free(time);
            }

            // when the console is closed due to wrong input or ^C it sends to the manager forced shutdown so he shutdowns as well
            if(strcmp(msg_console,"forced_shutdown") == 0){
                // activate stop flag for shutdown

                flag = 1;
                break;
            }
        }
        
        // read for inotifying evetnts
        ssize_t n_read;
        // the buffer of which comes of inotifying is bytes when we read so the size will be 16 + name 
        while( (n_read = read(inotify_fd,inotify_msg,4096)) > 0){
            char *temp = inotify_msg;
            while (temp < inotify_msg + n_read) {
                
                //read the current event
                current_event = (const struct inotify_event *) temp;

                // a file wis added
                if (current_event->mask & IN_CREATE) {
                    inotify_change(current_event->name,"ADDED",worker_list,current_event->wd,pairs_list,worker_limit,worker_queue);
                }
                
                // a file is deleted
                if (current_event->mask &  IN_DELETE) {
                    inotify_change(current_event->name,"DELETED",worker_list,current_event->wd,pairs_list,worker_limit,worker_queue);
                }

                // a file is modified
                if (current_event->mask & IN_MODIFY) {
                    inotify_change(current_event->name,"MODIFIED",worker_list,current_event->wd,pairs_list,worker_limit,worker_queue);
                }
                // go to the next message of inotifying if it exists
                temp += sizeof(struct inotify_event) + current_event->len;
            }
        }
        // check if there is any pending process in queue
        while(queue_size(worker_queue) != 0 && worker_limit > list_size(worker_list)){
            Sync_info *info = queue_front(worker_queue);
            call_worker_from_pending_queue(info,worker_list,inotify_fd,man,fss_out);
            queue_remove_front(worker_queue);
        }
        
    }
    
    // close, free and unlinks so there are not leaks
    free(inotify_msg);
    free(msg_console);
    free(buffer);
    close(man);
    close(conf);
    close(fss_in);
    close(fss_out);
    unlink("fss_in");
    unlink("fss_out");
    queue_destroy(worker_queue);
    list_destroy(pairs_list);
    list_destroy(worker_list);
    return 0;
}