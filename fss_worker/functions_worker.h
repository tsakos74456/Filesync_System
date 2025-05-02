#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// function which is called when the filename = ALL  and operation = FULL. It copies all the files from source dir to target dir
void full_sync(const char* source_dir, const char* target_dir);

// function which copies the source file to the target one. returns true if it suceeds otherwrise false and in the error_buffer i put the according error
bool copy_file(const char* source_file, const char* target_file,const char *file_name, char *error_buffer);

// function which adds the file to the target dir
void add_file(const char *source_dir,const char *target_dir,const char *filename);

// function which deletes the file to the target dir
void delete_file(const char *source_dir,const char *target_dir,const char *filename);

// function which modifies the file to the target dir
void modify_file(const char *source_dir,const char *target_dir,const char *filename);

// function which copies the source file to the target one. returns true if it suceeds otherwrise false and in the error_buffer i put the according error
// the difference with copy file is that it doesn't create the file just overwrite its data
bool copy_modified_file(const char* source_path, const char* target_path,const char *filename, char *error_buffer);

// function called on sync to remove the files that were deleted from the source dir
void remove_extra_files_from_target_dir(const char *source_dir, const char *target_dir);