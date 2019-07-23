




































#include "primpl.h"

#if defined(WIN95)






#pragma warning(disable : 4101)
#endif


void _PR_InitLocks(void)
{
	_PR_MD_INIT_LOCKS();
}





void _PR_IntsOn(_PRCPU *cpu)
{
    PRUintn missed, pri, i;
    _PRInterruptTable *it;
    PRThread *me;

    PR_ASSERT(cpu);   
    PR_ASSERT(_PR_MD_GET_INTSOFF() > 0);
	me = _PR_MD_CURRENT_THREAD();
    PR_ASSERT(!(me->flags & _PR_IDLE_THREAD));

    














    for (i = 0; i < 2; i++) {
        cpu->where = (1 - i);
        missed = cpu->u.missed[i];
        if (missed != 0) {
            cpu->u.missed[i] = 0;
            for (it = _pr_interruptTable; it->name; it++) {
                if (missed & it->missed_bit) {
                    PR_LOG(_pr_sched_lm, PR_LOG_MIN,
                           ("IntsOn[0]: %s intr", it->name));
                    (*it->handler)();
                }
            }
        }
    }

    if (cpu->u.missed[3] != 0) {
        _PRCPU *cpu;

		_PR_THREAD_LOCK(me);
        me->state = _PR_RUNNABLE;
        pri = me->priority;

        cpu = me->cpu;
		_PR_RUNQ_LOCK(cpu);
        _PR_ADD_RUNQ(me, cpu, pri);
		_PR_RUNQ_UNLOCK(cpu);
		_PR_THREAD_UNLOCK(me);
        _PR_MD_SWITCH_CONTEXT(me);
    }
}






void _PR_UnblockLockWaiter(PRLock *lock)
{
    PRThread *t = NULL;
    PRThread *me;
    PRCList *q;

    q = lock->waitQ.next;
    PR_ASSERT(q != &lock->waitQ);
    while (q != &lock->waitQ) {
        
        t = _PR_THREAD_CONDQ_PTR(q);

		




        _PR_THREAD_LOCK(t);

        if (t->flags & _PR_SUSPENDING) {
            q = q->next;
            _PR_THREAD_UNLOCK(t);
            continue;
        }

        
	    PR_ASSERT(t->state == _PR_LOCK_WAIT);
	    PR_ASSERT(t->wait.lock == lock);
        t->wait.lock = 0;
        PR_REMOVE_LINK(&t->waitQLinks);         

		











		
        if ( !_PR_IS_NATIVE_THREAD(t) ) {

            t->state = _PR_RUNNABLE;

            me = _PR_MD_CURRENT_THREAD();

            _PR_AddThreadToRunQ(me, t);
            _PR_THREAD_UNLOCK(t);
        } else {
            t->state = _PR_RUNNING;
            _PR_THREAD_UNLOCK(t);
        }
        _PR_MD_WAKEUP_WAITER(t);
        break;
    }
    return;
}




PR_IMPLEMENT(PRLock*) PR_NewLock(void)
{
    PRLock *lock;

    if (!_pr_initialized) _PR_ImplicitInitialization();

    lock = PR_NEWZAP(PRLock);
    if (lock) {
        if (_PR_MD_NEW_LOCK(&lock->ilock) == PR_FAILURE) {
		PR_DELETE(lock);
		return(NULL);
	}
        PR_INIT_CLIST(&lock->links);
        PR_INIT_CLIST(&lock->waitQ);
    }
    return lock;
}






PR_IMPLEMENT(void) PR_DestroyLock(PRLock *lock)
{
    PR_ASSERT(lock->owner == 0);
	_PR_MD_FREE_LOCK(&lock->ilock);
    PR_DELETE(lock);
}

extern PRThread *suspendAllThread;



PR_IMPLEMENT(void) PR_Lock(PRLock *lock)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRIntn is;
    PRThread *t;
    PRCList *q;

    PR_ASSERT(me != suspendAllThread); 
    PR_ASSERT(!(me->flags & _PR_IDLE_THREAD));
    PR_ASSERT(lock != NULL);
#ifdef _PR_GLOBAL_THREADS_ONLY 
    PR_ASSERT(lock->owner != me);
    _PR_MD_LOCK(&lock->ilock);
    lock->owner = me;
    return;
