



































#ifndef CAIRO_PATH_PRIVATE_H
#define CAIRO_PATH_PRIVATE_H

#include "cairoint.h"

extern const cairo_private cairo_path_t _cairo_path_nil;

cairo_private cairo_path_t *
_cairo_path_create (cairo_path_fixed_t *path,
		    cairo_gstate_t     *gstate);

cairo_private cairo_path_t *
_cairo_path_create_flat (cairo_path_fixed_t *path,
			 cairo_gstate_t     *gstate);

cairo_private cairo_path_t *
_cairo_path_create_in_error (cairo_status_t status);

cairo_private cairo_status_t
_cairo_path_append_to_context (const cairo_path_t	*path,
			       cairo_t			*cr);

#endif 
