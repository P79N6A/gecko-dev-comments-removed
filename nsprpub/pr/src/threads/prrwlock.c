




































#include "primpl.h"

#include <string.h>

#if defined(HPUX) && defined(_PR_PTHREADS) && !defined(_PR_DCETHREADS)

#include <pthread.h>
#define HAVE_UNIX98_RWLOCK
#define RWLOCK_T pthread_rwlock_t
#define RWLOCK_INIT(lock) pthread_rwlock_init(lock, NULL)
#define RWLOCK_DESTROY(lock) pthread_rwlock_destroy(lock)
#define RWLOCK_RDLOCK(lock) pthread_rwlock_rdlock(lock)
#define RWLOCK_WRLOCK(lock) pthread_rwlock_wrlock(lock)
#define RWLOCK_UNLOCK(lock) pthread_rwlock_unlock(lock)

#elif defined(SOLARIS) && (defined(_PR_PTHREADS) \
        || defined(_PR_GLOBAL_THREADS_ONLY))

#include <synch.h>
#define HAVE_UI_RWLOCK
#define RWLOCK_T rwlock_t
#define RWLOCK_INIT(lock) rwlock_init(lock, USYNC_THREAD, NULL)
#define RWLOCK_DESTROY(lock) rwlock_destroy(lock)
#define RWLOCK_RDLOCK(lock) rw_rdlock(lock)
#define RWLOCK_WRLOCK(lock) rw_wrlock(lock)
#define RWLOCK_UNLOCK(lock) rw_unlock(lock)

#endif




struct PRRWLock {
	char			*rw_name;			
	PRUint32		rw_rank;			

#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
	RWLOCK_T		rw_lock;
#else
    PRLock			*rw_lock;
	PRInt32			rw_lock_cnt;		
										
										
	PRUint32		rw_reader_cnt;		
	PRUint32		rw_writer_cnt;		
	PRCondVar   	*rw_reader_waitq;	
	PRCondVar   	*rw_writer_waitq;	
#ifdef DEBUG
    PRThread 		*rw_owner;			
#endif
#endif
};

#ifdef DEBUG
#define _PR_RWLOCK_RANK_ORDER_DEBUG


#endif

#ifdef _PR_RWLOCK_RANK_ORDER_DEBUG

static PRUintn	pr_thread_rwlock_key;			
static PRUintn	pr_thread_rwlock_alloc_failed;

#define	_PR_RWLOCK_RANK_ORDER_LIMIT	10

typedef struct thread_rwlock_stack {
	PRInt32		trs_index;									
	PRRWLock	*trs_stack[_PR_RWLOCK_RANK_ORDER_LIMIT];	


} thread_rwlock_stack;

static void _PR_SET_THREAD_RWLOCK_RANK(PRRWLock *rwlock);
static PRUint32 _PR_GET_THREAD_RWLOCK_RANK(void);
static void _PR_UNSET_THREAD_RWLOCK_RANK(PRRWLock *rwlock);
static void _PR_RELEASE_LOCK_STACK(void *lock_stack);

#endif











PR_IMPLEMENT(PRRWLock *)
PR_NewRWLock(PRUint32 lock_rank, const char *lock_name)
{
    PRRWLock *rwlock;
#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
	int err;
#endif

    if (!_pr_initialized) _PR_ImplicitInitialization();

    rwlock = PR_NEWZAP(PRRWLock);
    if (rwlock == NULL)
		return NULL;

	rwlock->rw_rank = lock_rank;
	if (lock_name != NULL) {
		rwlock->rw_name = (char*) PR_Malloc(strlen(lock_name) + 1);
    	if (rwlock->rw_name == NULL) {
			PR_DELETE(rwlock);
			return(NULL);
		}
		strcpy(rwlock->rw_name, lock_name);
	} else {
		rwlock->rw_name = NULL;
	}
	
#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
	err = RWLOCK_INIT(&rwlock->rw_lock);
	if (err != 0) {
		PR_SetError(PR_UNKNOWN_ERROR, err);
		PR_Free(rwlock->rw_name);
		PR_DELETE(rwlock);
		return NULL;
	}
	return rwlock;
#else
	rwlock->rw_lock = PR_NewLock();
    if (rwlock->rw_lock == NULL) {
		goto failed;
	}
	rwlock->rw_reader_waitq = PR_NewCondVar(rwlock->rw_lock);
    if (rwlock->rw_reader_waitq == NULL) {
		goto failed;
	}
	rwlock->rw_writer_waitq = PR_NewCondVar(rwlock->rw_lock);
    if (rwlock->rw_writer_waitq == NULL) {
		goto failed;
	}
	rwlock->rw_reader_cnt = 0;
	rwlock->rw_writer_cnt = 0;
	rwlock->rw_lock_cnt = 0;
	return rwlock;

failed:
	if (rwlock->rw_reader_waitq != NULL) {
		PR_DestroyCondVar(rwlock->rw_reader_waitq);	
	}
	if (rwlock->rw_lock != NULL) {
		PR_DestroyLock(rwlock->rw_lock);
	}
	PR_Free(rwlock->rw_name);
	PR_DELETE(rwlock);
	return NULL;
#endif
}




