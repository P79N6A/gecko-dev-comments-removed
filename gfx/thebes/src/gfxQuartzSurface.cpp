




































#include "gfxQuartzSurface.h"

#include "cairo-nquartz.h"

gfxQuartzSurface::gfxQuartzSurface(const gfxSize& size, gfxImageFormat format)
    : mSize(size)
{
    cairo_surface_t *surf = cairo_nquartz_surface_create
        ((cairo_format_t) format, floor(size.width), floor(size.height));

    mCGContext = cairo_nquartz_surface_get_cg_context (surf);

    CGContextRetain(mCGContext);

    Init(surf);
}

gfxQuartzSurface::gfxQuartzSurface(CGContextRef context,
                                   PRBool y_grows_down,
                                   const gfxSize& size)
    : mCGContext(context), mSize(size)
{
    cairo_surface_t *surf = 
        cairo_nquartz_surface_create_for_cg_context(context,
                                                    floor(size.width),
                                                    floor(size.height),
                                                    y_grows_down);

    CGContextRetain(mCGContext);

    Init(surf);
}

gfxQuartzSurface::gfxQuartzSurface(cairo_surface_t *csurf) :
    mSize(-1.0, -1.0)
{
    mCGContext = cairo_nquartz_surface_get_cg_context (csurf);
    CGContextRetain (mCGContext);

    Init(csurf, PR_TRUE);
}

gfxQuartzSurface::~gfxQuartzSurface()
{
    CGContextRelease(mCGContext);
}
