





































#ifndef GFX_QUARTZSURFACE_H
#define GFX_QUARTZSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include <Carbon/Carbon.h>

class gfxContext;

class THEBES_API gfxQuartzSurface : public gfxASurface {
public:
    gfxQuartzSurface(const gfxSize& size, gfxImageFormat format, PRBool aForPrinting = PR_FALSE);
    gfxQuartzSurface(CGContextRef context, const gfxSize& size, PRBool aForPrinting = PR_FALSE);
    gfxQuartzSurface(cairo_surface_t *csurf, PRBool aForPrinting = PR_FALSE);
    gfxQuartzSurface(unsigned char *data, const gfxSize& size, long stride, gfxImageFormat format, PRBool aForPrinting = PR_FALSE);

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
    CGContextRef mCGContext;
    gfxSize      mSize;
    PRPackedBool mForPrinting;
};

#endif 
