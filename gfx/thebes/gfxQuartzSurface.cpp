




































#include "gfxQuartzSurface.h"
#include "gfxContext.h"

#include "cairo-quartz.h"

void
gfxQuartzSurface::MakeInvalid()
{
    mSize = gfxIntSize(-1, -1);    
}

gfxQuartzSurface::gfxQuartzSurface(const gfxSize& desiredSize, gfxImageFormat format,
                                   bool aForPrinting)
    : mCGContext(NULL), mSize(desiredSize), mForPrinting(aForPrinting)
{
    gfxIntSize size((unsigned int) floor(desiredSize.width),
                    (unsigned int) floor(desiredSize.height));
    if (!CheckSurfaceSize(size))
        MakeInvalid();

    unsigned int width = static_cast<unsigned int>(mSize.width);
    unsigned int height = static_cast<unsigned int>(mSize.height);

    cairo_surface_t *surf = cairo_quartz_surface_create
        ((cairo_format_t) format, width, height);

    mCGContext = cairo_quartz_surface_get_cg_context (surf);

    CGContextRetain(mCGContext);

    Init(surf);
}

gfxQuartzSurface::gfxQuartzSurface(CGContextRef context,
                                   const gfxSize& desiredSize,
                                   bool aForPrinting)
    : mCGContext(context), mSize(desiredSize), mForPrinting(aForPrinting)
{
    gfxIntSize size((unsigned int) floor(desiredSize.width),
                    (unsigned int) floor(desiredSize.height));
    if (!CheckSurfaceSize(size))
        MakeInvalid();

    unsigned int width = static_cast<unsigned int>(mSize.width);
    unsigned int height = static_cast<unsigned int>(mSize.height);

    cairo_surface_t *surf = 
        cairo_quartz_surface_create_for_cg_context(context,
                                                   width, height);

    CGContextRetain(mCGContext);

    Init(surf);
}

gfxQuartzSurface::gfxQuartzSurface(cairo_surface_t *csurf,
                                   bool aForPrinting) :
    mSize(-1.0, -1.0), mForPrinting(aForPrinting)
{
    mCGContext = cairo_quartz_surface_get_cg_context (csurf);
    CGContextRetain (mCGContext);

    Init(csurf, PR_TRUE);
}

gfxQuartzSurface::gfxQuartzSurface(unsigned char *data,
                                   const gfxSize& desiredSize,
                                   long stride,
                                   gfxImageFormat format,
                                   bool aForPrinting)
    : mCGContext(nsnull), mSize(desiredSize), mForPrinting(aForPrinting)
{
    gfxIntSize size((unsigned int) floor(desiredSize.width),
                    (unsigned int) floor(desiredSize.height));
    if (!CheckSurfaceSize(size))
        MakeInvalid();

    unsigned int width = static_cast<unsigned int>(mSize.width);
    unsigned int height = static_cast<unsigned int>(mSize.height);

    cairo_surface_t *surf = cairo_quartz_surface_create_for_data
        (data, (cairo_format_t) format, width, height, stride);

    mCGContext = cairo_quartz_surface_get_cg_context (surf);

    CGContextRetain(mCGContext);

    Init(surf);
}

already_AddRefed<gfxASurface>
gfxQuartzSurface::CreateSimilarSurface(gfxContentType aType,
                                       const gfxIntSize& aSize)
{
    cairo_surface_t *surface =
        cairo_quartz_surface_create_cg_layer(mSurface, (cairo_content_t)aType,
                                             aSize.width, aSize.height);
    if (cairo_surface_status(surface)) {
        cairo_surface_destroy(surface);
        return nsnull;
    }

    nsRefPtr<gfxASurface> result = Wrap(surface);
    cairo_surface_destroy(surface);
    return result.forget();
}

CGContextRef
gfxQuartzSurface::GetCGContextWithClip(gfxContext *ctx)
{
	return cairo_quartz_get_cg_context_with_clip(ctx->GetCairo());
}

PRInt32 gfxQuartzSurface::GetDefaultContextFlags() const
{
    if (mForPrinting)
        return gfxContext::FLAG_DISABLE_SNAPPING |
               gfxContext::FLAG_DISABLE_COPY_BACKGROUND;

    return 0;
}

already_AddRefed<gfxImageSurface> gfxQuartzSurface::GetAsImageSurface()
{
    cairo_surface_t *surface = cairo_quartz_surface_get_image(mSurface);
    if (!surface)
        return nsnull;

    nsRefPtr<gfxASurface> img = Wrap(surface);

    
    
    
    
    gfxImageSurface* imgSurface = static_cast<gfxImageSurface*> (img.forget().get());
    imgSurface->Release();

    return imgSurface;
}

gfxQuartzSurface::~gfxQuartzSurface()
{
    CGContextRelease(mCGContext);
}
