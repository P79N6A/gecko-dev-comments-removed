




































#include "primpl.h"
#include <signal.h>
#include <string.h>

#if defined(WIN95)                                                                         






#pragma warning(disable : 4101)
#endif          


PRLock *_pr_activeLock;
PRInt32 _pr_primordialExitCount;   











PRCondVar *_pr_primordialExitCVar; 




PRLock *_pr_deadQLock;
PRUint32 _pr_numNativeDead;
PRUint32 _pr_numUserDead;
PRCList _pr_deadNativeQ;
PRCList _pr_deadUserQ;

PRUint32 _pr_join_counter;

PRUint32 _pr_local_threads;
PRUint32 _pr_global_threads;

PRBool suspendAllOn = PR_FALSE;
PRThread *suspendAllThread = NULL;

extern PRCList _pr_active_global_threadQ;
extern PRCList _pr_active_local_threadQ;

static void _PR_DecrActiveThreadCount(PRThread *thread);
static PRThread *_PR_AttachThread(PRThreadType, PRThreadPriority, PRThreadStack *);
static void _PR_InitializeNativeStack(PRThreadStack *ts);
static void _PR_InitializeRecycledThread(PRThread *thread);
static void _PR_UserRunThread(void);

void _PR_InitThreads(PRThreadType type, PRThreadPriority priority,
    PRUintn maxPTDs)
{
    PRThread *thread;
    PRThreadStack *stack;

    _pr_terminationCVLock = PR_NewLock();
    _pr_activeLock = PR_NewLock();

#ifndef HAVE_CUSTOM_USER_THREADS
    stack = PR_NEWZAP(PRThreadStack);
#ifdef HAVE_STACK_GROWING_UP
    stack->stackTop = (char*) ((((long)&type) >> _pr_pageShift)
                  << _pr_pageShift);
#else
#if defined(SOLARIS) || defined (UNIXWARE) && defined (USR_SVR4_THREADS)
    stack->stackTop = (char*) &thread;
#else
    stack->stackTop = (char*) ((((long)&type + _pr_pageSize - 1)
                >> _pr_pageShift) << _pr_pageShift);
#endif
#endif
#else
    
    stack = PR_NEWZAP(PRThreadStack);
    if (stack) {
        stack->stackSize = 0;
        _PR_InitializeNativeStack(stack);
    }
#endif 

    thread = _PR_AttachThread(type, priority, stack);
    if (thread) {
        _PR_MD_SET_CURRENT_THREAD(thread);

        if (type == PR_SYSTEM_THREAD) {
            thread->flags = _PR_SYSTEM;
            _pr_systemActive++;
            _pr_primordialExitCount = 0;
        } else {
            _pr_userActive++;
            _pr_primordialExitCount = 1;
        }
    thread->no_sched = 1;
    _pr_primordialExitCVar = PR_NewCondVar(_pr_activeLock);
    }

    if (!thread) PR_Abort();
#ifdef _PR_LOCAL_THREADS_ONLY
    thread->flags |= _PR_PRIMORDIAL;
#else
    thread->flags |= _PR_PRIMORDIAL | _PR_GLOBAL_SCOPE;
#endif

    



    if (_PR_MD_INIT_THREAD(thread) == PR_FAILURE) {
        


    }

    if (_PR_IS_NATIVE_THREAD(thread)) {
        PR_APPEND_LINK(&thread->active, &_PR_ACTIVE_GLOBAL_THREADQ());
        _pr_global_threads++;
    } else {
        PR_APPEND_LINK(&thread->active, &_PR_ACTIVE_LOCAL_THREADQ());
        _pr_local_threads++;
    }

    _pr_recycleThreads = 0;
    _pr_deadQLock = PR_NewLock();
    _pr_numNativeDead = 0;
    _pr_numUserDead = 0;
    PR_INIT_CLIST(&_pr_deadNativeQ);
    PR_INIT_CLIST(&_pr_deadUserQ);
}

void _PR_CleanupThreads(void)
{
    if (_pr_terminationCVLock) {
        PR_DestroyLock(_pr_terminationCVLock);
        _pr_terminationCVLock = NULL;
    }
    if (_pr_activeLock) {
        PR_DestroyLock(_pr_activeLock);
        _pr_activeLock = NULL;
    }
    if (_pr_primordialExitCVar) {
        PR_DestroyCondVar(_pr_primordialExitCVar);
        _pr_primordialExitCVar = NULL;
    }
    
    if (_pr_deadQLock) {
        PR_DestroyLock(_pr_deadQLock);
        _pr_deadQLock = NULL;
    }
}




static void _PR_InitializeNativeStack(PRThreadStack *ts)
{
    if( ts && (ts->stackTop == 0) ) {
        ts->allocSize = ts->stackSize;

        


#ifdef HAVE_STACK_GROWING_UP
    ts->allocBase = (char*) ((((long)&ts) >> _pr_pageShift)
                  << _pr_pageShift);
        ts->stackBottom = ts->allocBase + ts->stackSize;
        ts->stackTop = ts->allocBase;
#else
        ts->allocBase = (char*) ((((long)&ts + _pr_pageSize - 1)
                >> _pr_pageShift) << _pr_pageShift);
        ts->stackTop    = ts->allocBase;
        ts->stackBottom = ts->allocBase - ts->stackSize;
#endif
    }
}

void _PR_NotifyJoinWaiters(PRThread *thread)
{
    





    
    PR_ASSERT(thread == _PR_MD_CURRENT_THREAD());
    if (thread->term != NULL) {
        PR_Lock(_pr_terminationCVLock);
        _PR_THREAD_LOCK(thread);
        thread->state = _PR_JOIN_WAIT;
        if ( !_PR_IS_NATIVE_THREAD(thread) ) {
            _PR_MISCQ_LOCK(thread->cpu);
            _PR_ADD_JOINQ(thread, thread->cpu);
            _PR_MISCQ_UNLOCK(thread->cpu);
        }
        _PR_THREAD_UNLOCK(thread);
        PR_NotifyCondVar(thread->term);
        PR_Unlock(_pr_terminationCVLock);
        _PR_MD_WAIT(thread, PR_INTERVAL_NO_TIMEOUT);
        PR_ASSERT(thread->state != _PR_JOIN_WAIT);
    }

}








