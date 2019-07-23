




































#include "prmem.h"

#include "gfxImageSurface.h"

#include "cairo.h"

gfxImageSurface::gfxImageSurface(const gfxIntSize& size, gfxImageFormat format) :
    mSize(size), mFormat(format)
{
    mStride = ComputeStride();

    if (!CheckSurfaceSize(size))
        return;

    
    if (mSize.height * mStride > 0) {
        mData = (unsigned char *) malloc(mSize.height * mStride);
        if (!mData)
            return;
    } else {
        mData = nsnull;
    }

    mOwnsData = PR_TRUE;

    cairo_surface_t *surface =
        cairo_image_surface_create_for_data((unsigned char*)mData,
                                            (cairo_format_t)format,
                                            mSize.width,
                                            mSize.height,
                                            mStride);
    Init(surface);
}

gfxImageSurface::gfxImageSurface(cairo_surface_t *csurf)
{
    mSize.width = cairo_image_surface_get_width(csurf);
    mSize.height = cairo_image_surface_get_height(csurf);
    mData = cairo_image_surface_get_data(csurf);
    mFormat = (gfxImageFormat) cairo_image_surface_get_format(csurf);
    mOwnsData = PR_FALSE;
    mStride = cairo_image_surface_get_stride(csurf);

    Init(csurf, PR_TRUE);
}

gfxImageSurface::~gfxImageSurface()
{
    if (!mSurfaceValid)
        return;

    if (mOwnsData) {
        free(mData);
        mData = nsnull;
    }
}

long
gfxImageSurface::ComputeStride() const
{
    long stride;

    if (mFormat == ImageFormatARGB32)
        stride = mSize.width * 4;
    else if (mFormat == ImageFormatRGB24)
        stride = mSize.width * 4;
    else if (mFormat == ImageFormatA8)
        stride = mSize.width;
    else if (mFormat == ImageFormatA1) {
        stride = (mSize.width + 7) / 8;
    } else {
        NS_WARNING("Unknown format specified to gfxImageSurface!");
        stride = mSize.width * 4;
    }

    stride = ((stride + 3) / 4) * 4;

    return stride;
}
