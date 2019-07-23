



































#ifndef CAIRO_XCB_XRENDER_H
#define CAIRO_XCB_XRENDER_H

#include <cairo.h>

#if CAIRO_HAS_XCB_SURFACE

#include <xcb/xcb.h>
#include <xcb/render.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_xcb_surface_create_with_xrender_format (xcb_connection_t	    *c,
					      xcb_drawable_t	     drawable,
					      xcb_screen_t		    *screen,
					      xcb_render_pictforminfo_t *format,
					      int		     width,
					      int		     height);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the xcb backend
#endif 

#endif 