static void _PR_InitializeRecycledThread(PRThread *thread)
{
    



#ifdef DEBUG
    if (thread->privateData) {
        unsigned int i;
        for (i = 0; i < thread->tpdLength; i++) {
            PR_ASSERT(thread->privateData[i] == NULL);
        }
    }
#endif
    PR_ASSERT(thread->dumpArg == 0 && thread->dump == 0);
    PR_ASSERT(thread->errorString == 0 && thread->errorStringSize == 0);
    PR_ASSERT(thread->errorStringLength == 0);

    
    thread->errorCode = thread->osErrorCode = 0;
    thread->io_pending = thread->io_suspended = PR_FALSE;
    thread->environment = 0;
    PR_INIT_CLIST(&thread->lockList);
}

PRStatus _PR_RecycleThread(PRThread *thread)
{
    if ( _PR_IS_NATIVE_THREAD(thread) &&
            _PR_NUM_DEADNATIVE < _pr_recycleThreads) {
        _PR_DEADQ_LOCK;
        PR_APPEND_LINK(&thread->links, &_PR_DEADNATIVEQ);
        _PR_INC_DEADNATIVE;
        _PR_DEADQ_UNLOCK;
    return (PR_SUCCESS);
    } else if ( !_PR_IS_NATIVE_THREAD(thread) &&
                _PR_NUM_DEADUSER < _pr_recycleThreads) {
        _PR_DEADQ_LOCK;
        PR_APPEND_LINK(&thread->links, &_PR_DEADUSERQ);
        _PR_INC_DEADUSER;
        _PR_DEADQ_UNLOCK;
    return (PR_SUCCESS);
    }
    return (PR_FAILURE);
}













static void
_PR_DecrActiveThreadCount(PRThread *thread)
{
    PR_Lock(_pr_activeLock);
    if (thread->flags & _PR_SYSTEM) {
        _pr_systemActive--;
    } else {
        _pr_userActive--;
        if (_pr_userActive == _pr_primordialExitCount) {
            PR_NotifyCondVar(_pr_primordialExitCVar);
        }
    }
    PR_Unlock(_pr_activeLock);
}




static void
_PR_DestroyThread(PRThread *thread)
{
    _PR_MD_FREE_LOCK(&thread->threadLock);
    PR_DELETE(thread);
}

void
_PR_NativeDestroyThread(PRThread *thread)
{
    if(thread->term) {
        PR_DestroyCondVar(thread->term);
        thread->term = 0;
    }
    if (NULL != thread->privateData) {
        PR_ASSERT(0 != thread->tpdLength);
        PR_DELETE(thread->privateData);
        thread->tpdLength = 0;
    }
    PR_DELETE(thread->stack);
    _PR_DestroyThread(thread);
}

void
_PR_UserDestroyThread(PRThread *thread)
{
    if(thread->term) {
        PR_DestroyCondVar(thread->term);
        thread->term = 0;
    }
    if (NULL != thread->privateData) {
        PR_ASSERT(0 != thread->tpdLength);
        PR_DELETE(thread->privateData);
        thread->tpdLength = 0;
    }
    _PR_MD_FREE_LOCK(&thread->threadLock);
    if (thread->threadAllocatedOnStack == 1) {
        _PR_MD_CLEAN_THREAD(thread);
        




        _PR_FreeStack(thread->stack);
    } else {
#ifdef WINNT
        _PR_MD_CLEAN_THREAD(thread);
#else
        





        PR_ASSERT(thread->flags & _PR_PRIMORDIAL);
#endif
    }
}







void _PR_NativeRunThread(void *arg)
{
    PRThread *thread = (PRThread *)arg;

    _PR_MD_SET_CURRENT_THREAD(thread);

    _PR_MD_SET_CURRENT_CPU(NULL);

    
    _PR_InitializeNativeStack(thread->stack);

    
    if (_PR_MD_INIT_THREAD(thread) == PR_FAILURE) {
        



        return;
    }

    while(1) {
        thread->state = _PR_RUNNING;

        


        PR_Lock(_pr_activeLock);
        PR_APPEND_LINK(&thread->active, &_PR_ACTIVE_GLOBAL_THREADQ());
        _pr_global_threads++;
        PR_Unlock(_pr_activeLock);

        (*thread->startFunc)(thread->arg);

        






        PR_ASSERT(thread->io_pending == PR_FALSE);
        






        PR_ASSERT(thread->io_suspended == PR_FALSE);

        


        PR_Lock(_pr_activeLock);
        PR_REMOVE_LINK(&thread->active);
        _pr_global_threads--;
        PR_Unlock(_pr_activeLock);

        PR_LOG(_pr_thread_lm, PR_LOG_MIN, ("thread exiting"));

        
        _PR_CleanupThread(thread);

        _PR_NotifyJoinWaiters(thread);

        _PR_DecrActiveThreadCount(thread);

        thread->state = _PR_DEAD_STATE;

        if (!_pr_recycleThreads || (_PR_RecycleThread(thread) ==
                        PR_FAILURE)) {
            




            _PR_MD_EXIT_THREAD(thread);
            


            _PR_NativeDestroyThread(thread);
            


            return;
        }

        
        _PR_MD_WAIT(thread, PR_INTERVAL_NO_TIMEOUT);
    }
}

static void _PR_UserRunThread(void)
{
    PRThread *thread = _PR_MD_CURRENT_THREAD();
    PRIntn is;

    if (_MD_LAST_THREAD())
    _MD_LAST_THREAD()->no_sched = 0;

#ifdef HAVE_CUSTOM_USER_THREADS
    if (thread->stack == NULL) {
        thread->stack = PR_NEWZAP(PRThreadStack);
        _PR_InitializeNativeStack(thread->stack);
    }
#endif 

    while(1) {
        
        if ( !_PR_IS_NATIVE_THREAD(thread)) _PR_MD_SET_INTSOFF(0);

    


    if (!(thread->flags & _PR_IDLE_THREAD)) {
        PR_Lock(_pr_activeLock);
        PR_APPEND_LINK(&thread->active, &_PR_ACTIVE_LOCAL_THREADQ());
        _pr_local_threads++;
        PR_Unlock(_pr_activeLock);
    }

        (*thread->startFunc)(thread->arg);

        






        PR_ASSERT(thread->io_pending == PR_FALSE);
        






        PR_ASSERT(thread->io_suspended == PR_FALSE);

        PR_Lock(_pr_activeLock);
    


    if (!(thread->flags & _PR_IDLE_THREAD)) {
           PR_REMOVE_LINK(&thread->active);
        _pr_local_threads--;
    }
    PR_Unlock(_pr_activeLock);
        PR_LOG(_pr_thread_lm, PR_LOG_MIN, ("thread exiting"));

        
        _PR_CleanupThread(thread);

        _PR_INTSOFF(is);    

        _PR_NotifyJoinWaiters(thread);

    _PR_DecrActiveThreadCount(thread);

        thread->state = _PR_DEAD_STATE;

        if (!_pr_recycleThreads || (_PR_RecycleThread(thread) ==
                        PR_FAILURE)) {
            


        _PR_UserDestroyThread(thread);
        }

        



        {
            PRInt32 is;
            _PR_INTSOFF(is);
            _PR_MD_SWITCH_CONTEXT(thread);
        }

        
    }
}

