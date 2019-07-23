




































#include "primpl.h"


extern PRBool suspendAllOn;
extern PRThread *suspendAllThread;

extern void _MD_SET_PRIORITY(_MDThread *md, PRThreadPriority newPri);

PRIntervalTime _MD_Solaris_TicksPerSecond(void)
{
    



    return 100000UL;
}



PRIntervalTime _MD_Solaris_GetInterval(void)
{
    union {
	hrtime_t hrt;  
	PRInt64 pr64;
    } time;
    PRInt64 resolution;
    PRIntervalTime ticks;

    time.hrt = gethrtime();  
    



    LL_I2L(resolution, 10000);
    LL_DIV(time.pr64, time.pr64, resolution);
    LL_L2UI(ticks, time.pr64);
    return ticks;
}

#ifdef _PR_PTHREADS
void _MD_EarlyInit(void)
{
}

PRWord *_MD_HomeGCRegisters(PRThread *t, PRIntn isCurrent, PRIntn *np)
{
	*np = 0;
	return NULL;
}
#endif 

#if !defined(i386) && !defined(IS_64)
#if defined(_PR_HAVE_ATOMIC_OPS)











#include <thread.h>
#include <synch.h>

static mutex_t _solaris_atomic = DEFAULTMUTEX;

PRInt32
_MD_AtomicIncrement(PRInt32 *val)
{
    PRInt32 rv;
    if (mutex_lock(&_solaris_atomic) != 0)
        PR_ASSERT(0);

    rv = ++(*val);

    if (mutex_unlock(&_solaris_atomic) != 0)\
        PR_ASSERT(0);

	return rv;
}

PRInt32
_MD_AtomicAdd(PRInt32 *ptr, PRInt32 val)
{
    PRInt32 rv;
    if (mutex_lock(&_solaris_atomic) != 0)
        PR_ASSERT(0);

    rv = ((*ptr) += val);

    if (mutex_unlock(&_solaris_atomic) != 0)\
        PR_ASSERT(0);

	return rv;
}

PRInt32
_MD_AtomicDecrement(PRInt32 *val)
{
    PRInt32 rv;
    if (mutex_lock(&_solaris_atomic) != 0)
        PR_ASSERT(0);

    rv = --(*val);

    if (mutex_unlock(&_solaris_atomic) != 0)\
        PR_ASSERT(0);

	return rv;
}

PRInt32
_MD_AtomicSet(PRInt32 *val, PRInt32 newval)
{
    PRInt32 rv;
    if (mutex_lock(&_solaris_atomic) != 0)
        PR_ASSERT(0);

    rv = *val;
    *val = newval;

    if (mutex_unlock(&_solaris_atomic) != 0)\
        PR_ASSERT(0);

	return rv;
}
#endif  
#endif  

#if defined(_PR_GLOBAL_THREADS_ONLY)
#include <signal.h>
#include <errno.h>
#include <fcntl.h>
#include <thread.h>

#include <sys/lwp.h>
#include <sys/procfs.h>
#include <sys/syscall.h>
extern int syscall();  

static sigset_t old_mask;	
static PRIntn gcprio;		

THREAD_KEY_T threadid_key;
THREAD_KEY_T cpuid_key;
THREAD_KEY_T last_thread_key;
static sigset_t set, oldset;

static void
threadid_key_destructor(void *value)
{
    PRThread *me = (PRThread *)value;
    PR_ASSERT(me != NULL);
    
    if (me->flags & _PR_ATTACHED) {
        






        _PR_MD_SET_CURRENT_THREAD(me);
        _PRI_DetachThread();
    }
}

void _MD_EarlyInit(void)
{
    THR_KEYCREATE(&threadid_key, threadid_key_destructor);
    THR_KEYCREATE(&cpuid_key, NULL);
    THR_KEYCREATE(&last_thread_key, NULL);
    sigemptyset(&set);
    sigaddset(&set, SIGALRM);
}

