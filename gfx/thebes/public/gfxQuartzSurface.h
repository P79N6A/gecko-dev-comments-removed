





































#ifndef GFX_QUARTZSURFACE_H
#define GFX_QUARTZSURFACE_H

#include "gfxASurface.h"

#include <Carbon/Carbon.h>

class THEBES_API gfxQuartzSurface : public gfxASurface {
public:
    gfxQuartzSurface(const gfxSize& size, gfxImageFormat format);
    gfxQuartzSurface(CGContextRef context, const gfxSize& size);
    gfxQuartzSurface(cairo_surface_t *csurf);

    virtual ~gfxQuartzSurface();

    const gfxSize& GetSize() const { return mSize; }

    CGContextRef GetCGContext() { return mCGContext; }

protected:
    CGContextRef mCGContext;
    gfxSize mSize;
};

#endif 
