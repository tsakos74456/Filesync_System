#include "functions_manager.h"
#include <time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <errno.h>
#include <sys/inotify.h>

// a small comment about what each function does and how it is used can be found on README.md
bool check_flags_manager(const int argc, char **argv){
    if (argc < 5){
        perror("Use the flags correctly!\n");
        return 0;
    }
    if (strcmp(argv[1],"-l")){
        perror("The -l flag is not used correctly\n");
        return 0;
    }
    if (strcmp(argv[3],"-c")){
        perror("The -c flag is not used correctly\n");
        return 0;
    }
    return 1;
}

void add_sync_task(Sync_info *new_info, List current_workers,const int fd, const int inotify_fd){ 
    new_info->active = true;
    // for inotify
    int inotify_descriptor = inotify_add_watch(inotify_fd, new_info->source_dir, IN_CREATE | IN_MODIFY | IN_DELETE);
    new_info->init = inotify_descriptor;
    // create pipe
    int p[2];
    if(pipe(p) == -1){
        perror("Creation of pipe failed\n");
        exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("Failed to fork!\n");
        exit(-1);
    }
    // child process use exec and send it to the worker
    else if(pid == 0){
        close(p[0]);    //close read channel
        dup2(p[1],1);  //redirect stdout -> write channel
        execl("./worker","worker",new_info->source_dir,new_info->target_dir,"ALL","FULL",NULL);
        perror("exec failed");
        exit(1);
    }

    else if (pid > 0){
        close(p[1]); //close the write channel


        // keep the worker in a list 
        Worker_info *work = malloc(sizeof(Worker_info));
        if (work == NULL){
            perror("Failed memory allocation for worker info");
            exit(1);
        }
        work->fd_pipe = p[0];
        work->worker_pid = pid;
        work->info = new_info;
        work->operation = strdup("FULL");
        list_insert_next(current_workers, list_last(current_workers), work);

        // print according message
        char *time=get_timestamp();
        char *message = malloc(1024);
        if (message == NULL)
            perror("failed memory allocation for message's add\n");
        snprintf(message,1024,"[%s] Added directory: %s -> %s\n[%s] Monitoring started for %s\n",time,new_info->source_dir,
            new_info->target_dir,time,new_info->source_dir);
        printf("%s",message);
        write(fd, message, strlen(message));
        free(time);
        free(message);
    }
}

void update_pair(Sync_info *new_info, const char *EXEC_REP,const int console_fd,const int man_fd,const pid_t pid,const char *operation){
    int error_count = 0, i = 0 ;
    while(EXEC_REP[i] != '\0'){
        // find the amount of errors 
        if (EXEC_REP[i] == '\n' && EXEC_REP[i + 1] == '-' &&  (strncmp(&EXEC_REP[i + 2], "File", 4) == 0 || strncmp(&EXEC_REP[i + 2], "DIR", 3) == 0)) {
            error_count++;
        }
        i++;
    }
    // find the status
    i = 0;
    int j = 0;
    char status[8];
    // iterate until the status part
    while (EXEC_REP[i] != '\0' && strncmp(&EXEC_REP[i], "STATUS:", 7) != 0) {
        i++;
    }
    // skip the 7 characters of STATUS:
    i = i + 7;
    while (EXEC_REP[i] != '\0' && EXEC_REP[i] != '\n' && EXEC_REP[i] != ' ') {
        status[j++] = EXEC_REP[i++];
    }
    status[j] = '\0';

    // update the syncinfo
    new_info->error_count += error_count;
    if(new_info->status != NULL)
        free(new_info->status);
    new_info->status = strdup(status);
    if(new_info->last_sync_time != NULL)
        free(new_info->last_sync_time);
    new_info->last_sync_time = get_timestamp();
    new_info->now_syncing = 0;

    // if the worker comes from manual syncing from console I print the according message to the console
    if(new_info->synced){
        new_info->synced = 0;
        char msg[256];
        char *time = get_timestamp();
        snprintf(msg,1024,"[%s] Sync completed %s -> %s Errors:%d\n", time, new_info->source_dir,new_info->target_dir,new_info->error_count);
        write(1,msg,strlen(msg));
        write_to_console(msg,console_fd);
        write(man_fd,msg,strlen(msg));
        free(time);
    }

    print_manager_log(new_info,EXEC_REP,man_fd,pid,operation);
}


char* get_timestamp() {
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);

    char *buffer=malloc(256);  // Enough space for "YYYY-MM-DD HH:MM:SS\0"
    if (buffer == NULL) {
        perror("Allocation failed for buffer");
        exit(1);
    }
    strftime(buffer, 20, "%Y-%m-%d %H:%M:%S", tm_info);
    return buffer;
}