#else  

	if (_native_threads_only) {
		PR_ASSERT(lock->owner != me);
		_PR_MD_LOCK(&lock->ilock);
		lock->owner = me;
		return;
	}

    if (!_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSOFF(is);

    PR_ASSERT(_PR_IS_NATIVE_THREAD(me) || _PR_MD_GET_INTSOFF() != 0);

retry:
    _PR_LOCK_LOCK(lock);
    if (lock->owner == 0) {
        
        lock->owner = me;
        lock->priority = me->priority;
		
        PR_APPEND_LINK(&lock->links, &me->lockList);
        _PR_LOCK_UNLOCK(lock);
    	if (!_PR_IS_NATIVE_THREAD(me))
        	_PR_FAST_INTSON(is);
        return;
    }

    
    PR_ASSERT(lock->owner != me);

    PR_ASSERT(_PR_IS_NATIVE_THREAD(me) || _PR_MD_GET_INTSOFF() != 0);

#if 0
    if (me->priority > lock->owner->priority) {
        



        lock->boostPriority = me->priority;
        _PR_SetThreadPriority(lock->owner, me->priority);
    }
#endif

    




    q = lock->waitQ.next;
    if (q == &lock->waitQ || _PR_THREAD_CONDQ_PTR(q)->priority ==
      	_PR_THREAD_CONDQ_PTR(lock->waitQ.prev)->priority) {
		



		q = &lock->waitQ;
    } else {
		
		
		while (q != &lock->waitQ) {
			t = _PR_THREAD_CONDQ_PTR(lock->waitQ.next);
			if (me->priority > t->priority) {
				
				break;
			}
			q = q->next;
		}
	}
    PR_INSERT_BEFORE(&me->waitQLinks, q);

	






    _PR_THREAD_LOCK(me);
    me->state = _PR_LOCK_WAIT;
    me->wait.lock = lock;
    _PR_THREAD_UNLOCK(me);

    _PR_LOCK_UNLOCK(lock);

    _PR_MD_WAIT(me, PR_INTERVAL_NO_TIMEOUT);
	goto retry;

#endif  
}




PR_IMPLEMENT(PRStatus) PR_Unlock(PRLock *lock)
{
    PRCList *q;
    PRThreadPriority pri, boost;
    PRIntn is;
    PRThread *me = _PR_MD_CURRENT_THREAD();

    PR_ASSERT(lock != NULL);
    PR_ASSERT(lock->owner == me);
    PR_ASSERT(me != suspendAllThread); 
    PR_ASSERT(!(me->flags & _PR_IDLE_THREAD));
    if (lock->owner != me) {
        return PR_FAILURE;
    }

#ifdef _PR_GLOBAL_THREADS_ONLY 
    lock->owner = 0;
    _PR_MD_UNLOCK(&lock->ilock);
    return PR_SUCCESS;
#else  

	if (_native_threads_only) {
		lock->owner = 0;
		_PR_MD_UNLOCK(&lock->ilock);
		return PR_SUCCESS;
	}

    if (!_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSOFF(is);
    _PR_LOCK_LOCK(lock);

	
    PR_REMOVE_LINK(&lock->links);
    pri = lock->priority;
    boost = lock->boostPriority;
    if (boost > pri) {
        





        q = me->lockList.next;
        while (q != &me->lockList) {
            PRLock *ll = _PR_LOCK_PTR(q);
            if (ll->boostPriority > pri) {
                pri = ll->boostPriority;
            }
            q = q->next;
        }
        if (pri != me->priority) {
            _PR_SetThreadPriority(me, pri);
        }
    }

    
    q = lock->waitQ.next;
    if (q != &lock->waitQ)
        _PR_UnblockLockWaiter(lock);
    lock->boostPriority = PR_PRIORITY_LOW;
    lock->owner = 0;
    _PR_LOCK_UNLOCK(lock);
    if (!_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSON(is);
    return PR_SUCCESS;
#endif  
}





PR_IMPLEMENT(void) PR_AssertCurrentThreadOwnsLock(PRLock *lock)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PR_ASSERT(lock->owner == me);
}






PR_IMPLEMENT(PRBool) PR_TestAndLock(PRLock *lock)
{
    PRThread *me = _PR_MD_CURRENT_THREAD();
    PRBool rv = PR_FALSE;
    PRIntn is;

#ifdef _PR_GLOBAL_THREADS_ONLY 
    is = _PR_MD_TEST_AND_LOCK(&lock->ilock);
    if (is == 0) {
        lock->owner = me;
        return PR_TRUE;
    }
    return PR_FALSE;
#else  

#ifndef _PR_LOCAL_THREADS_ONLY
	if (_native_threads_only) {
		is = _PR_MD_TEST_AND_LOCK(&lock->ilock);
		if (is == 0) {
			lock->owner = me;
			return PR_TRUE;
		}
    	return PR_FALSE;
	}
#endif

    if (!_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSOFF(is);

    _PR_LOCK_LOCK(lock);
    if (lock->owner == 0) {
        
        lock->owner = me;
        lock->priority = me->priority;
		
        PR_APPEND_LINK(&lock->links, &me->lockList);
        rv = PR_TRUE;
    }
    _PR_LOCK_UNLOCK(lock);

    if (!_PR_IS_NATIVE_THREAD(me))
    	_PR_INTSON(is);
    return rv;
#endif  
}






PR_IMPLEMENT(PRStatus) PRP_TryLock(PRLock *lock)
    { return (PR_TestAndLock(lock)) ? PR_SUCCESS : PR_FAILURE; }
