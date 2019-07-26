
























#ifndef _EVTHREAD_INTERNAL_H_
#define _EVTHREAD_INTERNAL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include "event2/thread.h"
#include "event2/event-config.h"
#include "util-internal.h"

struct event_base;

#ifndef WIN32




#define EVTHREAD_EXPOSE_STRUCTS
#endif

#if ! defined(_EVENT_DISABLE_THREAD_SUPPORT) && defined(EVTHREAD_EXPOSE_STRUCTS)


extern struct evthread_lock_callbacks _evthread_lock_fns;
extern struct evthread_condition_callbacks _evthread_cond_fns;
extern unsigned long (*_evthread_id_fn)(void);
extern int _evthread_lock_debugging_enabled;


#define EVTHREAD_GET_ID() \
	(_evthread_id_fn ? _evthread_id_fn() : 1)



#define EVBASE_IN_THREAD(base)				 \
	(_evthread_id_fn == NULL ||			 \
	(base)->th_owner_id == _evthread_id_fn())




#define EVBASE_NEED_NOTIFY(base)			 \
	(_evthread_id_fn != NULL &&			 \
	    (base)->running_loop &&			 \
	    (base)->th_owner_id != _evthread_id_fn())



#define EVTHREAD_ALLOC_LOCK(lockvar, locktype)		\
	((lockvar) = _evthread_lock_fns.alloc ?		\
	    _evthread_lock_fns.alloc(locktype) : NULL)


#define EVTHREAD_FREE_LOCK(lockvar, locktype)				\
	do {								\
		void *_lock_tmp_ = (lockvar);				\
		if (_lock_tmp_ && _evthread_lock_fns.free)		\
			_evthread_lock_fns.free(_lock_tmp_, (locktype)); \
	} while (0)


#define EVLOCK_LOCK(lockvar,mode)					\
	do {								\
		if (lockvar)						\
			_evthread_lock_fns.lock(mode, lockvar);		\
	} while (0)


#define EVLOCK_UNLOCK(lockvar,mode)					\
	do {								\
		if (lockvar)						\
			_evthread_lock_fns.unlock(mode, lockvar);	\
	} while (0)


#define _EVLOCK_SORTLOCKS(lockvar1, lockvar2)				\
	do {								\
		if (lockvar1 && lockvar2 && lockvar1 > lockvar2) {	\
			void *tmp = lockvar1;				\
			lockvar1 = lockvar2;				\
			lockvar2 = tmp;					\
		}							\
	} while (0)



#define EVBASE_ACQUIRE_LOCK(base, lockvar) do {				\
		EVLOCK_LOCK((base)->lockvar, 0);			\
	} while (0)


#define EVBASE_RELEASE_LOCK(base, lockvar) do {				\
		EVLOCK_UNLOCK((base)->lockvar, 0);			\
	} while (0)



#define EVLOCK_ASSERT_LOCKED(lock)					\
	do {								\
		if ((lock) && _evthread_lock_debugging_enabled) {	\
			EVUTIL_ASSERT(_evthread_is_debug_lock_held(lock)); \
		}							\
	} while (0)



static inline int EVLOCK_TRY_LOCK(void *lock);
static inline int
EVLOCK_TRY_LOCK(void *lock)
{
	if (lock && _evthread_lock_fns.lock) {
		int r = _evthread_lock_fns.lock(EVTHREAD_TRY, lock);
		return !r;
	} else {
		

		return 1;
	}
}


#define EVTHREAD_ALLOC_COND(condvar)					\
	do {								\
		(condvar) = _evthread_cond_fns.alloc_condition ?	\
		    _evthread_cond_fns.alloc_condition(0) : NULL;	\
	} while (0)

#define EVTHREAD_FREE_COND(cond)					\
	do {								\
		if (cond)						\
			_evthread_cond_fns.free_condition((cond));	\
	} while (0)

#define EVTHREAD_COND_SIGNAL(cond)					\
	( (cond) ? _evthread_cond_fns.signal_condition((cond), 0) : 0 )

#define EVTHREAD_COND_BROADCAST(cond)					\
	( (cond) ? _evthread_cond_fns.signal_condition((cond), 1) : 0 )




#define EVTHREAD_COND_WAIT(cond, lock)					\
	( (cond) ? _evthread_cond_fns.wait_condition((cond), (lock), NULL) : 0 )


