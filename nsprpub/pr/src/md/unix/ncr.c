







































#include "primpl.h"

#include <setjmp.h>

void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
}

#ifdef ALARMS_BREAK_TCP 

PRInt32 _MD_connect(PRInt32 osfd, const PRNetAddr *addr, PRInt32 addrlen,
                        PRIntervalTime timeout)
{
    PRInt32 rv;

    _MD_BLOCK_CLOCK_INTERRUPTS();
    rv = _connect(osfd,addr,addrlen);
    _MD_UNBLOCK_CLOCK_INTERRUPTS();
}

PRInt32 _MD_accept(PRInt32 osfd, PRNetAddr *addr, PRInt32 addrlen,
                        PRIntervalTime timeout)
{
    PRInt32 rv;

    _MD_BLOCK_CLOCK_INTERRUPTS();
    rv = _accept(osfd,addr,addrlen);
    _MD_UNBLOCK_CLOCK_INTERRUPTS();
    return(rv);
}
#endif






#ifdef _PR_HAVE_ATOMIC_OPS

#include  <stdio.h>
static FILE *_uw_semf;

void
_MD_INIT_ATOMIC(void)
{
    
    if ((_uw_semf = tmpfile()) == NULL)
        PR_ASSERT(0);

    return;
}

void
_MD_ATOMIC_INCREMENT(PRInt32 *val)
{
    flockfile(_uw_semf);
    (*val)++;
    unflockfile(_uw_semf);
}

void
_MD_ATOMIC_ADD(PRInt32 *ptr, PRInt32 val)
{
    flockfile(_uw_semf);
    (*ptr) += val;
    unflockfile(_uw_semf);
}


void
_MD_ATOMIC_DECREMENT(PRInt32 *val)
{
    flockfile(_uw_semf);
    (*val)--;
    unflockfile(_uw_semf);
}

void
_MD_ATOMIC_SET(PRInt32 *val, PRInt32 newval)
{
    flockfile(_uw_semf);
    *val = newval;
    unflockfile(_uw_semf);
}
#endif

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
    PR_NOT_REACHED("_MD_YIELD should not be called for Unixware.");
}

PRStatus
_MD_CREATE_THREAD(
    PRThread *thread,
    void (*start) (void *),
    PRUintn priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for Unixware.");
    return PR_FAILURE;
}










#define NEED_LOCALTIME_R
#define NEED_GMTIME_R
#define NEED_ASCTIME_R
#define NEED_STRTOK_R
#define NEED_CTIME_R

#if defined (NEED_LOCALTIME_R) || defined (NEED_CTIME_R) || defined (NEED_ASCTIME_R) || defined (NEED_GMTIME_R) || defined (NEED_STRTOK_R)
#include "prlock.h"
#endif

#if defined (NEED_LOCALTIME_R)

static PRLock *localtime_r_monitor = NULL;

struct tm *localtime_r (const time_t *clock, struct tm *result)
{
    struct tm *tmPtr;
    int needLock = PR_Initialized();  



    if (needLock) {
        if (localtime_r_monitor == NULL) {

            localtime_r_monitor = PR_NewLock();
        }
        PR_Lock(localtime_r_monitor);
    }

    






    tmPtr = localtime(clock);
    if (tmPtr) {
        *result = *tmPtr;
    } else {
        result = NULL;
    }

    if (needLock) PR_Unlock(localtime_r_monitor);

    return result;
}

#endif

#if defined (NEED_GMTIME_R)

static PRLock *gmtime_r_monitor = NULL;

struct tm *gmtime_r (const time_t *clock, struct tm *result)
{
    struct tm *tmPtr;
    int needLock = PR_Initialized();  



    if (needLock) {
        if (gmtime_r_monitor == NULL) {
            gmtime_r_monitor = PR_NewLock();
        }
        PR_Lock(gmtime_r_monitor);
    }

    tmPtr = gmtime(clock);
    if (tmPtr) {
        *result = *tmPtr;
    } else {
        result = NULL;
    }

    if (needLock) PR_Unlock(gmtime_r_monitor);

    return result;
}

#endif

#if defined (NEED_CTIME_R)

static PRLock *ctime_r_monitor = NULL;

char  *ctime_r (const time_t *clock, char *buf, int buflen)
{
    char *cbuf;
    int needLock = PR_Initialized();  



    if (needLock) {

        if (ctime_r_monitor == NULL) {
            ctime_r_monitor = PR_NewLock();
        }
        PR_Lock(ctime_r_monitor);
    }

    cbuf = ctime (clock);
    if (cbuf) {
        strncpy (buf, cbuf, buflen - 1);
        buf[buflen - 1] = 0;
    }

    if (needLock) PR_Unlock(ctime_r_monitor);

    return cbuf;
}

#endif

#if defined (NEED_ASCTIME_R)

static PRLock *asctime_r_monitor = NULL;


char  *asctime_r (const struct tm  *tm, char *buf, int buflen)
{
    char *cbuf;
    int needLock = PR_Initialized();  



    if (needLock) {
        if (asctime_r_monitor == NULL) {
            asctime_r_monitor = PR_NewLock();
        }
        PR_Lock(asctime_r_monitor);
    }

    cbuf = asctime (tm);
    if (cbuf) {
        strncpy (buf, cbuf, buflen - 1);
        buf[buflen - 1] = 0;
    }

    if (needLock) PR_Unlock(asctime_r_monitor);

    return cbuf;

}
#endif

#if defined (NEED_STRTOK_R)

char *
strtok_r (s, delim, last)
        register char *s;
        register const char *delim;
        register char **last;
{
        register char *spanp;
        register int c, sc;
        char *tok;


        if (s == NULL && (s = *last) == NULL)
                return (NULL);

        


cont:

        c = *s++;
        for (spanp = (char *)delim; (sc = *spanp++) != 0;) {
                if (c == sc)
                        goto cont;
        }

        if (c == 0) {           
                *last = NULL;
                return (NULL);
        }
        tok = s - 1;

        



        for (;;) {
                c = *s++;
                spanp = (char *)delim;
                do {
                        if ((sc = *spanp++) == c) {
                                if (c == 0)
                                        s = NULL;

                                else
                                        s[-1] = 0;
                                *last = s;
                                return (tok);
                        }
                } while (sc != 0);
        }
        
}

#endif
