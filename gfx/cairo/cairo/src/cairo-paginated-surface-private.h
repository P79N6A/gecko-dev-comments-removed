


































#ifndef CAIRO_PAGINATED_SURFACE_H
#define CAIRO_PAGINATED_SURFACE_H

#include "cairo.h"

#include "cairo-surface-private.h"

typedef struct _cairo_paginated_surface {
    cairo_surface_t base;

    
    cairo_surface_t *target;

    cairo_content_t content;

    
    const cairo_paginated_surface_backend_t *backend;

    


    cairo_surface_t *recording_surface;

    int page_num;
    cairo_bool_t page_is_blank;
} cairo_paginated_surface_t;

#endif 
