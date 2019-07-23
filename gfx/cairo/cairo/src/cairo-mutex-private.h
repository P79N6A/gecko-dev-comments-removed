







































#ifndef CAIRO_MUTEX_PRIVATE_H
#define CAIRO_MUTEX_PRIVATE_H

#if HAVE_CONFIG_H
#include "config.h"
#endif

#include <cairo-features.h>

#include "cairo-mutex-type-private.h"


#ifndef CAIRO_MUTEX_LOCK
# error "CAIRO_MUTEX_LOCK not defined.  Check cairo-mutex-type-private.h."
#endif
#ifndef CAIRO_MUTEX_UNLOCK
# error "CAIRO_MUTEX_UNLOCK not defined.  Check cairo-mutex-type-private.h."
#endif
#ifndef CAIRO_MUTEX_NIL_INITIALIZER
# error "CAIRO_MUTEX_NIL_INITIALIZER not defined.  Check cairo-mutex-type-private.h."
#endif

CAIRO_BEGIN_DECLS


#define CAIRO_MUTEX_DECLARE(mutex) extern cairo_mutex_t mutex
#include "cairo-mutex-list-private.h"
#undef CAIRO_MUTEX_DECLARE



#undef _CAIRO_MUTEX_USE_STATIC_INITIALIZER
#undef _CAIRO_MUTEX_USE_STATIC_FINALIZER


#ifdef CAIRO_MUTEX_INIT



# ifndef CAIRO_MUTEX_INITIALIZE
#  define CAIRO_MUTEX_INITIALIZE() do {	\
       if (!_cairo_mutex_initialized)	\
           _cairo_mutex_initialize ();	\
   } while(0)

   cairo_private void _cairo_mutex_initialize (void);

   
#  define _CAIRO_MUTEX_USE_STATIC_INITIALIZER 1
# endif 

#else 


# ifndef CAIRO_MUTEX_INITIALIZE
#  define CAIRO_MUTEX_INITIALIZE() CAIRO_MUTEX_NOOP
# endif 


# define CAIRO_MUTEX_INIT(mutex) do {				\
      cairo_mutex_t _tmp_mutex = CAIRO_MUTEX_NIL_INITIALIZER;	\
      memcpy (&(mutex), &_tmp_mutex, sizeof (_tmp_mutex));	\
  } while (0)

#endif 


#ifdef CAIRO_MUTEX_FINI



# ifndef CAIRO_MUTEX_FINALIZE
#  define CAIRO_MUTEX_FINALIZE() do {	\
       if (_cairo_mutex_initialized)	\
           _cairo_mutex_finalize ();	\
   } while(0)

   cairo_private void _cairo_mutex_finalize (void);

   
#  define _CAIRO_MUTEX_USE_STATIC_FINALIZER 1
# endif 

#else 


# ifndef CAIRO_MUTEX_FINALIZE
#  define CAIRO_MUTEX_FINALIZE() CAIRO_MUTEX_NOOP
# endif 


# define CAIRO_MUTEX_FINI(mutex)	CAIRO_MUTEX_NOOP1(mutex)

#endif 


#ifndef _CAIRO_MUTEX_USE_STATIC_INITIALIZER
#define _CAIRO_MUTEX_USE_STATIC_INITIALIZER 0
#endif
#ifndef _CAIRO_MUTEX_USE_STATIC_FINALIZER
#define _CAIRO_MUTEX_USE_STATIC_FINALIZER 0
#endif


#if _CAIRO_MUTEX_USE_STATIC_INITIALIZER || _CAIRO_MUTEX_USE_STATIC_FINALIZER
  cairo_private extern cairo_bool_t _cairo_mutex_initialized;
#endif


CAIRO_END_DECLS


#ifndef CAIRO_MUTEX_INITIALIZE
# error "CAIRO_MUTEX_INITIALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_FINALIZE
# error "CAIRO_MUTEX_FINALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_LOCK
# error "CAIRO_MUTEX_LOCK not defined"
#endif
#ifndef CAIRO_MUTEX_UNLOCK
# error "CAIRO_MUTEX_UNLOCK not defined"
#endif
#ifndef CAIRO_MUTEX_INIT
# error "CAIRO_MUTEX_INIT not defined"
#endif
#ifndef CAIRO_MUTEX_FINI
# error "CAIRO_MUTEX_FINI not defined"
#endif
#ifndef CAIRO_MUTEX_NIL_INITIALIZER
# error "CAIRO_MUTEX_NIL_INITIALIZER not defined"
#endif

#endif
