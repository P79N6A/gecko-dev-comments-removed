




#ifndef GFX_QUARTZSURFACE_H
#define GFX_QUARTZSURFACE_H

#include "gfxASurface.h"
#include "nsSize.h"
#include "gfxPoint.h"

#include <Carbon/Carbon.h>

class gfxContext;
class gfxImageSurface;

class gfxQuartzSurface : public gfxASurface {
public:
    gfxQuartzSurface(const gfxSize& size, gfxImageFormat format);
    gfxQuartzSurface(CGContextRef context, const gfxSize& size);
    gfxQuartzSurface(CGContextRef context, const gfxIntSize& size);
    gfxQuartzSurface(cairo_surface_t *csurf, const gfxIntSize& aSize);
    gfxQuartzSurface(unsigned char *data, const gfxSize& size, long stride, gfxImageFormat format);
    gfxQuartzSurface(unsigned char *data, const gfxIntSize& size, long stride, gfxImageFormat format);

    virtual ~gfxQuartzSurface();

    virtual already_AddRefed<gfxASurface> CreateSimilarSurface(gfxContentType aType,
                                                               const gfxIntSize& aSize);

    virtual const gfxIntSize GetSize() const { return gfxIntSize(mSize.width, mSize.height); }

    CGContextRef GetCGContext() { return mCGContext; }

    CGContextRef GetCGContextWithClip(gfxContext *ctx);

    already_AddRefed<gfxImageSurface> GetAsImageSurface();

protected:
    void MakeInvalid();

    CGContextRef mCGContext;
    gfxSize      mSize;
};

#endif 
