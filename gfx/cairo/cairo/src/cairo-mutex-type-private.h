







































#ifndef CAIRO_MUTEX_TYPE_PRIVATE_H
#define CAIRO_MUTEX_TYPE_PRIVATE_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <cairo-features.h>

CAIRO_BEGIN_DECLS

#ifndef MOZILLA_CAIRO_NOT_DEFINED
#define CAIRO_NO_MUTEX 1
#endif


#define CAIRO_MUTEX_NOOP	do {/*no-op*/} while (0)

#define CAIRO_MUTEX_NOOP1(expr)        do { if (expr) ; } while (0)





















































































#if CAIRO_NO_MUTEX



  typedef int cairo_mutex_t;

# define CAIRO_MUTEX_INITIALIZE() CAIRO_MUTEX_NOOP
# define CAIRO_MUTEX_LOCK(mutex) do { while (mutex) ; (mutex) = 1; } while (0)
# define CAIRO_MUTEX_UNLOCK(mutex) (mutex) = 0
# define CAIRO_MUTEX_NIL_INITIALIZER 0

#elif HAVE_PTHREAD_H 

# include <pthread.h>

  typedef pthread_mutex_t cairo_mutex_t;

# define CAIRO_MUTEX_LOCK(mutex) pthread_mutex_lock (&(mutex))
# define CAIRO_MUTEX_UNLOCK(mutex) pthread_mutex_unlock (&(mutex))
# define CAIRO_MUTEX_FINI(mutex) pthread_mutex_destroy (&(mutex))
# define CAIRO_MUTEX_FINALIZE() CAIRO_MUTEX_NOOP
# define CAIRO_MUTEX_NIL_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#elif HAVE_WINDOWS_H 

# include <windows.h>

  typedef CRITICAL_SECTION cairo_mutex_t;

# define CAIRO_MUTEX_LOCK(mutex) EnterCriticalSection (&(mutex))
# define CAIRO_MUTEX_UNLOCK(mutex) LeaveCriticalSection (&(mutex))
# define CAIRO_MUTEX_INIT(mutex) InitializeCriticalSection (&(mutex))
# define CAIRO_MUTEX_FINI(mutex) DeleteCriticalSection (&(mutex))
# define CAIRO_MUTEX_NIL_INITIALIZER { NULL, 0, 0, NULL, NULL, 0 }

#elif defined __OS2__ 

# define INCL_BASE
# define INCL_PM
# include <os2.h>

  typedef HMTX cairo_mutex_t;

# define CAIRO_MUTEX_LOCK(mutex) DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT)
# define CAIRO_MUTEX_UNLOCK(mutex) DosReleaseMutexSem(mutex)
# define CAIRO_MUTEX_INIT(mutex) DosCreateMutexSem (NULL, &(mutex), 0L, FALSE)
# define CAIRO_MUTEX_FINI(mutex) DosCloseMutexSem (mutex)
# define CAIRO_MUTEX_NIL_INITIALIZER 0

#elif CAIRO_HAS_BEOS_SURFACE 

  typedef BLocker* cairo_mutex_t;

# define CAIRO_MUTEX_LOCK(mutex) (mutex)->Lock()
# define CAIRO_MUTEX_UNLOCK(mutex) (mutex)->Unlock()
# define CAIRO_MUTEX_INIT(mutex) (mutex) = new BLocker()
# define CAIRO_MUTEX_FINI(mutex) delete (mutex)
# define CAIRO_MUTEX_NIL_INITIALIZER NULL

#else 

# error "XXX: No mutex implementation found.  Cairo will not work with multiple threads.  Define CAIRO_NO_MUTEX to 1 to acknowledge and accept this limitation and compile cairo without thread-safety support."


#endif

CAIRO_END_DECLS

#endif
