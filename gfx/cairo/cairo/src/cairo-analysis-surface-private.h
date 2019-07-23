

































#ifndef CAIRO_ANALYSIS_SURFACE_H
#define CAIRO_ANALYSIS_SURFACE_H

#include "cairoint.h"

cairo_private cairo_surface_t *
_cairo_analysis_surface_create (cairo_surface_t		*target,
				int			 width,
				int			 height);

cairo_private pixman_region16_t *
_cairo_analysis_surface_get_supported (cairo_surface_t *surface);

cairo_private pixman_region16_t *
_cairo_analysis_surface_get_unsupported (cairo_surface_t *unsupported);

cairo_private cairo_bool_t
_cairo_analysis_surface_has_unsupported (cairo_surface_t *unsupported);

#endif 