PRStatus _MD_CreateThread(PRThread *thread, 
					void (*start)(void *), 
					PRThreadPriority priority,
					PRThreadScope scope, 
					PRThreadState state, 
					PRUint32 stackSize) 
{
	PRInt32 flags;
	
    
    thr_sigsetmask(SIG_BLOCK, &set, &oldset); 

    










    flags = THR_SUSPENDED|THR_DETACHED;
    if (_PR_IS_GCABLE_THREAD(thread) || (thread->flags & _PR_BOUND_THREAD) ||
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

    _MD_SET_PRIORITY(&(thread->md), priority);

    
    if (thr_continue( thread->md.handle ) ) {
	return PR_FAILURE;
    }
    return PR_SUCCESS;
}

void _MD_cleanup_thread(PRThread *thread)
{
    thread_t hdl;

    hdl = thread->md.handle;

    




    if ( thread != _PR_MD_CURRENT_THREAD() ) {
        thr_suspend(hdl);
    }
    PR_LOG(_pr_thread_lm, PR_LOG_MIN,
            ("(0X%x)[DestroyThread]\n", thread));

    _MD_DESTROY_SEM(&thread->md.waiter_sem);
}

void _MD_exit_thread(PRThread *thread)
{
    _MD_CLEAN_THREAD(thread);
    _MD_SET_CURRENT_THREAD(NULL);
}

void _MD_SET_PRIORITY(_MDThread *md_thread,
        PRThreadPriority newPri)
{
	PRIntn nativePri;

	if (newPri < PR_PRIORITY_FIRST) {
		newPri = PR_PRIORITY_FIRST;
	} else if (newPri > PR_PRIORITY_LAST) {
		newPri = PR_PRIORITY_LAST;
	}
	
	nativePri = newPri * 127 / PR_PRIORITY_LAST;
	if(thr_setprio((thread_t)md_thread->handle, nativePri)) {
		PR_LOG(_pr_thread_lm, PR_LOG_MIN,
		   ("_PR_SetThreadPriority: can't set thread priority\n"));
	}
}

void _MD_WAIT_CV(
    struct _MDCVar *md_cv, struct _MDLock *md_lock, PRIntervalTime timeout)
{
    struct timespec tt;
    PRUint32 msec;
    PRThread *me = _PR_MD_CURRENT_THREAD();

	PR_ASSERT((!suspendAllOn) || (suspendAllThread != me));

    if (PR_INTERVAL_NO_TIMEOUT == timeout) {
        COND_WAIT(&md_cv->cv, &md_lock->lock);
    } else {
        msec = PR_IntervalToMilliseconds(timeout);

        GETTIME(&tt);
        tt.tv_sec += msec / PR_MSEC_PER_SEC;
        tt.tv_nsec += (msec % PR_MSEC_PER_SEC) * PR_NSEC_PER_MSEC;
        
        if (tt.tv_nsec >= PR_NSEC_PER_SEC) {
            tt.tv_sec++;
            tt.tv_nsec -= PR_NSEC_PER_SEC;
        }
        COND_TIMEDWAIT(&md_cv->cv, &md_lock->lock, &tt);
    }
}

void _MD_lock(struct _MDLock *md_lock)
{
#ifdef DEBUG
    


    PRLock *lock;
    
    if ((suspendAllOn) && (suspendAllThread == _PR_MD_CURRENT_THREAD())) {
        lock = ((PRLock *) ((char*) (md_lock) - offsetof(PRLock,ilock)));
	PR_ASSERT(lock->owner == NULL);
        return;
    }
#endif 

    mutex_lock(&md_lock->lock);
}

PRThread *_pr_attached_thread_tls()
{
    PRThread *ret;

    thr_getspecific(threadid_key, (void **)&ret);
    return ret;
}

PRThread *_pr_current_thread_tls()
{
    PRThread *thread;

    thread = _MD_GET_ATTACHED_THREAD();

    if (NULL == thread) {
        thread = _PRI_AttachThread(
            PR_USER_THREAD, PR_PRIORITY_NORMAL, NULL, 0);
    }
    PR_ASSERT(thread != NULL);

    return thread;
}

PRStatus
_MD_wait(PRThread *thread, PRIntervalTime ticks)
{
        _MD_WAIT_SEM(&thread->md.waiter_sem);
        return PR_SUCCESS;
}

PRStatus
_MD_WakeupWaiter(PRThread *thread)
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

void
_MD_InitIO(void)
{
    _MD_NEW_LOCK(&_pr_ioq_lock);
}

PRStatus _MD_InitializeThread(PRThread *thread)
{
    if (!_PR_IS_NATIVE_THREAD(thread))
        return PR_SUCCESS;
    



    thread->md.threadID = sol_curthread();
		


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


void solaris_msec_sleep(int n)
{
    struct timespec ts;

    ts.tv_sec = 0;
    ts.tv_nsec = 1000000*n;
    if (syscall(SYS_nanosleep, &ts, 0, 0) < 0) {
        PR_ASSERT(0);
    }
}      

#define VALID_SP(sp, bottom, top)   \
	(((uint_t)(sp)) > ((uint_t)(bottom)) && ((uint_t)(sp)) < ((uint_t)(top)))

void solaris_record_regs(PRThread *t, prstatus_t *lwpstatus)
{
#ifdef sparc
	long *regs = (long *)&t->md.context.uc_mcontext.gregs[0];

	PR_ASSERT(_PR_IS_GCABLE_THREAD(t));
	PR_ASSERT(t->md.threadID == lwpstatus->pr_reg[REG_G7]);

	t->md.sp = lwpstatus->pr_reg[REG_SP];
	PR_ASSERT(VALID_SP(t->md.sp, t->stack->stackBottom, t->stack->stackTop));

	regs[0] = lwpstatus->pr_reg[R_G1];
	regs[1] = lwpstatus->pr_reg[R_G2];
	regs[2] = lwpstatus->pr_reg[R_G3];
	regs[3] = lwpstatus->pr_reg[R_G4];
	regs[4] = lwpstatus->pr_reg[R_O0];
	regs[5] = lwpstatus->pr_reg[R_O1];
	regs[6] = lwpstatus->pr_reg[R_O2];
	regs[7] = lwpstatus->pr_reg[R_O3];
	regs[8] = lwpstatus->pr_reg[R_O4];
	regs[9] = lwpstatus->pr_reg[R_O5];
	regs[10] = lwpstatus->pr_reg[R_O6];
	regs[11] = lwpstatus->pr_reg[R_O7];
#elif defined(i386)
	


	PR_ASSERT(0);
	PR_ASSERT(t->md.threadID == lwpstatus->pr_reg[GS]);
	t->md.sp = lwpstatus->pr_reg[UESP];
#endif
}   

void solaris_preempt_off()
{
    sigset_t set;
 
    (void)sigfillset(&set);
    syscall(SYS_sigprocmask, SIG_SETMASK, &set, &old_mask);
}

void solaris_preempt_on()
{
    syscall(SYS_sigprocmask, SIG_SETMASK, &old_mask, NULL);      
}

int solaris_open_main_proc_fd()
{
    char buf[30];
    int fd;

    
    PR_snprintf(buf, sizeof(buf), "/proc/%ld", getpid());
    if ( (fd = syscall(SYS_open, buf, O_RDONLY)) < 0) {
        return -1;
    }
    return fd;
}




int solaris_open_lwp(lwpid_t id, int lwp_main_proc_fd)
{
    int result;

    if ( (result = syscall(SYS_ioctl, lwp_main_proc_fd, PIOCOPENLWP, &id)) <0)
        return -1; 

    return result;
}
void _MD_Begin_SuspendAll()
{
    solaris_preempt_off();

    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("Begin_SuspendAll\n"));
    
    thr_getprio(thr_self(), &gcprio);
    thr_setprio(thr_self(), 0x7fffffff); 
    suspendAllOn = PR_TRUE;
    suspendAllThread = _PR_MD_CURRENT_THREAD();
}

void _MD_End_SuspendAll()
{
}

void _MD_End_ResumeAll()
{
    PR_LOG(_pr_gc_lm, PR_LOG_ALWAYS, ("End_ResumeAll\n"));
    thr_setprio(thr_self(), gcprio);
    solaris_preempt_on();
    suspendAllThread = NULL;
    suspendAllOn = PR_FALSE;
}

void _MD_Suspend(PRThread *thr)
{
   int lwp_fd, result;
   prstatus_t lwpstatus;
   int lwp_main_proc_fd = 0;
  
   if (!_PR_IS_GCABLE_THREAD(thr) || !suspendAllOn){
     



      PR_ASSERT(!suspendAllOn);
      thr_suspend(thr->md.handle);
      return;
   }

    






    if (thr->flags & _PR_PRIMORDIAL)
      return;
    
    




    
    if (!suspendAllOn || thr->md.lwpid == -1)
      return;

    if (_lwp_suspend(thr->md.lwpid) < 0) { 
       PR_ASSERT(0);
       return;
    }

    if ( (lwp_main_proc_fd = solaris_open_main_proc_fd()) < 0) {
        PR_ASSERT(0);
        return;   
    }

   if ( (lwp_fd = solaris_open_lwp(thr->md.lwpid, lwp_main_proc_fd)) < 0) {
           PR_ASSERT(0);
           close(lwp_main_proc_fd);
	   return;
   }
   if ( (result = syscall(SYS_ioctl, lwp_fd, PIOCSTATUS, &lwpstatus)) < 0) {
            
           close(lwp_fd);
           close(lwp_main_proc_fd);
	   return;
   }
            while ( !(lwpstatus.pr_flags & PR_STOPPED) ) {
                if ( (result = syscall(SYS_ioctl, lwp_fd, PIOCSTATUS, &lwpstatus)) < 0) {
                    PR_ASSERT(0);  
                    break;
                }
                solaris_msec_sleep(1);
            }
            solaris_record_regs(thr, &lwpstatus);
            close(lwp_fd);
   close(lwp_main_proc_fd);
}

#ifdef OLD_CODE

void _MD_SuspendAll()
{
    









    PRThread *current = _PR_MD_CURRENT_THREAD();
    prstatus_t status, lwpstatus;
    int result, index, lwp_fd;
    lwpid_t me = _lwp_self();
    int err;
    int lwp_main_proc_fd;

    solaris_preempt_off();

    
    thr_getprio(thr_self(), &gcprio);
    thr_setprio(thr_self(), 0x7fffffff); 

    current->md.sp = (uint_t)&me;	

    if ( (lwp_main_proc_fd = solaris_open_main_proc_fd()) < 0) {
        PR_ASSERT(0);
        solaris_preempt_on();
        return;   
    }

    if ( (result = syscall(SYS_ioctl, lwp_main_proc_fd, PIOCSTATUS, &status)) < 0) {
        err = errno;
        PR_ASSERT(0);
        goto failure;   
    }

    num_lwps = status.pr_nlwp;

    if ( (all_lwps = (lwpid_t *)PR_MALLOC((num_lwps+1) * sizeof(lwpid_t)))==NULL) {
        PR_ASSERT(0);
        goto failure;   
    }
           
    if ( (result = syscall(SYS_ioctl, lwp_main_proc_fd, PIOCLWPIDS, all_lwps)) < 0) {
        PR_ASSERT(0);
        PR_DELETE(all_lwps);
        goto failure;   
    }

    for (index=0; index< num_lwps; index++) {
        if (all_lwps[index] != me)  {
            if (_lwp_suspend(all_lwps[index]) < 0) { 
                
                all_lwps[index] = me;	
            }
        }
    }

    


    for (index=0; index< num_lwps; index++) {
        if (all_lwps[index] != me)  {
            if ( (lwp_fd = solaris_open_lwp(all_lwps[index], lwp_main_proc_fd)) < 0) {
                PR_ASSERT(0);
                PR_DELETE(all_lwps);
                all_lwps = NULL;
                goto failure;   
            }

            if ( (result = syscall(SYS_ioctl, lwp_fd, PIOCSTATUS, &lwpstatus)) < 0) {
                
                close(lwp_fd);
                continue;
            }
            while ( !(lwpstatus.pr_flags & PR_STOPPED) ) {
                if ( (result = syscall(SYS_ioctl, lwp_fd, PIOCSTATUS, &lwpstatus)) < 0) {
                    PR_ASSERT(0);  
                    break;
                }
                solaris_msec_sleep(1);
            }
            solaris_record_regs(&lwpstatus);
            close(lwp_fd);
        }
    }

    close(lwp_main_proc_fd);

    return;
failure:
    solaris_preempt_on();
    thr_setprio(thr_self(), gcprio);
    close(lwp_main_proc_fd);
    return;
}

void _MD_ResumeAll()
{
    int i;
    lwpid_t me = _lwp_self();
 
    for (i=0; i < num_lwps; i++) {
        if (all_lwps[i] == me)
            continue;
        if ( _lwp_continue(all_lwps[i]) < 0) {
            PR_ASSERT(0);  
        }
    }

    
    thr_setprio(thr_self(), gcprio);
    solaris_preempt_on();
    PR_DELETE(all_lwps);
    all_lwps = NULL;
}
#endif 

#ifdef USE_SETJMP
PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
		(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
}
#else
PRWord *_MD_HomeGCRegisters(PRThread *t, PRIntn isCurrent, PRIntn *np)
{
    if (isCurrent) {
		(void) getcontext(CONTEXT(t));
    }
    *np = NGREG;
    return (PRWord*) &t->md.context.uc_mcontext.gregs[0];
}
#endif  

#else 

#if defined(_PR_LOCAL_THREADS_ONLY)

void _MD_EarlyInit(void)
{
}

void _MD_SolarisInit()
{
    _PR_UnixInit();
}

void
_MD_SET_PRIORITY(_MDThread *thread, PRThreadPriority newPri)
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
	PR_ASSERT((thread == NULL) || (!(thread->flags & _PR_GLOBAL_SCOPE)));
    return PR_SUCCESS;
}


