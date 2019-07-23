



































#ifndef CAIRO_REFRENCE_COUNT_PRIVATE_H
#define CAIRO_REFRENCE_COUNT_PRIVATE_H

#include "cairo-atomic-private.h"


typedef struct {
    cairo_atomic_int_t ref_count;
} cairo_reference_count_t;

#define _cairo_reference_count_inc(RC) _cairo_atomic_int_inc (&(RC)->ref_count)
#define _cairo_reference_count_dec_and_test(RC) _cairo_atomic_int_dec_and_test (&(RC)->ref_count)

#define CAIRO_REFERENCE_COUNT_INIT(RC, VALUE) ((RC)->ref_count = (VALUE))

#define CAIRO_REFERENCE_COUNT_GET_VALUE(RC) _cairo_atomic_int_get (&(RC)->ref_count)
#define CAIRO_REFERENCE_COUNT_SET_VALUE(RC, VALUE) _cairo_atomic_int_set (&(RC)->ref_count, (VALUE))

#define CAIRO_REFERENCE_COUNT_INVALID_VALUE ((cairo_atomic_int_t) -1)
#define CAIRO_REFERENCE_COUNT_INVALID {CAIRO_REFERENCE_COUNT_INVALID_VALUE}

#define CAIRO_REFERENCE_COUNT_IS_INVALID(RC) (CAIRO_REFERENCE_COUNT_GET_VALUE (RC) == CAIRO_REFERENCE_COUNT_INVALID_VALUE)

#define CAIRO_REFERENCE_COUNT_HAS_REFERENCE(RC) (CAIRO_REFERENCE_COUNT_GET_VALUE (RC) > 0)

#endif