PR_IMPLEMENT(void)
PR_DestroyRWLock(PRRWLock *rwlock)
{
#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
	int err;
	err = RWLOCK_DESTROY(&rwlock->rw_lock);
	PR_ASSERT(err == 0);
#else
	PR_ASSERT(rwlock->rw_reader_cnt == 0);
	PR_DestroyCondVar(rwlock->rw_reader_waitq);	
	PR_DestroyCondVar(rwlock->rw_writer_waitq);	
	PR_DestroyLock(rwlock->rw_lock);
#endif
	if (rwlock->rw_name != NULL)
		PR_Free(rwlock->rw_name);
    PR_DELETE(rwlock);
}




PR_IMPLEMENT(void)
PR_RWLock_Rlock(PRRWLock *rwlock)
{
#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
int err;
#endif

#ifdef _PR_RWLOCK_RANK_ORDER_DEBUG
	




	PR_ASSERT((rwlock->rw_rank == PR_RWLOCK_RANK_NONE) || 
					(rwlock->rw_rank >= _PR_GET_THREAD_RWLOCK_RANK()));
#endif

#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
	err = RWLOCK_RDLOCK(&rwlock->rw_lock);
	PR_ASSERT(err == 0);
#else
	PR_Lock(rwlock->rw_lock);
	


	while ((rwlock->rw_lock_cnt < 0) ||
			(rwlock->rw_writer_cnt > 0)) {
		rwlock->rw_reader_cnt++;
		PR_WaitCondVar(rwlock->rw_reader_waitq, PR_INTERVAL_NO_TIMEOUT);
		rwlock->rw_reader_cnt--;
	}
	


	rwlock->rw_lock_cnt++;

	PR_Unlock(rwlock->rw_lock);
#endif

#ifdef _PR_RWLOCK_RANK_ORDER_DEBUG
	


	_PR_SET_THREAD_RWLOCK_RANK(rwlock);
#endif
}




PR_IMPLEMENT(void)
PR_RWLock_Wlock(PRRWLock *rwlock)
{
#if defined(DEBUG)
PRThread *me = PR_GetCurrentThread();
#endif
#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
int err;
#endif

#ifdef _PR_RWLOCK_RANK_ORDER_DEBUG
	




	PR_ASSERT((rwlock->rw_rank == PR_RWLOCK_RANK_NONE) || 
					(rwlock->rw_rank >= _PR_GET_THREAD_RWLOCK_RANK()));
#endif

#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
	err = RWLOCK_WRLOCK(&rwlock->rw_lock);
	PR_ASSERT(err == 0);
#else
	PR_Lock(rwlock->rw_lock);
	


	while (rwlock->rw_lock_cnt != 0) {
		rwlock->rw_writer_cnt++;
		PR_WaitCondVar(rwlock->rw_writer_waitq, PR_INTERVAL_NO_TIMEOUT);
		rwlock->rw_writer_cnt--;
	}
	


	rwlock->rw_lock_cnt--;
	PR_ASSERT(rwlock->rw_lock_cnt == -1);
#ifdef DEBUG
	PR_ASSERT(me != NULL);
	rwlock->rw_owner = me;
#endif
	PR_Unlock(rwlock->rw_lock);
#endif

#ifdef _PR_RWLOCK_RANK_ORDER_DEBUG
	


	_PR_SET_THREAD_RWLOCK_RANK(rwlock);
#endif
}




