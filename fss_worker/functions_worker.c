#include "functions_worker.h"
#include <dirent.h>
#include <sys/types.h>
#include <string.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#define BUFSIZE 1024

// safe malloc for char*
char* safe_malloc(size_t size) {
    char* ptr = malloc(size);
    if (ptr == NULL) {
        perror("Memory allocation failed");
        exit(1);
    }
    return ptr;
}

void full_sync(const char* source_dir, const char* target_dir){
    char *EXEC_REP = safe_malloc(10000);
    DIR* my_dir = opendir(source_dir);
    
    
    if(my_dir == NULL){
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:ERROR\nDETAILS: 0 files copied, ALL files skipped\nERRORS:\n-DIR %s: %s\nEXEC_REPORT_END\n",source_dir,strerror(errno));
        write(1,EXEC_REP,strlen(EXEC_REP));
        free(EXEC_REP);
        return;
    }
    
    struct dirent *current;

    char *temp = safe_malloc(BUFSIZE);
    char *error_buffer = safe_malloc(BUFSIZE);
    char *source_path = safe_malloc(BUFSIZE);
    char *target_path = safe_malloc(BUFSIZE);

    int skip_num = 0;
    int copied_num = 0;
    
    struct stat buf;

    // make sure the target dir doesn't exist and if not create it
    if (stat(target_dir, &buf) == -1) {
        if (mkdir(target_dir, 0744) == -1) {
            snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:ERROR\nDETAILS: 0 files copied, ALL files skipped\nERRORS:\n-DIR %s: %s\nEXEC_REPORT_END\n",target_dir,strerror(errno));
            write(1,EXEC_REP,strlen(EXEC_REP));

            closedir(my_dir);
            free(EXEC_REP);
            free(source_path);
            free(target_path);
            free(error_buffer);
            return;
        }
    }
    
    while((current = readdir(my_dir))){
        // these are folders which represent the current and the parent folder and they exist in every folder so skip them
        if(strcmp(current->d_name, ".") == 0 || strcmp(current->d_name, "..") == 0)
            continue;

        // error occured
        if(current == NULL){
            skip_num++;
            snprintf(temp,BUFSIZE,"-File %s: %s\n",current->d_name,strerror(errno));
            strcat(error_buffer,temp);
            continue;
        }
        // make the path name
        snprintf(source_path,BUFSIZE,"./%s/%s",source_dir,current->d_name);
        snprintf(target_path,BUFSIZE,"./%s/%s",target_dir,current->d_name);
        remove_extra_files_from_target_dir(source_dir,target_dir);

        // call the copy file and depnding on their output manage the counters for the files
        if(copy_file(source_path,target_path,current->d_name,error_buffer))
            copied_num++;
        else
            skip_num++;

    }

    // make and send the exec report to the manager 
    if(skip_num == 0){
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:SUCCESS\nDETAILS: %d files copied, %d files skipped\nERRORS:\n%sEXEC_REPORT_END\n ",
        copied_num,skip_num,error_buffer);
    }
    else{
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:PARTIAL\nDETAILS: %d files copied, %d files skipped\nERRORS:\n%sEXEC_REPORT_END\n",
        copied_num,skip_num,error_buffer);
        
    } 

    // we have put STDOUD on p[1] so when we use printf it goes to the manager
    write(1,EXEC_REP,strlen(EXEC_REP));

    // free the pointers we used
    closedir(my_dir);
    free(EXEC_REP);
    free(source_path);
    free(target_path);
    free(error_buffer);
}

bool copy_file(const char* source_path, const char* target_path,const char *filename, char *error_buffer){
    int source, target;
    char *temp = safe_malloc(BUFSIZE);

    // open source file and target file and if any error occur write them on the error buffer and return false
    source = open(source_path, O_RDONLY);
    if(source < 0){
        snprintf(temp,BUFSIZE,"-File %s: %s\n",filename,strerror(errno));
        strcat(error_buffer,temp);
        free(temp);
        return false;   
    }
    target = open(target_path, O_CREAT | O_WRONLY|O_TRUNC ,0666);
    if(target < 0){
        snprintf(temp,BUFSIZE,"-File %s: %s\n",filename,strerror(errno));
        strcat(error_buffer,temp);
        free(temp);
        close(source);
        return false;
    }

    // copying from source to target
    char msg[BUFSIZE];
    ssize_t bytes;
    while((bytes = read(source,msg,sizeof(msg)) )> 0){
        write(target,msg,bytes);
    }

    free(temp);
    close(source);
    close(target);
    return true;
}



void add_file(const char *source_dir,const char *target_dir,const char *filename){
 
    char *EXEC_REP = safe_malloc(BUFSIZE);
    char *temp = safe_malloc(BUFSIZE);
    char *error_buffer = safe_malloc(BUFSIZE);
    char *source_path = safe_malloc(BUFSIZE);
    char *target_path = safe_malloc(BUFSIZE);

    // make the path names 
    snprintf(source_path,BUFSIZE,"./%s/%s",source_dir,filename);
    snprintf(target_path,BUFSIZE,"./%s/%s",target_dir,filename);

    // copy files and write according EXEC REPORT dor them
    if(copy_file(source_path,target_path,filename,error_buffer))
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:SUCCESS\nDETAILS: File: %s\nERRORS:\n%sEXEC_REPORT_END\n",filename,error_buffer);

    else
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:ERROR\nDETAILS: File: %s\nERRORS:\n%sEXEC_REPORT_END\n",filename,error_buffer);

    write(1,EXEC_REP,strlen(EXEC_REP));
    free(EXEC_REP);
    free(temp);
    free(error_buffer);
    free(source_path);
    free(target_path);
}