void _PR_SetThreadPriority(PRThread *thread, PRThreadPriority newPri)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRIntn is;

    if ( _PR_IS_NATIVE_THREAD(thread) ) {
        _PR_MD_SET_PRIORITY(&(thread->md), newPri);
        return;
    }

    if (!_PR_IS_NATIVE_THREAD(me))
    _PR_INTSOFF(is);
    _PR_THREAD_LOCK(thread);
    if (newPri != thread->priority) {
    _PRCPU *cpu = thread->cpu;

    switch (thread->state) {
      case _PR_RUNNING:
        

            _PR_RUNQ_LOCK(cpu);
        thread->priority = newPri;
        if (_PR_RUNQREADYMASK(cpu) >> (newPri + 1)) {
            if (!_PR_IS_NATIVE_THREAD(me))
                    _PR_SET_RESCHED_FLAG();
        }
            _PR_RUNQ_UNLOCK(cpu);
        break;

      case _PR_RUNNABLE:

        _PR_RUNQ_LOCK(cpu);
            
            _PR_DEL_RUNQ(thread);
            thread->priority = newPri;
            PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));
            _PR_ADD_RUNQ(thread, cpu, newPri);
        _PR_RUNQ_UNLOCK(cpu);

            if (newPri > me->priority) {
            if (!_PR_IS_NATIVE_THREAD(me))
                    _PR_SET_RESCHED_FLAG();
            }

        break;

      case _PR_LOCK_WAIT:
      case _PR_COND_WAIT:
      case _PR_IO_WAIT:
      case _PR_SUSPENDED:

        thread->priority = newPri;
        break;
    }
    }
    _PR_THREAD_UNLOCK(thread);
    if (!_PR_IS_NATIVE_THREAD(me))
    _PR_INTSON(is);
}




static void _PR_Suspend(PRThread *thread)
{
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(thread != me);
    PR_ASSERT(!_PR_IS_NATIVE_THREAD(thread) || (!thread->cpu));

    if (!_PR_IS_NATIVE_THREAD(me))
        _PR_INTSOFF(is);
    _PR_THREAD_LOCK(thread);
    switch (thread->state) {
      case _PR_RUNNABLE:
        if (!_PR_IS_NATIVE_THREAD(thread)) {
            _PR_RUNQ_LOCK(thread->cpu);
            _PR_DEL_RUNQ(thread);
            _PR_RUNQ_UNLOCK(thread->cpu);

            _PR_MISCQ_LOCK(thread->cpu);
            _PR_ADD_SUSPENDQ(thread, thread->cpu);
            _PR_MISCQ_UNLOCK(thread->cpu);
        } else {
            


             PR_ASSERT(0);
        }
        thread->state = _PR_SUSPENDED;
        break;

      case _PR_RUNNING:
        



        PR_ASSERT(0);
        break;

      case _PR_LOCK_WAIT:
      case _PR_IO_WAIT:
      case _PR_COND_WAIT:
        if (_PR_IS_NATIVE_THREAD(thread)) {
            _PR_MD_SUSPEND_THREAD(thread);
    }
        thread->flags |= _PR_SUSPENDING;
        break;

      default:
        PR_Abort();
    }
    _PR_THREAD_UNLOCK(thread);
    if (!_PR_IS_NATIVE_THREAD(me))
    _PR_INTSON(is);
}

static void _PR_Resume(PRThread *thread)
{
    PRThreadPriority pri;
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if (!_PR_IS_NATIVE_THREAD(me))
    _PR_INTSOFF(is);
    _PR_THREAD_LOCK(thread);
    switch (thread->state) {
      case _PR_SUSPENDED:
        thread->state = _PR_RUNNABLE;
        thread->flags &= ~_PR_SUSPENDING;
        if (!_PR_IS_NATIVE_THREAD(thread)) {
            _PR_MISCQ_LOCK(thread->cpu);
            _PR_DEL_SUSPENDQ(thread);
            _PR_MISCQ_UNLOCK(thread->cpu);

            pri = thread->priority;

            _PR_RUNQ_LOCK(thread->cpu);
            _PR_ADD_RUNQ(thread, thread->cpu, pri);
            _PR_RUNQ_UNLOCK(thread->cpu);

            if (pri > _PR_MD_CURRENT_THREAD()->priority) {
                if (!_PR_IS_NATIVE_THREAD(me))
                    _PR_SET_RESCHED_FLAG();
            }
        } else {
            PR_ASSERT(0);
        }
        break;

      case _PR_IO_WAIT:
      case _PR_COND_WAIT:
        thread->flags &= ~_PR_SUSPENDING;

        break;

      case _PR_LOCK_WAIT: 
      {
        PRLock *wLock = thread->wait.lock;

        thread->flags &= ~_PR_SUSPENDING;
 
        _PR_LOCK_LOCK(wLock);
        if (thread->wait.lock->owner == 0) {
            _PR_UnblockLockWaiter(thread->wait.lock);
        }
        _PR_LOCK_UNLOCK(wLock);
        break;
      }
      case _PR_RUNNABLE:
        break;
      case _PR_RUNNING:
        



        PR_ASSERT(0);
        break;

      default:
    



        PR_Abort();
    }
    _PR_THREAD_UNLOCK(thread);
    if (!_PR_IS_NATIVE_THREAD(me))
        _PR_INTSON(is);

}

