








#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "unicode/utypes.h"
#include "unicode/uclean.h"
#include "umutex.h"
#include "threadtest.h"

AbstractThreadTest::~AbstractThreadTest() {}






extern  AbstractThreadTest *createStringTest();
extern  AbstractThreadTest *createConvertTest();








#if U_PLATFORM_USES_ONLY_WIN32_API

#include "Windows.h"
#include "process.h"



typedef void (*ThreadFunc)(void *);

class ThreadFuncs           
{                           
public:
    static void            Sleep(int millis) {::Sleep(millis);};
    static void            startThread(ThreadFunc, void *param);
    static unsigned long   getCurrentMillis();
    static void            yield() {::Sleep(0);};
};

void ThreadFuncs::startThread(ThreadFunc func, void *param)
{
    unsigned long x;
    x = _beginthread(func, 0x10000, param);
    if (x == -1)
    {
        fprintf(stderr, "Error starting thread.  Errno = %d\n", errno);
        exit(-1);
    }
}

unsigned long ThreadFuncs::getCurrentMillis()
{
    return (unsigned long)::GetTickCount();
}





#else






#include <pthread.h>
#include <unistd.h>
#include <errno.h>
#include <sched.h>
#include <sys/timeb.h>


extern "C" {


typedef void (*ThreadFunc)(void *);
typedef void *(*pthreadfunc)(void *);

class ThreadFuncs           
{                           
public:
    static void            Sleep(int millis);
    static void            startThread(ThreadFunc, void *param);
    static unsigned long   getCurrentMillis();
    static void            yield() {sched_yield();};
};

void ThreadFuncs::Sleep(int millis)
{
   int seconds = millis/1000;
   if (seconds <= 0) seconds = 1;
   ::sleep(seconds);
}


void ThreadFuncs::startThread(ThreadFunc func, void *param)
{
    unsigned long x;

    pthread_t tId;
    
#if defined(_HP_UX) && defined(XML_USE_DCE)
    x = pthread_create( &tId, pthread_attr_default,  (pthreadfunc)func,  param);
#else
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    x = pthread_create( &tId, &attr,  (pthreadfunc)func,  param);
#endif
    if (x == -1)
    {
        fprintf(stderr, "Error starting thread.  Errno = %d\n", errno);
        exit(-1);
    }
}

unsigned long ThreadFuncs::getCurrentMillis() {
    timeb aTime;
    ftime(&aTime);
    return (unsigned long)(aTime.time*1000 + aTime.millitm);
}
}




#endif












const int MAXINFILES = 25;
struct RunInfo
{
    bool                quiet;
    bool                verbose;
    int                 numThreads;
    int                 totalTime;
    int                 checkTime;
    AbstractThreadTest *fTest;
    bool                stopFlag;
    bool                exitFlag;
    int32_t             runningThreads;
};










struct ThreadInfo
{
    bool    fHeartBeat;            
                                   
    unsigned int     fCycles;      
    int              fThreadNum;   
    ThreadInfo() {
        fHeartBeat = false;
        fCycles = 0;
        fThreadNum = -1;
    }
};








RunInfo         gRunInfo;
ThreadInfo      *gThreadInfo;
UMTX            gStopMutex;        
UMTX            gInfoMutex;        
                                   














void parseCommandLine(int argc, char **argv)
{
    gRunInfo.quiet = false;               
    gRunInfo.verbose = false;
    gRunInfo.numThreads = 2;
    gRunInfo.totalTime = 0;
    gRunInfo.checkTime = 10;

    try             
    {
        int argnum = 1;
        while (argnum < argc)
        {
            if      (strcmp(argv[argnum], "-quiet") == 0)
                gRunInfo.quiet = true;
            else if (strcmp(argv[argnum], "-verbose") == 0)
                gRunInfo.verbose = true;
            else if (strcmp(argv[argnum], "--help") == 0 ||
                    (strcmp(argv[argnum],     "?")      == 0)) {throw 1; }
                
            else if (strcmp(argv[argnum], "-threads") == 0)
            {
                ++argnum;
                if (argnum >= argc)
                    throw 1;
                gRunInfo.numThreads = atoi(argv[argnum]);
                if (gRunInfo.numThreads < 0)
                    throw 1;
            }
            else if (strcmp(argv[argnum], "-time") == 0)
            {
                ++argnum;
                if (argnum >= argc)
                    throw 1;
                gRunInfo.totalTime = atoi(argv[argnum]);
                if (gRunInfo.totalTime < 1)
                    throw 1;
            }
            else if (strcmp(argv[argnum], "-ctime") == 0)
            {
                ++argnum;
                if (argnum >= argc)
                    throw 1;
                gRunInfo.checkTime = atoi(argv[argnum]);
                if (gRunInfo.checkTime < 1)
                    throw 1;
            }
            else if (strcmp(argv[argnum], "string") == 0)
            {
                gRunInfo.fTest = createStringTest();
            }
            else if (strcmp(argv[argnum], "convert") == 0)
            {
                gRunInfo.fTest = createConvertTest();
            }
           else  
            {
                fprintf(stderr, "Unrecognized command line option.  Scanning \"%s\"\n",
                    argv[argnum]);
                throw 1;
            }
            argnum++;
        }
        
        
        if (gRunInfo.fTest == NULL) {
            fprintf(stderr, "No test specified.\n");
            throw 1;
        }

    }
    catch (int)
    {
        fprintf(stderr, "usage:  threadtest [-threads nnn] [-time nnn] [-quiet] [-verbose] test-name\n"
            "     -quiet         Suppress periodic status display. \n"
            "     -verbose       Display extra messages. \n"
            "     -threads nnn   Number of threads.  Default is 2. \n"
            "     -time nnn      Total time to run, in seconds.  Default is forever.\n"
            "     -ctime nnn     Time between extra consistency checks, in seconds.  Default 10\n"
            "     testname       string | convert\n"
            );
        exit(1);
    }
}













