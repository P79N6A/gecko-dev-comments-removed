



































#ifndef CAIRO_QUARTZ_H
#define CAIRO_QUARTZ_H

#include <cairo.h>

#if CAIRO_HAS_QUARTZ_SURFACE

#include <Carbon/Carbon.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_quartz_surface_create (CGContextRef    context,
			     int	     width,
			     int	     height,
			     cairo_bool_t    y_grows_down);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the quartz backend
#endif 

#endif 
