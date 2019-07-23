



































#ifndef CAIRO_TEE_SURFACE_PRIVATE_H
#define CAIRO_TEE_SURFACE_PRIVATE_H

#include "cairoint.h"

cairo_private cairo_surface_t *
_cairo_tee_surface_find_match (void *abstract_surface,
			       const cairo_surface_backend_t *backend,
			       cairo_content_t content);

#endif 
