


































#ifndef CAIRO_SURFACE_SNAPSHOT_PRIVATE_H
#define CAIRO_SURFACE_SNAPSHOT_PRIVATE_H

#include "cairo-surface-private.h"

struct _cairo_surface_snapshot {
    cairo_surface_t base;

    cairo_surface_t *target;
    cairo_surface_t *clone;
};

#endif 
