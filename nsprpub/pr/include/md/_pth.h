




































#ifndef nspr_pth_defs_h_
#define nspr_pth_defs_h_




#define _PR_MD_BLOCK_CLOCK_INTERRUPTS()
#define _PR_MD_UNBLOCK_CLOCK_INTERRUPTS()
#define _PR_MD_DISABLE_CLOCK_INTERRUPTS()
#define _PR_MD_ENABLE_CLOCK_INTERRUPTS()





#ifdef _PR_DCETHREADS
#define _PT_PTHREAD_MUTEXATTR_INIT        pthread_mutexattr_create
#define _PT_PTHREAD_MUTEXATTR_DESTROY     pthread_mutexattr_delete
#define _PT_PTHREAD_MUTEX_INIT(m, a)      pthread_mutex_init(&(m), a)
#define _PT_PTHREAD_MUTEX_IS_LOCKED(m)    (0 == pthread_mutex_trylock(&(m)))
#define _PT_PTHREAD_CONDATTR_INIT         pthread_condattr_create
#define _PT_PTHREAD_COND_INIT(m, a)       pthread_cond_init(&(m), a)
#define _PT_PTHREAD_CONDATTR_DESTROY      pthread_condattr_delete










#elif defined(BSDI)











#define _PT_PTHREAD_MUTEXATTR_INIT(x)     0
#define _PT_PTHREAD_MUTEXATTR_DESTROY(x)
#define _PT_PTHREAD_MUTEX_INIT(m, a)      (memset(&(m), 0, sizeof(m)), \
                                      pthread_mutex_init(&(m), NULL))
#define _PT_PTHREAD_MUTEX_IS_LOCKED(m)    (EBUSY == pthread_mutex_trylock(&(m)))
#define _PT_PTHREAD_CONDATTR_INIT(x)      0
#define _PT_PTHREAD_CONDATTR_DESTROY(x)
#define _PT_PTHREAD_COND_INIT(m, a)       (memset(&(m), 0, sizeof(m)), \
                                      pthread_cond_init(&(m), NULL))
#else
#define _PT_PTHREAD_MUTEXATTR_INIT        pthread_mutexattr_init
#define _PT_PTHREAD_MUTEXATTR_DESTROY     pthread_mutexattr_destroy
#define _PT_PTHREAD_MUTEX_INIT(m, a)      pthread_mutex_init(&(m), &(a))
#if defined(FREEBSD)
#define _PT_PTHREAD_MUTEX_IS_LOCKED(m)    pt_pthread_mutex_is_locked(&(m))
#else
#define _PT_PTHREAD_MUTEX_IS_LOCKED(m)    (EBUSY == pthread_mutex_trylock(&(m)))
#endif
#define _PT_PTHREAD_CONDATTR_INIT         pthread_condattr_init
#define _PT_PTHREAD_CONDATTR_DESTROY      pthread_condattr_destroy
#define _PT_PTHREAD_COND_INIT(m, a)       pthread_cond_init(&(m), &(a))
#endif






























#if defined(_PR_DCETHREADS)
#define _PT_PTHREAD_INVALIDATE_THR_HANDLE(t) \
	memset(&(t), 0, sizeof(pthread_t))
#define _PT_PTHREAD_THR_HANDLE_IS_INVALID(t) \
	(!memcmp(&(t), &pt_zero_tid, sizeof(pthread_t)))
#define _PT_PTHREAD_COPY_THR_HANDLE(st, dt)   (dt) = (st)
#elif defined(IRIX) || defined(OSF1) || defined(AIX) || defined(SOLARIS) \
	|| defined(LINUX) || defined(__GNU__) || defined(__GLIBC__) \
	|| defined(HPUX) || defined(FREEBSD) \
	|| defined(NETBSD) || defined(OPENBSD) || defined(BSDI) \
	|| defined(NTO) || defined(DARWIN) \
	|| defined(UNIXWARE) || defined(RISCOS)	|| defined(SYMBIAN)
#ifdef __GNU__

#error Using Hurd pthreads
#endif
#define _PT_PTHREAD_INVALIDATE_THR_HANDLE(t)  (t) = 0
#define _PT_PTHREAD_THR_HANDLE_IS_INVALID(t)  (t) == 0
#define _PT_PTHREAD_COPY_THR_HANDLE(st, dt)   (dt) = (st)
#else 
#error "pthreads is not supported for this architecture"
#endif

#if defined(_PR_DCETHREADS)
#define _PT_PTHREAD_ATTR_INIT            pthread_attr_create
#define _PT_PTHREAD_ATTR_DESTROY         pthread_attr_delete
#define _PT_PTHREAD_CREATE(t, a, f, r)   pthread_create(t, a, f, r) 
#define _PT_PTHREAD_KEY_CREATE           pthread_keycreate
#define _PT_PTHREAD_ATTR_SETSCHEDPOLICY  pthread_attr_setsched
#define _PT_PTHREAD_ATTR_GETSTACKSIZE(a, s) \
                                     (*(s) = pthread_attr_getstacksize(*(a)), 0)
#define _PT_PTHREAD_GETSPECIFIC(k, r) \
		pthread_getspecific((k), (pthread_addr_t *) &(r))
