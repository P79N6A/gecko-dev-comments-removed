




































#include "gfxQuartzSurface.h"
#include "gfxContext.h"

#include "cairo-quartz.h"

gfxQuartzSurface::gfxQuartzSurface(const gfxSize& size, gfxImageFormat format,
                                   PRBool aForPrinting)
    : mSize(size), mForPrinting(aForPrinting)
{
    if (!CheckSurfaceSize(gfxIntSize((int)size.width, (int)size.height)))
        return;

    cairo_surface_t *surf = cairo_quartz_surface_create
        ((cairo_format_t) format, floor(size.width), floor(size.height));

    mCGContext = cairo_quartz_surface_get_cg_context (surf);

    CGContextRetain(mCGContext);

    Init(surf);
}

gfxQuartzSurface::gfxQuartzSurface(CGContextRef context,
                                   const gfxSize& size,
                                   PRBool aForPrinting)
    : mCGContext(context), mSize(size), mForPrinting(aForPrinting)
{
    cairo_surface_t *surf = 
        cairo_quartz_surface_create_for_cg_context(context,
                                                   floor(size.width),
                                                   floor(size.height));

    CGContextRetain(mCGContext);

    Init(surf);
}

gfxQuartzSurface::gfxQuartzSurface(cairo_surface_t *csurf,
                                   PRBool aForPrinting) :
    mSize(-1.0, -1.0), mForPrinting(aForPrinting)
{
    mCGContext = cairo_quartz_surface_get_cg_context (csurf);
    CGContextRetain (mCGContext);

    Init(csurf, PR_TRUE);
}

PRInt32 gfxQuartzSurface::GetDefaultContextFlags() const
{
    if (mForPrinting)
        return gfxContext::FLAG_DISABLE_SNAPPING;

    return 0;
}

gfxQuartzSurface::~gfxQuartzSurface()
{
    CGContextRelease(mCGContext);
}
