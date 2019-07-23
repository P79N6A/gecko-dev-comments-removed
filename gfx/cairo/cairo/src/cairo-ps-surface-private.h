






































#ifndef CAIRO_PS_SURFACE_PRIVATE_H
#define CAIRO_PS_SURFACE_PRIVATE_H

#include "cairo-ps.h"

#include "cairo-surface-private.h"

typedef struct cairo_ps_surface {
    cairo_surface_t base;

    




    cairo_output_stream_t *final_stream;

    FILE *tmpfile;
    cairo_output_stream_t *stream;

    double width;
    double height;
    double max_width;
    double max_height;

    int num_pages;

    cairo_paginated_mode_t paginated_mode;

    cairo_bool_t force_fallbacks;

    cairo_scaled_font_subsets_t *font_subsets;

    cairo_array_t dsc_header_comments;
    cairo_array_t dsc_setup_comments;
    cairo_array_t dsc_page_setup_comments;

    cairo_array_t *dsc_comment_target;
} cairo_ps_surface_t;

#endif 