#if !defined(_PR_LOCAL_THREADS_ONLY) && defined(XP_UNIX)
static PRThread *get_thread(_PRCPU *cpu, PRBool *wakeup_cpus)
{
    PRThread *thread;
    PRIntn pri;
    PRUint32 r;
    PRCList *qp;
    PRIntn priMin, priMax;

    _PR_RUNQ_LOCK(cpu);
    r = _PR_RUNQREADYMASK(cpu);
    if (r==0) {
        priMin = priMax = PR_PRIORITY_FIRST;
    } else if (r == (1<<PR_PRIORITY_NORMAL) ) {
        priMin = priMax = PR_PRIORITY_NORMAL;
    } else {
        priMin = PR_PRIORITY_FIRST;
        priMax = PR_PRIORITY_LAST;
    }
    thread = NULL;
    for (pri = priMax; pri >= priMin ; pri-- ) {
    if (r & (1 << pri)) {
            for (qp = _PR_RUNQ(cpu)[pri].next; 
                 qp != &_PR_RUNQ(cpu)[pri];
                 qp = qp->next) {
                thread = _PR_THREAD_PTR(qp);
                


                PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));
                if (thread->no_sched) {
                    thread = NULL;
                    





                    *wakeup_cpus = PR_TRUE;
                    continue;
                } else if (thread->flags & _PR_BOUND_THREAD) {
                    



                    thread = NULL;
#ifdef IRIX
					_PR_MD_WAKEUP_PRIMORDIAL_CPU();
#endif
                    continue;
                } else if (thread->io_pending == PR_TRUE) {
                    





                    thread = NULL;
                    continue;
                } else {
                    
                    _PR_DEL_RUNQ(thread);
                    _PR_RUNQ_UNLOCK(cpu);
                    return(thread);
                }
            }
        }
        thread = NULL;
    }
    _PR_RUNQ_UNLOCK(cpu);
    return(thread);
}
#endif 










void _PR_Schedule(void)
{
    PRThread *thread, *me = _PR_MD_CURRENT_THREAD();
    _PRCPU *cpu = _PR_MD_CURRENT_CPU();
    PRIntn pri;
    PRUint32 r;
    PRCList *qp;
    PRIntn priMin, priMax;
#if !defined(_PR_LOCAL_THREADS_ONLY) && defined(XP_UNIX)
    PRBool wakeup_cpus;
#endif

    
    PR_ASSERT(_PR_IS_NATIVE_THREAD(me) || _PR_MD_GET_INTSOFF() != 0);

    
    _PR_CLEAR_RESCHED_FLAG();

    



    _PR_RUNQ_LOCK(cpu);
    






    if ((thread = suspendAllThread) != 0) {
    if ((!(thread->no_sched)) && (thread->state == _PR_RUNNABLE)) {
            
            _PR_DEL_RUNQ(thread);
            _PR_RUNQ_UNLOCK(cpu);
            goto found_thread;
    } else {
            thread = NULL;
            _PR_RUNQ_UNLOCK(cpu);
            goto idle_thread;
    }
    }
    r = _PR_RUNQREADYMASK(cpu);
    if (r==0) {
        priMin = priMax = PR_PRIORITY_FIRST;
    } else if (r == (1<<PR_PRIORITY_NORMAL) ) {
        priMin = priMax = PR_PRIORITY_NORMAL;
    } else {
        priMin = PR_PRIORITY_FIRST;
        priMax = PR_PRIORITY_LAST;
    }
    thread = NULL;
    for (pri = priMax; pri >= priMin ; pri-- ) {
    if (r & (1 << pri)) {
            for (qp = _PR_RUNQ(cpu)[pri].next; 
                 qp != &_PR_RUNQ(cpu)[pri];
                 qp = qp->next) {
                thread = _PR_THREAD_PTR(qp);
                


                PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));
                if ((thread->no_sched) && (me != thread)){
                    thread = NULL;
                    continue;
                } else {
                    
                    _PR_DEL_RUNQ(thread);
                    _PR_RUNQ_UNLOCK(cpu);
                    goto found_thread;
                }
            }
        }
        thread = NULL;
    }
    _PR_RUNQ_UNLOCK(cpu);

#if !defined(_PR_LOCAL_THREADS_ONLY) && defined(XP_UNIX)

    wakeup_cpus = PR_FALSE;
    _PR_CPU_LIST_LOCK();
    for (qp = _PR_CPUQ().next; qp != &_PR_CPUQ(); qp = qp->next) {
        if (cpu != _PR_CPU_PTR(qp)) {
            if ((thread = get_thread(_PR_CPU_PTR(qp), &wakeup_cpus))
                                        != NULL) {
                thread->cpu = cpu;
                _PR_CPU_LIST_UNLOCK();
                if (wakeup_cpus == PR_TRUE)
                    _PR_MD_WAKEUP_CPUS();
                goto found_thread;
            }
        }
    }
    _PR_CPU_LIST_UNLOCK();
    if (wakeup_cpus == PR_TRUE)
        _PR_MD_WAKEUP_CPUS();

#endif        

idle_thread:
   


    PR_LOG(_pr_sched_lm, PR_LOG_MAX, ("pausing"));
    thread = _PR_MD_CURRENT_CPU()->idle_thread;

found_thread:
    PR_ASSERT((me == thread) || ((thread->state == _PR_RUNNABLE) &&
                    (!(thread->no_sched))));

    
    PR_LOG(_pr_sched_lm, PR_LOG_MAX,
       ("switching to %d[%p]", thread->id, thread));
    PR_ASSERT(thread->state != _PR_RUNNING);
    thread->state = _PR_RUNNING;
 
    



	PR_ASSERT(thread->cpu == _PR_MD_CURRENT_CPU());
    if (thread != me) 
        _PR_MD_RESTORE_CONTEXT(thread);
#if 0
    



    PR_NOT_REACHED("impossible return from schedule");
#endif
}






static PRThread *
_PR_AttachThread(PRThreadType type, PRThreadPriority priority,
    PRThreadStack *stack)
{
    PRThread *thread;
    char *mem;

    if (priority > PR_PRIORITY_LAST) {
        priority = PR_PRIORITY_LAST;
    } else if (priority < PR_PRIORITY_FIRST) {
        priority = PR_PRIORITY_FIRST;
    }

    mem = (char*) PR_CALLOC(sizeof(PRThread));
    if (mem) {
        thread = (PRThread*) mem;
        thread->priority = priority;
        thread->stack = stack;
        thread->state = _PR_RUNNING;
        PR_INIT_CLIST(&thread->lockList);
        if (_PR_MD_NEW_LOCK(&thread->threadLock) == PR_FAILURE) {
        PR_DELETE(thread);
        return 0;
    }

        return thread;
    }
    return 0;
}



