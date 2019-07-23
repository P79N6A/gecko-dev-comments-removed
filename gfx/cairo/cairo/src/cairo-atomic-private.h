



































#ifndef CAIRO_ATOMIC_PRIVATE_H
#define CAIRO_ATOMIC_PRIVATE_H

# include "cairo-compiler-private.h"

#if HAVE_CONFIG_H
#include "config.h"
#endif

CAIRO_BEGIN_DECLS

#if HAVE_INTEL_ATOMIC_PRIMITIVES

#define HAS_ATOMIC_OPS 1

typedef int cairo_atomic_int_t;

# define _cairo_atomic_int_inc(x) ((void) __sync_fetch_and_add(x, 1))
# define _cairo_atomic_int_dec_and_test(x) (__sync_fetch_and_add(x, -1) == 1)
# define _cairo_atomic_int_cmpxchg(x, oldv, newv) __sync_val_compare_and_swap (x, oldv, newv)

#if SIZEOF_VOID_P==SIZEOF_INT
typedef int cairo_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG
typedef long cairo_atomic_intptr_t;
#elif SIZEOF_VOID_P==SIZEOF_LONG_LONG
typedef long long cairo_atomic_intptr_t;
#else
#error No matching integer pointer type
#endif

# define _cairo_atomic_ptr_cmpxchg(x, oldv, newv) \
    (void*)__sync_val_compare_and_swap ((cairo_atomic_intptr_t*)x, (cairo_atomic_intptr_t)oldv, (cairo_atomic_intptr_t)newv)

#endif


#ifndef HAS_ATOMIC_OPS

typedef int cairo_atomic_int_t;

cairo_private void
_cairo_atomic_int_inc (int *x);

cairo_private cairo_bool_t
_cairo_atomic_int_dec_and_test (int *x);

cairo_private int
_cairo_atomic_int_cmpxchg (int *x, int oldv, int newv);

cairo_private void *
_cairo_atomic_ptr_cmpxchg (void **x, void *oldv, void *newv);

#endif


#ifdef ATOMIC_OP_NEEDS_MEMORY_BARRIER

# include "cairo-compiler-private.h"

cairo_private int
_cairo_atomic_int_get (int *x);

cairo_private void
_cairo_atomic_int_set (int *x, int value);

#else

# define _cairo_atomic_int_get(x) (*x)
# define _cairo_atomic_int_set(x, value) ((*x) = value)

#endif

#define _cairo_atomic_uint_get(x) _cairo_atomic_int_get(x)
#define _cairo_atomic_uint_cmpxchg(x, oldv, newv) \
    _cairo_atomic_int_cmpxchg((int *)x, oldv, newv)

#define _cairo_status_set_error(status, err) do { \
    /* hide compiler warnings about cairo_status_t != int (gcc treats its as \
     * an unsigned integer instead, and about ignoring the return value. */  \
    int ret__ = _cairo_atomic_int_cmpxchg ((int *) status, CAIRO_STATUS_SUCCESS, err); \
    (void) ret__; \
} while (0)

CAIRO_END_DECLS

#endif
