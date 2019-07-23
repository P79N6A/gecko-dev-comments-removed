








































#include "plgetopt.h"

#include "prgc.h"
#include "prinit.h"
#include "prmon.h"
#include "prinrval.h"
#include "private/pprthred.h"

#include <stdio.h>
#include <stdlib.h>

static PRMonitor *mon;
static PRInt32 threads, waiting, iterations;
static PRInt32 scanCount, finalizeCount, freeCount;

PRIntn failed_already=0;
PRIntn debug_mode;


typedef struct Array {
    PRUintn size;
    void *body[1];
} Array;

int arrayTypeIndex;

static void PR_CALLBACK ScanArray(void *a)
{

	scanCount++;
}

static void PR_CALLBACK FinalizeArray(void *a)
{
	
	finalizeCount++;
}

static void PR_CALLBACK FreeArray(void *a)
{

	freeCount++;
}

static Array *NewArray(PRUintn size)
{
    Array *a;

    a = (Array *)PR_AllocMemory(sizeof(Array) + size*sizeof(void*) - 1*sizeof(void*),
                       arrayTypeIndex, PR_ALLOC_CLEAN);

	

    if (a)
        a->size = size;
    return a;
}

GCType arrayType = {
    ScanArray,
    FinalizeArray,
    0,
    0,
    FreeArray,
    0
};

static void Initialize(void)
{
    PR_InitGC(0, 0, 0, PR_GLOBAL_THREAD);
    arrayTypeIndex = PR_RegisterType(&arrayType);
}

static void PR_CALLBACK AllocateLikeMad(void *arg)
{
    Array *prev;
    PRInt32 i;
	PRInt32 count;

	count = (PRInt32)arg;
    prev = 0;
    for (i = 0; i < count; i++) {
        Array *leak = NewArray(i & 511);
        if ((i & 1023) == 0) {
            prev = 0;                   
        } else {
            if (i & 1) {
                prev = leak;            
            }
        }
    }
    PR_EnterMonitor(mon);
    waiting++;
    PR_Notify(mon);
    PR_ExitMonitor(mon);
}

int main(int argc, char **argv)
{
    PRIntervalTime start, stop, usec;
    double d;
    PRIntn i, totalIterations;
	





	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "dt:c:");

	threads = 10;
	iterations = 100;

	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
        if (PL_OPT_BAD == os) {
            fprintf(stderr, "Invalid command-line option\n");
            exit(1);
        }
        switch (opt->option)
        {
        case 'd':  
			debug_mode = 1;
            break;
        case 't':  
            threads = atoi(opt->value);
            break;
        case 'c':  
            iterations = atoi(opt->value);
            break;
        default:
            break;
        }
    }
	PL_DestroyOptState(opt);

    fprintf(stderr, "t is %ld, i is %ld\n", (long) threads, (long) iterations);
	

    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 5);
    PR_STDIO_INIT();
    Initialize();

    
    start = PR_IntervalNow();
    mon = PR_NewMonitor();
    PR_EnterMonitor(mon);
    waiting = 0;
    for (i = 0; i < threads; i++) {
        (void) PR_CreateThreadGCAble(PR_USER_THREAD,
                               AllocateLikeMad, (void*)iterations,
						      PR_PRIORITY_NORMAL,
						      PR_LOCAL_THREAD,
		    				  PR_UNJOINABLE_THREAD,
						      0);
    }
    while (waiting != threads) {
        PR_Wait(mon, PR_INTERVAL_NO_TIMEOUT);
    }
    PR_ExitMonitor(mon);

	PR_GC();
	PR_ForceFinalize();	

	totalIterations = iterations * threads;












    stop = PR_IntervalNow();
    
    usec = stop = stop - start;
    d = (double)usec;

    if (debug_mode) printf("%40s: %6.2f usec\n", "GC allocation", d / (iterations * threads));
	else {
		if (d == 0.0) failed_already = PR_TRUE;

	}

    PR_Cleanup();
	if(failed_already)	
	{
	    printf("FAIL\n");
		return 1;
	}
	else
	{
	    printf("PASS\n");
		return 0;
	}
}
