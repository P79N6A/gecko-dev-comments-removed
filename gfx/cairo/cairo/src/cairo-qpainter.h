



































#ifndef CAIRO_QPAINTER_H
#define CAIRO_QPAINTER_H

#include <cairo.h>

#if CAIRO_HAS_QPAINTER_SURFACE

class QPainter;

CAIRO_BEGIN_DECLS

cairo_public cairo_surface_t *
cairo_qpainter_surface_create (QPainter *painter);

cairo_public cairo_surface_t *
cairo_qpainter_surface_create_with_qimage (cairo_format_t format,
                                           int width,
                                           int height);

cairo_public cairo_surface_t *
cairo_qpainter_surface_create_with_qpixmap (int width,
                                            int height);

cairo_public QPainter *
cairo_qpainter_surface_get_qpainter (cairo_surface_t *surface);

CAIRO_END_DECLS

#else 

# error Cairo was not compiled with support for the QPainter backend

#endif 

#endif 