PR_IMPLEMENT(PRThread*) 
_PR_NativeCreateThread(PRThreadType type,
                     void (*start)(void *arg),
                     void *arg,
                     PRThreadPriority priority,
                     PRThreadScope scope,
                     PRThreadState state,
                     PRUint32 stackSize,
                     PRUint32 flags)
{
    PRThread *thread;

    thread = _PR_AttachThread(type, priority, NULL);

    if (thread) {
        PR_Lock(_pr_activeLock);
        thread->flags = (flags | _PR_GLOBAL_SCOPE);
        thread->id = ++_pr_utid;
        if (type == PR_SYSTEM_THREAD) {
            thread->flags |= _PR_SYSTEM;
            _pr_systemActive++;
        } else {
            _pr_userActive++;
        }
        PR_Unlock(_pr_activeLock);

        thread->stack = PR_NEWZAP(PRThreadStack);
        if (!thread->stack) {
            PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
            goto done;
        }
        thread->stack->stackSize = stackSize?stackSize:_MD_DEFAULT_STACK_SIZE;
        thread->stack->thr = thread;
        thread->startFunc = start;
        thread->arg = arg;

        



        if (state == PR_JOINABLE_THREAD) {
            thread->term = PR_NewCondVar(_pr_terminationCVLock);
        if (thread->term == NULL) {
        PR_DELETE(thread->stack);
        goto done;
        }
        }

    thread->state = _PR_RUNNING;
        if (_PR_MD_CREATE_THREAD(thread, _PR_NativeRunThread, priority,
            scope,state,stackSize) == PR_SUCCESS) {
            return thread;
        }
        if (thread->term) {
            PR_DestroyCondVar(thread->term);
            thread->term = NULL;
        }
    PR_DELETE(thread->stack);
    }

done:
    if (thread) {
    _PR_DecrActiveThreadCount(thread);
        _PR_DestroyThread(thread);
    }
    return NULL;
}



