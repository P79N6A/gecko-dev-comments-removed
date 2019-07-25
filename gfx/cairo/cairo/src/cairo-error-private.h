




































#ifndef _CAIRO_ERROR_PRIVATE_H_
#define _CAIRO_ERROR_PRIVATE_H_

#include "cairo.h"
#include "cairo-compiler-private.h"

CAIRO_BEGIN_DECLS

#define _cairo_status_is_error(status) \
    (status != CAIRO_STATUS_SUCCESS && status <= CAIRO_STATUS_LAST_STATUS)

cairo_private cairo_status_t
_cairo_error (cairo_status_t status);


#define _cairo_error_throw(status) do { \
    cairo_status_t status__ = _cairo_error (status); \
    (void) status__; \
} while (0)

CAIRO_END_DECLS

#endif 
