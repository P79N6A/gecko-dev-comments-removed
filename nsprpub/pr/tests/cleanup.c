




































#include "prprf.h"
#include "prio.h"
#include "prinit.h"
#include "prthread.h"
#include "prinrval.h"

#include "plgetopt.h"

#include <stdlib.h>

static void PR_CALLBACK Thread(void *sleep)
{
    PR_Sleep(PR_SecondsToInterval((PRUint32)sleep));
    printf("Thread exiting\n");
}

static void Help(void)
{
    PRFileDesc *err = PR_GetSpecialFD(PR_StandardError);
    PR_fprintf(err, "Cleanup usage: [-g] [-s n] [-t n] [-c n] [-h]\n");
    PR_fprintf(err, "\t-c   Call cleanup before exiting     (default: false)\n");
    PR_fprintf(err, "\t-G   Use global threads only         (default: local)\n");
    PR_fprintf(err, "\t-t n Number of threads involved      (default: 1)\n");
    PR_fprintf(err, "\t-s n Seconds thread(s) should dally  (defaut: 10)\n");
    PR_fprintf(err, "\t-S n Seconds main() should dally     (defaut: 5)\n");
    PR_fprintf(err, "\t-C n Value to set concurrency        (default 1)\n");
    PR_fprintf(err, "\t-h   This message and nothing else\n");
}  

int main(int argc, char **argv)
{
    PLOptStatus os;
    PRBool cleanup = PR_FALSE;
	PRThreadScope type = PR_LOCAL_THREAD;
    PRFileDesc *err = PR_GetSpecialFD(PR_StandardError);
    PLOptState *opt = PL_CreateOptState(argc, argv, "Ghs:S:t:cC:");
    PRIntn concurrency = 1, child_sleep = 10, main_sleep = 5, threads = 1;

    PR_STDIO_INIT();
    while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
        if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'c':  
            cleanup = PR_TRUE;
            break;
        case 'G':  
            type = PR_GLOBAL_THREAD;
            break;
        case 's':  
            child_sleep = atoi(opt->value);
            break;
        case 'S':  
            main_sleep = atoi(opt->value);
            break;
        case 'C':  
            concurrency = atoi(opt->value);
            break;
        case 't':  
            threads = atoi(opt->value);
            break;
        case 'h':  
            Help();  
            return 2;  
            break;
         default:
            break;
        }
    }
    PL_DestroyOptState(opt);

    PR_fprintf(err, "Cleanup settings\n");
    PR_fprintf(err, "\tThread type: %s\n",
        (PR_LOCAL_THREAD == type) ? "LOCAL" : "GLOBAL");
    PR_fprintf(err, "\tConcurrency: %d\n", concurrency);
    PR_fprintf(err, "\tNumber of threads: %d\n", threads);
    PR_fprintf(err, "\tThread sleep: %d\n", child_sleep);
    PR_fprintf(err, "\tMain sleep: %d\n", main_sleep); 
    PR_fprintf(err, "\tCleanup will %sbe called\n\n", (cleanup) ? "" : "NOT "); 

    PR_SetConcurrency(concurrency);

	while (threads-- > 0)
		(void)PR_CreateThread(
        	PR_USER_THREAD, Thread, (void*)child_sleep, PR_PRIORITY_NORMAL,
       		type, PR_UNJOINABLE_THREAD, 0);
    PR_Sleep(PR_SecondsToInterval(main_sleep));

    if (cleanup) PR_Cleanup();

    PR_fprintf(err, "main() exiting\n");
    return 0;
}  