void destroy_sync_info(Pointer info) {
    Sync_info *curr = info;
    free(curr->source_dir);
    free(curr->target_dir);
    free(curr->operation_for_queue);
    free(curr->filename_for_queue);
    if(curr->status != NULL)
        free(curr->status);
    if(curr->last_sync_time != NULL)
        free(curr->last_sync_time);    
    free(curr);
}

bool check_existance_pair(const char *source_dir, const List pair_lists){
    for(ListNode node = list_first(pair_lists) ; node != LIST_EOF ; node = list_next(pair_lists,node)){
        Sync_info *info = (Sync_info *)list_node_value(pair_lists,node);
        if(strcmp(source_dir,info->source_dir) == 0){
            // match found
            return true;
        }
    }
    // no match found
    return false;
}

bool check_exist_fifo(const char *fifo){
    struct stat status;
    if (stat(fifo, &status) == 0)
        return true;
    return false;
}

void initial_full_sync(Queue workers_queue,const int worker_limit, List current_workers,const int fd,const int inotify_fd){
    // if there are available worker I call them otherwise I put the tasks in the queue
    while(list_size(current_workers) < worker_limit && queue_size(workers_queue) != 0){
        add_sync_task(queue_front(workers_queue),current_workers,fd,inotify_fd);
        queue_remove_front(workers_queue);

    }
}

void add_dir(const char *source_dir,const char *target_dir, List list_dir, Queue workers_queue, List current_workers, const int worker_limit,const int console_fd, const int man_fd,const int inotify_fd){
    
    Sync_info *new_info = malloc(sizeof(Sync_info));
    if(new_info == NULL)
            perror("Fail to allocate memory for info\n");
    
    new_info->source_dir = strdup(source_dir);
    new_info->target_dir = strdup(target_dir);
    new_info->active = true;
    new_info->now_syncing = true;
    new_info->initial = false;
    new_info->status = NULL;     
    new_info->error_count = 0;  
    new_info->last_sync_time = NULL; 
    new_info->synced = 0;
    new_info->filename_for_queue = strdup("ALL");       
    new_info->operation_for_queue = strdup("FULL");
    
    // if there is any available worker I call him otherwise I put the tasks in the queue
    if(worker_limit > list_size(current_workers)){
        int inotify_descriptor = inotify_add_watch(inotify_fd, new_info->source_dir, IN_CREATE | IN_MODIFY | IN_DELETE);
        new_info->init = inotify_descriptor;
        list_insert_next(list_dir,list_last(list_dir),new_info);
        
        // create pipe
        int p[2];
        if(pipe(p) == -1){
            perror("Creation of pipe failed\n");
            exit(1);
        }

        pid_t pid = fork();
        if(pid == -1){
            perror("Failed to fork!\n");
            exit(-1);
        }
        // child process use exec and send it to the worker
        else if(pid == 0){
            close(p[0]);    //close read channel
            dup2(p[1],1);  //redirect stdout -> write channel
            execl("./worker","worker",new_info->source_dir,new_info->target_dir,"ALL","FULL",NULL);
            perror("exec failed");
            exit(1);
        }

        else if (pid > 0){
            close(p[1]); //close the write channel

            // keep the worker in a list 
            Worker_info *work = malloc(sizeof(Worker_info));
            if (work == NULL){
                perror("Failed memory allocation for worker info");
                exit(1);
            }
            work->fd_pipe = p[0];
            work->worker_pid = pid;
            work->info = new_info;
            work->operation = strdup("FULL");
            list_insert_next(current_workers, list_last(current_workers), work);

            // print according message
            char *time=get_timestamp();
            char *message = malloc(1024);
            if (message == NULL)
                perror("failed memory allocation for message's add\n");
            snprintf(message,1024,"[%s] Added directory: %s -> %s\n[%s] Monitoring started for %s\n",time,new_info->source_dir,
            new_info->target_dir,time,new_info->source_dir);
            write_to_console(message,console_fd);
            write(man_fd,message,strlen(message));
            printf("%s",message);
            free(time);
            free(message);
        }
    }
    else
        queue_insert_back(workers_queue,new_info);
}


