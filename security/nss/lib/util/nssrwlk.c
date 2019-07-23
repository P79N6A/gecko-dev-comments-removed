



































#include "nssrwlk.h"
#include "nspr.h"

PR_BEGIN_EXTERN_C




struct nssRWLockStr {
    PZLock *        rw_lock;
    char   *        rw_name;            
    PRUint32        rw_rank;            
    PRInt32         rw_writer_locks;    
    PRInt32         rw_reader_locks;    
                                        
    PRUint32        rw_waiting_readers; 
    PRUint32        rw_waiting_writers; 
    PZCondVar *     rw_reader_waitq;    
    PZCondVar *     rw_writer_waitq;    
    PRThread  *     rw_owner;           
                                        
};

PR_END_EXTERN_C

#include <string.h>

#ifdef DEBUG_RANK_ORDER
#define NSS_RWLOCK_RANK_ORDER_DEBUG


#endif

#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG

static PRUintn  nss_thread_rwlock_initialized;
static PRUintn  nss_thread_rwlock;               
static PRUintn  nss_thread_rwlock_alloc_failed;

#define _NSS_RWLOCK_RANK_ORDER_LIMIT 10

typedef struct thread_rwlock_stack {
    PRInt32     trs_index;                                  
    NSSRWLock    *trs_stack[_NSS_RWLOCK_RANK_ORDER_LIMIT];  

} thread_rwlock_stack;


static PRUint32 nssRWLock_GetThreadRank(PRThread *me);
static void     nssRWLock_SetThreadRank(PRThread *me, NSSRWLock *rwlock);
static void     nssRWLock_UnsetThreadRank(PRThread *me, NSSRWLock *rwlock);
static void     nssRWLock_ReleaseLockStack(void *lock_stack);

#endif

#define UNTIL(x) while(!(x))











NSSRWLock *
NSSRWLock_New(PRUint32 lock_rank, const char *lock_name)
{
    NSSRWLock *rwlock;

    rwlock = PR_NEWZAP(NSSRWLock);
    if (rwlock == NULL)
        return NULL;

    rwlock->rw_lock = PZ_NewLock(nssILockRWLock);
    if (rwlock->rw_lock == NULL) {
	goto loser;
    }
    rwlock->rw_reader_waitq = PZ_NewCondVar(rwlock->rw_lock);
    if (rwlock->rw_reader_waitq == NULL) {
	goto loser;
    }
    rwlock->rw_writer_waitq = PZ_NewCondVar(rwlock->rw_lock);
    if (rwlock->rw_writer_waitq == NULL) {
	goto loser;
    }
    if (lock_name != NULL) {
        rwlock->rw_name = (char*) PR_Malloc(strlen(lock_name) + 1);
        if (rwlock->rw_name == NULL) {
	    goto loser;
        }
        strcpy(rwlock->rw_name, lock_name);
    } else {
        rwlock->rw_name = NULL;
    }
    rwlock->rw_rank            = lock_rank;
    rwlock->rw_waiting_readers = 0;
    rwlock->rw_waiting_writers = 0;
    rwlock->rw_reader_locks    = 0;
    rwlock->rw_writer_locks    = 0;

    return rwlock;

loser:
    NSSRWLock_Destroy(rwlock);
    return(NULL);
}




void
NSSRWLock_Destroy(NSSRWLock *rwlock)
{
    PR_ASSERT(rwlock != NULL);
    PR_ASSERT(rwlock->rw_waiting_readers == 0);

    

    if (rwlock->rw_name)
    	PR_Free(rwlock->rw_name);
    if (rwlock->rw_reader_waitq)
    	PZ_DestroyCondVar(rwlock->rw_reader_waitq);
    if (rwlock->rw_writer_waitq)
	PZ_DestroyCondVar(rwlock->rw_writer_waitq);
    if (rwlock->rw_lock)
	PZ_DestroyLock(rwlock->rw_lock);
    PR_DELETE(rwlock);
}




void
NSSRWLock_LockRead(NSSRWLock *rwlock)
{
    PRThread *me = PR_GetCurrentThread();

    PZ_Lock(rwlock->rw_lock);
#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG

    




    PR_ASSERT((rwlock->rw_rank == NSS_RWLOCK_RANK_NONE) ||
              (rwlock->rw_rank >= nssRWLock_GetThreadRank(me)));
#endif
    


    UNTIL ( (rwlock->rw_owner == me) ||		  
	   ((rwlock->rw_owner == NULL) &&	  
	    (rwlock->rw_waiting_writers == 0))) { 

	rwlock->rw_waiting_readers++;
	PZ_WaitCondVar(rwlock->rw_reader_waitq, PR_INTERVAL_NO_TIMEOUT);
	rwlock->rw_waiting_readers--;
    }
    rwlock->rw_reader_locks++; 		

    PZ_Unlock(rwlock->rw_lock);

#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG
    nssRWLock_SetThreadRank(me, rwlock);
#endif
}



void
NSSRWLock_UnlockRead(NSSRWLock *rwlock)
{
    PZ_Lock(rwlock->rw_lock);

    PR_ASSERT(rwlock->rw_reader_locks > 0); 

    if ((  rwlock->rw_reader_locks  > 0)  &&	
        (--rwlock->rw_reader_locks == 0)  &&	
	(  rwlock->rw_owner        == NULL) &&	
	(  rwlock->rw_waiting_writers > 0)) {	

	PZ_NotifyCondVar(rwlock->rw_writer_waitq); 
    }

    PZ_Unlock(rwlock->rw_lock);

#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG
    


    nssRWLock_UnsetThreadRank(me, rwlock);
#endif
    return;
}




