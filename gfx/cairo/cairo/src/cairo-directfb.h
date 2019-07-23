



































#ifndef CAIRO_DIRECTFB_H
#define CAIRO_DIRECTFB_H

#include <cairo.h>

#ifdef  CAIRO_HAS_DIRECTFB_SURFACE

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_directfb_surface_create (IDirectFB *dfb,IDirectFBSurface *surface);

CAIRO_END_DECLS

#endif 
#endif 
