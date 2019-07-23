




































#ifndef GFX_QPAINTERSURFACE_H
#define GFX_QPAINTERSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

class QPainter;

class THEBES_API gfxQPainterSurface : public gfxASurface {
public:
    gfxQPainterSurface(QPainter *painter);
    gfxQPainterSurface(const gfxIntSize& size, gfxImageFormat format);
    gfxQPainterSurface(const gfxIntSize& size);

    gfxQPainterSurface(cairo_surface_t *csurf);

    virtual ~gfxQPainterSurface();

    QPainter *GetQPainter() { return mPainter; }

protected:
    QPainter *mPainter;
};

#endif 
