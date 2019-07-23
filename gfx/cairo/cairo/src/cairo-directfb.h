



































#ifndef CAIRO_DIRECTFB_H
#define CAIRO_DIRECTFB_H

#include <cairo.h>

#ifdef  CAIRO_HAS_DIRECTFB_SURFACE

#include <directfb.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_directfb_surface_create (IDirectFB *dfb,IDirectFBSurface *surface);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the directfb backend
#endif 

#endif 
