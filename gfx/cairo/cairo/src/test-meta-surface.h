


































#ifndef TEST_META_SURFACE_H
#define TEST_META_SURFACE_H

#include "cairo.h"

CAIRO_BEGIN_DECLS

cairo_surface_t *
_cairo_test_meta_surface_create (cairo_content_t	content,
			   int			width,
			   int			height);

CAIRO_END_DECLS

#endif 
