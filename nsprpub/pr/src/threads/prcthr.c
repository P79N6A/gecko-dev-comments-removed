




































#include "primpl.h"

#if defined(WIN95)






#pragma warning(disable : 4101)
#endif


extern PRLock *_pr_sleeplock;  







void _PR_CleanupThread(PRThread *thread)
{
    
    _PR_DestroyThreadPrivate(thread);

    
    if (thread->dumpArg) {
        PR_DELETE(thread->dumpArg);
    }
    thread->dump = 0;

    PR_DELETE(thread->errorString);
    thread->errorStringSize = 0;
    thread->errorStringLength = 0;
    thread->environment = NULL;
}

PR_IMPLEMENT(PRStatus) PR_Yield()
{
    static PRBool warning = PR_TRUE;
    if (warning) warning = _PR_Obsolete(
        "PR_Yield()", "PR_Sleep(PR_INTERVAL_NO_WAIT)");
    return (PR_Sleep(PR_INTERVAL_NO_WAIT));
}











PR_IMPLEMENT(PRStatus) PR_Sleep(PRIntervalTime timeout)
{
    PRStatus rv = PR_SUCCESS;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (PR_INTERVAL_NO_WAIT == timeout)
    {
        


        PRIntn is;
        PRThread *me = PR_GetCurrentThread();
        PRUintn pri = me->priority;
        _PRCPU *cpu = _PR_MD_CURRENT_CPU();

        if ( _PR_IS_NATIVE_THREAD(me) ) _PR_MD_YIELD();
        else
        {
            _PR_INTSOFF(is);
            _PR_RUNQ_LOCK(cpu);
            if (_PR_RUNQREADYMASK(cpu) >> pri) {
                me->cpu = cpu;
                me->state = _PR_RUNNABLE;
                _PR_ADD_RUNQ(me, cpu, pri);
                _PR_RUNQ_UNLOCK(cpu);

                PR_LOG(_pr_sched_lm, PR_LOG_MIN, ("PR_Yield: yielding"));
                _PR_MD_SWITCH_CONTEXT(me);
                PR_LOG(_pr_sched_lm, PR_LOG_MIN, ("PR_Yield: done"));

                _PR_FAST_INTSON(is);
            }
            else
            {
                _PR_RUNQ_UNLOCK(cpu);
                _PR_INTSON(is);
            }
        }
    }
    else
    {
        





        PRCondVar *cv;
        PRIntervalTime timein;

        timein = PR_IntervalNow();
        cv = PR_NewCondVar(_pr_sleeplock);
        PR_ASSERT(cv != NULL);
        PR_Lock(_pr_sleeplock);
        do
        {
            PRIntervalTime delta = PR_IntervalNow() - timein;
            if (delta > timeout) break;
            rv = PR_WaitCondVar(cv, timeout - delta);
        } while (rv == PR_SUCCESS);
        PR_Unlock(_pr_sleeplock);
        PR_DestroyCondVar(cv);
    }
    return rv;
}

PR_IMPLEMENT(PRUint32) PR_GetThreadID(PRThread *thread)
{
    return thread->id;
}

PR_IMPLEMENT(PRThreadPriority) PR_GetThreadPriority(const PRThread *thread)
{
    return (PRThreadPriority) thread->priority;
}

PR_IMPLEMENT(PRThread *) PR_GetCurrentThread()
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    return _PR_MD_CURRENT_THREAD();
}







PR_IMPLEMENT(PRStatus) PR_Interrupt(PRThread *thread)
{
#ifdef _PR_GLOBAL_THREADS_ONLY
    PRCondVar *victim;

    _PR_THREAD_LOCK(thread);
    thread->flags |= _PR_INTERRUPT;
    victim = thread->wait.cvar;
    _PR_THREAD_UNLOCK(thread);
    if ((NULL != victim) && (!(thread->flags & _PR_INTERRUPT_BLOCKED))) {
        int haveLock = (victim->lock->owner == _PR_MD_CURRENT_THREAD());

        if (!haveLock) PR_Lock(victim->lock);
        PR_NotifyAllCondVar(victim);
        if (!haveLock) PR_Unlock(victim->lock);
    }
    return PR_SUCCESS;
#else  
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

            if (!_PR_IS_NATIVE_THREAD(me))
            	_PR_INTSOFF(is);

            _PR_THREAD_LOCK(thread);
            thread->flags |= _PR_INTERRUPT;
        switch (thread->state) {
                case _PR_COND_WAIT:
                        



						if (!(thread->flags & _PR_INTERRUPT_BLOCKED))
                        	_PR_NotifyLockedThread(thread);
                        break;
                case _PR_IO_WAIT:
                        




#if defined(XP_UNIX) || defined(WINNT) || defined(WIN16)
						if (!(thread->flags & _PR_INTERRUPT_BLOCKED))
                        	_PR_Unblock_IO_Wait(thread);
#else
                        _PR_THREAD_UNLOCK(thread);
#endif
                        break;
                case _PR_RUNNING:
                case _PR_RUNNABLE:
                case _PR_LOCK_WAIT:
                default:
                            _PR_THREAD_UNLOCK(thread);
                        break;
        }
            if (!_PR_IS_NATIVE_THREAD(me))
            	_PR_INTSON(is);
            return PR_SUCCESS;
#endif  
}




