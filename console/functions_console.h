#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <signal.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <time.h>

// functon which returns the needed timestamp
char* get_timestamp();

// help function to print the output to the unser and the console-logfile
void print_command(const int fd,const char *source_dir,const char *target_dir, const char *command,const char* time);

// prints to the console log file the manager response
void print_manager_response(const int fd, const char *msg);

// function which reads what the manager sends
void read_from_manager(char *response, const int fd_out, const int console_file, bool *flag, bool *last_flag);