#define EVTHREAD_COND_WAIT_TIMED(cond, lock, tv)			\
	( (cond) ? _evthread_cond_fns.wait_condition((cond), (lock), (tv)) : 0 )


#define EVTHREAD_LOCKING_ENABLED()		\
	(_evthread_lock_fns.lock != NULL)

#elif ! defined(_EVENT_DISABLE_THREAD_SUPPORT)

unsigned long _evthreadimpl_get_id(void);
int _evthreadimpl_is_lock_debugging_enabled(void);
void *_evthreadimpl_lock_alloc(unsigned locktype);
void _evthreadimpl_lock_free(void *lock, unsigned locktype);
int _evthreadimpl_lock_lock(unsigned mode, void *lock);
int _evthreadimpl_lock_unlock(unsigned mode, void *lock);
void *_evthreadimpl_cond_alloc(unsigned condtype);
void _evthreadimpl_cond_free(void *cond);
int _evthreadimpl_cond_signal(void *cond, int broadcast);
int _evthreadimpl_cond_wait(void *cond, void *lock, const struct timeval *tv);
int _evthreadimpl_locking_enabled(void);

#define EVTHREAD_GET_ID() _evthreadimpl_get_id()
#define EVBASE_IN_THREAD(base)				\
	((base)->th_owner_id == _evthreadimpl_get_id())
#define EVBASE_NEED_NOTIFY(base)			 \
	((base)->running_loop &&			 \
	    ((base)->th_owner_id != _evthreadimpl_get_id()))

#define EVTHREAD_ALLOC_LOCK(lockvar, locktype)		\
	((lockvar) = _evthreadimpl_lock_alloc(locktype))

#define EVTHREAD_FREE_LOCK(lockvar, locktype)				\
	do {								\
		void *_lock_tmp_ = (lockvar);				\
		if (_lock_tmp_)						\
			_evthreadimpl_lock_free(_lock_tmp_, (locktype)); \
	} while (0)


#define EVLOCK_LOCK(lockvar,mode)					\
	do {								\
		if (lockvar)						\
			_evthreadimpl_lock_lock(mode, lockvar);		\
	} while (0)


#define EVLOCK_UNLOCK(lockvar,mode)					\
	do {								\
		if (lockvar)						\
			_evthreadimpl_lock_unlock(mode, lockvar);	\
	} while (0)



#define EVBASE_ACQUIRE_LOCK(base, lockvar) do {				\
		EVLOCK_LOCK((base)->lockvar, 0);			\
	} while (0)


#define EVBASE_RELEASE_LOCK(base, lockvar) do {				\
		EVLOCK_UNLOCK((base)->lockvar, 0);			\
	} while (0)



#define EVLOCK_ASSERT_LOCKED(lock)					\
	do {								\
		if ((lock) && _evthreadimpl_is_lock_debugging_enabled()) { \
			EVUTIL_ASSERT(_evthread_is_debug_lock_held(lock)); \
		}							\
	} while (0)



static inline int EVLOCK_TRY_LOCK(void *lock);
static inline int
EVLOCK_TRY_LOCK(void *lock)
{
	if (lock) {
		int r = _evthreadimpl_lock_lock(EVTHREAD_TRY, lock);
		return !r;
	} else {
		

		return 1;
	}
}


#define EVTHREAD_ALLOC_COND(condvar)					\
	do {								\
		(condvar) = _evthreadimpl_cond_alloc(0);		\
	} while (0)

#define EVTHREAD_FREE_COND(cond)					\
	do {								\
		if (cond)						\
			_evthreadimpl_cond_free((cond));		\
	} while (0)

#define EVTHREAD_COND_SIGNAL(cond)					\
	( (cond) ? _evthreadimpl_cond_signal((cond), 0) : 0 )

#define EVTHREAD_COND_BROADCAST(cond)					\
	( (cond) ? _evthreadimpl_cond_signal((cond), 1) : 0 )




#define EVTHREAD_COND_WAIT(cond, lock)					\
	( (cond) ? _evthreadimpl_cond_wait((cond), (lock), NULL) : 0 )


#define EVTHREAD_COND_WAIT_TIMED(cond, lock, tv)			\
	( (cond) ? _evthreadimpl_cond_wait((cond), (lock), (tv)) : 0 )