void
NSSRWLock_LockWrite(NSSRWLock *rwlock)
{
    PRThread *me = PR_GetCurrentThread();

    PZ_Lock(rwlock->rw_lock);
#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG
    




    PR_ASSERT((rwlock->rw_rank == NSS_RWLOCK_RANK_NONE) ||
                    (rwlock->rw_rank >= nssRWLock_GetThreadRank(me)));
#endif
    


    PR_ASSERT(rwlock->rw_reader_locks >= 0);
    PR_ASSERT(me != NULL);

    UNTIL ( (rwlock->rw_owner == me) ||           
	   ((rwlock->rw_owner == NULL) &&	  
	    (rwlock->rw_reader_locks == 0))) {    

        rwlock->rw_waiting_writers++;
        PZ_WaitCondVar(rwlock->rw_writer_waitq, PR_INTERVAL_NO_TIMEOUT);
        rwlock->rw_waiting_writers--;
	PR_ASSERT(rwlock->rw_reader_locks >= 0);
    }

    PR_ASSERT(rwlock->rw_reader_locks == 0);
    


    rwlock->rw_owner = me;
    rwlock->rw_writer_locks++; 		

    PZ_Unlock(rwlock->rw_lock);

#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG
    


    nssRWLock_SetThreadRank(me,rwlock);
#endif
}



void
NSSRWLock_UnlockWrite(NSSRWLock *rwlock)
{
    PRThread *me = PR_GetCurrentThread();

    PZ_Lock(rwlock->rw_lock);
    PR_ASSERT(rwlock->rw_owner == me); 
    PR_ASSERT(rwlock->rw_writer_locks > 0); 

    if (  rwlock->rw_owner        == me  &&	
          rwlock->rw_writer_locks  > 0   &&	
        --rwlock->rw_writer_locks == 0) {	

	rwlock->rw_owner = NULL;		

	
	if (rwlock->rw_waiting_writers > 0) {
	    if (rwlock->rw_reader_locks == 0)
		PZ_NotifyCondVar(rwlock->rw_writer_waitq);
	} else if (rwlock->rw_waiting_readers > 0) {
	    PZ_NotifyAllCondVar(rwlock->rw_reader_waitq);
	}
    }
    PZ_Unlock(rwlock->rw_lock);

#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG
    


    nssRWLock_UnsetThreadRank(me, rwlock);
#endif
    return;
}


PRBool
NSSRWLock_HaveWriteLock(NSSRWLock *rwlock) {
    PRBool ownWriteLock;
    PRThread *me = PR_GetCurrentThread();

    





#if UNNECESSARY
    PZ_Lock(rwlock->rw_lock);	
#endif
    ownWriteLock = (PRBool)(me == rwlock->rw_owner);
#if UNNECESSARY
    PZ_Unlock(rwlock->rw_lock);
#endif
    return ownWriteLock;
}

#ifdef NSS_RWLOCK_RANK_ORDER_DEBUG








static void
nssRWLock_SetThreadRank(PRThread *me, NSSRWLock *rwlock)
{
    thread_rwlock_stack *lock_stack;
    PRStatus rv;

    


    if (!nss_thread_rwlock_initialized) {
        


        if (!nss_thread_rwlock_alloc_failed) {
            if (PR_NewThreadPrivateIndex(&nss_thread_rwlock,
                                        nssRWLock_ReleaseLockStack)
                                                == PR_FAILURE) {
                nss_thread_rwlock_alloc_failed = 1;
                return;
            }
        } else
            return;
    }
    


    if ((lock_stack = PR_GetThreadPrivate(nss_thread_rwlock)) == NULL) {
        lock_stack = (thread_rwlock_stack *)
                        PR_CALLOC(1 * sizeof(thread_rwlock_stack));
        if (lock_stack) {
            rv = PR_SetThreadPrivate(nss_thread_rwlock, lock_stack);
            if (rv == PR_FAILURE) {
                PR_DELETE(lock_stack);
                nss_thread_rwlock_alloc_failed = 1;
                return;
            }
        } else {
            nss_thread_rwlock_alloc_failed = 1;
            return;
        }
    }
    


    if (lock_stack) {
        if (lock_stack->trs_index < _NSS_RWLOCK_RANK_ORDER_LIMIT)
            lock_stack->trs_stack[lock_stack->trs_index++] = rwlock;
    }
    nss_thread_rwlock_initialized = 1;
}

static void
nssRWLock_ReleaseLockStack(void *lock_stack)
{
    PR_ASSERT(lock_stack);
    PR_DELETE(lock_stack);
}








static PRUint32
nssRWLock_GetThreadRank(PRThread *me)
{
    thread_rwlock_stack *lock_stack;

    if (nss_thread_rwlock_initialized) {
        if ((lock_stack = PR_GetThreadPrivate(nss_thread_rwlock)) == NULL)
            return (NSS_RWLOCK_RANK_NONE);
        else
            return(lock_stack->trs_stack[lock_stack->trs_index - 1]->rw_rank);

    } else
            return (NSS_RWLOCK_RANK_NONE);
}








static void
nssRWLock_UnsetThreadRank(PRThread *me, NSSRWLock *rwlock)
{
    thread_rwlock_stack *lock_stack;
    int new_index = 0, index, done = 0;

    if (!nss_thread_rwlock_initialized)
        return;

    lock_stack = PR_GetThreadPrivate(nss_thread_rwlock);

    PR_ASSERT(lock_stack != NULL);

    index = lock_stack->trs_index - 1;
    while (index-- >= 0) {
        if ((lock_stack->trs_stack[index] == rwlock) && !done)  {
            


            lock_stack->trs_stack[index] = NULL;
            done = 1;
        }
        



        if ((lock_stack->trs_stack[index] != NULL) && !new_index)
            new_index = index + 1;
        if (done && new_index)
            break;
    }
    


    lock_stack->trs_index = new_index;

}

#endif  
