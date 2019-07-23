


























































#include <plgetopt.h>
#include <prprf.h>
#include <prio.h>

#include "rccv.h"
#include "rcthread.h"
#include "rcfileio.h"
#include "rclock.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static PRFileDesc *output;
static PRIntn debug_mode = 0;
static PRIntn failed_already = 0;

class HammerData
{
public:
    typedef enum {
        sg_go, sg_stop, sg_done} Action;
    typedef enum {
        sg_okay, sg_open, sg_close, sg_delete, sg_write, sg_seek} Problem;

	virtual ~HammerData();
	HammerData(RCLock* lock, RCCondition *cond, PRUint32 clip);
    virtual PRUint32 Random();

    Action action;
    Problem problem;
    PRUint32 writes;
    RCInterval timein;
friend class Hammer;
private:
    RCLock *ml;
    RCCondition *cv;
    PRUint32 limit;

    PRFloat64 seed;
};  

class Hammer: public HammerData, public RCThread
{
public:
    virtual ~Hammer();
    Hammer(RCThread::Scope scope, RCLock* lock, RCCondition *cond, PRUint32 clip);

private:
    void RootFunction();

};

static PRInt32 pageSize = 1024;
static const char* baseName = "./";
static const char *programName = "Random File";




















PRUint32 HammerData::Random()
{
    PRUint32 rv;
    PRUint64 shift;
    RCInterval now = RCInterval(RCInterval::now);
    PRFloat64 random = seed * (PRFloat64)((PRIntervalTime)now);
    LL_USHR(shift, *((PRUint64*)&random), 16);
    LL_L2UI(rv, shift);
    seed = (PRFloat64)rv;
    return rv;
}  

Hammer::~Hammer() { }

Hammer::Hammer(
    RCThread::Scope scope, RCLock* lock, RCCondition *cond, PRUint32 clip):
	HammerData(lock, cond, clip), RCThread(scope, RCThread::joinable, 0) { }

HammerData::~HammerData() { }

HammerData::HammerData(RCLock* lock, RCCondition *cond, PRUint32 clip)
{
    ml = lock;
    cv = cond;
    writes = 0;
    limit = clip;
    seed = 0x58a9382;
    action = HammerData::sg_go;
    problem = HammerData::sg_okay;
    timein = RCInterval(RCInterval::now);
}  



























void Hammer::RootFunction()
{
    PRUint32 index;
    RCFileIO file;
    char filename[30];
    const char zero = 0;
    PRStatus rv = PR_SUCCESS;

    limit = (Random() % limit) + 1;

    (void)sprintf(filename, "%ssg%04p.dat", baseName, this);

    if (debug_mode) PR_fprintf(output, "Starting work on %s\n", filename);

    while (PR_TRUE)
    {
        PRUint64 bytes;
        PRUint32 minor = (Random() % limit) + 1;
        PRUint32 random = (Random() % limit) + 1;
        PRUint32 pages = (Random() % limit) + 10;
        while (minor-- > 0)
        {
            problem = sg_okay;
            if (action != sg_go) goto finished;
            problem = sg_open;
            rv = file.Open(filename, PR_RDWR|PR_CREATE_FILE, 0666);
            if (PR_FAILURE == rv) goto finished;
            for (index = 0; index < pages; index++)
            {
                problem = sg_okay;
                if (action != sg_go) goto close;
                problem = sg_seek;
                bytes = file.Seek(pageSize * index, RCFileIO::set);
                if (bytes != pageSize * index) goto close;
                problem = sg_write;
                bytes = file.Write(&zero, sizeof(zero));
                if (bytes <= 0) goto close;
                writes += 1;
            }
            problem = sg_close;
            rv = file.Close();
            if (rv != PR_SUCCESS) goto purge;

            problem = sg_okay;
            if (action != sg_go) goto purge;

            problem = sg_open;
            rv = file.Open(filename, PR_RDWR, 0666);
            if (PR_FAILURE == rv) goto finished;
            for (index = 0; index < pages; index++)
            {
                problem = sg_okay;
                if (action != sg_go) goto close;
                problem = sg_seek;
                bytes = file.Seek(pageSize * index, RCFileIO::set);
                if (bytes != pageSize * index) goto close;
                problem = sg_write;
                bytes = file.Write(&zero, sizeof(zero));
                if (bytes <= 0) goto close;
                writes += 1;
                random = (random + 511) % pages;
            }
            problem = sg_close;
            rv = file.Close();
            if (rv != PR_SUCCESS) goto purge;
            problem = sg_delete;
            rv = file.Delete(filename);
            if (rv != PR_SUCCESS) goto finished;
       }
    }

close:
    (void)file.Close();
purge:
    (void)file.Delete(filename);
finished:
    RCEnter scope(ml);
    action = HammerData::sg_done;
    cv->Notify();

    if (debug_mode) PR_fprintf(output, "Ending work on %s\n", filename);

    return;
}  

