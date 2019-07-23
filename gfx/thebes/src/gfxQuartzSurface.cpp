




































#include "gfxQuartzSurface.h"
#include "gfxContext.h"

#include "cairo-quartz.h"

gfxQuartzSurface::gfxQuartzSurface(const gfxSize& size, gfxImageFormat format,
                                   PRBool aForPrinting)
    : mCGContext(NULL), mSize(size), mForPrinting(aForPrinting)
{
    unsigned int width = (unsigned int) floor(size.width);
    unsigned int height = (unsigned int) floor(size.height);

    if (!CheckSurfaceSize(gfxIntSize(width, height)))
        return;

    cairo_surface_t *surf = cairo_quartz_surface_create
        ((cairo_format_t) format, width, height);

    mCGContext = cairo_quartz_surface_get_cg_context (surf);

    CGContextRetain(mCGContext);

    Init(surf);
}

gfxQuartzSurface::gfxQuartzSurface(CGContextRef context,
                                   const gfxSize& size,
                                   PRBool aForPrinting)
    : mCGContext(context), mSize(size), mForPrinting(aForPrinting)
{
    unsigned int width = (unsigned int) floor(size.width);
    unsigned int height = (unsigned int) floor(size.height);

    cairo_surface_t *surf = 
        cairo_quartz_surface_create_for_cg_context(context,
                                                   width, height);

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

CGContextRef
gfxQuartzSurface::GetCGContextWithClip(gfxContext *ctx)
{
	return cairo_quartz_get_cg_context_with_clip(ctx->GetCairo());
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
