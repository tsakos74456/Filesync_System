# Compiler and flags
CC      = gcc
CFLAGS  = -Wall -Wextra -g -MMD

# Files
SRCS_MANAGER = manager/fss_manager.c lib/ADTQueue.c manager/functions_manager.c lib/ADTList.c 
SRCS_CONSOLE = console/fss_console.c console/functions_console.c
SRCS_WORKER =  fss_worker/fss_worker.c fss_worker/functions_worker.c

OBJS_MANAGER = $(SRCS_MANAGER:.c=.o)
OBJS_CONSOLE = $(SRCS_CONSOLE:.c=.o)
OBJS_WORKER = $(SRCS_WORKER:.c=.o)

TARGET_MANAGER = fss_manager
TARGET_CONSOLE = fss_console
TARGET_WORKER = worker

# Default target: build the executable
all: $(TARGET_CONSOLE) $(TARGET_MANAGER) $(TARGET_WORKER)

# Link object files into the executable related to fss_manager
$(TARGET_MANAGER): $(OBJS_MANAGER)
	$(CC) $(CFLAGS) -o $@ $^

# Run the fss_manager
run_m: $(TARGET_MANAGER) $(TARGET_WORKER)
	./$(TARGET_MANAGER) -l ./testing/manager_logfile.txt -c ./testing/config_file.txt -n 1


# Run the fss_manager using valgrind for memory checking
valgrind_m: $(TARGET_MANAGER) $(TARGET_WORKER)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET_MANAGER) -l ./testing/manager_logfile.txt -c ./testing/config_file.txt -n 1

# Clean up compiled files related to fss_manager
clean_m:
	rm -f $(OBJS_MANAGER) $(TARGET_MANAGER) $(OBJS_MANAGER:.o=.d)


# Link object files into the executable related to console
$(TARGET_CONSOLE): $(OBJS_CONSOLE)
	$(CC) $(CFLAGS) -o $@ $^

# Run the fss_console
run_c: $(TARGET_CONSOLE)
	./$(TARGET_CONSOLE) -l ./testing/console-logfile.txt

# Run the fss_console using valgrind for memory checking
valgrind_c: $(TARGET_CONSOLE)
	valgrind --leak-check=full --show-leak-kinds=all ./$(TARGET_CONSOLE) -l ./testing/console-logfile.txt

# Clean up compiled files related to fss_console
clean_c:
	rm -f $(OBJS_CONSOLE) $(TARGET_CONSOLE) $(OBJS_CONSOLE:.o=.d)
	
# Link object files into the executable related to fss_worker but they are a part of manager as well
$(TARGET_WORKER): $(OBJS_WORKER)
	$(CC) $(CFLAGS) -o $@ $^

clean_w:
	rm -f $(TARGET_WORKER) $(OBJS_WORKER) $(OBJS_WORKER:.o=.d)

# Clean up compiled files
clean:
	$(MAKE) clean_c	
	$(MAKE) clean_m
	$(MAKE) clean_w
	rm -f fss_in fss_out 
	rm -f ./testing/manager_logfile.txt ./testing/console-logfile.txt

#dependency files
-include $(OBJS_MANAGER:.o=.d) $(OBJS_CONSOLE:.o=.d) $(OBJS_WORKER:.o=.d)

# commands names
.PHONY: all clean clean_m clean_c clean_w run_m run_c valgrind_m valgrind_c