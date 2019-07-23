


























































#include "plgetopt.h"

#include "prinit.h"
#include "prthread.h"
#include "prlock.h"
#include "prcvar.h"
#include "prmem.h"
#include "prinrval.h"
#include "prio.h"

#include <string.h>
#include <stdio.h>

static PRIntn debug_mode = 0;
static PRIntn failed_already=0;
static PRThreadScope thread_scope = PR_LOCAL_THREAD;

typedef enum {sg_go, sg_stop, sg_done} Action;
typedef enum {sg_okay, sg_open, sg_close, sg_delete, sg_write, sg_seek} Problem;

typedef struct Hammer_s {
    PRLock *ml;
    PRCondVar *cv;
    PRUint32 id;
    PRUint32 limit;
    PRUint32 writes;
    PRThread *thread;
    PRIntervalTime timein;
    Action action;
    Problem problem;
} Hammer_t;

#define DEFAULT_LIMIT		10
#define DEFAULT_THREADS		2
#define DEFAULT_LOOPS		1

static PRInt32 pageSize = 1024;
static const char* baseName = "./";
static const char *programName = "Random File";




















static PRUint32 RandomNum(void)
{
    PRUint32 rv;
    PRUint64 shift;
    static PRFloat64 seed = 0x58a9382;  
    PRFloat64 random = seed * (PRFloat64)PR_IntervalNow();
    LL_USHR(shift, *((PRUint64*)&random), 16);
    LL_L2UI(rv, shift);
    seed = (PRFloat64)rv;
    return rv;
}  


























static void PR_CALLBACK Thread(void *arg)
{
    PRUint32 index;
    char filename[30];
    const char zero = 0;
    PRFileDesc *file = NULL;
    PRStatus rv = PR_SUCCESS;
    Hammer_t *cd = (Hammer_t*)arg;

    (void)sprintf(filename, "%ssg%04ld.dat", baseName, cd->id);

    if (debug_mode) printf("Starting work on %s\n", filename);

    while (PR_TRUE)
    {
        PRUint32 bytes;
        PRUint32 minor = (RandomNum() % cd->limit) + 1;
        PRUint32 random = (RandomNum() % cd->limit) + 1;
        PRUint32 pages = (RandomNum() % cd->limit) + 10;
        while (minor-- > 0)
        {
            cd->problem = sg_okay;
            if (cd->action != sg_go) goto finished;
            cd->problem = sg_open;
            file = PR_Open(filename, PR_RDWR|PR_CREATE_FILE, 0666);
            if (file == NULL) goto finished;
            for (index = 0; index < pages; index++)
            {
                cd->problem = sg_okay;
                if (cd->action != sg_go) goto close;
                cd->problem = sg_seek;
                bytes = PR_Seek(file, pageSize * index, PR_SEEK_SET);
                if (bytes != pageSize * index) goto close;
                cd->problem = sg_write;
                bytes = PR_Write(file, &zero, sizeof(zero));
                if (bytes <= 0) goto close;
                cd->writes += 1;
            }
            cd->problem = sg_close;
            rv = PR_Close(file);
            if (rv != PR_SUCCESS) goto purge;

            cd->problem = sg_okay;
            if (cd->action != sg_go) goto purge;

            cd->problem = sg_open;
            file = PR_Open(filename, PR_RDWR, 0666);
            for (index = 0; index < pages; index++)
            {
                cd->problem = sg_okay;
                if (cd->action != sg_go) goto close;
                cd->problem = sg_seek;
                bytes = PR_Seek(file, pageSize * index, PR_SEEK_SET);
                if (bytes != pageSize * index) goto close;
                cd->problem = sg_write;
                bytes = PR_Write(file, &zero, sizeof(zero));
                if (bytes <= 0) goto close;
                cd->writes += 1;
                random = (random + 511) % pages;
            }
            cd->problem = sg_close;
            rv = PR_Close(file);
            if (rv != PR_SUCCESS) goto purge;
            cd->problem = sg_delete;
            rv = PR_Delete(filename);
            if (rv != PR_SUCCESS) goto finished;
       }
    }

close:
    (void)PR_Close(file);
purge:
    (void)PR_Delete(filename);
finished:
    PR_Lock(cd->ml);
    cd->action = sg_done;
    PR_NotifyCondVar(cd->cv);
    PR_Unlock(cd->ml);

    if (debug_mode) printf("Ending work on %s\n", filename);

    return;
}  

