







































#ifndef CAIRO_MUTEX_IMPL_PRIVATE_H
#define CAIRO_MUTEX_IMPL_PRIVATE_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include "cairo-features.h"

CAIRO_BEGIN_DECLS



#define CAIRO_MUTEX_IMPL_NOOP	do {/*no-op*/} while (0)

#define CAIRO_MUTEX_IMPL_NOOP1(expr)        do { if (expr) ; } while (0)

































































































#ifndef CAIRO_MUTEX_TYPE_PRIVATE_H
#error "Do not include cairo-mutex-impl-private.h directly.  Include cairo-mutex-type-private.h instead."
#endif

#if CAIRO_NO_MUTEX



  typedef int cairo_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_INITIALIZE() CAIRO_MUTEX_IMPL_NOOP
# define CAIRO_MUTEX_IMPL_LOCK(mutex) CAIRO_MUTEX_IMPL_NOOP1(mutex)
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) CAIRO_MUTEX_IMPL_NOOP1(mutex)
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER 0

#elif HAVE_PTHREAD_H 

# include <pthread.h>

  typedef pthread_mutex_t cairo_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_LOCK(mutex) pthread_mutex_lock (&(mutex))
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) pthread_mutex_unlock (&(mutex))
# define CAIRO_MUTEX_IMPL_FINI(mutex) pthread_mutex_destroy (&(mutex))
# define CAIRO_MUTEX_IMPL_FINALIZE() CAIRO_MUTEX_IMPL_NOOP
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER PTHREAD_MUTEX_INITIALIZER

#elif HAVE_WINDOWS_H 

# include <windows.h>

  typedef CRITICAL_SECTION cairo_mutex_impl_t;

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

# define CAIRO_MUTEX_IMPL_LOCK(mutex) DosRequestMutexSem(mutex, SEM_INDEFINITE_WAIT)
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) DosReleaseMutexSem(mutex)
# define CAIRO_MUTEX_IMPL_INIT(mutex) DosCreateMutexSem (NULL, &(mutex), 0L, FALSE)
# define CAIRO_MUTEX_IMPL_FINI(mutex) DosCloseMutexSem (mutex)
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER 0

#elif CAIRO_HAS_BEOS_SURFACE 

  typedef BLocker* cairo_mutex_impl_t;

# define CAIRO_MUTEX_IMPL_LOCK(mutex) (mutex)->Lock()
# define CAIRO_MUTEX_IMPL_UNLOCK(mutex) (mutex)->Unlock()
# define CAIRO_MUTEX_IMPL_INIT(mutex) (mutex) = new BLocker()
# define CAIRO_MUTEX_IMPL_FINI(mutex) delete (mutex)
# define CAIRO_MUTEX_IMPL_NIL_INITIALIZER NULL

#else 

# error "XXX: No mutex implementation found.  Cairo will not work with multiple threads.  Define CAIRO_NO_MUTEX to 1 to acknowledge and accept this limitation and compile cairo without thread-safety support."


#endif

CAIRO_END_DECLS

#endif
