




































#ifndef GFX_QPAINTERSURFACE_H
#define GFX_QPAINTERSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include "cairo-features.h"
#ifdef CAIRO_HAS_QT_SURFACE

class QPainter;
class QImage;

class THEBES_API gfxQPainterSurface : public gfxASurface {
public:
    gfxQPainterSurface(QPainter *painter);
    gfxQPainterSurface(const gfxIntSize& size, gfxImageFormat format);
    gfxQPainterSurface(const gfxIntSize& size, gfxContentType content);

    gfxQPainterSurface(cairo_surface_t *csurf);

    virtual ~gfxQPainterSurface();

    QPainter *GetQPainter() { return mPainter; }

    QImage *GetQImage();
    already_AddRefed<gfxImageSurface> GetImageSurface();

protected:
    QPainter *mPainter;
};

#endif

#endif 
