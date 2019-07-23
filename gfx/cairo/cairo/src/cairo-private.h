


































#ifndef CAIRO_PRIVATE_H
#define CAIRO_PRIVATE_H

#include "cairo-gstate-private.h"
#include "cairo-path-fixed-private.h"

struct _cairo {
    unsigned int ref_count;

    cairo_status_t status;

    cairo_path_fixed_t path;

    cairo_gstate_t *gstate;
};

#endif 
