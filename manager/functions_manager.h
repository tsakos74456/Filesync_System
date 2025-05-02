#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include "../lib/ADTList.h"
#include "../lib/ADTQueue.h" 
#include <sys/types.h>

// struct to save the info between the pairs
typedef struct{
    char *source_dir; // the source dir
    char *target_dir; // target dir
    bool active;       // 1 if it's active 0 if it isn't
    char *last_sync_time;
    int error_count;
    char *status;  // ERROR/PARTIAL/SUCCESS
    bool now_syncing;   // if a syncing is taking place right now it is true possible in queue
    bool synced; // if the worker synced a dir manually by the console so we print the according message which is needed
    bool initial; // it is true id this pair is read by the config all the other pairs read from console are false
    int init;   // it saves the file desctriptor of inotifying for this dir
    char *filename_for_queue;   // if the info gets in the queue cause there aren't any available worker 
                                // save the filename which is given to the worker here
    char *operation_for_queue;  // if the info gets in the queue cause there aren't any available worker 
                                // save the operation which is given to the worker here
} Sync_info;

typedef struct 
{
    Sync_info *info;  // pointer at the sync_info
    pid_t worker_pid;   //keep child's process (worker)
    int fd_pipe ;       // keep the file descriptor in order to read from the manager afterwards
    char *operation;    //keeps the opertion which is given to the worker
}Worker_info;


// checks the flags in case of error returns 0 otherwise 1
bool check_flags_manager(const int argc, char **argv);

// sync the pair source target and make some several checks to see if there are available workers or check if the source dir is already synced
// if the pair doesn't already exist add it to the list which contains the pairs
void add_sync_task(Sync_info *new_info, List current_workers,const int fd,const int inotify_fd);

void update_pair(Sync_info *new_info, const char *EXEC_REP,const int fd,const int man,const pid_t pid,const char *operation);

// functon which returns the needed timestamp
char* get_timestamp();

//function so the list doesn't have leaks to free the sync info correctly 
void destroy_sync_info(Pointer info);

// function to check if the source dir is already synced returns true if it exists
bool check_existance_pair(const char *source_dir, const List pair_lists);

// check if the fifo which is given exists
// return true if it exists
bool check_exist_fifo(const char *fifo);

// function which is called on the initial sync when we read pairs from the config_file
void initial_full_sync(Queue workers_queue, const int worker_limit, List current_workers,const int fd,const int inotify_fd);

// function which adds a pair of dir and starts syncing immediately
void add_dir(const char *source_dir,const char *target_dir, List list_dir, Queue workers_queue, List current_workers, const int worker_limit,const int console_fd,const int man_fd,const int inotify_fd);

// if the dir exists returns the info of the dir which is in the list dir
void status(const List list_dir, const char *source,const int fd);

// writes the given message to the console
void write_to_console(const char *message, const int fd);

// it synces the source even if the fir is inactive
void console_sync(const List list_dir, const char *source, const int console_fd, const int worker_limit, List worker_list,const int man_fd,const int iotify_fd,Queue workers_queue);

// it cancels the monitorring of the source dir
void cancel_dir(const List list_dir, const char *source, const int console_fd, const int man_fd,const int inotify_fd);

// just prints message to the manager log
void print_manager_log(const Sync_info *info, const char *msg,const int man,const pid_t pid,const char *operation);

// function which called when a change through inotifying is detected
void inotify_change(const char *filename, const char *operation, List current_workers,  const int fd, List pairs_list,const int worker_limit, Queue worker_queue);

// it forks exec the worker and creates a worker in the worker_list so the process's worker can be found in the signal handler
void call_worker(const char *filename, const char *operation, List current_workers, Sync_info *info);

// function which calls a worker when the task has got in the queue and it is only is this task is sync manually from console
void sync_console_from_queue(Sync_info *info,List current_workers,const int inotify_fd,const int man_fd,const int fd_console);

// function which calls a worker when the task has got in the queue and it is only is this task is a sync from the config file or add a dir from the console manualaly
void initial_sync_or_add_dir_from_queue(Sync_info *new_info, List current_workers, const int inotify_fd, const int man_fd,const int console_fd);

// calls a worker for an inotifying event that got in the queue cause there were not available workers 
void call_worker_from_pending_queue(Sync_info *info, List current_workers, const int inotify_fd, const int man_fd,const int console_fd); 


