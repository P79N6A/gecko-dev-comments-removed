




































#include "nsTimelineService.h"
#include "prlong.h"
#include "prprf.h"
#include "prenv.h"
#include "plhash.h"
#include "prlock.h"
#include "prinit.h"
#include "prinrval.h"
#include "prthread.h"

#ifdef MOZ_TIMELINE

#define MAXINDENT 20

static PRFileDesc *timelineFD = PR_STDERR;
static PRBool gTimelineDisabled = PR_TRUE;







static const PRUintn BAD_TLS_INDEX = (PRUintn) -1;
static PRUintn gTLSIndex = BAD_TLS_INDEX;

class TimelineThreadData {
public:
    TimelineThreadData() : initTime(0), indent(0),
                           disabled(PR_TRUE), timers(nsnull) {}
    ~TimelineThreadData() {if (timers) PL_HashTableDestroy(timers);}
    PRTime initTime;
    PRHashTable *timers;
    int indent;
    PRBool disabled;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(nsTimelineService, nsITimelineService)





class nsTimelineServiceTimer {
  public:
    nsTimelineServiceTimer();
    ~nsTimelineServiceTimer();
    void start();
    
    




    void stop(PRTime now);
    void reset();
    PRTime getAccum();
    PRTime getAccum(PRTime now);

