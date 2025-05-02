#include <stdio.h>
#include "functions_worker.h"
#include <string.h>

int main(int argc, char **argv){
    (void)argc;
    char *source_dir = argv[1];
    char *target_dir = argv[2];
    char *filename = argv[3];
    char *operation = argv[4];
    
    // check if the operation which is given is correct
    if(strcmp(operation,"FULL") != 0 && strcmp(operation,"ADDED") != 0 && strcmp(operation,"MODIFIED") != 0 && strcmp(operation,"DELETED") != 0){
        fprintf(stderr,"The operation which is given is not valid the proccess is terminated\n");
        return -1;
    }
    
    // according to the given operation call the worker's service
    if(strcmp(operation,"FULL") == 0 && strcmp(filename,"ALL") == 0)
        full_sync(source_dir,target_dir);

    else if(strcmp(operation,"ADDED") == 0)
        add_file(source_dir,target_dir,filename);
    
    else if(strcmp(operation,"DELETED") == 0)
        delete_file(source_dir,target_dir,filename);

    else if(strcmp(operation,"MODIFIED") == 0)
        modify_file(source_dir,target_dir,filename);

    return 0;
}