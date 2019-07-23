





































#include "primpl.h"
#include "prinrval.h"
#include "prtypes.h"

#if defined(WIN95)






#pragma warning(disable : 4101)
#endif






PRBool _PR_NotifyThread (PRThread *thread, PRThread *me)
{
    PRBool rv;

    PR_ASSERT(_PR_IS_NATIVE_THREAD(me) || _PR_MD_GET_INTSOFF() != 0);

    _PR_THREAD_LOCK(thread);
    PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));
    if ( !_PR_IS_NATIVE_THREAD(thread) ) {
        if (thread->wait.cvar != NULL) {
            thread->wait.cvar = NULL;

            _PR_SLEEPQ_LOCK(thread->cpu);
            


            if (thread->flags & (_PR_ON_SLEEPQ|_PR_ON_PAUSEQ))
                _PR_DEL_SLEEPQ(thread, PR_TRUE);
            _PR_SLEEPQ_UNLOCK(thread->cpu);

	    if (thread->flags & _PR_SUSPENDING) {
		



            	thread->state = _PR_SUSPENDED;
		_PR_MISCQ_LOCK(thread->cpu);
		_PR_ADD_SUSPENDQ(thread, thread->cpu);
		_PR_MISCQ_UNLOCK(thread->cpu);
            	_PR_THREAD_UNLOCK(thread);
	    } else {
            	
            	thread->state = _PR_RUNNABLE;
            	_PR_THREAD_UNLOCK(thread);

                _PR_AddThreadToRunQ(me, thread);
                _PR_MD_WAKEUP_WAITER(thread);
            }

            rv = PR_TRUE;
        } else {
            
            _PR_THREAD_UNLOCK(thread);
            rv = PR_FALSE;
        }
    } else { 
        if (thread->wait.cvar) {
            thread->wait.cvar = NULL;

	    if (thread->flags & _PR_SUSPENDING) {
		



            	thread->state = _PR_SUSPENDED;
	     } else
            	thread->state = _PR_RUNNING;
            _PR_THREAD_UNLOCK(thread);
            _PR_MD_WAKEUP_WAITER(thread);
            rv = PR_TRUE;
        } else {
            _PR_THREAD_UNLOCK(thread);
            rv = PR_FALSE;
        }    
    }    

    return rv;
}





void _PR_NotifyLockedThread (PRThread *thread)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRCondVar *cvar;
    PRThreadPriority pri;

    if ( !_PR_IS_NATIVE_THREAD(me))
    	PR_ASSERT(_PR_MD_GET_INTSOFF() != 0);

    cvar = thread->wait.cvar;
    thread->wait.cvar = NULL;
    _PR_THREAD_UNLOCK(thread);

    _PR_CVAR_LOCK(cvar);
    _PR_THREAD_LOCK(thread);

    if (!_PR_IS_NATIVE_THREAD(thread)) {
            _PR_SLEEPQ_LOCK(thread->cpu);
            


            if (thread->flags & (_PR_ON_SLEEPQ|_PR_ON_PAUSEQ))
                _PR_DEL_SLEEPQ(thread, PR_TRUE);
            _PR_SLEEPQ_UNLOCK(thread->cpu);

	    
	    pri = thread->priority;
	    thread->state = _PR_RUNNABLE;

	    PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));

            _PR_AddThreadToRunQ(me, thread);
            _PR_THREAD_UNLOCK(thread);

            _PR_MD_WAKEUP_WAITER(thread);
    } else {
	    if (thread->flags & _PR_SUSPENDING) {
		



            	thread->state = _PR_SUSPENDED;
	     } else
            	thread->state = _PR_RUNNING;
            _PR_THREAD_UNLOCK(thread);
            _PR_MD_WAKEUP_WAITER(thread);
    }    

    _PR_CVAR_UNLOCK(cvar);
    return;
}




PRStatus _PR_WaitCondVar(
    PRThread *thread, PRCondVar *cvar, PRLock *lock, PRIntervalTime timeout)
{
    PRIntn is;
    PRStatus rv = PR_SUCCESS;

    PR_ASSERT(thread == _PR_MD_CURRENT_THREAD());
    PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));

#ifdef _PR_GLOBAL_THREADS_ONLY
    if (_PR_PENDING_INTERRUPT(thread)) {
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        thread->flags &= ~_PR_INTERRUPT;
        return PR_FAILURE;
    }

    thread->wait.cvar = cvar;
    lock->owner = NULL;
    _PR_MD_WAIT_CV(&cvar->md,&lock->ilock, timeout);
    thread->wait.cvar = NULL;
    lock->owner = thread;
    if (_PR_PENDING_INTERRUPT(thread)) {
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        thread->flags &= ~_PR_INTERRUPT;
        return PR_FAILURE;
    }

    return PR_SUCCESS;
