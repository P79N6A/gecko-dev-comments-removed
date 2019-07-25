


































#ifndef CAIRO_GSTATE_PRIVATE_H
#define CAIRO_GSTATE_PRIVATE_H

#include "cairo-clip-private.h"

struct _cairo_gstate {
    cairo_operator_t op;

    double tolerance;
    cairo_antialias_t antialias;

    cairo_stroke_style_t stroke_style;

    cairo_fill_rule_t fill_rule;

    cairo_font_face_t *font_face;
    cairo_scaled_font_t *scaled_font;	
    cairo_scaled_font_t *previous_scaled_font;	
    cairo_matrix_t font_matrix;
    cairo_font_options_t font_options;

    cairo_clip_t clip;

    cairo_surface_t *target;		
    cairo_surface_t *parent_target;	
    cairo_surface_t *original_target;	

    cairo_matrix_t ctm;
    cairo_matrix_t ctm_inverse;
    cairo_matrix_t source_ctm_inverse; 

    cairo_pattern_t *source;

    struct _cairo_gstate *next;
};

#endif 
