SOS INSTRUTIONSFOR CONFIGURATION FILE:
the format of the configuration file must be kike that:
a space (' ') separates the source with the target directory and the target directory with the next pair is separated with change of line ('\n') even if it is the last pair in the file. For example:
user/photos backup/photos
user/acc backup/acc

Also the given paths must be relative not absolute. Otherwise it will just return as error as it won't find the path.

So the format should be like that after the last pair's target directory shoulf be a vhange of line otherwise this pair wouldn't be able to be read
FOR FSS_MANAGER AND FSS_WORKER: 

MAIN USE: Make the initial sync from the config's file info. It commands the workers in order to accomplish the tasks. It communicates with them through pipes and the workers can work coccurently. Also, it communicates with the console which takes commands fromt the user. It communicates with the console through 2 named pipes.

In the begging of fss_manager I check if the named pipes exist. If they already exist I delete them. After that, I take the arguments which are given and I create the named pipes.
I am reading the pairs source-target and I add them in the list if the source dir isn't already a part of a pair. 
When the config file is read and the pairs have been put in the list. I match them with the workers(if there is any avaialable) otherwise they are put in a queue(name: worker queue).
When the workers end their task the manager is informed and he prints on his terminal and in the manager_log_file it doesn't send them at the console as the pairs are read from the configuration file.
Also I have 2 lists one to save the info about the pairs of dirs which are read by the manager and another one to save the info about the worker so I can later find him in the signal handler(explained below).
After I am waiting for input from the console and I call the workers depending on the operation.
In the background workers are calling their functions' in order to accomplish their task and when they finish through the pipes they send the result to the manager so he can update the pair of dirs(Sync_info). In order to "catch" the process I have a signal handler and pair's info are updated there through the function update_pair.
In the end when the console gives the command to shutdown the manager has firstly to check if there is any active worker at that moment. If there is one or there is a pending job in the queue the manager has to wait him and afterwards the program shutdowns by printing and sending to console the according messages.

About signal handler: The manager has a signal handler which is called when a worker is finished. When this happen a flag is raised in it and the manager checks which proccess was, prints and sends to console the according messages, finds the worker who did the process and prepare him for another job. 

QUEUEING: If there are no available workers when I need them I add the pending job in the queue and when a worker ends his task will immeidiately get one from the queue. 

INOTIFYING EVENTS: If a dir is monitored and something is changed in it the manager will be informed though inotify and afterwards he will assign it on a worker.

These are all done with the help of 3 data structures. The first one is called "pairs_list" in which I save the pairs of the sources and the according info about them (more info about the Sync_info there are in the functions_manager.h). The second structure is a list again called "worker_list" in which I save the info about the active workers and the third one is the queue "worker_queue" in which I save the pending jobs if there is not any available worker. In the pairs_list and in the queue I pass the same pointer of sync info so they share the data and that is the reason only one of these structures free its content.

When a worker is called I redirect stdout -> write channel so they can communicate through it.

FOR FSS_SCRIPT.SH:

Main use: find monitored, stopped dirs and delete the manager log or a target dir which is given as arg

In the begging I check the given flags and if the file or dir exists. Then if the command is listAll, listMonitored or list Stopped I read from the manager log the dirs and I keep the info about manager log's content in maps. One for the timestamp, one for the dir's status, one for target dir and one in order to keep which directories have been stopped monitoring. So according the command I use some of them. If the command is purge I delete the path or file which is given in the parameter path.

FOR FSS_CONSOLE:

MAIN USE: the console takes input from the user and provides it to the manager through a FIFO. 

More specifically, it checks if the input is correct and after that it opens the fifo. It sends the command with the pair dirs or whatever is need to the manager so he can start the required process for the command. Also he reads the manager's response and prints it at the terminal and in the console_log_file.
As I understood from the assignemnt in the console log file I just print the commands and the message I get from the manager. The message with this format ([TIMESTAMP] [SOURCE_DIR] [TARGET_DIR] [WORKER_PID] [OPERATION] [RESULT] [DETAILS]) must not be in this file just in the manager_log. In the loop that reads the input I am using sleep as the user won't write the command quicker than 0.5sec so I won't waste pc's resources like that and in the meantime the console will have time to read the manager's response. Moreover, it is not needed but if the console is terminated violently with ^C I will free the file descriptors so the leaks will be minimized. The correct way to terminate the console is with shutdown and the console waits for the final message from the manager in order to shutdown itself. This is done with the help of two flags which are called flag and last_flag(more info as comments in the code of how they work). In the .h files there are comments regarding the functions' operation.


FOR WORKER:

MAIN USE: does the main job of copying deleting modifyin and full syncing and reply to manager 

When I call the worker, he checks if the arguments are correct and then starts the job. If thr args are FULL ALL he starts a full sync from the source dir to the target one and he copies file to file with the help of the function copy_file. If the operation is ADDED a file is added so copy only he file which was added. The same with operation DELETED juts delete the file. If the operation is MODIFIED it copies the whole file not the differences between the two files but the difference with ADDED is that when I open the file I don't create it so if the file doesn't exist it will be an error.

MAKEFILE: In the makefile there is separate compilation of the programs and some other actions.

There are flags the compiler is the gcc and I build the sources, objs and target as variables so if I decide to cahnge the name, it won't be needed to do it manually
make run_m: in order to run the manager. this command makes a file with the name "manager_logfile.txt" for manager log and the configuration file must have the name "config_file.txt" otherwise it needs to be changed manually for now the -n is 1. Same rules apply for make valgrind_m
make run_c: in order to run the console. It produces a file with the name "console-logfile.txt". same applies for make valgrind_c
no command for worker as to start as I don't need to call him as a program individually as he is called by the manager

make valgrind_m and make valgrind_c to run the two programms with valgrind so I can observe possible leaks or errors.(until now errors and leaks don't exist)

Then in order to clean the .o and executable files I have:
make clean_c: removes all .o and exe files which are connected with the console
make clean_m: removes all .o and exe files which are connected with the manager
make clean_w: removes all .o and exe files which are connected with the worker
make clean: removes all .o and exe files with fss_in and fss_out but this is checks in the fss_manager.c as well  

In order to compile all the programs just make or make all.

During compile, dependency files are created in the according folders

There is not any make run for script as it is a program which depends on the args of the user so the command to start it may vary 
It is located in the file ./script

In the end, I have uploaded a small test in the folder testing in which someone can test my program. It can be run just with make run_m and make run_c as in the MAKFEILE the given path is the one of my test. Any file which is produced by my actions are saved inside the folder testing so the others file or the main folder of the project doesn't have garbage.

Sources:
lectures' presentations
https://www.qnx.com/developers/docs/6.4.1/neutrino/lib_ref/d/dirent.html#:~:text=Description%3A,d_ino
https://pubs.opengroup.org/onlinepubs/009604599/functions/opendir.html
https://pubs.opengroup.org/onlinepubs/7908799/xsh/readdir.html
