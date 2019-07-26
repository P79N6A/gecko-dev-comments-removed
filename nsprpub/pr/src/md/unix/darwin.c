




#include "primpl.h"

#include <mach/mach_time.h>

void _MD_EarlyInit(void)
{
}





static mach_timebase_info_data_t machTimebaseInfo;

void _PR_Mach_IntervalInit(void)
{
    kern_return_t rv;

    rv = mach_timebase_info(&machTimebaseInfo);
    PR_ASSERT(rv == KERN_SUCCESS);
}

PRIntervalTime _PR_Mach_GetInterval(void)
{
    uint64_t time;

    



    time = mach_absolute_time();
    time = time * machTimebaseInfo.numer / machTimebaseInfo.denom /
           PR_NSEC_PER_MSEC;
    return (PRIntervalTime)time;
}  

PRIntervalTime _PR_Mach_TicksPerSecond(void)
{
    return 1000;
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
#if !defined(_PR_PTHREADS)
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
#else
	*np = 0;
	return NULL;
#endif
}

#if !defined(_PR_PTHREADS)
void
_MD_SET_PRIORITY(_MDThread *thread, PRUintn newPri)
{
    return;
}

PRStatus
_MD_InitializeThread(PRThread *thread)
{
	return PR_SUCCESS;
}

PRStatus
_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
    PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    _PR_MD_SWITCH_CONTEXT(thread);
    return PR_SUCCESS;
}

PRStatus
_MD_WAKEUP_WAITER(PRThread *thread)
{
    if (thread) {
	PR_ASSERT(!(thread->flags & _PR_GLOBAL_SCOPE));
    }
    return PR_SUCCESS;
}


void
_MD_YIELD(void)
{
    PR_NOT_REACHED("_MD_YIELD should not be called for Darwin.");
}

PRStatus
_MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for Darwin.");
	return PR_FAILURE;
}
#endif 



