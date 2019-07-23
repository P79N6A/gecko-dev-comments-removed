




































#include "primpl.h"

#if !defined (USE_SVR4_THREADS)





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
    PRThreadPriority priority,
    PRThreadScope scope,
    PRThreadState state,
    PRUint32 stackSize)
{
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for Unixware.");
}

#else  







#include <thread.h>
#include <synch.h>

static mutex_t _unixware_atomic = DEFAULTMUTEX;

#define TEST_THEN_ADD(where, inc) \
    if (mutex_lock(&_unixware_atomic) != 0)\
        PR_ASSERT(0);\
    *where += inc;\
    if (mutex_unlock(&_unixware_atomic) != 0)\
        PR_ASSERT(0);

#define TEST_THEN_SET(where, val) \
    if (mutex_lock(&_unixware_atomic) != 0)\
        PR_ASSERT(0);\
    *where = val;\
    if (mutex_unlock(&_unixware_atomic) != 0)\
        PR_ASSERT(0);

void
_MD_INIT_ATOMIC(void)
{
}

void
_MD_ATOMIC_INCREMENT(PRInt32 *val)
{
    TEST_THEN_ADD(val, 1);
}

void
_MD_ATOMIC_ADD(PRInt32 *ptr, PRInt32 val)
{
    TEST_THEN_ADD(ptr, val);
}

void
_MD_ATOMIC_DECREMENT(PRInt32 *val)
{
    TEST_THEN_ADD(val, 0xffffffff);
}

void
_MD_ATOMIC_SET(PRInt32 *val, PRInt32 newval)
{
    TEST_THEN_SET(val, newval);
}

#include <signal.h>
#include <errno.h>
#include <fcntl.h>

#include <sys/lwp.h>
#include <sys/procfs.h>
#include <sys/syscall.h>


THREAD_KEY_T threadid_key;
THREAD_KEY_T cpuid_key;
THREAD_KEY_T last_thread_key;
static sigset_t set, oldset;

void _MD_EarlyInit(void)
{
    THR_KEYCREATE(&threadid_key, NULL);
    THR_KEYCREATE(&cpuid_key, NULL);
    THR_KEYCREATE(&last_thread_key, NULL);
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
}

PRStatus _MD_CREATE_THREAD(PRThread *thread, 
					void (*start)(void *), 
					PRThreadPriority priority,
					PRThreadScope scope, 
					PRThreadState state, 
					PRUint32 stackSize) 
{
	long flags;
	
    
    thr_sigsetmask(SIG_BLOCK, &set, &oldset); 

    flags = (state == PR_JOINABLE_THREAD ? THR_SUSPENDED 
			   : THR_SUSPENDED|THR_DETACHED);
	if (_PR_IS_GCABLE_THREAD(thread) ||
							(scope == PR_GLOBAL_BOUND_THREAD))
		flags |= THR_BOUND;

    if (thr_create(NULL, thread->stack->stackSize,
                  (void *(*)(void *)) start, (void *) thread, 
				  flags,
                  &thread->md.handle)) {
        thr_sigsetmask(SIG_SETMASK, &oldset, NULL); 
	return PR_FAILURE;
    }


    



    thread->md.lwpid = -1;
    thr_sigsetmask(SIG_SETMASK, &oldset, NULL); 
    _MD_NEW_SEM(&thread->md.waiter_sem, 0);

	if ((scope == PR_GLOBAL_THREAD) || (scope == PR_GLOBAL_BOUND_THREAD)) {
		thread->flags |= _PR_GLOBAL_SCOPE;
    }

    






    {
    int pri;
    pri = thread->priority;
    thread->priority = 100;
    PR_SetThreadPriority( thread, pri );

    PR_LOG(_pr_thread_lm, PR_LOG_MIN, 
            ("(0X%x)[Start]: on to runq at priority %d",
            thread, thread->priority));
    }

    
    if (thr_continue( thread->md.handle ) ) {
	return PR_FAILURE;
    }
    return PR_SUCCESS;
}

void _MD_cleanup_thread(PRThread *thread)
{
    thread_t hdl;
    PRMonitor *mon;

    hdl = thread->md.handle;

    




    if ( thread != _PR_MD_CURRENT_THREAD() ) {
        thr_suspend(hdl);
    }
    PR_LOG(_pr_thread_lm, PR_LOG_MIN,
            ("(0X%x)[DestroyThread]\n", thread));

    _MD_DESTROY_SEM(&thread->md.waiter_sem);
}

void _MD_SET_PRIORITY(_MDThread *md_thread, PRUintn newPri)
{
	if(thr_setprio((thread_t)md_thread->handle, newPri)) {
		PR_LOG(_pr_thread_lm, PR_LOG_MIN,
		   ("_PR_SetThreadPriority: can't set thread priority\n"));
	}
}

void _MD_WAIT_CV(
    struct _MDCVar *md_cv, struct _MDLock *md_lock, PRIntervalTime timeout)
{
    struct timespec tt;
    PRUint32 msec;
    int rv;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    msec = PR_IntervalToMilliseconds(timeout);

