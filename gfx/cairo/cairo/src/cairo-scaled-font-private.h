




































#ifndef CAIRO_SCALED_FONT_PRIVATE_H
#define CAIRO_SCALED_FONT_PRIVATE_H

#include "cairo.h"

#include "cairo-types-private.h"
#include "cairo-mutex-type-private.h"
#include "cairo-reference-count-private.h"

struct _cairo_scaled_font {
    





























    
    cairo_hash_entry_t hash_entry;

    
    cairo_status_t status;
    cairo_reference_count_t ref_count;
    cairo_user_data_array_t user_data;

    
    cairo_font_face_t *font_face; 
    cairo_matrix_t font_matrix;	  
    cairo_matrix_t ctm;	          
    cairo_font_options_t options;

    
    cairo_matrix_t scale;	  
    cairo_matrix_t scale_inverse; 
    cairo_font_extents_t extents; 

    
    cairo_mutex_t mutex;

    cairo_cache_t *glyphs;	  

    




    const cairo_surface_backend_t *surface_backend;
    void *surface_private;

    
    const cairo_scaled_font_backend_t *backend;
};

#endif 