void status(const List list_dir, const char *source,const int fd){
    char *message = malloc(1024);
    if (message == NULL)
        perror("failed memory allocation for message's status\n");
    // check if the pair  exists 
    for(ListNode node = list_first(list_dir) ; node != LIST_EOF ; node = list_next(list_dir,node)){
        Sync_info *info = (Sync_info *)list_node_value(list_dir,node);
        // pair found print status
        if(strcmp(info->source_dir,source) == 0 ){
            char *time = get_timestamp();
            snprintf(message,1024,"[%s] Status requested for %s\nDirectory: %s\nTarget: %s\nLast Sync: %s\nErrors: %d\nStatus: %s\n",time,source,info->source_dir,
            info->target_dir,info->last_sync_time,info->error_count,info->active ? "Active" : "Inactive");
            printf("%s",message);
            write_to_console(message,fd);
            free(time);
            free(message);
            return;
        }
    }
    // if doesn't exist print according message
    char *time = get_timestamp();         
    snprintf(message,1024,"[%s] Directory not monitored: %s\n", time, source);
    printf("%s",message);
    write_to_console(message,fd);
    free(time);
    free(message);
}

void write_to_console(const char *message, const int fd){
    if(write(fd,message,strlen(message)) < 0){
        // check if for any reason the other end is closed
        if (errno == EPIPE) {
            printf("Manager closed the pipe. Exiting console.\n");
        }
        perror("write to fss_in");
    }
}


void console_sync(const List list_dir, const char *source, const int fd_console, const int worker_limit, List current_workers,const int man_fd,const int inotify_fd, Queue workers_queue){
    char *message = malloc(1024);
    if (message == NULL)
        perror("failed memory allocation for message's status\n");
    for(ListNode node = list_first(list_dir) ; node != LIST_EOF ; node = list_next(list_dir,node)){
        Sync_info *info = (Sync_info *)list_node_value(list_dir,node);
        //found
        if(strcmp(info->source_dir,source) == 0 ){
            //as this a worker is syncing the dir so don't do it again
            if(info->now_syncing){
                char *time = get_timestamp();
                snprintf(message,1024,"[%s]  Sync already in progress %s\n", time, source);
                printf("%s",message);
                write_to_console(message,fd_console);
                free(message);
                free(time);
                return;
            }
            else{
                info->active = 1;
                info->synced = 1; 
                info->now_syncing = 1;
                if (info->operation_for_queue != NULL)
                    free(info->operation_for_queue);
                if (info->filename_for_queue != NULL)
                    free(info->filename_for_queue);

                info->operation_for_queue = strdup("console_sync");
                info->filename_for_queue = strdup("ALL");
                if(worker_limit > list_size(current_workers)){
                    int inotify_descriptor = inotify_add_watch(inotify_fd, info->source_dir, IN_CREATE | IN_MODIFY | IN_DELETE);
                    info->init = inotify_descriptor;
                    // create pipe
                    int p[2];
                    if(pipe(p) == -1){
                        perror("Creation of pipe failed\n");
                        exit(1);
                    }
            
                    pid_t pid = fork();
                    if(pid == -1){
                        perror("Failed to fork!\n");
                        exit(-1);
                    }
                    // child process use exec and send it to the worker
                    else if(pid == 0){
                        close(p[0]);    //close read channel
                        dup2(p[1],1);  //redirect stdout -> write channel
                        execl("./worker","worker",info->source_dir,info->target_dir,"ALL","FULL",NULL);
                        perror("exec failed");
                        exit(1);
                    }
            
                    else if (pid > 0){
                        close(p[1]); //close the write channel
                        
                        // keep the worker in a list 
                        Worker_info *work = malloc(sizeof(Worker_info));
                        if (work == NULL){
                            perror("Failed memory allocation for worker info");
                            exit(1);
                        }
                        work->fd_pipe = p[0];
                        work->worker_pid = pid;
                        work->info = info;
                        work->operation = strdup("FULL");
                        list_insert_next(current_workers, list_last(current_workers), work);
                        char *time = get_timestamp();
                        
                        // print according message
                        snprintf(message,1024,"[%s] Syncing directory: %s -> %s\n",time,info->source_dir,info->target_dir);
                        write_to_console(message,fd_console);
                        write(man_fd,message,strlen(message));
                        printf("%s",message);
                        free(time);
                        free(message);
                        return;
                    }
                }

                else{
                    queue_insert_back(workers_queue,info);
                }
            }
        }
    }

    // i added this. It wasn't asked but I think the console and the user should be updated for this error if the pair doesn't exist
    char *time = get_timestamp();
    snprintf(message,1024,"[%s] Directory not monitored: %s\n", time, source);
    printf("%s",message);
    write_to_console(message,fd_console);
    free(time);
    free(message);
}