PR_IMPLEMENT(void) PR_ClearInterrupt()
{
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

        if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
    _PR_THREAD_LOCK(me);
         me->flags &= ~_PR_INTERRUPT;
    _PR_THREAD_UNLOCK(me);
        if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);
}

PR_IMPLEMENT(void) PR_BlockInterrupt()
{
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
    _PR_THREAD_LOCK(me);
    _PR_THREAD_BLOCK_INTERRUPT(me);
    _PR_THREAD_UNLOCK(me);
    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);
}  

PR_IMPLEMENT(void) PR_UnblockInterrupt()
{
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
    _PR_THREAD_LOCK(me);
    _PR_THREAD_UNBLOCK_INTERRUPT(me);
    _PR_THREAD_UNLOCK(me);
    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);
}  




PR_IMPLEMENT(void *) PR_GetSP(PRThread *thread)
{
        return (void *)_PR_MD_GET_SP(thread);
}

PR_IMPLEMENT(void*) GetExecutionEnvironment(PRThread *thread)
{
        return thread->environment;
}

PR_IMPLEMENT(void) SetExecutionEnvironment(PRThread *thread, void *env)
{
        thread->environment = env;
}


PR_IMPLEMENT(PRInt32) PR_GetThreadAffinityMask(PRThread *thread, PRUint32 *mask)
{
#ifdef HAVE_THREAD_AFFINITY
    return _PR_MD_GETTHREADAFFINITYMASK(thread, mask);
#else
    return 0;
#endif
}

PR_IMPLEMENT(PRInt32) PR_SetThreadAffinityMask(PRThread *thread, PRUint32 mask )
{
#ifdef HAVE_THREAD_AFFINITY
#ifndef IRIX
    return _PR_MD_SETTHREADAFFINITYMASK(thread, mask);
#else
	return 0;
#endif
#else
    return 0;
#endif
}



PR_IMPLEMENT(PRInt32) PR_SetCPUAffinityMask(PRUint32 mask)
{
#ifdef HAVE_THREAD_AFFINITY
    PRCList *qp;
    extern PRUint32 _pr_cpu_affinity_mask;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    _pr_cpu_affinity_mask = mask;

    qp = _PR_CPUQ().next;
    while(qp != &_PR_CPUQ()) {
        _PRCPU *cpu;

        cpu = _PR_CPU_PTR(qp);
        PR_SetThreadAffinityMask(cpu->thread, mask);

        qp = qp->next;
    }
#endif

    return 0;
}

PRUint32 _pr_recycleThreads = 0;
PR_IMPLEMENT(void) PR_SetThreadRecycleMode(PRUint32 count)
{
    _pr_recycleThreads = count;
}

PR_IMPLEMENT(PRThread*) PR_CreateThreadGCAble(PRThreadType type,
                                     void (*start)(void *arg),
                                     void *arg,
                                     PRThreadPriority priority,
                                     PRThreadScope scope,
                                     PRThreadState state,
                                     PRUint32 stackSize)
{
    return _PR_CreateThread(type, start, arg, priority, scope, state, 
                            stackSize, _PR_GCABLE_THREAD);
}

#ifdef SOLARIS
PR_IMPLEMENT(PRThread*) PR_CreateThreadBound(PRThreadType type,
                                     void (*start)(void *arg),
                                     void *arg,
                                     PRUintn priority,
                                     PRThreadScope scope,
                                     PRThreadState state,
                                     PRUint32 stackSize)
{
    return _PR_CreateThread(type, start, arg, priority, scope, state, 
                            stackSize, _PR_BOUND_THREAD);
}
#endif


PR_IMPLEMENT(PRThread*) PR_AttachThreadGCAble(
    PRThreadType type, PRThreadPriority priority, PRThreadStack *stack)
{
    
    PR_SetError(PR_NOT_IMPLEMENTED_ERROR, 0);
    return NULL;
}

PR_IMPLEMENT(void) PR_SetThreadGCAble()
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    PR_Lock(_pr_activeLock);
        _PR_MD_CURRENT_THREAD()->flags |= _PR_GCABLE_THREAD;
        PR_Unlock(_pr_activeLock);        
}

PR_IMPLEMENT(void) PR_ClearThreadGCAble()
{
    if (!_pr_initialized) _PR_ImplicitInitialization();
    PR_Lock(_pr_activeLock);
        _PR_MD_CURRENT_THREAD()->flags &= (~_PR_GCABLE_THREAD);
        PR_Unlock(_pr_activeLock);
}

PR_IMPLEMENT(PRThreadScope) PR_GetThreadScope(const PRThread *thread)
{
    if (!_pr_initialized) _PR_ImplicitInitialization();

    if (_PR_IS_NATIVE_THREAD(thread)) {
    	return (thread->flags & _PR_BOUND_THREAD) ? PR_GLOBAL_BOUND_THREAD :
										PR_GLOBAL_THREAD;
    } else
        return PR_LOCAL_THREAD;
}

PR_IMPLEMENT(PRThreadType) PR_GetThreadType(const PRThread *thread)
{
    return (thread->flags & _PR_SYSTEM) ? PR_SYSTEM_THREAD : PR_USER_THREAD;
}

PR_IMPLEMENT(PRThreadState) PR_GetThreadState(const PRThread *thread)
{
    return (NULL == thread->term) ? PR_UNJOINABLE_THREAD : PR_JOINABLE_THREAD;
}  
