




#ifndef GFX_QPAINTERSURFACE_H
#define GFX_QPAINTERSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include "cairo-features.h"
#ifdef CAIRO_HAS_QT_SURFACE

class QPainter;
class QImage;

class gfxQPainterSurface : public gfxASurface {
public:
    gfxQPainterSurface(QPainter *painter);
    gfxQPainterSurface(const mozilla::gfx::IntSize& size, gfxImageFormat format);
    gfxQPainterSurface(const mozilla::gfx::IntSize& size, gfxContentType content);

    gfxQPainterSurface(cairo_surface_t *csurf);

    virtual ~gfxQPainterSurface();

    QPainter *GetQPainter() { return mPainter; }

    QImage *GetQImage();
    already_AddRefed<gfxImageSurface> GetAsImageSurface();

protected:
    QPainter *mPainter;
};

#endif

#endif 
