




































#include "prmem.h"

#include "gfxImageSurface.h"

#include "cairo.h"

gfxImageSurface::gfxImageSurface(unsigned char *aData, const gfxIntSize& aSize,
                                 long aStride, gfxImageFormat aFormat)
  : mSize(aSize)
  , mOwnsData(PR_FALSE)
  , mData(aData)
  , mFormat(aFormat)
  , mStride(aStride)
{
    if (!CheckSurfaceSize(aSize))
        return;

    cairo_surface_t *surface =
        cairo_image_surface_create_for_data((unsigned char*)mData,
                                            (cairo_format_t)mFormat,
                                            mSize.width,
                                            mSize.height,
                                            mStride);

    
    
    
    
    
    Init(surface);
}

gfxImageSurface::gfxImageSurface(const gfxIntSize& size, gfxImageFormat format) :
    mSize(size), mOwnsData(PR_FALSE), mFormat(format)
{
    mStride = ComputeStride();

    if (!CheckSurfaceSize(size))
        return;

    
    if (mSize.height * mStride > 0) {
        mData = (unsigned char *) calloc(mSize.height, mStride);
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
    if (mOwnsData)
        free(mData);
}

long
gfxImageSurface::ComputeStride() const
{
    long stride;

    if (mFormat == ImageFormatARGB32)
        stride = mSize.width * 4;
    else if (mFormat == ImageFormatRGB24)
        stride = mSize.width * 4;
    else if (mFormat == ImageFormatRGB16)
        stride = mSize.width * 2;
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

PRBool
gfxImageSurface::CopyFrom(gfxImageSurface *other)
{
    if (other->mSize != mSize)
    {
        return PR_FALSE;
    }

    if (other->mFormat != mFormat &&
        !(other->mFormat == ImageFormatARGB32 && mFormat == ImageFormatRGB24) &&
        !(other->mFormat == ImageFormatRGB24 && mFormat == ImageFormatARGB32))
    {
        return PR_FALSE;
    }

    if (other->mStride == mStride) {
        memcpy (mData, other->mData, mStride * mSize.height);
    } else {
        int lineSize = PR_MIN(other->mStride, mStride);
        for (int i = 0; i < mSize.height; i++) {
            unsigned char *src = other->mData + other->mStride * i;
            unsigned char *dst = mData + mStride * i;

            memcpy (dst, src, lineSize);
        }
    }

    return PR_TRUE;
}
