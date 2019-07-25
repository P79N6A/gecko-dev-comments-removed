




































#include "prmem.h"

#include "gfxAlphaRecovery.h"
#include "gfxImageSurface.h"

#include "cairo.h"

gfxImageSurface::gfxImageSurface()
  : mSize(0, 0),
    mOwnsData(PR_FALSE),
    mFormat(ImageFormatUnknown),
    mStride(0)
{
}

void
gfxImageSurface::InitFromSurface(cairo_surface_t *csurf)
{
    mSize.width = cairo_image_surface_get_width(csurf);
    mSize.height = cairo_image_surface_get_height(csurf);
    mData = cairo_image_surface_get_data(csurf);
    mFormat = (gfxImageFormat) cairo_image_surface_get_format(csurf);
    mOwnsData = PR_FALSE;
    mStride = cairo_image_surface_get_stride(csurf);

    Init(csurf, PR_TRUE);
}

gfxImageSurface::gfxImageSurface(unsigned char *aData, const gfxIntSize& aSize,
                                 long aStride, gfxImageFormat aFormat)
{
    InitWithData(aData, aSize, aStride, aFormat);
}

void
gfxImageSurface::InitWithData(unsigned char *aData, const gfxIntSize& aSize,
                              long aStride, gfxImageFormat aFormat)
{
    mSize = aSize;
    mOwnsData = PR_FALSE;
    mData = aData;
    mFormat = aFormat;
    mStride = aStride;

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

static void*
TryAllocAlignedBytes(size_t aSize)
{
    
#if defined(HAVE_POSIX_MEMALIGN) || defined(HAVE_JEMALLOC_POSIX_MEMALIGN)
    void* ptr;
    
    
    return moz_posix_memalign(&ptr,
                              1 << gfxAlphaRecovery::GoodAlignmentLog2(),
                              aSize) ?
             nsnull : ptr;
#else
    
    return moz_malloc(aSize);
#endif
}

gfxImageSurface::gfxImageSurface(const gfxIntSize& size, gfxImageFormat format) :
    mSize(size), mOwnsData(PR_FALSE), mData(nsnull), mFormat(format)
{
    mStride = ComputeStride();

    if (!CheckSurfaceSize(size))
        return;

    
    if (mSize.height * mStride > 0) {

        
        
        mData = (unsigned char *) TryAllocAlignedBytes(mSize.height * mStride);
        if (!mData)
            return;
        memset(mData, 0, mSize.height * mStride);
    }

    mOwnsData = PR_TRUE;

    cairo_surface_t *surface =
        cairo_image_surface_create_for_data((unsigned char*)mData,
                                            (cairo_format_t)format,
                                            mSize.width,
                                            mSize.height,
                                            mStride);

    Init(surface);

    if (mSurfaceValid) {
        RecordMemoryUsed(mSize.height * ComputeStride() +
                         sizeof(gfxImageSurface));
    }
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
gfxImageSurface::ComputeStride(const gfxIntSize& aSize, gfxImageFormat aFormat)
{
    long stride;

    if (aFormat == ImageFormatARGB32)
        stride = aSize.width * 4;
    else if (aFormat == ImageFormatRGB24)
        stride = aSize.width * 4;
    else if (aFormat == ImageFormatRGB16_565)
        stride = aSize.width * 2;
    else if (aFormat == ImageFormatA8)
        stride = aSize.width;
    else if (aFormat == ImageFormatA1) {
        stride = (aSize.width + 7) / 8;
    } else {
        NS_WARNING("Unknown format specified to gfxImageSurface!");
        stride = aSize.width * 4;
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

already_AddRefed<gfxSubimageSurface>
gfxImageSurface::GetSubimage(const gfxRect& aRect)
{
    gfxRect r(aRect);
    r.Round();
    unsigned char* subData = Data() +
        (Stride() * (int)r.Y()) +
        (int)r.X() * gfxASurface::BytePerPixelFromFormat(Format());

    nsRefPtr<gfxSubimageSurface> image =
        new gfxSubimageSurface(this, subData,
                               gfxIntSize((int)r.Width(), (int)r.Height()));

    return image.forget().get();
}

gfxSubimageSurface::gfxSubimageSurface(gfxImageSurface* aParent,
                                       unsigned char* aData,
                                       const gfxIntSize& aSize)
  : gfxImageSurface(aData, aSize, aParent->Stride(), aParent->Format())
  , mParent(aParent)
{
}

already_AddRefed<gfxImageSurface>
gfxImageSurface::GetAsImageSurface()
{
  nsRefPtr<gfxImageSurface> surface = this;
  return surface.forget();
}

void
gfxImageSurface::MovePixels(const nsIntRect& aSourceRect,
                            const nsIntPoint& aDestTopLeft)
{
    const nsIntRect bounds(0, 0, mSize.width, mSize.height);
    nsIntPoint offset = aDestTopLeft - aSourceRect.TopLeft(); 
    nsIntRect clippedSource = aSourceRect;
    clippedSource.IntersectRect(clippedSource, bounds);
    nsIntRect clippedDest = clippedSource + offset;
    clippedDest.IntersectRect(clippedDest, bounds);
    const nsIntRect dest = clippedDest;
    const nsIntRect source = dest - offset;
    
    
    NS_ABORT_IF_FALSE(bounds.Contains(dest) && bounds.Contains(source) &&
                      aSourceRect.Contains(source) &&
                      nsIntRect(aDestTopLeft, aSourceRect.Size()).Contains(dest) &&
                      source.Size() == dest.Size() &&
                      offset == (dest.TopLeft() - source.TopLeft()),
                      "Messed up clipping, crash or corruption will follow");
    if (source.IsEmpty() || source.IsEqualInterior(dest)) {
        return;
    }

    long naturalStride = ComputeStride(mSize, mFormat);
    if (mStride == naturalStride && dest.width == bounds.width) {
        
        
        
        unsigned char* dst = mData + dest.y * mStride;
        const unsigned char* src = mData + source.y * mStride;
        size_t nBytes = dest.height * mStride;
        memmove(dst, src, nBytes);
        return;
    }

    
    const PRInt32 bpp = BytePerPixelFromFormat(mFormat);
    const size_t nRowBytes = dest.width * bpp;
    
    
    
    
    unsigned char* dstRow;
    unsigned char* srcRow;
    unsigned char* endSrcRow;   
    long stride;
    if (dest.y > source.y) {
        
        
        
        stride = -mStride;
        dstRow = mData + dest.x * bpp + (dest.YMost() - 1) * mStride;
        srcRow = mData + source.x * bpp + (source.YMost() - 1) * mStride;
        endSrcRow = mData + source.x * bpp + (source.y - 1) * mStride;
    } else {
        stride = mStride;
        dstRow = mData + dest.x * bpp + dest.y * mStride;
        srcRow = mData + source.x * bpp + source.y * mStride;
        endSrcRow = mData + source.x * bpp + source.YMost() * mStride;
    }

    for (; srcRow != endSrcRow; dstRow += stride, srcRow += stride) {
        memmove(dstRow, srcRow, nRowBytes);
    }
}
