







































#ifndef CAIRO_MUTEX_PRIVATE_H
#define CAIRO_MUTEX_PRIVATE_H

#include "cairo-mutex-type-private.h"

CAIRO_BEGIN_DECLS

#if _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER
cairo_private void _cairo_mutex_initialize (void);
#endif
#if _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER
cairo_private void _cairo_mutex_finalize (void);
#endif

#if _CAIRO_MUTEX_IMPL_USE_STATIC_INITIALIZER || _CAIRO_MUTEX_IMPL_USE_STATIC_FINALIZER
  cairo_private extern cairo_bool_t _cairo_mutex_initialized;
#endif



#define CAIRO_MUTEX_DECLARE(mutex) cairo_private extern cairo_mutex_t mutex;
#include "cairo-mutex-list-private.h"
#undef CAIRO_MUTEX_DECLARE

CAIRO_END_DECLS

#endif
