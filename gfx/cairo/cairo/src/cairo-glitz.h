



































#ifndef CAIRO_GLITZ_H
#define CAIRO_GLITZ_H

#include <cairo.h>

#if CAIRO_HAS_GLITZ_SURFACE

#include <glitz.h>

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_glitz_surface_create (glitz_surface_t *surface);

CAIRO_END_DECLS

#else  
# error Cairo was not compiled with support for the glitz backend
#endif 

#endif 
