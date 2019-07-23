




































#include "primpl.h"
#include <setjmp.h>

#if defined(HPUX_LW_TIMER)

#include <machine/inline.h>
#include <machine/clock.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/pstat.h>

int __lw_get_thread_times(int which, int64_t *sample, int64_t *time);

static double msecond_per_itick;

void _PR_HPUX_LW_IntervalInit(void)
{
    struct pst_processor psp;
    int iticksperclktick, clk_tck;
    int rv;

    rv = pstat_getprocessor(&psp, sizeof(psp), 1, 0);
    PR_ASSERT(rv != -1);

    iticksperclktick = psp.psp_iticksperclktick;
    clk_tck = sysconf(_SC_CLK_TCK);
    msecond_per_itick = (1000.0)/(double)(iticksperclktick * clk_tck);
}

PRIntervalTime _PR_HPUX_LW_GetInterval(void)
{
    int64_t time, sample;

    __lw_get_thread_times(1, &sample, &time);
    



    return (time * msecond_per_itick);
}
#endif  

#if !defined(PTHREADS_USER)

void _MD_EarlyInit(void)
{
#ifndef _PR_PTHREADS
    























#ifdef HPUX9
#define PIDOOMA_STACK_SIZE 524288
#define BACKTRACE_SIZE 8192
    {
        jmp_buf jb;
        char *newstack;
        char *oldstack;

        if(!setjmp(jb)) {
            newstack = (char *) PR_MALLOC(PIDOOMA_STACK_SIZE);
	    oldstack = (char *) (*(((int *) jb) + 1) - BACKTRACE_SIZE);
            memcpy(newstack, oldstack, BACKTRACE_SIZE);
            *(((int *) jb) + 1) = (int) (newstack + BACKTRACE_SIZE);
            longjmp(jb, 1);
        }
    }
#endif  
#endif  
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
#ifndef _PR_PTHREADS
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

#ifndef _PR_PTHREADS
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
    PR_NOT_REACHED("_MD_YIELD should not be called for HP-UX.");
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
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for HP-UX.");
}
#endif 

void
_MD_suspend_thread(PRThread *thread)
{
#ifdef _PR_PTHREADS
#endif
}

void
_MD_resume_thread(PRThread *thread)
{
#ifdef _PR_PTHREADS
#endif
}
#endif 









char *
strchr(const char *s, int c)
{
    char ch;

    if (!s) {
        return NULL;
    }

    ch = (char) c;

    while ((*s) && ((*s) != ch)) {
        s++;
    }

    if ((*s) == ch) {
        return (char *) s;
    }

    return NULL;
}












int memcmp(const void *s1, const void *s2, size_t n)
{
    register unsigned char *p1 = (unsigned char *) s1,
            *p2 = (unsigned char *) s2;

    while (n-- > 0) {
        register int r = ((int) ((unsigned int) *p1)) 
                - ((int) ((unsigned int) *p2));
        if (r) return r;
        p1++; p2++;
    }
    return 0; 
}