static Hammer_t hammer[100];
static PRCondVar *cv;






























int main(int argc, char **argv)
{
    PRLock *ml;
    PRUint32 id = 0;
    int active, poll;
    PRIntervalTime interleave;
    PRIntervalTime duration = 0;
    int limit = 0, loops = 0, threads = 0, times;
    PRUint32 writes, writesMin = 0x7fffffff, writesTot = 0, durationTot = 0, writesMax = 0;

    const char *where[] = {"okay", "open", "close", "delete", "write", "seek"};

	





	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "Gdl:t:i:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'G':  
			thread_scope = PR_GLOBAL_THREAD;
            break;
        case 'd':  
			debug_mode = 1;
            break;
        case 'l':  
			limit = atoi(opt->value);
            break;
        case 't':  
			threads = atoi(opt->value);
            break;
        case 'i':  
			loops = atoi(opt->value);
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

 
	
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    interleave = PR_SecondsToInterval(10);

    ml = PR_NewLock();
    cv = PR_NewCondVar(ml);

    if (loops == 0) loops = DEFAULT_LOOPS;
    if (limit == 0) limit = DEFAULT_LIMIT;
    if (threads == 0) threads = DEFAULT_THREADS;

    if (debug_mode) printf(
        "%s: Using loops = %d, threads = %d, limit = %d and %s threads\n",
        programName, loops, threads, limit,
        (thread_scope == PR_LOCAL_THREAD) ? "LOCAL" : "GLOBAL");

    for (times = 0; times < loops; ++times)
    {
        if (debug_mode) printf("%s: Setting concurrency level to %d\n", programName, times + 1);
        PR_SetConcurrency(times + 1);
        for (active = 0; active < threads; active++)
        {
            hammer[active].ml = ml;
            hammer[active].cv = cv;
            hammer[active].id = id++;
            hammer[active].writes = 0;
            hammer[active].action = sg_go;
            hammer[active].problem = sg_okay;
            hammer[active].limit = (RandomNum() % limit) + 1;
            hammer[active].timein = PR_IntervalNow();
            hammer[active].thread = PR_CreateThread(
                PR_USER_THREAD, Thread, &hammer[active],
                PR_GetThreadPriority(PR_GetCurrentThread()),
                thread_scope, PR_JOINABLE_THREAD, 0);

            PR_Lock(ml);
            PR_WaitCondVar(cv, interleave);  
            PR_Unlock(ml);
        }

        



        PR_Lock(ml);
        for (poll = 0; poll < threads; poll++)
        {
            if (hammer[poll].action == sg_go)  
                hammer[poll].action = sg_stop;  
        }
        PR_Unlock(ml);

        while (active > 0)
        {
            for (poll = 0; poll < threads; poll++)
            {
                PR_Lock(ml);
                while (hammer[poll].action < sg_done)
                    PR_WaitCondVar(cv, PR_INTERVAL_NO_TIMEOUT);
                PR_Unlock(ml);

                active -= 1;  
                (void)PR_JoinThread(hammer[poll].thread);
                hammer[poll].thread = NULL;
                if (hammer[poll].problem == sg_okay)
                {
                    duration = PR_IntervalToMilliseconds(
                        PR_IntervalNow() - hammer[poll].timein);
                    writes = hammer[poll].writes * 1000 / duration;
                    if (writes < writesMin) 
                        writesMin = writes;
                    if (writes > writesMax) 
                        writesMax = writes;
                    writesTot += hammer[poll].writes;
                    durationTot += duration;
                }
                else
                    if (debug_mode) printf(
                        "%s: test failed %s after %ld seconds\n",
                        programName, where[hammer[poll].problem], duration);
					else failed_already=1;
            }
        }
    }
    if (debug_mode) printf(
        "%s: [%ld [%ld] %ld] writes/sec average\n",
        programName, writesMin, writesTot * 1000 / durationTot, writesMax);

    PR_DestroyCondVar(cv);
    PR_DestroyLock(ml);

	if (failed_already) 
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
