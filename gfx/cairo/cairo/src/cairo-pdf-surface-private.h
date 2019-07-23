







































#ifndef CAIRO_PDF_SURFACE_PRIVATE_H
#define CAIRO_PDF_SURFACE_PRIVATE_H

#include "cairo-pdf.h"

#include "cairo-surface-private.h"

typedef struct _cairo_pdf_resource {
    unsigned int id;
} cairo_pdf_resource_t;

typedef struct _cairo_pdf_group_resources {
    cairo_array_t alphas;
    cairo_array_t smasks;
    cairo_array_t patterns;
    cairo_array_t xobjects;
    cairo_array_t fonts;
} cairo_pdf_group_resources_t;

typedef struct _cairo_pdf_surface cairo_pdf_surface_t;

struct _cairo_pdf_surface {
    cairo_surface_t base;

    

    cairo_output_stream_t *output;

    double width;
    double height;
    cairo_matrix_t cairo_to_pdf;

    cairo_array_t objects;
    cairo_array_t pages;
    cairo_array_t rgb_linear_functions;
    cairo_array_t alpha_linear_functions;
    cairo_array_t knockout_group;
    cairo_array_t content_group;

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
    } pdf_stream;

    struct {
	cairo_bool_t active;
	cairo_output_stream_t *stream;
	cairo_output_stream_t *old_output;
	cairo_pdf_group_resources_t resources;
	cairo_bool_t is_knockout;
	cairo_pdf_resource_t first_object;
    } group_stream;

    struct {
	cairo_bool_t active;
	cairo_output_stream_t *stream;
	cairo_output_stream_t *old_output;
	cairo_pdf_group_resources_t resources;
    } content_stream;

    struct {
	cairo_pattern_type_t type;
	double red;
	double green;
	double blue;
	double alpha;
	cairo_pdf_resource_t smask;
	cairo_pdf_resource_t pattern;
    } emitted_pattern;

    cairo_array_t *current_group;
    cairo_pdf_group_resources_t *current_resources;

    cairo_paginated_mode_t paginated_mode;

    cairo_bool_t force_fallbacks;

    cairo_surface_t *paginated_surface;
};

#endif 