    GETTIME (&tt);

    tt.tv_sec += msec / PR_MSEC_PER_SEC;
    tt.tv_nsec += (msec % PR_MSEC_PER_SEC) * PR_NSEC_PER_MSEC;
    
    if (tt.tv_nsec >= PR_NSEC_PER_SEC) {
        tt.tv_sec++;
        tt.tv_nsec -= PR_NSEC_PER_SEC;
    }
    me->md.sp = unixware_getsp();


    


    COND_TIMEDWAIT(&md_cv->cv, &md_lock->lock, &tt);
}

void _MD_lock(struct _MDLock *md_lock)
{
    mutex_lock(&md_lock->lock);
}

void _MD_unlock(struct _MDLock *md_lock)
{
    mutex_unlock(&((md_lock)->lock));
}


PRThread *_pr_current_thread_tls()
{
    PRThread *ret;

    thr_getspecific(threadid_key, (void **)&ret);
    return ret;
}

PRStatus
_MD_WAIT(PRThread *thread, PRIntervalTime ticks)
{
        _MD_WAIT_SEM(&thread->md.waiter_sem);
        return PR_SUCCESS;
}

PRStatus
_MD_WAKEUP_WAITER(PRThread *thread)
{
	if (thread == NULL) {
		return PR_SUCCESS;
	}
	_MD_POST_SEM(&thread->md.waiter_sem);
	return PR_SUCCESS;
}

_PRCPU *_pr_current_cpu_tls()
{
    _PRCPU *ret;

    thr_getspecific(cpuid_key, (void **)&ret);
    return ret;
}

PRThread *_pr_last_thread_tls()
{
    PRThread *ret;

    thr_getspecific(last_thread_key, (void **)&ret);
    return ret;
}

_MDLock _pr_ioq_lock;

void _MD_INIT_IO (void)
{
    _MD_NEW_LOCK(&_pr_ioq_lock);
}

PRStatus _MD_InitializeThread(PRThread *thread)
{
    if (!_PR_IS_NATIVE_THREAD(thread))
        return;
		


    thread->md.sp = (uint_t) thread->stack->allocBase - sizeof(long);
    thread->md.lwpid = _lwp_self();
    thread->md.handle = THR_SELF();

	


    thread->flags |= _PR_GLOBAL_SCOPE;

 	




	if (thread->flags & (_PR_PRIMORDIAL | _PR_ATTACHED)) {
	    _MD_NEW_SEM(&thread->md.waiter_sem, 0);
	    _MD_SET_PRIORITY(&(thread->md), thread->priority);
	}
	return PR_SUCCESS;
}

static sigset_t old_mask;	
static int gcprio;		
static lwpid_t *all_lwps=NULL;	
static int num_lwps ;
static int suspendAllOn = 0;

#define VALID_SP(sp, bottom, top)	\
       (((uint_t)(sp)) > ((uint_t)(bottom)) && ((uint_t)(sp)) < ((uint_t)(top)))

void unixware_preempt_off()
{
    sigset_t set;
    (void)sigfillset(&set);
    sigprocmask (SIG_SETMASK, &set, &old_mask);
}

void unixware_preempt_on()
{
    sigprocmask (SIG_SETMASK, &old_mask, NULL);      
}

void _MD_Begin_SuspendAll()
{
    unixware_preempt_off();

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin_SuspendAll\n"));
    
    thr_getprio(thr_self(), &gcprio);
    thr_setprio(thr_self(), 0x7fffffff); 
    suspendAllOn = 1;
}

void _MD_End_SuspendAll()
{
}

void _MD_End_ResumeAll()
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("End_ResumeAll\n"));
    thr_setprio(thr_self(), gcprio);
    unixware_preempt_on();
    suspendAllOn = 0;
}

void _MD_Suspend(PRThread *thr)
{
   int lwp_fd, result;
   int lwp_main_proc_fd = 0;

    thr_suspend(thr->md.handle);
    if (!_PR_IS_GCABLE_THREAD(thr))
      return;
    






    if (thr->flags & _PR_PRIMORDIAL)
      return;

    
    if (!suspendAllOn || thr->md.lwpid == -1)
      return;

}
void _MD_Resume(PRThread *thr)
{
   if (!_PR_IS_GCABLE_THREAD(thr) || !suspendAllOn){
     



      PR_ASSERT(!suspendAllOn);
      thr_continue(thr->md.handle);
      return;
   }
   if (thr->md.lwpid == -1)
     return;
 
   if ( _lwp_continue(thr->md.lwpid) < 0) {
      PR_ASSERT(0);  
   }
}


PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
	(void) getcontext(CONTEXT(t));	
    }
    *np = NGREG;
    if (t->md.lwpid == -1)
      memset(&t->md.context.uc_mcontext.gregs[0], 0, NGREG * sizeof(PRWord));
    return (PRWord*) &t->md.context.uc_mcontext.gregs[0];
}

int
_pr_unixware_clock_gettime (struct timespec *tp)
{
    struct timeval tv;
 
    gettimeofday(&tv, NULL);
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;
    return 0;
}


#endif 
