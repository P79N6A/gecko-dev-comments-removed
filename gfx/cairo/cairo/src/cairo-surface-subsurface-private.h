


































#ifndef CAIRO_SURFACE_SUBSURFACE_PRIVATE_H
#define CAIRO_SURFACE_SUBSURFACE_PRIVATE_H

#include "cairo-surface-private.h"

struct _cairo_surface_subsurface {
    cairo_surface_t base;

    cairo_rectangle_int_t extents;

    cairo_surface_t *target;
};

#endif 
