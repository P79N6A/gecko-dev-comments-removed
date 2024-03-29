




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
    gfxQuartzSurface(CGContextRef context, const mozilla::gfx::IntSize& size);
    gfxQuartzSurface(cairo_surface_t *csurf, const mozilla::gfx::IntSize& aSize);
    gfxQuartzSurface(unsigned char *data, const gfxSize& size, long stride, gfxImageFormat format);
    gfxQuartzSurface(unsigned char *data, const mozilla::gfx::IntSize& size, long stride, gfxImageFormat format);

    virtual ~gfxQuartzSurface();

    virtual already_AddRefed<gfxASurface> CreateSimilarSurface(gfxContentType aType,
                                                               const mozilla::gfx::IntSize& aSize);

    virtual const mozilla::gfx::IntSize GetSize() const { return mozilla::gfx::IntSize(mSize.width, mSize.height); }

    CGContextRef GetCGContext() { return mCGContext; }

    CGContextRef GetCGContextWithClip(gfxContext *ctx);

    already_AddRefed<gfxImageSurface> GetAsImageSurface();

protected:
    void MakeInvalid();

    CGContextRef mCGContext;
    gfxSize      mSize;
};

#endif 