static Hammer* hammer[100];






























PRIntn main (PRIntn argc, char *argv[])
{
    RCLock ml;
	PLOptStatus os;
    RCCondition cv(&ml);
    PRUint32 writesMax = 0, durationTot = 0;
    RCThread::Scope thread_scope = RCThread::local;
    PRUint32 writes, writesMin = 0x7fffffff, writesTot = 0;
    PRIntn active, poll, limit = 0, max_virtual_procs = 0, threads = 0, virtual_procs;
    RCInterval interleave(RCInterval::FromMilliseconds(10000)), duration(0);

    const char *where[] = {"okay", "open", "close", "delete", "write", "seek"};

	PLOptState *opt = PL_CreateOptState(argc, argv, "Gdl:t:i:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
	case 0:
		baseName = opt->value;
		break;
        case 'G':  
		thread_scope = RCThread::global;
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
			max_virtual_procs = atoi(opt->value);
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);
    output = PR_GetSpecialFD(PR_StandardOutput);

 
 
    cv.SetTimeout(interleave);
	
    if (max_virtual_procs == 0) max_virtual_procs = 2;
    if (limit == 0) limit = 57;
    if (threads == 0) threads = 10;

    if (debug_mode) PR_fprintf(output,
        "%s: Using %d virtual processors, %d threads, limit = %d and %s threads\n",
        programName, max_virtual_procs, threads, limit,
        (thread_scope == RCThread::local) ? "LOCAL" : "GLOBAL");

    for (virtual_procs = 0; virtual_procs < max_virtual_procs; ++virtual_procs)
    {
        if (debug_mode)
			PR_fprintf(output,
				"%s: Setting number of virtual processors to %d\n",
				programName, virtual_procs + 1);
		RCPrimordialThread::SetVirtualProcessors(virtual_procs + 1);
        for (active = 0; active < threads; active++)
        {
            hammer[active] = new Hammer(thread_scope, &ml, &cv, limit);
            hammer[active]->Start();  
            RCThread::Sleep(interleave);  
        }

        



        {
            RCEnter scope(&ml);
            for (poll = 0; poll < threads; poll++)
            {
                if (hammer[poll]->action == HammerData::sg_go)  
                    hammer[poll]->action = HammerData::sg_stop;  
            }
        }

        while (active > 0)
        {
            for (poll = 0; poll < threads; poll++)
            {
                ml.Acquire();
                while (hammer[poll]->action < HammerData::sg_done) cv.Wait();
                ml.Release();

                if (hammer[poll]->problem == HammerData::sg_okay)
                {
                    duration = RCInterval(RCInterval::now) - hammer[poll]->timein;
                    writes = hammer[poll]->writes * 1000 / duration;
                    if (writes < writesMin)  writesMin = writes;
                    if (writes > writesMax) writesMax = writes;
                    writesTot += hammer[poll]->writes;
                    durationTot += duration;
                }
                else
                {
                    if (debug_mode) PR_fprintf(output,
                        "%s: test failed %s after %ld seconds\n",
                        programName, where[hammer[poll]->problem], duration);
					else failed_already=1;
                }
                active -= 1;  
                (void)hammer[poll]->Join();
                hammer[poll] = NULL;
            }
        }
        if (debug_mode) PR_fprintf(output,
            "%s: [%ld [%ld] %ld] writes/sec average\n",
            programName, writesMin,
            writesTot * 1000 / durationTot, writesMax);
    }

        failed_already |= (PR_FAILURE == RCPrimordialThread::Cleanup());
	    PR_fprintf(output, "%s\n", (failed_already) ? "FAIL\n" : "PASS\n");
		return failed_already;
}  