#else  

    if ( !_PR_IS_NATIVE_THREAD(thread))
    	_PR_INTSOFF(is);

    _PR_CVAR_LOCK(cvar);
    _PR_THREAD_LOCK(thread);

    if (_PR_PENDING_INTERRUPT(thread)) {
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        thread->flags &= ~_PR_INTERRUPT;
    	_PR_CVAR_UNLOCK(cvar);
    	_PR_THREAD_UNLOCK(thread);
    	if ( !_PR_IS_NATIVE_THREAD(thread))
    		_PR_INTSON(is);
        return PR_FAILURE;
    }

    thread->state = _PR_COND_WAIT;
    thread->wait.cvar = cvar;

    


    PR_APPEND_LINK(&thread->waitQLinks, &cvar->condQ);

    



    if ( !_PR_IS_NATIVE_THREAD(thread) ) {
        _PR_SLEEPQ_LOCK(thread->cpu);
        _PR_ADD_SLEEPQ(thread, timeout);
        _PR_SLEEPQ_UNLOCK(thread->cpu);
    }
    _PR_CVAR_UNLOCK(cvar);
    _PR_THREAD_UNLOCK(thread);
   
    



    PR_Unlock(lock);

    PR_LOG(_pr_cvar_lm, PR_LOG_MIN,
	   ("PR_Wait: cvar=%p waiting for %d", cvar, timeout));

    rv = _PR_MD_WAIT(thread, timeout);

    _PR_CVAR_LOCK(cvar);
    PR_REMOVE_LINK(&thread->waitQLinks);
    _PR_CVAR_UNLOCK(cvar);

    PR_LOG(_pr_cvar_lm, PR_LOG_MIN,
	   ("PR_Wait: cvar=%p done waiting", cvar));

    if ( !_PR_IS_NATIVE_THREAD(thread))
    	_PR_INTSON(is);

    
    PR_Lock(lock);

    if (_PR_PENDING_INTERRUPT(thread)) {
        PR_SetError(PR_PENDING_INTERRUPT_ERROR, 0);
        thread->flags &= ~_PR_INTERRUPT;
        return PR_FAILURE;
    }

    return rv;
#endif  
}