PR_IMPLEMENT(PRThread*) _PR_CreateThread(PRThreadType type,
                     void (*start)(void *arg),
                     void *arg,
                     PRThreadPriority priority,
                     PRThreadScope scope,
                     PRThreadState state,
                     PRUint32 stackSize,
                     PRUint32 flags)
{
    PRThread *me;
    PRThread *thread = NULL;
    PRThreadStack *stack;
    char *top;
    PRIntn is;
    PRIntn native = 0;
    PRIntn useRecycled = 0;
    PRBool status;

    



    if (priority > PR_PRIORITY_LAST) {
        priority = PR_PRIORITY_LAST;
    } else if (priority < PR_PRIORITY_FIRST) {
        priority = PR_PRIORITY_FIRST;
    }
        
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (! (flags & _PR_IDLE_THREAD))
        me = _PR_MD_CURRENT_THREAD();

#if    defined(_PR_GLOBAL_THREADS_ONLY)
	


    if (scope == PR_LOCAL_THREAD)
    	scope = PR_GLOBAL_THREAD;
#endif

	if (_native_threads_only)
		scope = PR_GLOBAL_THREAD;

    native = (((scope == PR_GLOBAL_THREAD)|| (scope == PR_GLOBAL_BOUND_THREAD))
							&& _PR_IS_NATIVE_THREAD_SUPPORTED());

    _PR_ADJUST_STACKSIZE(stackSize);

    if (native) {
    



    flags &= ~_PR_IDLE_THREAD;
        flags |= _PR_GLOBAL_SCOPE;
        if (_PR_NUM_DEADNATIVE > 0) {
            _PR_DEADQ_LOCK;

            if (_PR_NUM_DEADNATIVE == 0) { 
                _PR_DEADQ_UNLOCK;
            } else {
                thread = _PR_THREAD_PTR(_PR_DEADNATIVEQ.next);
                PR_REMOVE_LINK(&thread->links);
                _PR_DEC_DEADNATIVE;
                _PR_DEADQ_UNLOCK;

                _PR_InitializeRecycledThread(thread);
                thread->startFunc = start;
                thread->arg = arg;
            thread->flags = (flags | _PR_GLOBAL_SCOPE);
            if (type == PR_SYSTEM_THREAD)
            {
                thread->flags |= _PR_SYSTEM;
                PR_AtomicIncrement(&_pr_systemActive);
            }
            else PR_AtomicIncrement(&_pr_userActive);

            if (state == PR_JOINABLE_THREAD) {
                if (!thread->term) 
                       thread->term = PR_NewCondVar(_pr_terminationCVLock);
            }
        else {
                if(thread->term) {
                    PR_DestroyCondVar(thread->term);
                        thread->term = 0;
            }
            }

                thread->priority = priority;
        _PR_MD_SET_PRIORITY(&(thread->md), priority);
        
        thread->state = _PR_RUNNING;
                _PR_MD_WAKEUP_WAITER(thread);
        return thread;
            }
        }
        thread = _PR_NativeCreateThread(type, start, arg, priority, 
                                            scope, state, stackSize, flags);
    } else {
        if (_PR_NUM_DEADUSER > 0) {
            _PR_DEADQ_LOCK;

            if (_PR_NUM_DEADUSER == 0) {  
                _PR_DEADQ_UNLOCK;
            } else {
                PRCList *ptr;

                


                ptr = _PR_DEADUSERQ.next;
                while( ptr != &_PR_DEADUSERQ ) {
                    thread = _PR_THREAD_PTR(ptr);
                    if ((thread->stack->stackSize >= stackSize) &&
                (!thread->no_sched)) {
                        PR_REMOVE_LINK(&thread->links);
                        _PR_DEC_DEADUSER;
                        break;
                    } else {
                        ptr = ptr->next;
                        thread = NULL;
                    }
                } 

                _PR_DEADQ_UNLOCK;

               if (thread) {
                    _PR_InitializeRecycledThread(thread);
                    thread->startFunc = start;
                    thread->arg = arg;
                    thread->priority = priority;
            if (state == PR_JOINABLE_THREAD) {
            if (!thread->term) 
               thread->term = PR_NewCondVar(_pr_terminationCVLock);
            } else {
            if(thread->term) {
               PR_DestroyCondVar(thread->term);
                thread->term = 0;
            }
            }
                    useRecycled++;
                }
            }
        } 
        if (thread == NULL) {
#ifndef HAVE_CUSTOM_USER_THREADS
            stack = _PR_NewStack(stackSize);
            if (!stack) {
                PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
                return NULL;
            }

            
            top = stack->stackTop;
#ifdef HAVE_STACK_GROWING_UP
            thread = (PRThread*) top;
            top = top + sizeof(PRThread);
            


            if ((PRUptrdiff)top & 0x3f) {
                top = (char*)(((PRUptrdiff)top + 0x40) & ~0x3f);
            }
#else
            top = top - sizeof(PRThread);
            thread = (PRThread*) top;
            


            if ((PRUptrdiff)top & 0x3f) {
                top = (char*)((PRUptrdiff)top & ~0x3f);
            }
#endif
#if defined(GC_LEAK_DETECTOR)
            




            thread = PR_NEW(PRThread);
#endif
            stack->thr = thread;
            memset(thread, 0, sizeof(PRThread));
            thread->threadAllocatedOnStack = 1;
#else
            thread = _PR_MD_CREATE_USER_THREAD(stackSize, start, arg);
            if (!thread) {
                PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
                return NULL;
            }
            thread->threadAllocatedOnStack = 0;
            stack = NULL;
            top = NULL;
#endif

            
            thread->tpdLength = 0;
            thread->privateData = NULL;
            thread->stack = stack;
            thread->priority = priority;
            thread->startFunc = start;
            thread->arg = arg;
            PR_INIT_CLIST(&thread->lockList);

            if (_PR_MD_INIT_THREAD(thread) == PR_FAILURE) {
                if (thread->threadAllocatedOnStack == 1)
                    _PR_FreeStack(thread->stack);
                else {
                    PR_DELETE(thread);
                }
                PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
                return NULL;
            }

            if (_PR_MD_NEW_LOCK(&thread->threadLock) == PR_FAILURE) {
                if (thread->threadAllocatedOnStack == 1)
                    _PR_FreeStack(thread->stack);
                else {
                    PR_DELETE(thread->privateData);
                    PR_DELETE(thread);
                }
                PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
                return NULL;
            }

            _PR_MD_INIT_CONTEXT(thread, top, _PR_UserRunThread, &status);

            if (status == PR_FALSE) {
                _PR_MD_FREE_LOCK(&thread->threadLock);
                if (thread->threadAllocatedOnStack == 1)
                    _PR_FreeStack(thread->stack);
                else {
                    PR_DELETE(thread->privateData);
                    PR_DELETE(thread);
                }
                return NULL;
            }

            



            if (state == PR_JOINABLE_THREAD) {
                thread->term = PR_NewCondVar(_pr_terminationCVLock);
                if (thread->term == NULL) {
                    _PR_MD_FREE_LOCK(&thread->threadLock);
                    if (thread->threadAllocatedOnStack == 1)
                        _PR_FreeStack(thread->stack);
                    else {
                        PR_DELETE(thread->privateData);
                        PR_DELETE(thread);
                    }
                    return NULL;
                }
            }
  
        }
  
        
        PR_Lock(_pr_activeLock);
        thread->flags = flags;
        thread->id = ++_pr_utid;
        if (type == PR_SYSTEM_THREAD) {
            thread->flags |= _PR_SYSTEM;
            _pr_systemActive++;
        } else {
            _pr_userActive++;
        }

        
        thread->state = _PR_RUNNABLE;
    


        PR_Unlock(_pr_activeLock);

        if ((! (thread->flags & _PR_IDLE_THREAD)) && _PR_IS_NATIVE_THREAD(me) )
            thread->cpu = _PR_GetPrimordialCPU();
        else
            thread->cpu = _PR_MD_CURRENT_CPU();

        PR_ASSERT(!_PR_IS_NATIVE_THREAD(thread));

        if ((! (thread->flags & _PR_IDLE_THREAD)) && !_PR_IS_NATIVE_THREAD(me)) {
            _PR_INTSOFF(is);
            _PR_RUNQ_LOCK(thread->cpu);
            _PR_ADD_RUNQ(thread, thread->cpu, priority);
            _PR_RUNQ_UNLOCK(thread->cpu);
        }

        if (thread->flags & _PR_IDLE_THREAD) {
            





            _PR_MD_WAKEUP_WAITER(NULL);
        } else if (_PR_IS_NATIVE_THREAD(me)) {
            _PR_MD_WAKEUP_WAITER(thread);
        }
        if ((! (thread->flags & _PR_IDLE_THREAD)) && !_PR_IS_NATIVE_THREAD(me) )
            _PR_INTSON(is);
    }

    return thread;
}

PR_IMPLEMENT(PRThread*) PR_CreateThread(PRThreadType type,
                     void (*start)(void *arg),
                     void *arg,
                     PRThreadPriority priority,
                     PRThreadScope scope,
                     PRThreadState state,
                     PRUint32 stackSize)
{
    return _PR_CreateThread(type, start, arg, priority, scope, state, 
                            stackSize, 0);
}













PRThread* _PRI_AttachThread(PRThreadType type,
    PRThreadPriority priority, PRThreadStack *stack, PRUint32 flags)
{
    PRThread *thread;

    if ((thread = _PR_MD_GET_ATTACHED_THREAD()) != NULL) {
        return thread;
    }
    _PR_MD_SET_CURRENT_THREAD(NULL);

    
    _PR_MD_SET_CURRENT_CPU(NULL);

    thread = _PR_AttachThread(type, priority, stack);
    if (thread) {
        PRIntn is;

        _PR_MD_SET_CURRENT_THREAD(thread);

        thread->flags = flags | _PR_GLOBAL_SCOPE | _PR_ATTACHED;

        if (!stack) {
            thread->stack = PR_NEWZAP(PRThreadStack);
            if (!thread->stack) {
                _PR_DestroyThread(thread);
                return NULL;
            }
            thread->stack->stackSize = _MD_DEFAULT_STACK_SIZE;
        }
        PR_INIT_CLIST(&thread->links);

        if (_PR_MD_INIT_ATTACHED_THREAD(thread) == PR_FAILURE) {
                PR_DELETE(thread->stack);
                _PR_DestroyThread(thread);
                return NULL;
        }

        _PR_MD_SET_CURRENT_CPU(NULL);

        if (_PR_MD_CURRENT_CPU()) {
            _PR_INTSOFF(is);
            PR_Lock(_pr_activeLock);
        }
        if (type == PR_SYSTEM_THREAD) {
            thread->flags |= _PR_SYSTEM;
            _pr_systemActive++;
        } else {
            _pr_userActive++;
        }
        if (_PR_MD_CURRENT_CPU()) {
            PR_Unlock(_pr_activeLock);
            _PR_INTSON(is);
        }
    }
    return thread;
}