extern "C" {

void threadMain (void *param)
{
    ThreadInfo   *thInfo = (ThreadInfo *)param;

    if (gRunInfo.verbose)
        printf("Thread #%d: starting\n", thInfo->fThreadNum);
    umtx_atomic_inc(&gRunInfo.runningThreads);

    
    
    while (true)
    {
        if (gRunInfo.verbose )
            printf("Thread #%d: starting loop\n", thInfo->fThreadNum);

        
        
        
        
        umtx_lock(&gInfoMutex);
        UBool stop = gRunInfo.stopFlag;  
        umtx_unlock(&gInfoMutex);

        if (stop) {
            if (gRunInfo.verbose) {
                fprintf(stderr, "Thread #%d: suspending\n", thInfo->fThreadNum);
            }
            umtx_atomic_dec(&gRunInfo.runningThreads);
            while (gRunInfo.stopFlag) {
                umtx_lock(&gStopMutex);
                umtx_unlock(&gStopMutex);
            }
            umtx_atomic_inc(&gRunInfo.runningThreads);
            if (gRunInfo.verbose) {
                fprintf(stderr, "Thread #%d: restarting\n", thInfo->fThreadNum);
            }
        }

        
        
        
        gRunInfo.fTest->runOnce();

        umtx_lock(&gInfoMutex);
        thInfo->fHeartBeat = true;
        thInfo->fCycles++;
        UBool exitNow = gRunInfo.exitFlag;
        umtx_unlock(&gInfoMutex);

        
        
        
        if (exitNow) {
            break;
        }
    }
            
    umtx_atomic_dec(&gRunInfo.runningThreads);

    
    return;
}

}










int main (int argc, char **argv)
{
    
    
    
    parseCommandLine(argc, argv);


    
    
    

    if (gRunInfo.numThreads == 0)
        exit(0);

    gRunInfo.exitFlag = FALSE;
    gRunInfo.stopFlag = TRUE;      
    umtx_lock(&gStopMutex);

    gThreadInfo = new ThreadInfo[gRunInfo.numThreads];
    int threadNum;
    for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++)
    {
        gThreadInfo[threadNum].fThreadNum = threadNum;
        ThreadFuncs::startThread(threadMain, &gThreadInfo[threadNum]);
    }


    unsigned long startTime = ThreadFuncs::getCurrentMillis();
    int elapsedSeconds = 0;
    int timeSinceCheck = 0;

    
    
    
    gRunInfo.stopFlag = FALSE;       
    umtx_unlock(&gStopMutex);      

    
    
    
    
    
    
    
    
    
    while (gRunInfo.totalTime == 0 || gRunInfo.totalTime > elapsedSeconds)
    {
        ThreadFuncs::Sleep(1000);      

        if (gRunInfo.quiet == false && gRunInfo.verbose == false)
        {
            char c = '+';
            int threadNum;
            umtx_lock(&gInfoMutex);
            for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++)
            {
                if (gThreadInfo[threadNum].fHeartBeat == false)
                {
                    c = '.';
                    break;
                };
            }
            umtx_unlock(&gInfoMutex);
            fputc(c, stdout);
            fflush(stdout);
            if (c == '+')
                for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++)
                    gThreadInfo[threadNum].fHeartBeat = false;
        }

        
        
        
        timeSinceCheck -= elapsedSeconds;
        elapsedSeconds = (ThreadFuncs::getCurrentMillis() - startTime) / 1000;
        timeSinceCheck += elapsedSeconds;

        
        
        
        if (timeSinceCheck >= gRunInfo.checkTime) {
            if (gRunInfo.verbose) {
                fprintf(stderr, "Main: suspending all threads\n");
            }
            umtx_lock(&gStopMutex);               
            gRunInfo.stopFlag = TRUE;
            for (;;) {
                umtx_lock(&gInfoMutex);
                UBool done = gRunInfo.runningThreads == 0;
                umtx_unlock(&gInfoMutex);
                if (done) { break;}
                ThreadFuncs::yield();
            }


            
            gRunInfo.fTest->check();
            if (gRunInfo.quiet == false && gRunInfo.verbose == false) {
                fputc('C', stdout);
            }

            if (gRunInfo.verbose) {
                fprintf(stderr, "Main: starting all threads.\n");
            }
            gRunInfo.stopFlag = FALSE;       
            umtx_unlock(&gStopMutex);      
            timeSinceCheck = 0;
        }
    };

    
    
    
    
    gRunInfo.exitFlag = true;
    for (;;) {
        umtx_lock(&gInfoMutex);
        UBool done = gRunInfo.runningThreads == 0;
        umtx_unlock(&gInfoMutex);
        if (done) { break;}
        ThreadFuncs::yield();
    }

    
    
    
    double totalCyclesCompleted = 0;
    for (threadNum=0; threadNum < gRunInfo.numThreads; threadNum++) {
        totalCyclesCompleted += gThreadInfo[threadNum].fCycles;
    }

    double cyclesPerMinute = totalCyclesCompleted / (double(gRunInfo.totalTime) / double(60));
    printf("\n%8.1f cycles per minute.", cyclesPerMinute);

    
    
    
    delete gRunInfo.fTest;
    delete [] gThreadInfo;
    umtx_destroy(&gInfoMutex);
    umtx_destroy(&gStopMutex);
    u_cleanup();

    return 0;
}


