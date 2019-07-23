







































#ifndef CAIRO_SVG_SURFACE_PRIVATE_H
#define CAIRO_SVG_SURFACE_PRIVATE_H

#include "cairo-svg.h"

#include "cairo-surface-private.h"

typedef struct cairo_svg_document cairo_svg_document_t;

typedef struct cairo_svg_surface {
    cairo_surface_t base;

    cairo_content_t content;

    unsigned int id;

    double width;
    double height;

    cairo_svg_document_t *document;

    cairo_output_stream_t *xml_node;
    cairo_array_t	   page_set;

    unsigned int clip_level;
    unsigned int base_clip;
    cairo_bool_t is_base_clip_emitted;

    cairo_paginated_mode_t paginated_mode;

    cairo_bool_t force_fallbacks;
} cairo_svg_surface_t;

#endif 
