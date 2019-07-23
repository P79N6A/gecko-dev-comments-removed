
























































#include "plgetopt.h"
#include "prttools.h"

#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


















static void Test_Result (int result)
{
    if (result == PASS)
        printf ("PASS\n");
    else
        printf ("FAIL\n");
    exit (1);
}









static void PR_CALLBACK lowPriority(void *arg)
{
}

static void PR_CALLBACK highPriority(void *arg)
{
}

static void PR_CALLBACK unjoinable(void *arg)
{
    PR_Sleep(PR_INTERVAL_NO_TIMEOUT);
}

void runTest(PRThreadScope scope1, PRThreadScope scope2)
{
    PRThread *low,*high;

    
    
    low = PR_CreateThread(PR_USER_THREAD,
                     lowPriority, 0, 
                     PR_PRIORITY_LOW,
                     scope1,
                     PR_JOINABLE_THREAD,
                     0);
    if (!low) {
        if (debug_mode) printf("\tcannot create low priority thread\n");
        else Test_Result(FAIL);
        return;
    }

    high = PR_CreateThread(PR_USER_THREAD,
                     highPriority, 0, 
                     PR_PRIORITY_HIGH,
                     scope2,
                     PR_JOINABLE_THREAD,
                     0);
    if (!high) {
        if (debug_mode) printf("\tcannot create high priority thread\n");
        else Test_Result(FAIL);
        return;
    }

    
    if (PR_JoinThread(low) == PR_FAILURE) {
        if (debug_mode) printf("\tcannot join low priority thread\n");
        else Test_Result (FAIL);
        return;
    } else {
        if (debug_mode) printf("\tjoined low priority thread\n");
    }
    if (PR_JoinThread(high) == PR_FAILURE) {
        if (debug_mode) printf("\tcannot join high priority thread\n");
        else Test_Result(FAIL);
        return;
    } else {
        if (debug_mode) printf("\tjoined high priority thread\n");
    }
}

void joinWithUnjoinable(void)
{
    PRThread *thread;

    
    
    thread = PR_CreateThread(PR_USER_THREAD,
                     unjoinable, 0, 
                     PR_PRIORITY_NORMAL,
                     PR_GLOBAL_THREAD,
                     PR_UNJOINABLE_THREAD,
                     0);
    if (!thread) {
        if (debug_mode) printf("\tcannot create unjoinable thread\n");
        else Test_Result(FAIL);
        return;
    }

    if (PR_JoinThread(thread) == PR_SUCCESS) {
        if (debug_mode) printf("\tsuccessfully joined with unjoinable thread?!\n");
        else Test_Result(FAIL);
        return;
    } else {
        if (debug_mode) printf("\tcannot join with unjoinable thread, as expected\n");
        if (PR_GetError() != PR_INVALID_ARGUMENT_ERROR) {
            if (debug_mode) printf("\tWrong error code\n");
            else Test_Result(FAIL);
            return;
        }
    }
    if (PR_Interrupt(thread) == PR_FAILURE) {
        if (debug_mode) printf("\tcannot interrupt unjoinable thread\n");
        else Test_Result(FAIL);
        return;
    } else {
        if (debug_mode) printf("\tinterrupted unjoinable thread\n");
    }
}

static PRIntn PR_CALLBACK RealMain(int argc, char **argv)
{
    





    
    PLOptStatus os;
    PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  
            debug_mode = 1;
            break;
         default:
            break;
        }
    }
    PL_DestroyOptState(opt);

 
    printf("User-User test\n");
    runTest(PR_LOCAL_THREAD, PR_LOCAL_THREAD);
    printf("User-Kernel test\n");
    runTest(PR_LOCAL_THREAD, PR_GLOBAL_THREAD);
    printf("Kernel-User test\n");
    runTest(PR_GLOBAL_THREAD, PR_LOCAL_THREAD);
    printf("Kernel-Kernel test\n");
    runTest(PR_GLOBAL_THREAD, PR_GLOBAL_THREAD);
    printf("Join with unjoinable thread\n");
    joinWithUnjoinable();

    printf("PASSED\n");

    return 0;
}




int main(int argc, char **argv)
{
    PRIntn rv;
    
    PR_STDIO_INIT();
    rv = PR_Initialize(RealMain, argc, argv, 0);
    return rv;
}  
