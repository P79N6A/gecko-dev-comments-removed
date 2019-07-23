





































#ifndef GFX_QUARTZSURFACE_H
#define GFX_QUARTZSURFACE_H

#include "gfxASurface.h"
#include "gfxImageSurface.h"

#include <Carbon/Carbon.h>

class THEBES_API gfxQuartzSurface : public gfxASurface {
public:
    gfxQuartzSurface(const gfxSize& size, gfxImageFormat format, PRBool aForPrinting = PR_FALSE);
    gfxQuartzSurface(CGContextRef context, const gfxSize& size, PRBool aForPrinting = PR_FALSE);
    gfxQuartzSurface(cairo_surface_t *csurf, PRBool aForPrinting = PR_FALSE);

    virtual ~gfxQuartzSurface();

    const gfxSize& GetSize() const { return mSize; }

    CGContextRef GetCGContext() { return mCGContext; }

    virtual PRInt32 GetDefaultContextFlags() const;

    already_AddRefed<gfxImageSurface> GetImageSurface();

protected:
    CGContextRef mCGContext;
    gfxSize      mSize;
    PRPackedBool mForPrinting;
};

#endif 
