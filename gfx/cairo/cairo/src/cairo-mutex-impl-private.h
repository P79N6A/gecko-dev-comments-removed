







































#ifndef CAIRO_MUTEX_IMPL_PRIVATE_H
#define CAIRO_MUTEX_IMPL_PRIVATE_H

#include "cairo.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

#if HAVE_LOCKDEP
#include <lockdep.h>
#endif


#define CAIRO_MUTEX_IMPL_NOOP	do {/*no-op*/} while (0)

#define CAIRO_MUTEX_IMPL_NOOP1(expr)        do { (void)(expr); } while (0)





































































































#if CAIRO_NO_MUTEX



  typedef int cairo_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_NO 1
# define CAIRO_MUTEX_IMPL_INITIALIZE() CAIRO_MUTEX_IMPL_NOOP
# define CAIRO_MUTEX_IMPL_LOCK(mutex) CAIRO_MUTEX_IMPL_NOOP1(mutex)
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) CAIRO_MUTEX_IMPL_NOOP1(mutex)
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER 0

# define CAIRO_MUTEX_HAS_RECURSIVE_IMPL 1

  typedef int cairo_recursive_mutex_impl_t;

# define CAIRO_RECURSIVE_MUTEX_IMPL_INIT(mutex)
# define CAIRO_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER 0

#elif defined(_WIN32) 

#define WIN32_LEAN_AND_MEAN

#if !defined(WINVER) || (WINVER < 0x0500)
# define WINVER 0x0500
#endif
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
# define _WIN32_WINNT 0x0500
#endif

# include <windows.h>

  typedef CRITICAL_SECTION cairo_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_WIN32 1
# define CAIRO_MUTEX_IMPL_LOCK(mutex) EnterCriticalSection (&(mutex))
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) LeaveCriticalSection (&(mutex))
# define CAIRO_MUTEX_IMPL_INIT(mutex) InitializeCriticalSection (&(mutex))
# define CAIRO_MUTEX_IMPL_FINI(mutex) DeleteCriticalSection (&(mutex))
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER { NULL, 0, 0, NULL, NULL, 0 }

#elif defined __OS2__ 

# define INCL_BASE
# define INCL_PM
# include <os2.h>

  typedef HMTX cairo_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_OS2 1
# define CAIRO_MUTEX_IMPL_LOCK(mutex) DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT)
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) DosReleaseMutexSem(mutex)
# define CAIRO_MUTEX_IMPL_INIT(mutex) DosCreateMutexSem (NULL, &(mutex), 0L, FALSE)
# define CAIRO_MUTEX_IMPL_FINI(mutex) DosCloseMutexSem (mutex)
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER 0

#elif CAIRO_HAS_BEOS_SURFACE 

  typedef BLocker* cairo_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_BEOS 1
# define CAIRO_MUTEX_IMPL_LOCK(mutex) (mutex)->Lock()
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) (mutex)->Unlock()
# define CAIRO_MUTEX_IMPL_INIT(mutex) (mutex) = new BLocker()
# define CAIRO_MUTEX_IMPL_FINI(mutex) delete (mutex)
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER NULL

#elif CAIRO_HAS_PTHREAD 

# include <pthread.h>

  typedef pthread_mutex_t cairo_mutex_impl_t;
  typedef pthread_mutex_t cairo_recursive_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_PTHREAD 1
#if HAVE_LOCKDEP

# define CAIRO_MUTEX_IMPL_INIT(mutex) pthread_mutex_init (&(mutex), NULL)
#endif
# define CAIRO_MUTEX_IMPL_LOCK(mutex) pthread_mutex_lock (&(mutex))
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) pthread_mutex_unlock (&(mutex))
#if HAVE_LOCKDEP
# define CAIRO_MUTEX_IS_LOCKED(mutex) LOCKDEP_IS_LOCKED (&(mutex))
# define CAIRO_MUTEX_IS_UNLOCKED(mutex) LOCKDEP_IS_UNLOCKED (&(mutex))
#endif
# define CAIRO_MUTEX_IMPL_FINI(mutex) pthread_mutex_destroy (&(mutex))
#if ! HAVE_LOCKDEP
# define CAIRO_MUTEX_IMPL_FINALIZE() CAIRO_MUTEX_IMPL_NOOP
#endif
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER PTHREAD_MUTEX_INITIALIZER

# define CAIRO_MUTEX_HAS_RECURSIVE_IMPL 1
# define CAIRO_RECURSIVE_MUTEX_IMPL_INIT(mutex) do { \
    pthread_mutexattr_t attr; \
    pthread_mutexattr_init (&attr); \
    pthread_mutexattr_settype (&attr, PTHREAD_MUTEX_RECURSIVE); \
    pthread_mutex_init (&(mutex), &attr); \
    pthread_mutexattr_destroy (&attr); \
} while (0)
# define CAIRO_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP

#else 

# error "XXX: No mutex implementation found.  Cairo will not work with multiple threads.  Define CAIRO_NO_MUTEX to 1 to acknowledge and accept this limitation and compile cairo without thread-safety support."

#endif


#if ! CAIRO_MUTEX_HAS_RECURSIVE_IMPL

# define CAIRO_MUTEX_HAS_RECURSIVE_IMPL 1

  typedef cairo_mutex_impl_t cairo_recursive_mutex_impl_t;

# define CAIRO_RECURSIVE_MUTEX_IMPL_INIT(mutex) CAIRO_MUTEX_IMPL_INIT(mutex)
# define CAIRO_RECURSIVE_MUTEX_IMPL_NIL_INITIALIZER CAIRO_MUTEX_IMPL_NIL_INITIALIZER

#endif

#endif
