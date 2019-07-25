



























#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <ctype.h>
#include <signal.h>
#include <sys/mman.h>
#include <errno.h>

#include "linker.h"

#include <sys/socket.h>
#include <cutils/sockets.h>

void notify_gdb_of_libraries();

#define  RETRY_ON_EINTR(ret,cond) \
    do { \
        ret = (cond); \
    } while (ret < 0 && errno == EINTR)

void debugger_signal_handler(int n)
{
    unsigned tid;
    int s;

    
    signal(SIGUSR1, SIG_IGN);

    tid = gettid();
    s = socket_local_client("android:debuggerd",
            ANDROID_SOCKET_NAMESPACE_ABSTRACT, SOCK_STREAM);

    if(s >= 0) {
        




        int  ret;

        RETRY_ON_EINTR(ret, write(s, &tid, sizeof(unsigned)));
        if (ret == sizeof(unsigned)) {
            

            RETRY_ON_EINTR(ret, read(s, &tid, 1));
            notify_gdb_of_libraries();
        }
        close(s);
    }

    
    signal(n, SIG_IGN);
}

void debugger_init()
{
    signal(SIGILL, debugger_signal_handler);
    signal(SIGABRT, debugger_signal_handler);
    signal(SIGBUS, debugger_signal_handler);
    signal(SIGFPE, debugger_signal_handler);
    signal(SIGSEGV, debugger_signal_handler);
    signal(SIGSTKFLT, debugger_signal_handler);
    signal(SIGPIPE, debugger_signal_handler);
}