PR_IMPLEMENT(void)
PR_RWLock_Unlock(PRRWLock *rwlock)
{
#if defined(DEBUG)
PRThread *me = PR_GetCurrentThread();
#endif
#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
int err;
#endif

#if defined(HAVE_UNIX98_RWLOCK) || defined(HAVE_UI_RWLOCK)
	err = RWLOCK_UNLOCK(&rwlock->rw_lock);
	PR_ASSERT(err == 0);
#else
	PR_Lock(rwlock->rw_lock);
	


	PR_ASSERT(rwlock->rw_lock_cnt != 0);
	if (rwlock->rw_lock_cnt > 0) {

		


		rwlock->rw_lock_cnt--;
		if (rwlock->rw_lock_cnt == 0) {
			


			if (rwlock->rw_writer_cnt > 0)
				PR_NotifyCondVar(rwlock->rw_writer_waitq);
		}
	} else {
		PR_ASSERT(rwlock->rw_lock_cnt == -1);

		rwlock->rw_lock_cnt = 0;
#ifdef DEBUG
    	PR_ASSERT(rwlock->rw_owner == me);
    	rwlock->rw_owner = NULL;
#endif
		


		if (rwlock->rw_writer_cnt > 0)
			PR_NotifyCondVar(rwlock->rw_writer_waitq);
		


		else if (rwlock->rw_reader_cnt > 0)
			PR_NotifyAllCondVar(rwlock->rw_reader_waitq);
	}
	PR_Unlock(rwlock->rw_lock);
#endif

#ifdef _PR_RWLOCK_RANK_ORDER_DEBUG
	


	_PR_UNSET_THREAD_RWLOCK_RANK(rwlock);
#endif
	return;
}

#ifndef _PR_RWLOCK_RANK_ORDER_DEBUG

void _PR_InitRWLocks(void) { }

#else

void _PR_InitRWLocks(void)
{
	


	if (PR_NewThreadPrivateIndex(&pr_thread_rwlock_key,
			_PR_RELEASE_LOCK_STACK) == PR_FAILURE) {
		pr_thread_rwlock_alloc_failed = 1;
		return;
	}
}








static void
_PR_SET_THREAD_RWLOCK_RANK(PRRWLock *rwlock)
{
thread_rwlock_stack *lock_stack;
PRStatus rv;

	


	if ((lock_stack = PR_GetThreadPrivate(pr_thread_rwlock_key)) == NULL) {
		lock_stack = (thread_rwlock_stack *)
						PR_CALLOC(1 * sizeof(thread_rwlock_stack));
		if (lock_stack) {
			rv = PR_SetThreadPrivate(pr_thread_rwlock_key, lock_stack);
			if (rv == PR_FAILURE) {
				PR_DELETE(lock_stack);
				pr_thread_rwlock_alloc_failed = 1;
				return;
			}
		} else {
			pr_thread_rwlock_alloc_failed = 1;
			return;
		}
	}
	


	if (lock_stack) {
		if (lock_stack->trs_index < _PR_RWLOCK_RANK_ORDER_LIMIT)
			lock_stack->trs_stack[lock_stack->trs_index++] = rwlock;	
	}
}

static void
_PR_RELEASE_LOCK_STACK(void *lock_stack)
{
	PR_ASSERT(lock_stack);
	PR_DELETE(lock_stack);
}







	
static PRUint32
_PR_GET_THREAD_RWLOCK_RANK(void)
{
	thread_rwlock_stack *lock_stack;

	if ((lock_stack = PR_GetThreadPrivate(pr_thread_rwlock_key)) == NULL)
		return (PR_RWLOCK_RANK_NONE);
	else
		return(lock_stack->trs_stack[lock_stack->trs_index - 1]->rw_rank);
}







	
static void
_PR_UNSET_THREAD_RWLOCK_RANK(PRRWLock *rwlock)
{
	thread_rwlock_stack *lock_stack;
	int new_index = 0, index, done = 0;

	lock_stack = PR_GetThreadPrivate(pr_thread_rwlock_key);

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
