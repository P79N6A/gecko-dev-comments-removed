



































#ifndef CAIRO_XCB_H
#define CAIRO_XCB_H

#include "cairo.h"

#if CAIRO_HAS_XCB_SURFACE

#include <xcb/xcb.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_xcb_surface_create (xcb_connection_t *c,
			  xcb_drawable_t	 drawable,
			  xcb_visualtype_t *visual,
			  int		 width,
			  int		 height);

cairo_public cairo_surface_t *
cairo_xcb_surface_create_for_bitmap (xcb_connection_t *c,
				     xcb_pixmap_t	    bitmap,
				     xcb_screen_t	   *screen,
				     int	    width,
				     int	    height);

cairo_public void
cairo_xcb_surface_set_size (cairo_surface_t *surface,
			    int		     width,
			    int		     height);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the xcb backend
#endif 

#endif 