#define EVTHREAD_LOCKING_ENABLED()		\
	(_evthreadimpl_locking_enabled())

#else 

#define EVTHREAD_GET_ID()	1
#define EVTHREAD_ALLOC_LOCK(lockvar, locktype) _EVUTIL_NIL_STMT
#define EVTHREAD_FREE_LOCK(lockvar, locktype) _EVUTIL_NIL_STMT

#define EVLOCK_LOCK(lockvar, mode) _EVUTIL_NIL_STMT
#define EVLOCK_UNLOCK(lockvar, mode) _EVUTIL_NIL_STMT
#define EVLOCK_LOCK2(lock1,lock2,mode1,mode2) _EVUTIL_NIL_STMT
#define EVLOCK_UNLOCK2(lock1,lock2,mode1,mode2) _EVUTIL_NIL_STMT

#define EVBASE_IN_THREAD(base)	1
#define EVBASE_NEED_NOTIFY(base) 0
#define EVBASE_ACQUIRE_LOCK(base, lock) _EVUTIL_NIL_STMT
#define EVBASE_RELEASE_LOCK(base, lock) _EVUTIL_NIL_STMT
#define EVLOCK_ASSERT_LOCKED(lock) _EVUTIL_NIL_STMT

#define EVLOCK_TRY_LOCK(lock) 1

#define EVTHREAD_ALLOC_COND(condvar) _EVUTIL_NIL_STMT
#define EVTHREAD_FREE_COND(cond) _EVUTIL_NIL_STMT
#define EVTHREAD_COND_SIGNAL(cond) _EVUTIL_NIL_STMT
#define EVTHREAD_COND_BROADCAST(cond) _EVUTIL_NIL_STMT
#define EVTHREAD_COND_WAIT(cond, lock) _EVUTIL_NIL_STMT
#define EVTHREAD_COND_WAIT_TIMED(cond, lock, howlong) _EVUTIL_NIL_STMT

#define EVTHREAD_LOCKING_ENABLED() 0

#endif


#if ! defined(_EVENT_DISABLE_THREAD_SUPPORT)

#define _EVLOCK_SORTLOCKS(lockvar1, lockvar2)				\
	do {								\
		if (lockvar1 && lockvar2 && lockvar1 > lockvar2) {	\
			void *tmp = lockvar1;				\
			lockvar1 = lockvar2;				\
			lockvar2 = tmp;					\
		}							\
	} while (0)



#define EVLOCK_LOCK2(lock1,lock2,mode1,mode2)				\
	do {								\
		void *_lock1_tmplock = (lock1);				\
		void *_lock2_tmplock = (lock2);				\
		_EVLOCK_SORTLOCKS(_lock1_tmplock,_lock2_tmplock);	\
		EVLOCK_LOCK(_lock1_tmplock,mode1);			\
		if (_lock2_tmplock != _lock1_tmplock)			\
			EVLOCK_LOCK(_lock2_tmplock,mode2);		\
	} while (0)

#define EVLOCK_UNLOCK2(lock1,lock2,mode1,mode2)				\
	do {								\
		void *_lock1_tmplock = (lock1);				\
		void *_lock2_tmplock = (lock2);				\
		_EVLOCK_SORTLOCKS(_lock1_tmplock,_lock2_tmplock);	\
		if (_lock2_tmplock != _lock1_tmplock)			\
			EVLOCK_UNLOCK(_lock2_tmplock,mode2);		\
		EVLOCK_UNLOCK(_lock1_tmplock,mode1);			\
	} while (0)

int _evthread_is_debug_lock_held(void *lock);
void *_evthread_debug_get_real_lock(void *lock);

void *evthread_setup_global_lock_(void *lock_, unsigned locktype,
    int enable_locks);

#define EVTHREAD_SETUP_GLOBAL_LOCK(lockvar, locktype)			\
	do {								\
		lockvar = evthread_setup_global_lock_(lockvar,		\
		    (locktype), enable_locks);				\
		if (!lockvar) {						\
			event_warn("Couldn't allocate %s", #lockvar);	\
			return -1;					\
		}							\
	} while (0);

int event_global_setup_locks_(const int enable_locks);
int evsig_global_setup_locks_(const int enable_locks);
int evutil_secure_rng_global_setup_locks_(const int enable_locks);

#endif

#ifdef __cplusplus
}
#endif

#endif
