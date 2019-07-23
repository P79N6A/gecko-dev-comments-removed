

































#ifndef CAIRO_ANALYSIS_SURFACE_H
#define CAIRO_ANALYSIS_SURFACE_H

#include "cairoint.h"

cairo_private cairo_surface_t *
_cairo_analysis_surface_create (cairo_surface_t		*target,
				int			 width,
				int			 height);

cairo_private void
_cairo_analysis_surface_set_ctm (cairo_surface_t *surface,
				 cairo_matrix_t  *ctm);

cairo_private void
_cairo_analysis_surface_get_ctm (cairo_surface_t *surface,
				 cairo_matrix_t  *ctm);

cairo_private cairo_region_t *
_cairo_analysis_surface_get_supported (cairo_surface_t *surface);

cairo_private cairo_region_t *
_cairo_analysis_surface_get_unsupported (cairo_surface_t *surface);

cairo_private cairo_bool_t
_cairo_analysis_surface_has_supported (cairo_surface_t *surface);

cairo_private cairo_bool_t
_cairo_analysis_surface_has_unsupported (cairo_surface_t *surface);

cairo_private void
_cairo_analysis_surface_get_bounding_box (cairo_surface_t *surface,
					  cairo_box_t     *bbox);


cairo_private cairo_surface_t *
_cairo_null_surface_create (cairo_content_t content);

#endif 