void _PR_NotifyCondVar(PRCondVar *cvar, PRThread *me)
{
#ifdef _PR_GLOBAL_THREADS_ONLY
    _PR_MD_NOTIFY_CV(&cvar->md, &cvar->lock->ilock);
#else  

    PRCList *q;
    PRIntn is;

    if ( !_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSOFF(is);
    PR_ASSERT(_PR_IS_NATIVE_THREAD(me) || _PR_MD_GET_INTSOFF() != 0);

    _PR_CVAR_LOCK(cvar);
    q = cvar->condQ.next;
    while (q != &cvar->condQ) {
        PR_LOG(_pr_cvar_lm, PR_LOG_MIN, ("_PR_NotifyCondVar: cvar=%p", cvar));
        if (_PR_THREAD_CONDQ_PTR(q)->wait.cvar)  {
            if (_PR_NotifyThread(_PR_THREAD_CONDQ_PTR(q), me) == PR_TRUE)
                break;
        }
        q = q->next;
    }
    _PR_CVAR_UNLOCK(cvar);

    if ( !_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSON(is);

#endif  
}




PRUint32 _PR_CondVarToString(PRCondVar *cvar, char *buf, PRUint32 buflen)
{
    PRUint32 nb;

    if (cvar->lock->owner) {
	nb = PR_snprintf(buf, buflen, "[%p] owner=%ld[%p]",
			 cvar, cvar->lock->owner->id, cvar->lock->owner);
    } else {
	nb = PR_snprintf(buf, buflen, "[%p]", cvar);
    }
    return nb;
}





void _PR_ClockInterrupt(void)
{
    PRThread *thread, *me = _PR_MD_CURRENT_THREAD();
    _PRCPU *cpu = me->cpu;
    PRIntervalTime elapsed, now;
 
    PR_ASSERT(_PR_MD_GET_INTSOFF() != 0);
    
    now = PR_IntervalNow();
    elapsed = now - cpu->last_clock;
    cpu->last_clock = now;

    PR_LOG(_pr_clock_lm, PR_LOG_MAX,
	   ("ExpireWaits: elapsed=%lld usec", elapsed));

    while(1) {
        _PR_SLEEPQ_LOCK(cpu);
        if (_PR_SLEEPQ(cpu).next == &_PR_SLEEPQ(cpu)) {
            _PR_SLEEPQ_UNLOCK(cpu);
            break;
        }

        thread = _PR_THREAD_PTR(_PR_SLEEPQ(cpu).next);
        PR_ASSERT(thread->cpu == cpu);

        if (elapsed < thread->sleep) {
            thread->sleep -= elapsed;
            _PR_SLEEPQMAX(thread->cpu) -= elapsed;
            _PR_SLEEPQ_UNLOCK(cpu);
            break;
        }
        _PR_SLEEPQ_UNLOCK(cpu);

        PR_ASSERT(!_PR_IS_NATIVE_THREAD(thread));

        _PR_THREAD_LOCK(thread);

        if (thread->cpu != cpu) {
            





            _PR_THREAD_UNLOCK(thread);
            continue;
        }

        




        _PR_SLEEPQ_LOCK(cpu);
        PR_ASSERT(!(thread->flags & _PR_ON_PAUSEQ));
        if (thread->flags & _PR_ON_SLEEPQ) {
            _PR_DEL_SLEEPQ(thread, PR_FALSE);
            elapsed -= thread->sleep;
            _PR_SLEEPQ_UNLOCK(cpu);
        } else {
            
            _PR_SLEEPQ_UNLOCK(cpu);
            _PR_THREAD_UNLOCK(thread);
            continue;
        }

        
        if (thread->flags & _PR_SUSPENDING) {
		PR_ASSERT((thread->state == _PR_IO_WAIT) ||
				(thread->state == _PR_COND_WAIT));
            



            thread->wait.cvar = NULL;
            _PR_MISCQ_LOCK(cpu);
            thread->state = _PR_SUSPENDED;
            _PR_ADD_SUSPENDQ(thread, cpu);
            _PR_MISCQ_UNLOCK(cpu);
        } else {
            if (thread->wait.cvar) {
                PRThreadPriority pri;

                
                PR_ASSERT( !_PR_IS_NATIVE_THREAD(thread) );

                
                pri = thread->priority;
                thread->state = _PR_RUNNABLE;
                PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));

                PR_ASSERT(thread->cpu == cpu);
                _PR_RUNQ_LOCK(cpu);
                _PR_ADD_RUNQ(thread, cpu, pri);
                _PR_RUNQ_UNLOCK(cpu);

                if (pri > me->priority)
                    _PR_SET_RESCHED_FLAG();

                thread->wait.cvar = NULL;

                _PR_MD_WAKEUP_WAITER(thread);

            } else if (thread->io_pending == PR_TRUE) {
                
                int pri = thread->priority;

                thread->io_suspended = PR_TRUE;
#ifdef WINNT
				



                thread->md.thr_bound_cpu = cpu;
#endif

				PR_ASSERT(!(thread->flags & _PR_IDLE_THREAD));
                PR_ASSERT(thread->cpu == cpu);
                thread->state = _PR_RUNNABLE;
                _PR_RUNQ_LOCK(cpu);
                _PR_ADD_RUNQ(thread, cpu, pri);
                _PR_RUNQ_UNLOCK(cpu);
            }
        }
        _PR_THREAD_UNLOCK(thread);
    }
}













PR_IMPLEMENT(PRCondVar*) PR_NewCondVar(PRLock *lock)
{
    PRCondVar *cvar;

    PR_ASSERT(lock != NULL);

    cvar = PR_NEWZAP(PRCondVar);
    if (cvar) {
#ifdef _PR_GLOBAL_THREADS_ONLY
	if(_PR_MD_NEW_CV(&cvar->md)) {
		PR_DELETE(cvar);
		PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
		return NULL;
	}
#endif
        if (_PR_MD_NEW_LOCK(&(cvar->ilock)) == PR_FAILURE) {
		PR_DELETE(cvar);
		PR_SetError(PR_INSUFFICIENT_RESOURCES_ERROR, 0);
		return NULL;
	}
    cvar->lock = lock;
	PR_INIT_CLIST(&cvar->condQ);

    } else {
        PR_SetError(PR_OUT_OF_MEMORY_ERROR, 0);
    }
    return cvar;
}







