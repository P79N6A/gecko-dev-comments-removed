





































#ifndef GFX_QUARTZSURFACE_H
#define GFX_QUARTZSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include <Carbon/Carbon.h>

class gfxContext;

class THEBES_API gfxQuartzSurface : public gfxASurface {
public:
    gfxQuartzSurface(const gfxSize& size, gfxImageFormat format, bool aForPrinting = false);
    gfxQuartzSurface(CGContextRef context, const gfxSize& size, bool aForPrinting = false);
    gfxQuartzSurface(cairo_surface_t *csurf, bool aForPrinting = false);
    gfxQuartzSurface(unsigned char *data, const gfxSize& size, long stride, gfxImageFormat format, bool aForPrinting = false);

    virtual ~gfxQuartzSurface();

    virtual already_AddRefed<gfxASurface> CreateSimilarSurface(gfxContentType aType,
                                                               const gfxIntSize& aSize);

    virtual const gfxIntSize GetSize() const { return gfxIntSize(mSize.width, mSize.height); }

    CGContextRef GetCGContext() { return mCGContext; }

    CGContextRef GetCGContextWithClip(gfxContext *ctx);

    virtual PRInt32 GetDefaultContextFlags() const;

    already_AddRefed<gfxImageSurface> GetAsImageSurface();

    void MovePixels(const nsIntRect& aSourceRect,
                    const nsIntPoint& aDestTopLeft)
    {
        FastMovePixels(aSourceRect, aDestTopLeft);
    }

protected:
    void MakeInvalid();

    CGContextRef mCGContext;
    gfxSize      mSize;
    bool mForPrinting;
};

#endif 
