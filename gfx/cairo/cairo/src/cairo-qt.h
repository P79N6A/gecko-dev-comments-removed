



































#ifndef CAIRO_QT_H
#define CAIRO_QT_H

#include "cairo.h"

#if CAIRO_HAS_QT_SURFACE

#if defined(__cplusplus)

class QPainter;
class QImage;

cairo_public cairo_surface_t *
cairo_qt_surface_create (QPainter *painter);

cairo_public cairo_surface_t *
cairo_qt_surface_create_with_qimage (cairo_format_t format,
				     int width,
				     int height);

cairo_public cairo_surface_t *
cairo_qt_surface_create_with_qpixmap (cairo_content_t content,
				      int width,
				      int height);

cairo_public QPainter *
cairo_qt_surface_get_qpainter (cairo_surface_t *surface);







cairo_public cairo_surface_t *
cairo_qt_surface_get_image (cairo_surface_t *surface);

cairo_public QImage *
cairo_qt_surface_get_qimage (cairo_surface_t *surface);

#else 

# warning cairo-qt only exports a C++ interface

#endif 

#else 

# error Cairo was not compiled with support for the Qt backend

#endif 

#endif 