PR_IMPLEMENT(void) PR_DestroyCondVar(PRCondVar *cvar)
{
    PR_ASSERT(cvar->condQ.next == &cvar->condQ);

#ifdef _PR_GLOBAL_THREADS_ONLY
    _PR_MD_FREE_CV(&cvar->md);
#endif
    _PR_MD_FREE_LOCK(&(cvar->ilock));
 
    PR_DELETE(cvar);
}















extern PRThread *suspendAllThread;
PR_IMPLEMENT(PRStatus) PR_WaitCondVar(PRCondVar *cvar, PRIntervalTime timeout)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();

	PR_ASSERT(cvar->lock->owner == me);
	PR_ASSERT(me != suspendAllThread);
    	if (cvar->lock->owner != me) return PR_FAILURE;

	return _PR_WaitCondVar(me, cvar, cvar->lock, timeout);
}






PR_IMPLEMENT(PRStatus) PR_NotifyCondVar(PRCondVar *cvar)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(cvar->lock->owner == me);
    PR_ASSERT(me != suspendAllThread);
    if (cvar->lock->owner != me) return PR_FAILURE;

    _PR_NotifyCondVar(cvar, me);
    return PR_SUCCESS;
}






PR_IMPLEMENT(PRStatus) PR_NotifyAllCondVar(PRCondVar *cvar)
{
    PRCList *q;
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(cvar->lock->owner == me);
    if (cvar->lock->owner != me) return PR_FAILURE;

#ifdef _PR_GLOBAL_THREADS_ONLY
    _PR_MD_NOTIFYALL_CV(&cvar->md, &cvar->lock->ilock);
    return PR_SUCCESS;
#else  
    if ( !_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSOFF(is);
    _PR_CVAR_LOCK(cvar);
    q = cvar->condQ.next;
    while (q != &cvar->condQ) {
		PR_LOG(_pr_cvar_lm, PR_LOG_MIN, ("PR_NotifyAll: cvar=%p", cvar));
		_PR_NotifyThread(_PR_THREAD_CONDQ_PTR(q), me);
		q = q->next;
    }
    _PR_CVAR_UNLOCK(cvar);
    if (!_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSON(is);

    return PR_SUCCESS;
#endif  
}







#include "prpdce.h"

PR_IMPLEMENT(PRCondVar*) PRP_NewNakedCondVar(void)
{
    PRCondVar *cvar = PR_NEWZAP(PRCondVar);
    if (NULL != cvar)
    {
        if (_PR_MD_NEW_LOCK(&(cvar->ilock)) == PR_FAILURE)
        {
		    PR_DELETE(cvar); cvar = NULL;
	    }
	    else
	    {
	        PR_INIT_CLIST(&cvar->condQ);
            cvar->lock = _PR_NAKED_CV_LOCK;
	    }

    }
    return cvar;
}

PR_IMPLEMENT(void) PRP_DestroyNakedCondVar(PRCondVar *cvar)
{
    PR_ASSERT(cvar->condQ.next == &cvar->condQ);
    PR_ASSERT(_PR_NAKED_CV_LOCK == cvar->lock);

    _PR_MD_FREE_LOCK(&(cvar->ilock));
 
    PR_DELETE(cvar);
}

PR_IMPLEMENT(PRStatus) PRP_NakedWait(
	PRCondVar *cvar, PRLock *lock, PRIntervalTime timeout)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PR_ASSERT(_PR_NAKED_CV_LOCK == cvar->lock);
	return _PR_WaitCondVar(me, cvar, lock, timeout);
}  

PR_IMPLEMENT(PRStatus) PRP_NakedNotify(PRCondVar *cvar)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PR_ASSERT(_PR_NAKED_CV_LOCK == cvar->lock);

    _PR_NotifyCondVar(cvar, me);

    return PR_SUCCESS;
}  

PR_IMPLEMENT(PRStatus) PRP_NakedBroadcast(PRCondVar *cvar)
{
    PRCList *q;
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PR_ASSERT(_PR_NAKED_CV_LOCK == cvar->lock);

    if ( !_PR_IS_NATIVE_THREAD(me)) _PR_INTSOFF(is);
	_PR_MD_LOCK( &(cvar->ilock) );
    q = cvar->condQ.next;
    while (q != &cvar->condQ) {
		PR_LOG(_pr_cvar_lm, PR_LOG_MIN, ("PR_NotifyAll: cvar=%p", cvar));
		_PR_NotifyThread(_PR_THREAD_CONDQ_PTR(q), me);
		q = q->next;
    }
	_PR_MD_UNLOCK( &(cvar->ilock) );
    if (!_PR_IS_NATIVE_THREAD(me)) _PR_INTSON(is);

    return PR_SUCCESS;
}  