void delete_file(const char *source_dir,const char *target_dir,const char *filename){
    (void)source_dir;
    char *EXEC_REP = safe_malloc(10000);
    char *temp = safe_malloc(BUFSIZE);
    char *error_buffer = safe_malloc(BUFSIZE);
    char *source_path = safe_malloc(BUFSIZE);
    char *target_path = safe_malloc(BUFSIZE);

    // make target path
    snprintf(target_path,BUFSIZE,"./%s/%s",target_dir,filename);

    // delete it and make EXER REP
    if(unlink(target_path) == 0)
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:SUCCESS\nDETAILS: File: %s\nERRORS:\n%sEXEC_REPORT_END\n",filename,error_buffer);
    
    else {
        snprintf(temp,BUFSIZE,"-File %s: %s\n",filename,strerror(errno));
        strcat(error_buffer,temp);
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:ERROR\nDETAILS: File: %s\nERRORS:\n%sEXEC_REPORT_END\n",filename,error_buffer);
    }

    write(1,EXEC_REP,strlen(EXEC_REP));
    free(EXEC_REP);
    free(temp);
    free(error_buffer);
    free(source_path);
    free(target_path);

}

// function which modifies the file to the target dir
void modify_file(const char *source_dir,const char *target_dir,const char *filename){
    char *EXEC_REP = safe_malloc(10000);
    char *temp = safe_malloc(BUFSIZE);
    char *error_buffer = safe_malloc(BUFSIZE);
    char *source_path = safe_malloc(BUFSIZE);
    char *target_path = safe_malloc(BUFSIZE);

    // make source and target paths
    snprintf(source_path,BUFSIZE,"./%s/%s",source_dir,filename);
    snprintf(target_path,BUFSIZE,"./%s/%s",target_dir,filename);

    // modify the file and EXEC REP
    if(copy_modified_file(source_path,target_path,filename,error_buffer))
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:SUCCESS\nDETAILS: File: %s\nERRORS:\n%sEXEC_REPORT_END\n",filename,error_buffer);

    else 
        snprintf(EXEC_REP,10000,"EXEC_REPORT_START\nSTATUS:ERROR\nDETAILS: File: %s\nERRORS:\n%sEXEC_REPORT_END\n",filename,error_buffer);

    write(1,EXEC_REP,strlen(EXEC_REP));
    free(EXEC_REP);
    free(temp);
    free(error_buffer);
    free(source_path);
    free(target_path);
}

bool copy_modified_file(const char* source_path, const char* target_path,const char *filename, char *error_buffer){
    int source, target;
    char *temp = safe_malloc(BUFSIZE);

    // open source file and target file and if any error occur write them on the error buffer and return false
    source = open(source_path, O_RDONLY);
    if(source < 0){
        snprintf(temp,BUFSIZE,"-File %s: %s\n",filename,strerror(errno));
        strcat(error_buffer,temp);
        free(temp);
        return false;
    }

    // WE DON'T CREATE THE FILE IT MUST ALREADY EXIST AS IT IS ACTION: MODIFIED
    target = open(target_path, O_WRONLY|O_TRUNC ,0666);
    if(target < 0){
        snprintf(temp,BUFSIZE,"-File %s: %s\n",filename,strerror(errno));
        strcat(error_buffer,temp);
        free(temp);
        close(source);
        return false;
    }

    // copying from source to target
    char msg[BUFSIZE];
    ssize_t bytes;
    while((bytes = read(source,msg,sizeof(msg)) )> 0){
        write(target,msg,bytes);
    }

    close(source);
    close(target);
    return true;
}

void remove_extra_files_from_target_dir(const char *source_dir, const char *target_dir){
    DIR *tar = opendir(target_dir);
    if(tar == NULL)
        return;
        
    char *target_path = safe_malloc(BUFSIZE);
    char *source_path = safe_malloc(BUFSIZE);
    struct dirent *curr;
    while ((curr = readdir(tar)) != NULL) {
        // these are folders which represent the current and the parent folder and they exist in every folder so skip them
        if (strcmp(curr->d_name, ".") == 0 || strcmp(curr->d_name, "..") == 0)
            continue;
    
        
        snprintf(target_path, BUFSIZE, "%s/%s", target_dir, curr->d_name);
        snprintf(source_path, BUFSIZE, "%s/%s", source_dir, curr->d_name);

        // remove the files that don't exist in the source dir anymore
        struct stat st;
        if (stat(source_path, &st) == -1) 
                unlink(target_path);
    }

    closedir(tar);
    free(target_path);
    free(source_path);
}