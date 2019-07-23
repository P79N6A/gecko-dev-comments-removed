




































#ifndef CAIRO_SCALED_FONT_PRIVATE_H
#define CAIRO_SCALED_FONT_PRIVATE_H

#include "cairo.h"

#include "cairo-types-private.h"
#include "cairo-mutex-type-private.h"
#include "cairo-reference-count-private.h"

typedef struct _cairo_scaled_glyph_page cairo_scaled_glyph_page_t;

struct _cairo_scaled_font {
    





























    cairo_hash_entry_t hash_entry;

    
    cairo_status_t status;
    cairo_reference_count_t ref_count;
    cairo_user_data_array_t user_data;

    cairo_font_face_t *original_font_face; 

    
    cairo_font_face_t *font_face; 
    cairo_matrix_t font_matrix;	  
    cairo_matrix_t ctm;	          
    cairo_font_options_t options;

    unsigned int placeholder : 1; 
    unsigned int holdover : 1;
    unsigned int finished : 1;

    
    cairo_matrix_t scale;	     
    cairo_matrix_t scale_inverse;    
    double max_scale;		     
    cairo_font_extents_t extents;    
    cairo_font_extents_t fs_extents; 

    
    cairo_mutex_t mutex;

    cairo_hash_table_t *glyphs;
    cairo_scaled_glyph_page_t *glyph_pages;
    cairo_bool_t cache_frozen;
    cairo_bool_t global_cache_frozen;

    




    const cairo_surface_backend_t *surface_backend;
    void *surface_private;

    
    const cairo_scaled_font_backend_t *backend;
};

#endif 