void
_MD_YIELD(void)
{
    PR_NOT_REACHED("_MD_YIELD should not be called for Solaris");
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
    PR_NOT_REACHED("_MD_CREATE_THREAD should not be called for Solaris");
	return(PR_FAILURE);
}

#ifdef USE_SETJMP
PRWord *_MD_HomeGCRegisters(PRThread *t, int isCurrent, int *np)
{
    if (isCurrent) {
		(void) setjmp(CONTEXT(t));
    }
    *np = sizeof(CONTEXT(t)) / sizeof(PRWord);
    return (PRWord *) CONTEXT(t);
}
#else
PRWord *_MD_HomeGCRegisters(PRThread *t, PRIntn isCurrent, PRIntn *np)
{
    if (isCurrent) {
		(void) getcontext(CONTEXT(t));
    }
    *np = NGREG;
    return (PRWord*) &t->md.context.uc_mcontext.gregs[0];
}
#endif  

#endif  

#endif 

#ifndef _PR_PTHREADS
#if defined(i386) && defined(SOLARIS2_4)






int
_pr_solx86_clock_gettime(clockid_t clock_id, struct timespec *tp)
{
    struct timeval tv;

    if (clock_id != CLOCK_REALTIME) {
	errno = EINVAL;
	return -1;
    }

    gettimeofday(&tv, NULL);
    tp->tv_sec = tv.tv_sec;
    tp->tv_nsec = tv.tv_usec * 1000;
    return 0;
}
#endif  
#endif  
