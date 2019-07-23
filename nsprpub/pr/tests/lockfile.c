























































#include "plgetopt.h"

#include "prcmon.h"
#include "prerror.h"
#include "prinit.h"
#include "prinrval.h"
#include "prlock.h"
#include "prlog.h"
#include "prmon.h"
#include "prthread.h"
#include "prtypes.h"

#include "private/pprio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

PRIntn failed_already=0;
PRIntn debug_mode;

const static PRIntervalTime contention_interval = 50;

typedef struct LockContentious_s {
    PRLock *ml;
    PRInt32 loops;
    PRIntervalTime overhead;
    PRIntervalTime interval;
} LockContentious_t;

#define LOCKFILE "prlock.fil"



static PRIntervalTime NonContentiousLock(PRInt32 loops)
{
    PRFileDesc *_lockfile;
    while (loops-- > 0)
    {
        _lockfile = PR_Open(LOCKFILE, PR_CREATE_FILE|PR_RDWR, 0666);
        if (!_lockfile) {
            if (debug_mode) printf(
                "could not create lockfile: %d [%d]\n",
                PR_GetError(), PR_GetOSError());
            return PR_INTERVAL_NO_TIMEOUT;
        }
        PR_LockFile(_lockfile);
        PR_UnlockFile(_lockfile);
        PR_Close(_lockfile);
    }
    return 0;
}  

static void PR_CALLBACK LockContender(void *arg)
{
    LockContentious_t *contention = (LockContentious_t*)arg;
    PRFileDesc *_lockfile;
    while (contention->loops-- > 0)
    {
        _lockfile = PR_Open(LOCKFILE, PR_CREATE_FILE|PR_RDWR, 0666);
        if (!_lockfile) {
            if (debug_mode) printf(
                "could not create lockfile: %d [%d]\n",
                PR_GetError(), PR_GetOSError());
            break;
        }
        PR_LockFile(_lockfile);
        PR_Sleep(contention->interval);
        PR_UnlockFile(_lockfile);
        PR_Close(_lockfile);
    }

}  




static LockContentious_t contention;

static PRIntervalTime ContentiousLock(PRInt32 loops)
{
    PRStatus status;
    PRThread *thread = NULL;
    PRIntervalTime overhead, timein = PR_IntervalNow();

    contention.loops = loops;
    contention.overhead = 0;
    contention.ml = PR_NewLock();
    contention.interval = contention_interval;
    thread = PR_CreateThread(
        PR_USER_THREAD, LockContender, &contention,
        PR_PRIORITY_LOW, PR_LOCAL_THREAD, PR_JOINABLE_THREAD, 0);
    PR_ASSERT(thread != NULL);

    overhead = PR_IntervalNow() - timein;

    while (contention.loops > 0)
    {
        PR_Lock(contention.ml);
        contention.overhead += contention.interval;
        PR_Sleep(contention.interval);
        PR_Unlock(contention.ml);
    }

    timein = PR_IntervalNow();
    status = PR_JoinThread(thread);
    PR_DestroyLock(contention.ml);
    overhead += (PR_IntervalNow() - timein);
    return overhead + contention.overhead;
}  

static PRIntervalTime Test(
    const char* msg, PRIntervalTime (*test)(PRInt32 loops),
    PRInt32 loops, PRIntervalTime overhead)
{ 
    







    PRFloat64 elapsed;
    PRIntervalTime accountable, duration;    
    PRUintn spaces = strlen(msg);
    PRIntervalTime timeout, timein = PR_IntervalNow();
    PRIntervalTime predicted = test(loops);
    timeout = PR_IntervalNow();
    duration = timeout - timein;
    accountable = duration - predicted;
    accountable -= overhead;
    elapsed = (PRFloat64)PR_IntervalToMicroseconds(accountable);
    if (debug_mode) printf("%s:", msg);
    while (spaces++ < 50) if (debug_mode) printf(" ");
    if ((PRInt32)accountable < 0) {
        if (debug_mode) printf("*****.** usecs/iteration\n");
    } else {
        if (debug_mode) printf("%8.2f usecs/iteration\n", elapsed/loops);
    }
    return duration;
}  

int main(int argc,  char **argv)
{
    PRIntervalTime duration;
    PRUint32 cpu, cpus = 2;
    PRInt32 loops = 100;

	
	





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

 
	
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    if (argc > 1) loops = atoi(argv[1]);
    if (loops == 0) loops = 100;
    if (debug_mode) printf("Lock: Using %d loops\n", loops);

    cpus = (argc < 3) ? 2 : atoi(argv[2]);
    if (cpus == 0) cpus = 2;
    if (debug_mode) printf("Lock: Using %d cpu(s)\n", cpus);


    for (cpu = 1; cpu <= cpus; ++cpu)
    {
        if (debug_mode) printf("\nLockFile: Using %d CPU(s)\n", cpu);
        PR_SetConcurrency(cpu);
        
        duration = Test("LockFile non-contentious locking/unlocking", NonContentiousLock, loops, 0);
        (void)Test("LockFile contentious locking/unlocking", ContentiousLock, loops, duration);
    }

    PR_Delete(LOCKFILE);  

    if (debug_mode) printf("%s: test %s\n", "Lock(mutex) test", ((failed_already) ? "failed" : "passed"));
	if(failed_already)	
		return 1;
	else
		return 0;
}  


