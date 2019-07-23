


































#ifndef _CAIRO_DDRAW_H_
#define _CAIRO_DDRAW_H_

#include "cairo.h"

#if CAIRO_HAS_DDRAW_SURFACE

#include <windows.h>
#include <ddraw.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_ddraw_surface_create (LPDIRECTDRAW lpdd,
			    cairo_format_t format,
			    int width,
			    int height);

cairo_public cairo_surface_t *
cairo_ddraw_surface_create_alias (cairo_surface_t *surface,
				  int x,
				  int y,
				  int width,
				  int height);

cairo_public LPDIRECTDRAWSURFACE
cairo_ddraw_surface_get_ddraw_surface (cairo_surface_t *surface);

cairo_public cairo_surface_t *
cairo_ddraw_surface_get_image (cairo_surface_t *surface);


CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the direct draw backend
#endif 

#endif 
