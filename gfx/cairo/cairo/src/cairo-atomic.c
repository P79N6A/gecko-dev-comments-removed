
































#include "cairoint.h"

#include "cairo-atomic-private.h"
#include "cairo-mutex-private.h"

#ifndef HAS_ATOMIC_OPS
void
_cairo_atomic_int_inc (int *x)
{
    CAIRO_MUTEX_LOCK (_cairo_atomic_mutex);
    *x += 1;
    CAIRO_MUTEX_UNLOCK (_cairo_atomic_mutex);
}

cairo_bool_t
_cairo_atomic_int_dec_and_test (int *x)
{
    cairo_bool_t ret;

    CAIRO_MUTEX_LOCK (_cairo_atomic_mutex);
    ret = --*x == 0;
    CAIRO_MUTEX_UNLOCK (_cairo_atomic_mutex);

    return ret;
}

int
_cairo_atomic_int_cmpxchg (int *x, int oldv, int newv)
{
    int ret;

    CAIRO_MUTEX_LOCK (_cairo_atomic_mutex);
    ret = *x;
    if (ret == oldv)
	*x = newv;
    CAIRO_MUTEX_UNLOCK (_cairo_atomic_mutex);

    return ret;
}

#endif

#ifdef ATOMIC_OP_NEEDS_MEMORY_BARRIER
int
_cairo_atomic_int_get (int *x)
{
    int ret;

    CAIRO_MUTEX_LOCK (_cairo_atomic_mutex);
    ret = *x;
    CAIRO_MUTEX_UNLOCK (_cairo_atomic_mutex);

    return ret;
}

void
_cairo_atomic_int_set (int *x, int value)
{
    CAIRO_MUTEX_LOCK (_cairo_atomic_mutex);
    *x = value;
    CAIRO_MUTEX_UNLOCK (_cairo_atomic_mutex);
}
#endif
