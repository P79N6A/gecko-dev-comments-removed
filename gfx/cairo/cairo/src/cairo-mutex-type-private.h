







































#ifndef CAIRO_MUTEX_TYPE_PRIVATE_H
#define CAIRO_MUTEX_TYPE_PRIVATE_H

#include "cairo-compiler-private.h"
#include "cairo-mutex-impl-private.h"


#ifndef CAIRO_MUTEX_IMPL_LOCK
# error "CAIRO_MUTEX_IMPL_LOCK not defined.  Check cairo-mutex-impl-private.h."
#endif
#ifndef CAIRO_MUTEX_IMPL_UNLOCK
# error "CAIRO_MUTEX_IMPL_UNLOCK not defined.  Check cairo-mutex-impl-private.h."
#endif
#ifndef CAIRO_MUTEX_IMPL_NIL_INITIALIZER
# error "CAIRO_MUTEX_IMPL_NIL_INITIALIZER not defined.  Check cairo-mutex-impl-private.h."
#endif



#undef _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER
#undef _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER


#ifdef CAIRO_MUTEX_IMPL_INIT



# ifndef CAIRO_MUTEX_IMPL_INITIALIZE
#  define CAIRO_MUTEX_IMPL_INITIALIZE() do {	\
       if (!_cairo_mutex_initialized)	\
           _cairo_mutex_initialize ();	\
    } while(0)


#  define _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER 1
# endif 

#else 


# ifndef CAIRO_MUTEX_IMPL_INITIALIZE
#  define CAIRO_MUTEX_IMPL_INITIALIZE() CAIRO_MUTEX_IMPL_NOOP
# endif 


# define CAIRO_MUTEX_IMPL_INIT(mutex) do {				\
      cairo_mutex_t _tmp_mutex = CAIRO_MUTEX_IMPL_NIL_INITIALIZER;	\
      memcpy (&(mutex), &_tmp_mutex, sizeof (_tmp_mutex));	\
  } while (0)

#endif 

#ifdef CAIRO_MUTEX_IMPL_FINI



# ifndef CAIRO_MUTEX_IMPL_FINALIZE
#  define CAIRO_MUTEX_IMPL_FINALIZE() do {	\
       if (_cairo_mutex_initialized)	\
           _cairo_mutex_finalize ();	\
    } while(0)


#  define _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER 1
# endif 

#else 


# ifndef CAIRO_MUTEX_IMPL_FINALIZE
#  define CAIRO_MUTEX_IMPL_FINALIZE() CAIRO_MUTEX_IMPL_NOOP
# endif 


# define CAIRO_MUTEX_IMPL_FINI(mutex)	CAIRO_MUTEX_IMPL_NOOP1(mutex)

#endif 


#ifndef _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER
#define _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER 0
#endif
#ifndef _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER
#define _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER 0
#endif



#ifndef CAIRO_MUTEX_IMPL_INITIALIZE
# error "CAIRO_MUTEX_IMPL_INITIALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_FINALIZE
# error "CAIRO_MUTEX_IMPL_FINALIZE not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_LOCK
# error "CAIRO_MUTEX_IMPL_LOCK not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_UNLOCK
# error "CAIRO_MUTEX_IMPL_UNLOCK not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_INIT
# error "CAIRO_MUTEX_IMPL_INIT not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_FINI
# error "CAIRO_MUTEX_IMPL_FINI not defined"
#endif
#ifndef CAIRO_MUTEX_IMPL_NIL_INITIALIZER
# error "CAIRO_MUTEX_IMPL_NIL_INITIALIZER not defined"
#endif







#ifndef CAIRO_MUTEX_DEBUG
typedef cairo_mutex_impl_t cairo_mutex_t;
#else
# define cairo_mutex_t			cairo_mutex_impl_t
#endif

#define CAIRO_MUTEX_INITIALIZE		CAIRO_MUTEX_IMPL_INITIALIZE
#define CAIRO_MUTEX_FINALIZE		CAIRO_MUTEX_IMPL_FINALIZE
#define CAIRO_MUTEX_LOCK		CAIRO_MUTEX_IMPL_LOCK
#define CAIRO_MUTEX_UNLOCK		CAIRO_MUTEX_IMPL_UNLOCK
#define CAIRO_MUTEX_INIT		CAIRO_MUTEX_IMPL_INIT
#define CAIRO_MUTEX_FINI		CAIRO_MUTEX_IMPL_FINI
#define CAIRO_MUTEX_NIL_INITIALIZER	CAIRO_MUTEX_IMPL_NIL_INITIALIZER

#ifndef CAIRO_MUTEX_IS_LOCKED
# define CAIRO_MUTEX_IS_LOCKED(name) 1
#endif
#ifndef CAIRO_MUTEX_IS_UNLOCKED
# define CAIRO_MUTEX_IS_UNLOCKED(name) 1
#endif




#ifdef CAIRO_MUTEX_DEBUG



#endif 

#endif