void cancel_dir(const List list_dir, const char *source, const int console_fd, const int man_fd,const int inotify_fd){
    char *message = malloc(1024);
    if (message == NULL)
        perror("failed memory allocation for message's status\n");
    for(ListNode node = list_first(list_dir) ; node != LIST_EOF ; node = list_next(list_dir,node)){
        Sync_info *info = (Sync_info *)list_node_value(list_dir,node);
        // pair found and cancel active syncing
        if(strcmp(info->source_dir,source) == 0){
            info->active = false;
            char *time = get_timestamp();
            snprintf(message,1024,"[%s] Monitoring stopped for %s\n", time, source);
            printf("%s",message);
            write_to_console(message,console_fd);
            write(man_fd,message,strlen(message));
            inotify_rm_watch(inotify_fd, info->init);
            free(time);
            free(message);
            return;
        }
    }
    // the source dir is not monitored
    char *time = get_timestamp();
    snprintf(message,1024,"[%s] Directory not monitored: %s\n", time, source);
    printf("%s",message);
    write_to_console(message,console_fd);
    
    free(time);
    free(message);
}

void print_manager_log(const Sync_info *info, const char *msg,const int man,const pid_t pid,const char *operation){
    char *final = malloc(9216);
    if(final == NULL){
        perror("Failed to allocate memory\n");
        exit(1);
    }
    // find in the exec rep the part of the details 
    char *final_details = malloc(256);
    if(final_details == NULL){
        perror("Failed to allocate memory\n");
        exit(1);
    }
    char *details;

    if(strcmp(info->status,"ERROR") == 0  && (strcmp(operation,"ADDED") == 0 || strcmp(operation,"DELETED") == 0 || strcmp(operation,"MODIFIED") == 0)){
        details = strstr(msg,"ERRORS:");
        details += 8;
        int i = 0;
        
        while (*details != '\n'){
            final_details[i++] = *details++;
        }
        final_details[i] = '\0';

    }
    else{
        details = strstr(msg,"DETAILS:");
        details += 9;
        int i = 0;
        while (*details != '\n'){
            final_details[i++] = *details++;
           
        }
        final_details[i] = '\0';
    }
    
    snprintf(final,9216,"[%s] [%s] [%s] [%d] [%s] [%s] [%s]\n",info->last_sync_time,info->source_dir,info->target_dir,pid,operation,info->status,final_details);
    write(man,final,strlen(final));
    free(final);
    free(final_details); 
}

void inotify_change(const char *filename, const char *operation, List current_workers, const int fd,List pairs_list,const int worker_limit,Queue worker_queue){
    for(ListNode node = list_first(pairs_list) ; node != LIST_EOF ; node = list_next(pairs_list,node)){
        Sync_info *current_info = list_node_value(pairs_list,node);
        // find which dir is has changed
        if(current_info->init == fd){

            // the modified is called twice, when we modify a file so I set now syncing from here so the second time it will not call a worker
            if (strcmp(operation, "MODIFIED") == 0) {
                if (current_info->now_syncing)
                    return;
                current_info->now_syncing = true;
            }
            if (current_info->operation_for_queue != NULL)
                free(current_info->operation_for_queue);
            if (current_info->filename_for_queue != NULL)
                free(current_info->filename_for_queue);
            current_info->operation_for_queue = strdup(operation);
            current_info->filename_for_queue = strdup(filename);
            current_info->now_syncing = true;

            // if there are available worker we call them otherwise we put the tasks in the queue
            if(list_size(current_workers) < worker_limit){
                call_worker(filename,operation,current_workers,current_info);
                break;
            }

            // pending job in the queue
            else {
                queue_insert_back(worker_queue,current_info);
                break;
            }
        }
    }
}

void call_worker(const char *filename, const char *operation, List current_workers, Sync_info *info){
    info->now_syncing = 1;
    // create pipe
    int p[2];
    if(pipe(p) == -1){
        perror("Creation of pipe failed\n");
        exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("Failed to fork!\n");
        exit(-1);
    }
    // child process use exec and send it to the worker
    else if(pid == 0){
        close(p[0]);    //close read channel
        dup2(p[1],1);  //redirect stdout -> write channel
        execl("./worker","worker",info->source_dir,info->target_dir,filename,operation,NULL);
        perror("exec failed");
        exit(1);
    }

    else if (pid > 0){
        close(p[1]); //close the write channel

        // keep the worker in a list 
        Worker_info *work = malloc(sizeof(Worker_info));
        if (work == NULL){
            perror("Failed memory allocation for worker info");
            exit(1);
        }
        work->fd_pipe = p[0];
        work->worker_pid = pid;
        work->info = info;
        work->operation = strdup(operation);
        list_insert_next(current_workers, list_last(current_workers), work);
    }
}

