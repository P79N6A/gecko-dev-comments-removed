





































#ifndef CAIRO_REGION_PRIVATE_H
#define CAIRO_REGION_PRIVATE_H

#include "cairo-types-private.h"
#include "cairo-reference-count-private.h"

#include <pixman.h>

CAIRO_BEGIN_DECLS

struct _cairo_region {
    cairo_reference_count_t ref_count;
    cairo_status_t status;

    pixman_region32_t rgn;
};

cairo_private void
_cairo_region_init (cairo_region_t *region);

cairo_private void
_cairo_region_init_rectangle (cairo_region_t *region,
			      const cairo_rectangle_int_t *rectangle);

cairo_private void
_cairo_region_fini (cairo_region_t *region);

CAIRO_END_DECLS

#endif 
