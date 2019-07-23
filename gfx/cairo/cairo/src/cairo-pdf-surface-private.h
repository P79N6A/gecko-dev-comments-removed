





































#ifndef CAIRO_PDF_SURFACE_PRIVATE_H
#define CAIRO_PDF_SURFACE_PRIVATE_H

#include "cairo-pdf.h"

#include "cairo-surface-private.h"

typedef struct _cairo_pdf_resource {
    unsigned int id;
} cairo_pdf_resource_t;

typedef struct _cairo_pdf_surface cairo_pdf_surface_t;

struct _cairo_pdf_surface {
    cairo_surface_t base;

    

    cairo_output_stream_t *output;

    double width;
    double height;
    cairo_matrix_t cairo_to_pdf;

    cairo_array_t objects;
    cairo_array_t pages;
    cairo_array_t patterns;
    cairo_array_t xobjects;
    cairo_array_t streams;
    cairo_array_t alphas;
    cairo_array_t smasks;
    cairo_array_t rgb_linear_functions;
    cairo_array_t alpha_linear_functions;

    cairo_scaled_font_subsets_t *font_subsets;
    cairo_array_t fonts;

    cairo_pdf_resource_t next_available_resource;
    cairo_pdf_resource_t pages_resource;

    struct {
	cairo_bool_t active;
	cairo_pdf_resource_t self;
	cairo_pdf_resource_t length;
	long start_offset;
        cairo_bool_t compressed;
        cairo_output_stream_t *old_output;
    } current_stream;

    struct {
        cairo_pattern_type_t type;
        double red;
        double green;
        double blue;
        int alpha;
        cairo_pdf_resource_t smask;
        cairo_pdf_resource_t pattern;
    } emitted_pattern;

    cairo_bool_t has_clip;

    cairo_paginated_mode_t paginated_mode;

    cairo_bool_t force_fallbacks;
};

#endif 
