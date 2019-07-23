




































#ifndef CAIRO_SURFACE_PRIVATE_H
#define CAIRO_SURFACE_PRIVATE_H

#include "cairo.h"

#include "cairo-types-private.h"
#include "cairo-reference-count-private.h"

typedef void (*cairo_surface_func_t) (cairo_surface_t *);

struct _cairo_surface {
    const cairo_surface_backend_t *backend;

    


    cairo_surface_type_t type;

    cairo_content_t content;

    cairo_reference_count_t ref_count;
    cairo_status_t status;
    cairo_bool_t finished;
    unsigned int unique_id;

    cairo_user_data_array_t user_data;
    cairo_user_data_array_t mime_data;

    cairo_matrix_t device_transform;
    cairo_matrix_t device_transform_inverse;

    
    double x_resolution;
    double y_resolution;

    



    double x_fallback_resolution;
    double y_fallback_resolution;

    cairo_clip_t *clip;

    




    unsigned int next_clip_serial;
    







    unsigned int current_clip_serial;

    
    cairo_surface_t *snapshot_of;
    cairo_surface_func_t snapshot_detach;
    
    cairo_array_t snapshots;

    




    cairo_bool_t has_font_options;
    cairo_font_options_t font_options;
};

#endif 
