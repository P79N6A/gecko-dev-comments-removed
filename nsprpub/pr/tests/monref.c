










#include "prlog.h"
#include "prmon.h"
#include "prthread.h"

#include <stdio.h>
#include <stdlib.h>


static PRBool done = PR_FALSE;

static void ThreadFunc(void *arg)
{
    PRMonitor *mon = (PRMonitor *)arg;
    PRStatus rv;

    PR_EnterMonitor(mon);
    done = PR_TRUE;
    rv = PR_Notify(mon);
    PR_ASSERT(rv == PR_SUCCESS);
    rv = PR_ExitMonitor(mon);
    PR_ASSERT(rv == PR_SUCCESS);
}

int main()
{
    PRMonitor *mon;
    PRThread *thread;
    PRStatus rv;

    mon = PR_NewMonitor();
    if (!mon) {
        fprintf(stderr, "PR_NewMonitor failed\n");
        exit(1);
    }

    thread = PR_CreateThread(PR_USER_THREAD, ThreadFunc, mon,
                             PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD,
                             PR_JOINABLE_THREAD, 0);
    if (!thread) {
        fprintf(stderr, "PR_CreateThread failed\n");
        exit(1);
    }

    PR_EnterMonitor(mon);
    while (!done) {
        rv = PR_Wait(mon, PR_INTERVAL_NO_TIMEOUT);
        PR_ASSERT(rv == PR_SUCCESS);
    }
    rv = PR_ExitMonitor(mon);
    PR_ASSERT(rv == PR_SUCCESS);

    



    PR_DestroyMonitor(mon);

    rv = PR_JoinThread(thread);
    PR_ASSERT(rv == PR_SUCCESS);

    printf("PASS\n");
    return 0;
}
