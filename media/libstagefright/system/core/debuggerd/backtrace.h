















#ifndef _DEBUGGERD_BACKTRACE_H
#define _DEBUGGERD_BACKTRACE_H

#include <stddef.h>
#include <stdbool.h>
#include <sys/types.h>

#include <corkscrew/ptrace.h>



void dump_backtrace(int fd, int amfd, pid_t pid, pid_t tid, bool* detach_failed,
        int* total_sleep_time_usec);

#endif 