void call_worker_from_pending_queue(Sync_info *info, List current_workers, const int inotify_fd, const int man_fd,const int console_fd){
    // in case the info is coming from inotifying event
    if(strcmp(info->operation_for_queue, "DELETED") == 0 || strcmp(info->operation_for_queue, "ADDED") == 0 || strcmp(info->operation_for_queue, "MODIFIED") == 0)
        call_worker(info->filename_for_queue,info->operation_for_queue,current_workers,info);
    // the info is coming from initial sync, add dir or sync from user and we separate add_dir and initial sync with sync from console as the messages which are printed are different 
    else if(strcmp(info->operation_for_queue, "console_sync") == 0)
        sync_console_from_queue(info,current_workers,inotify_fd,man_fd,console_fd);
    else{
        initial_sync_or_add_dir_from_queue(info,current_workers,inotify_fd,man_fd,console_fd);
        
    }
}


void initial_sync_or_add_dir_from_queue(Sync_info *new_info, List current_workers, const int inotify_fd, const int man_fd,const int console_fd){
    int inotify_descriptor = inotify_add_watch(inotify_fd, new_info->source_dir, IN_CREATE | IN_MODIFY | IN_DELETE);
    new_info->init = inotify_descriptor;
        
    // create pipe
    int p[2];
    if(pipe(p) == -1){
        perror("Creation of pipe failed\n");
        exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("Failed to fork!\n");
        exit(-1);
    }
    // child process use exec and send it to the worker
    else if(pid == 0){
        close(p[0]);    //close read channel
        dup2(p[1],1);  //redirect stdout -> write channel
        execl("./worker","worker",new_info->source_dir,new_info->target_dir,"ALL","FULL",NULL);
        perror("exec failed");
        exit(1);
    }

    else if (pid > 0){
        close(p[1]); //close the write channel
         
        // keep the worker in a list 
        Worker_info *work = malloc(sizeof(Worker_info));
        if (work == NULL){
            perror("Failed memory allocation for worker info");
            exit(1);
        }
        work->fd_pipe = p[0];
        work->worker_pid = pid;
        work->info = new_info;
        work->operation = strdup("FULL");
        list_insert_next(current_workers, list_last(current_workers), work);

        // print according message
        char *time=get_timestamp();
        char *message = malloc(1024);
        if (message == NULL)
            perror("failed memory allocation for message's add\n");
        snprintf(message,1024,"[%s] Added directory: %s -> %s\n[%s] Monitoring started for %s\n",time,new_info->source_dir,
        new_info->target_dir,time,new_info->source_dir);
        if(!new_info->initial)
            write_to_console(message,console_fd);
        write(man_fd,message,strlen(message));
        printf("%s",message);
        free(time);
        free(message);
    }
}

void sync_console_from_queue(Sync_info *info,List current_workers,const int inotify_fd,const int man_fd,const int fd_console){
    char *message = malloc(1024);
    if (message == NULL)
        perror("failed memory allocation for message's status\n");
    int inotify_descriptor = inotify_add_watch(inotify_fd, info->source_dir, IN_CREATE | IN_MODIFY | IN_DELETE);
    info->init = inotify_descriptor;
    // create pipe
    int p[2];
    if(pipe(p) == -1){
        perror("Creation of pipe failed\n");
        exit(1);
    }

    pid_t pid = fork();
    if(pid == -1){
        perror("Failed to fork!\n");
        exit(-1);
    }
    // child process use exec and send it to the worker
    else if(pid == 0){
        close(p[0]);    //close read channel
        dup2(p[1],1);  //redirect stdout -> write channel
        execl("./worker","worker",info->source_dir,info->target_dir,"ALL","FULL",NULL);
        perror("exec failed");
        exit(1);
    }
    else if (pid > 0){
        close(p[1]); //close the write channel
        
        // keep the worker in a list 
        Worker_info *work = malloc(sizeof(Worker_info));
        if (work == NULL){
            perror("Failed memory allocation for worker info");
            exit(1);
        }
        work->fd_pipe = p[0];
        work->worker_pid = pid;
        work->info = info;
        work->operation = strdup("FULL");
        list_insert_next(current_workers, list_last(current_workers), work);

        // print according message
        char *time = get_timestamp();
        snprintf(message,1024,"[%s] Syncing directory: %s -> %s\n",time,info->source_dir,info->target_dir);
        write_to_console(message,fd_console);
        write(man_fd,message,strlen(message));
        printf("%s",message);
        free(time);
        free(message);
        return;
     
    }
}