PR_IMPLEMENT(PRThread*) PR_AttachThread(PRThreadType type,
    PRThreadPriority priority, PRThreadStack *stack)
{
    return PR_GetCurrentThread();
}

PR_IMPLEMENT(void) PR_DetachThread(void)
{
    



#if !defined(IRIX) && !defined(WIN32) \
        && !(defined(SOLARIS) && defined(_PR_GLOBAL_THREADS_ONLY))
    PRThread *me;
    if (_pr_initialized) {
        me = _PR_MD_GET_ATTACHED_THREAD();
        if ((me != NULL) && (me->flags & _PR_ATTACHED))
            _PRI_DetachThread();
    }
#endif
}

void _PRI_DetachThread(void)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();

	if (me->flags & _PR_PRIMORDIAL) {
		


		return;
	}
    PR_ASSERT(me->flags & _PR_ATTACHED);
    PR_ASSERT(_PR_IS_NATIVE_THREAD(me));
    _PR_CleanupThread(me);
    PR_DELETE(me->privateData);

    _PR_DecrActiveThreadCount(me);

    _PR_MD_CLEAN_THREAD(me);
    _PR_MD_SET_CURRENT_THREAD(NULL);
    if (!me->threadAllocatedOnStack) 
        PR_DELETE(me->stack);
    _PR_MD_FREE_LOCK(&me->threadLock);
    PR_DELETE(me);
}














PR_IMPLEMENT(PRStatus) PR_JoinThread(PRThread *thread)
{
    PRIntn is;
    PRCondVar *term;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if (!_PR_IS_NATIVE_THREAD(me))
        _PR_INTSOFF(is);
    term = thread->term;
    
    if (term == NULL) {
        PR_SetError(PR_INVALID_ARGUMENT_ERROR, 0);
        goto ErrorExit;
    }

    
    if (term->condQ.next != &term->condQ) {
        goto ErrorExit;
    }
    if (!_PR_IS_NATIVE_THREAD(me))
        _PR_INTSON(is);

    
    PR_Lock (_pr_terminationCVLock);
    while (thread->state != _PR_JOIN_WAIT) {
        (void) PR_WaitCondVar(term, PR_INTERVAL_NO_TIMEOUT);
    }
    (void) PR_Unlock (_pr_terminationCVLock);
    
    



    
    if (!_PR_IS_NATIVE_THREAD(me))
        _PR_INTSOFF(is);
    thread->state = _PR_RUNNABLE;
    if ( !_PR_IS_NATIVE_THREAD(thread) ) {
        _PR_THREAD_LOCK(thread);

        _PR_MISCQ_LOCK(thread->cpu);
        _PR_DEL_JOINQ(thread);
        _PR_MISCQ_UNLOCK(thread->cpu);

        _PR_AddThreadToRunQ(me, thread);
        _PR_THREAD_UNLOCK(thread);
    }
    if (!_PR_IS_NATIVE_THREAD(me))
        _PR_INTSON(is);

    _PR_MD_WAKEUP_WAITER(thread);

    return PR_SUCCESS;

ErrorExit:
    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);
    return PR_FAILURE;   
}

PR_IMPLEMENT(void) PR_SetThreadPriority(PRThread *thread,
    PRThreadPriority newPri)
{

    



    if ((PRIntn)newPri > (PRIntn)PR_PRIORITY_LAST) {
        newPri = PR_PRIORITY_LAST;
    } else if ((PRIntn)newPri < (PRIntn)PR_PRIORITY_FIRST) {
        newPri = PR_PRIORITY_FIRST;
    }
        
    if ( _PR_IS_NATIVE_THREAD(thread) ) {
        thread->priority = newPri;
        _PR_MD_SET_PRIORITY(&(thread->md), newPri);
    } else _PR_SetThreadPriority(thread, newPri);
}






PR_IMPLEMENT(void) PR_SuspendAll(void)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRCList *qp;

    


    PR_Lock(_pr_activeLock);
    suspendAllOn = PR_TRUE;
    suspendAllThread = _PR_MD_CURRENT_THREAD();
    _PR_MD_BEGIN_SUSPEND_ALL();
    for (qp = _PR_ACTIVE_LOCAL_THREADQ().next;
        qp != &_PR_ACTIVE_LOCAL_THREADQ(); qp = qp->next) {
        if ((me != _PR_ACTIVE_THREAD_PTR(qp)) && 
            _PR_IS_GCABLE_THREAD(_PR_ACTIVE_THREAD_PTR(qp))) {
            _PR_Suspend(_PR_ACTIVE_THREAD_PTR(qp));
                PR_ASSERT((_PR_ACTIVE_THREAD_PTR(qp))->state != _PR_RUNNING);
            }
    }
    for (qp = _PR_ACTIVE_GLOBAL_THREADQ().next;
        qp != &_PR_ACTIVE_GLOBAL_THREADQ(); qp = qp->next) {
        if ((me != _PR_ACTIVE_THREAD_PTR(qp)) &&
            _PR_IS_GCABLE_THREAD(_PR_ACTIVE_THREAD_PTR(qp)))
            
                _PR_MD_SUSPEND_THREAD(_PR_ACTIVE_THREAD_PTR(qp)); 
    }
    _PR_MD_END_SUSPEND_ALL();
}





