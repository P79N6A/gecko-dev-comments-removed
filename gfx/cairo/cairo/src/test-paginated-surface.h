


































#ifndef TEST_PAGINATED_SURFACE_H
#define TEST_PAGINATED_SURFACE_H

#include "cairo.h"

CAIRO_BEGIN_DECLS

cairo_surface_t *
_test_paginated_surface_create_for_data (unsigned char		*data,
					 cairo_content_t	 content,
					 int			 width,
					 int			 height,
					 int			 stride);

CAIRO_END_DECLS

#endif 
