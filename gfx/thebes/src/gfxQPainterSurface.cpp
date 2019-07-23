




































#include "gfxQPainterSurface.h"

#include "cairo-qpainter.h"

gfxQPainterSurface::gfxQPainterSurface(QPainter *painter)
{
    cairo_surface_t *csurf = cairo_qpainter_surface_create (painter);

    mPainter = painter;

    Init (csurf);
}

gfxQPainterSurface::gfxQPainterSurface(const gfxIntSize& size, gfxImageFormat format)
{
    cairo_surface_t *csurf = cairo_qpainter_surface_create_with_qimage ((cairo_format_t) format,
                                                                        size.width,
                                                                        size.height);
    mPainter = cairo_qpainter_surface_get_qpainter (csurf);

    Init (csurf);
}

gfxQPainterSurface::gfxQPainterSurface(const gfxIntSize& size)
{
    cairo_surface_t *csurf = cairo_qpainter_surface_create_with_qpixmap (size.width,
                                                                         size.height);
    mPainter = cairo_qpainter_surface_get_qpainter (csurf);

    Init (csurf);
}

gfxQPainterSurface::gfxQPainterSurface(cairo_surface_t *csurf)
{
    mPainter = cairo_qpainter_surface_get_qpainter (csurf);

    Init(csurf, PR_TRUE);
}

gfxQPainterSurface::~gfxQPainterSurface()
{
}