  private:
    PRTime mAccum;
    PRTime mStart;
    PRInt32 mRunning;
    PRThread *mOwnerThread; 
};

#define TIMER_CHECK_OWNER() \
    NS_ABORT_IF_FALSE(PR_GetCurrentThread() == mOwnerThread, \
                      "Timer used by non-owning thread")


nsTimelineServiceTimer::nsTimelineServiceTimer()
: mAccum(LL_ZERO), mStart(LL_ZERO), mRunning(0),
  mOwnerThread(PR_GetCurrentThread())
{
}

nsTimelineServiceTimer::~nsTimelineServiceTimer()
{
}

void nsTimelineServiceTimer::start()
{
    TIMER_CHECK_OWNER();
    if (!mRunning) {
        mStart = PR_Now();
    }
    mRunning++;
}

void nsTimelineServiceTimer::stop(PRTime now)
{
    TIMER_CHECK_OWNER();
    mRunning--;
    if (mRunning == 0) {
        PRTime delta, accum;
        LL_SUB(delta, now, mStart);
        LL_ADD(accum, mAccum, delta);
        mAccum = accum;
    }
}

void nsTimelineServiceTimer::reset()
{
  TIMER_CHECK_OWNER();
  mStart = 0;
  mAccum = 0;
}

PRTime nsTimelineServiceTimer::getAccum()
{
    TIMER_CHECK_OWNER();
    PRTime accum;

    if (!mRunning) {
        accum = mAccum;
    } else {
        PRTime delta;
        LL_SUB(delta, PR_Now(), mStart);
        LL_ADD(accum, mAccum, delta);
    }
    return accum;
}

PRTime nsTimelineServiceTimer::getAccum(PRTime now)
{
    TIMER_CHECK_OWNER();
    PRTime accum;

    if (!mRunning) {
        accum = mAccum;
    } else {
        PRTime delta;
        LL_SUB(delta, now, mStart);
        LL_ADD(accum, mAccum, delta);
    }
    return accum;
}

static TimelineThreadData *GetThisThreadData()
{
    NS_ABORT_IF_FALSE(gTLSIndex!=BAD_TLS_INDEX, "Our TLS not initialized");
    TimelineThreadData *new_data = nsnull;
    TimelineThreadData *data = (TimelineThreadData *)PR_GetThreadPrivate(gTLSIndex);
    if (data == nsnull) {
        
        new_data = new TimelineThreadData();
        if (!new_data)
            goto done;

        
        new_data->timers = PL_NewHashTable(100, PL_HashString, PL_CompareStrings,
                                 PL_CompareValues, NULL, NULL);
        if (new_data->timers==NULL)
            goto done;
        new_data->initTime = PR_Now();
        NS_ASSERTION(!gTimelineDisabled,
                         "Why are we creating new state when disabled?");
        new_data->disabled = PR_FALSE;
        data = new_data;
        new_data = nsnull;
        PR_SetThreadPrivate(gTLSIndex, data);
    }
done:
    if (new_data) 
        delete new_data;
    NS_ASSERTION(data, "TimelineService could not get thread-local data");
    return data;
}

extern "C" {
  static void ThreadDestruct (void *data);
  static PRStatus TimelineInit(void);
}

void ThreadDestruct( void *data )
{
    if (data)
        delete (TimelineThreadData *)data;
}




static PRCallOnceType initonce;

PRStatus TimelineInit(void)
{
    char *timeStr;
    char *fileName;
    const char *timelineEnable;
    PRInt32 secs, msecs;
    PRFileDesc *fd;
    PRInt64 tmp1, tmp2;

    PRStatus status = PR_NewThreadPrivateIndex( &gTLSIndex, ThreadDestruct );
    NS_ASSERTION(status==0, "TimelineService could not allocate TLS storage.");

    timeStr = PR_GetEnv("NS_TIMELINE_INIT_TIME");
    
    
    
    if (timeStr && *timeStr && (2 == PR_sscanf(timeStr, "%d.%d", &secs, &msecs))) {
        PRTime &initTime = GetThisThreadData()->initTime;
        LL_MUL(tmp1, (PRInt64)secs, 1000000);
        LL_MUL(tmp2, (PRInt64)msecs, 1000);
        LL_ADD(initTime, tmp1, tmp2);
    }

    
    fileName = PR_GetEnv("NS_TIMELINE_LOG_FILE");
    if (fileName && *fileName
        && (fd = PR_Open(fileName, PR_WRONLY | PR_CREATE_FILE | PR_TRUNCATE,
                         0666)) != NULL) {
        timelineFD = fd;
        PR_fprintf(fd,
                   "NOTE: due to asynchrony, the indentation that you see does"
                   " not necessarily correspond to nesting in the code.\n\n");
    }

    
    timelineEnable = PR_GetEnv("NS_TIMELINE_ENABLE");
    if (timelineEnable && *timelineEnable)
        gTimelineDisabled = PR_FALSE;
    return PR_SUCCESS;
}

static void ParseTime(PRTime tm, PRInt32& secs, PRInt32& msecs)
{
    PRTime llsecs, llmsecs, tmp;

    LL_DIV(llsecs, tm, 1000000);
    LL_MOD(tmp, tm, 1000000);
    LL_DIV(llmsecs, tmp, 1000);

    LL_L2I(secs, llsecs);
    LL_L2I(msecs, llmsecs);
}

static char *Indent(char *buf)
{
    int &indent = GetThisThreadData()->indent;
    int amount = indent;
    if (amount > MAXINDENT) {
        amount = MAXINDENT;
    }
    if (amount < 0) {
        amount = 0;
        indent = 0;
        PR_Write(timelineFD, "indent underflow!\n", 18);
    }
    while (amount--) {
        *buf++ = ' ';
    }
    return buf;
}

static void PrintTime(PRTime tm, const char *text, va_list args)
{
    PRInt32 secs, msecs;
    char pbuf[550], *pc, tbuf[550];

    ParseTime(tm, secs, msecs);

    
    
    pc = Indent(pbuf);
    PR_vsnprintf(pc, sizeof pbuf - (pc - pbuf), text, args);
    PR_snprintf(tbuf, sizeof tbuf, "%05d.%03d (%08p): %s\n",
                secs, msecs, PR_GetCurrentThread(), pbuf);
    PR_Write(timelineFD, tbuf, strlen(tbuf));
}




static nsresult NS_TimelineMarkV(const char *text, va_list args)
{
    PRTime elapsed,tmp;

    PR_CallOnce(&initonce, TimelineInit);

    TimelineThreadData *thread = GetThisThreadData();

    tmp = PR_Now();
    LL_SUB(elapsed, tmp, thread->initTime);

    PrintTime(elapsed, text, args);

    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineForceMark(const char *text, ...)
{
    va_list args;
    va_start(args, text);
    NS_TimelineMarkV(text, args);

    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineMark(const char *text, ...)
{
    va_list args;
    va_start(args, text);

    PR_CallOnce(&initonce, TimelineInit);

    if (gTimelineDisabled)
        return NS_ERROR_NOT_AVAILABLE;

    TimelineThreadData *thread = GetThisThreadData();

    if (thread->disabled)
        return NS_ERROR_NOT_AVAILABLE;

    NS_TimelineMarkV(text, args);

    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineStartTimer(const char *timerName)
{
    PR_CallOnce(&initonce, TimelineInit);

    if (gTimelineDisabled)
        return NS_ERROR_NOT_AVAILABLE;

    TimelineThreadData *thread = GetThisThreadData();

    if (thread->timers == NULL)
        return NS_ERROR_FAILURE;
    if (thread->disabled)
        return NS_ERROR_NOT_AVAILABLE;

    nsTimelineServiceTimer *timer
        = (nsTimelineServiceTimer *)PL_HashTableLookup(thread->timers, timerName);
    if (timer == NULL) {
        timer = new nsTimelineServiceTimer;
        if (!timer)
            return NS_ERROR_OUT_OF_MEMORY;

        PL_HashTableAdd(thread->timers, timerName, timer);
    }
    timer->start();
    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineStopTimer(const char *timerName)
{
    if (gTimelineDisabled)
        return NS_ERROR_NOT_AVAILABLE;
    




    PRTime now = PR_Now();

    TimelineThreadData *thread = GetThisThreadData();
    if (thread->timers == NULL)
        return NS_ERROR_FAILURE;
    if (thread->disabled)
        return NS_ERROR_NOT_AVAILABLE;
    nsTimelineServiceTimer *timer
        = (nsTimelineServiceTimer *)PL_HashTableLookup(thread->timers, timerName);
    if (timer == NULL) {
        return NS_ERROR_FAILURE;
    }

    timer->stop(now);

    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineMarkTimer(const char *timerName, const char *str)
{
    PR_CallOnce(&initonce, TimelineInit);

    if (gTimelineDisabled)
        return NS_ERROR_NOT_AVAILABLE;

    TimelineThreadData *thread = GetThisThreadData();
    if (thread->timers == NULL)
        return NS_ERROR_FAILURE;
    if (thread->disabled)
        return NS_ERROR_NOT_AVAILABLE;
    nsTimelineServiceTimer *timer
        = (nsTimelineServiceTimer *)PL_HashTableLookup(thread->timers, timerName);
    if (timer == NULL) {
        return NS_ERROR_FAILURE;
    }
    PRTime accum = timer->getAccum();

    char buf[500];
    PRInt32 sec, msec;
    ParseTime(accum, sec, msec);
    if (!str)
        PR_snprintf(buf, sizeof buf, "%s total: %d.%03d",
                    timerName, sec, msec);
    else
        PR_snprintf(buf, sizeof buf, "%s total: %d.%03d (%s)",
                    timerName, sec, msec, str);
    NS_TimelineMark(buf);

    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineResetTimer(const char *timerName)
{
    if (gTimelineDisabled)
        return NS_ERROR_NOT_AVAILABLE;

    TimelineThreadData *thread = GetThisThreadData();
    if (thread->timers == NULL)
        return NS_ERROR_FAILURE;
    if (thread->disabled)
        return NS_ERROR_NOT_AVAILABLE;
    nsTimelineServiceTimer *timer
        = (nsTimelineServiceTimer *)PL_HashTableLookup(thread->timers, timerName);
    if (timer == NULL) {
        return NS_ERROR_FAILURE;
    }

    timer->reset();
    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineIndent()
{
    if (gTimelineDisabled)
        return NS_ERROR_NOT_AVAILABLE;

    TimelineThreadData *thread = GetThisThreadData();
    if (thread->disabled)
        return NS_ERROR_NOT_AVAILABLE;
    thread->indent++;
    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineOutdent()
{
    if (gTimelineDisabled)
        return NS_ERROR_NOT_AVAILABLE;

    TimelineThreadData *thread = GetThisThreadData();
    if (thread->disabled)
        return NS_ERROR_NOT_AVAILABLE;
    thread->indent--;
    return NS_OK;
}

PR_IMPLEMENT(nsresult) NS_TimelineEnter(const char *text)
{
    nsresult rv = NS_TimelineMark("%s...", text);
    if (NS_FAILED(rv)) {
        return rv;
    }
    return NS_TimelineIndent();
}

PR_IMPLEMENT(nsresult) NS_TimelineLeave(const char *text)
{
    nsresult rv = NS_TimelineOutdent();
    if (NS_FAILED(rv)) {
        return rv;
    }
    return NS_TimelineMark("...%s", text);
}

nsTimelineService::nsTimelineService()
{
  
}


NS_IMETHODIMP nsTimelineService::Mark(const char *text)
{
    return NS_TimelineMark(text);
}


NS_IMETHODIMP nsTimelineService::StartTimer(const char *timerName)
{
    return NS_TimelineStartTimer(timerName);
}


NS_IMETHODIMP nsTimelineService::StopTimer(const char *timerName)
{
    return NS_TimelineStopTimer(timerName);
}


NS_IMETHODIMP nsTimelineService::MarkTimer(const char *timerName)
{
    return NS_TimelineMarkTimer(timerName);
}


NS_IMETHODIMP nsTimelineService::MarkTimerWithComment(const char *timerName, const char *comment)
{
    return NS_TimelineMarkTimer(timerName, comment);
}


NS_IMETHODIMP nsTimelineService::ResetTimer(const char *timerName)
{
    return NS_TimelineResetTimer(timerName);
}


NS_IMETHODIMP nsTimelineService::Indent()
{
    return NS_TimelineIndent();
}


NS_IMETHODIMP nsTimelineService::Outdent()
{
    return NS_TimelineOutdent();
}


NS_IMETHODIMP nsTimelineService::Enter(const char *text)
{
    return NS_TimelineEnter(text);
}


NS_IMETHODIMP nsTimelineService::Leave(const char *text)
{
    return NS_TimelineLeave(text);
}

#endif 
