




































#include <assert.h>

#include "gfxQPainterSurface.h"

#ifdef CAIRO_HAS_QT_SURFACE
#include "gfxImageSurface.h"

#include "cairo-qt.h"

gfxQPainterSurface::gfxQPainterSurface(QPainter *painter)
{
    cairo_surface_t *csurf = cairo_qt_surface_create (painter);

    mPainter = painter;

    Init (csurf);
}

gfxQPainterSurface::gfxQPainterSurface(const gfxIntSize& size, gfxImageFormat format)
{
    cairo_surface_t *csurf = cairo_qt_surface_create_with_qimage ((cairo_format_t) format,
                                                                        size.width,
                                                                        size.height);
    mPainter = cairo_qt_surface_get_qpainter (csurf);

    Init (csurf);
}

gfxQPainterSurface::gfxQPainterSurface(const gfxIntSize& size, gfxContentType content)
{
    cairo_surface_t *csurf = cairo_qt_surface_create_with_qpixmap ((cairo_content_t) content,
                                                                         size.width,
                                                                         size.height);
    mPainter = cairo_qt_surface_get_qpainter (csurf);

    Init (csurf);
}

gfxQPainterSurface::gfxQPainterSurface(cairo_surface_t *csurf)
{
    mPainter = cairo_qt_surface_get_qpainter (csurf);

    Init(csurf, PR_TRUE);
}

gfxQPainterSurface::~gfxQPainterSurface()
{
}

QImage *
gfxQPainterSurface::GetQImage()
{
    if (!mSurfaceValid)
        return nsnull;

    return cairo_qt_surface_get_qimage(CairoSurface());
}

already_AddRefed<gfxImageSurface>
gfxQPainterSurface::GetAsImageSurface()
{
    if (!mSurfaceValid)
        return nsnull;

    cairo_surface_t *isurf = cairo_qt_surface_get_image(CairoSurface());
    if (!isurf)
        return nsnull;

    assert(cairo_surface_get_type(isurf) == CAIRO_SURFACE_TYPE_IMAGE);

    nsRefPtr<gfxImageSurface> asurf = new gfxImageSurface(isurf);
    return asurf.forget();
}
#endif