#elif defined(_PR_PTHREADS)
#define _PT_PTHREAD_ATTR_INIT            pthread_attr_init
#define _PT_PTHREAD_ATTR_DESTROY         pthread_attr_destroy
#define _PT_PTHREAD_CREATE(t, a, f, r)   pthread_create(t, &a, f, r) 
#define _PT_PTHREAD_KEY_CREATE           pthread_key_create
#define _PT_PTHREAD_ATTR_SETSCHEDPOLICY  pthread_attr_setschedpolicy
#define _PT_PTHREAD_ATTR_GETSTACKSIZE(a, s) pthread_attr_getstacksize(a, s)
#define _PT_PTHREAD_GETSPECIFIC(k, r)    (r) = pthread_getspecific(k)
#else
#error "Cannot determine pthread strategy"
#endif

#if defined(_PR_DCETHREADS)
#define _PT_PTHREAD_EXPLICIT_SCHED      _PT_PTHREAD_DEFAULT_SCHED
#endif





#if defined(_PR_DCETHREADS)
#define PT_TRYLOCK_SUCCESS 1
#define PT_TRYLOCK_BUSY    0
#else
#define PT_TRYLOCK_SUCCESS 0
#define PT_TRYLOCK_BUSY    EBUSY
#endif




#if (defined(AIX) && !defined(AIX4_3_PLUS)) \
	|| defined(LINUX) || defined(__GNU__)|| defined(__GLIBC__) \
	|| defined(FREEBSD) || defined(NETBSD) || defined(OPENBSD) \
	|| defined(BSDI) || defined(UNIXWARE) \
	|| defined(DARWIN) || defined(SYMBIAN)
#define PT_NO_SIGTIMEDWAIT
#endif

#if defined(OSF1)
#define PT_PRIO_MIN            PRI_OTHER_MIN
#define PT_PRIO_MAX            PRI_OTHER_MAX
#elif defined(IRIX)
#include <sys/sched.h>
#define PT_PRIO_MIN            PX_PRIO_MIN
#define PT_PRIO_MAX            PX_PRIO_MAX
#elif defined(AIX)
#include <sys/priv.h>
#include <sys/sched.h>
#ifndef PTHREAD_CREATE_JOINABLE
#define PTHREAD_CREATE_JOINABLE     PTHREAD_CREATE_UNDETACHED
#endif
#define PT_PRIO_MIN            DEFAULT_PRIO
#define PT_PRIO_MAX            DEFAULT_PRIO
#elif defined(HPUX)

#if defined(_PR_DCETHREADS)
#define PT_PRIO_MIN            PRI_OTHER_MIN
#define PT_PRIO_MAX            PRI_OTHER_MAX
#else 
#include <sys/sched.h>
#define PT_PRIO_MIN            sched_get_priority_min(SCHED_OTHER)
#define PT_PRIO_MAX            sched_get_priority_max(SCHED_OTHER)
#endif 

#elif defined(LINUX) || defined(__GNU__) || defined(__GLIBC__) \
	|| defined(FREEBSD) || defined(SYMBIAN)
#define PT_PRIO_MIN            sched_get_priority_min(SCHED_OTHER)
#define PT_PRIO_MAX            sched_get_priority_max(SCHED_OTHER)
#elif defined(NTO)





#define PT_PRIO_MIN            0
#define PT_PRIO_MAX            30
#elif defined(SOLARIS)






#define PT_PRIO_MIN            1
#define PT_PRIO_MAX            127
#elif defined(OPENBSD)
#define PT_PRIO_MIN            0
#define PT_PRIO_MAX            31
#elif defined(NETBSD) \
	|| defined(BSDI) || defined(DARWIN) || defined(UNIXWARE) \
	|| defined(RISCOS) 
#define PT_PRIO_MIN            0
#define PT_PRIO_MAX            126
#else
#error "pthreads is not supported for this architecture"
#endif






#if defined(_PR_DCETHREADS)
#define _PT_PTHREAD_YIELD()            	pthread_yield()
#elif defined(OSF1)




#define _PT_PTHREAD_YIELD()            	pthread_yield_np()
#elif defined(AIX)
extern int (*_PT_aix_yield_fcn)();
#define _PT_PTHREAD_YIELD()			(*_PT_aix_yield_fcn)()
#elif defined(IRIX)
#include <time.h>
#define _PT_PTHREAD_YIELD() \
    PR_BEGIN_MACRO               				\
		struct timespec onemillisec = {0};		\
		onemillisec.tv_nsec = 1000000L;			\
        nanosleep(&onemillisec,NULL);			\
    PR_END_MACRO
#elif defined(HPUX) || defined(SOLARIS) \
	|| defined(LINUX) || defined(__GNU__) || defined(__GLIBC__) \
	|| defined(FREEBSD) || defined(NETBSD) || defined(OPENBSD) \
	|| defined(BSDI) || defined(NTO) || defined(DARWIN) \
	|| defined(UNIXWARE) || defined(RISCOS) || defined(SYMBIAN)
#define _PT_PTHREAD_YIELD()            	sched_yield()
#else
#error "Need to define _PT_PTHREAD_YIELD for this platform"
#endif

#endif 
