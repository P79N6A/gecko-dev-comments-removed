


































#ifndef CAIRO_BEOS_H
#define CAIRO_BEOS_H

#include <cairo.h>

#if CAIRO_HAS_BEOS_SURFACE

#include <View.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_beos_surface_create (BView* view);

cairo_public cairo_surface_t *
cairo_beos_surface_create_for_bitmap (BView*   view,
				      BBitmap* bmp);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the beos backend
#endif 

#endif 