PR_IMPLEMENT(void) PR_ResumeAll(void)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRCList *qp;

    


    _PR_MD_BEGIN_RESUME_ALL();
    for (qp = _PR_ACTIVE_LOCAL_THREADQ().next;
        qp != &_PR_ACTIVE_LOCAL_THREADQ(); qp = qp->next) {
        if ((me != _PR_ACTIVE_THREAD_PTR(qp)) && 
            _PR_IS_GCABLE_THREAD(_PR_ACTIVE_THREAD_PTR(qp)))
            _PR_Resume(_PR_ACTIVE_THREAD_PTR(qp));
    }
    for (qp = _PR_ACTIVE_GLOBAL_THREADQ().next;
        qp != &_PR_ACTIVE_GLOBAL_THREADQ(); qp = qp->next) {
        if ((me != _PR_ACTIVE_THREAD_PTR(qp)) &&
            _PR_IS_GCABLE_THREAD(_PR_ACTIVE_THREAD_PTR(qp)))
                _PR_MD_RESUME_THREAD(_PR_ACTIVE_THREAD_PTR(qp));
    }
    _PR_MD_END_RESUME_ALL();
    suspendAllThread = NULL;
    suspendAllOn = PR_FALSE;
    PR_Unlock(_pr_activeLock);
}

PR_IMPLEMENT(PRStatus) PR_EnumerateThreads(PREnumerator func, void *arg)
{
    PRCList *qp, *qp_next;
    PRIntn i = 0;
    PRStatus rv = PR_SUCCESS;
    PRThread* t;

    



    PR_ASSERT(suspendAllOn);

    









    


    for (qp = _PR_ACTIVE_LOCAL_THREADQ().next;
         qp != &_PR_ACTIVE_LOCAL_THREADQ(); qp = qp_next)
    {
        qp_next = qp->next;
        t = _PR_ACTIVE_THREAD_PTR(qp);
        if (_PR_IS_GCABLE_THREAD(t))
        {
            rv = (*func)(t, i, arg);
            if (rv != PR_SUCCESS)
                return rv;
            i++;
        }
    }
    for (qp = _PR_ACTIVE_GLOBAL_THREADQ().next;
         qp != &_PR_ACTIVE_GLOBAL_THREADQ(); qp = qp_next)
    {
        qp_next = qp->next;
        t = _PR_ACTIVE_THREAD_PTR(qp);
        if (_PR_IS_GCABLE_THREAD(t))
        {
            rv = (*func)(t, i, arg);
            if (rv != PR_SUCCESS)
                return rv;
            i++;
        }
    }
    return rv;
}








PR_IMPLEMENT(void)
_PR_AddSleepQ(PRThread *thread, PRIntervalTime timeout)
{
    _PRCPU *cpu = thread->cpu;

    if (timeout == PR_INTERVAL_NO_TIMEOUT) {
        
        PR_APPEND_LINK(&thread->links, &_PR_PAUSEQ(thread->cpu));
        thread->flags |= _PR_ON_PAUSEQ;
    } else {
        PRIntervalTime sleep;
        PRCList *q;
        PRThread *t;

        
        sleep = timeout;

        
        if (timeout >= _PR_SLEEPQMAX(cpu)) {
            PR_INSERT_BEFORE(&thread->links, &_PR_SLEEPQ(cpu));
            thread->sleep = timeout - _PR_SLEEPQMAX(cpu);
            _PR_SLEEPQMAX(cpu) = timeout;
        } else {
            
            q = _PR_SLEEPQ(cpu).next;

            
            while (q != &_PR_SLEEPQ(cpu)) {
                t = _PR_THREAD_PTR(q);
                if (sleep < t->sleep) {
                    
                    break;
                }
                sleep -= t->sleep;
                q = q->next;
            }
            thread->sleep = sleep;
            PR_INSERT_BEFORE(&thread->links, q);

            



            PR_ASSERT (thread->links.next != &_PR_SLEEPQ(cpu));
          
            t = _PR_THREAD_PTR(thread->links.next);
            PR_ASSERT(_PR_THREAD_PTR(t->links.prev) == thread);
            t->sleep -= sleep;
        }

        thread->flags |= _PR_ON_SLEEPQ;
    }
}












PR_IMPLEMENT(void)
_PR_DelSleepQ(PRThread *thread, PRBool propogate_time)
{
    _PRCPU *cpu = thread->cpu;

    
    if (thread->flags & (_PR_ON_PAUSEQ|_PR_ON_SLEEPQ)) {
        if (thread->flags & _PR_ON_SLEEPQ) {
            PRCList *q = thread->links.next;
            if (q != &_PR_SLEEPQ(cpu)) {
                if (propogate_time == PR_TRUE) {
                    PRThread *after = _PR_THREAD_PTR(q);
                    after->sleep += thread->sleep;
                } else 
                    _PR_SLEEPQMAX(cpu) -= thread->sleep;
            } else {
                


                if (thread->links.prev != &_PR_SLEEPQ(cpu))
                    _PR_SLEEPQMAX(cpu) -= thread->sleep;
                else
                    _PR_SLEEPQMAX(cpu) = 0;
            }
            thread->flags &= ~_PR_ON_SLEEPQ;
        } else {
            thread->flags &= ~_PR_ON_PAUSEQ;
        }
        PR_REMOVE_LINK(&thread->links);
    } else 
        PR_ASSERT(0);
}

void
_PR_AddThreadToRunQ(
    PRThread *me,     
    PRThread *thread) 
{
    PRThreadPriority pri = thread->priority;
    _PRCPU *cpu = thread->cpu;

    PR_ASSERT(!_PR_IS_NATIVE_THREAD(thread));

#if defined(WINNT)
    















    if ((!_PR_IS_NATIVE_THREAD(me) && (cpu == me->cpu)) ||
					(thread->md.thr_bound_cpu)) {
		PR_ASSERT(!thread->md.thr_bound_cpu ||
							(thread->md.thr_bound_cpu == cpu));
        _PR_RUNQ_LOCK(cpu);
        _PR_ADD_RUNQ(thread, cpu, pri);
        _PR_RUNQ_UNLOCK(cpu);
    }
#else
    _PR_RUNQ_LOCK(cpu);
    _PR_ADD_RUNQ(thread, cpu, pri);
    _PR_RUNQ_UNLOCK(cpu);
    if (!_PR_IS_NATIVE_THREAD(me) && (cpu == me->cpu)) {
        if (pri > me->priority) {
            _PR_SET_RESCHED_FLAG();
        }
    }
#endif
}
