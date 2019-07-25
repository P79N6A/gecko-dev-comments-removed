





































#include "Logging.h"
#include "SourceSurfaceSkia.h"
#include "skia/SkBitmap.h"
#include "skia/SkDevice.h"
#include "HelpersSkia.h"

namespace mozilla {
namespace gfx {

SourceSurfaceSkia::SourceSurfaceSkia()
{
}

SourceSurfaceSkia::~SourceSurfaceSkia()
{
}

IntSize
SourceSurfaceSkia::GetSize() const
{
  return mSize;
}

SurfaceFormat
SourceSurfaceSkia::GetFormat() const
{
  return mFormat;
}

bool 
SourceSurfaceSkia::InitFromData(unsigned char* aData,
                                const IntSize &aSize,
                                int32_t aStride,
                                SurfaceFormat aFormat)
{
  mBitmap.setConfig(GfxFormatToSkiaConfig(aFormat), aSize.width, aSize.height, aStride);
  if (!mBitmap.allocPixels()) {
    return false;
  }
  
  if (!mBitmap.copyPixelsFrom(aData, mBitmap.getSafeSize(), aStride)) {
    return false;
  }
  mSize = aSize;
  mFormat = aFormat;
  mStride = aStride;
  return true;
}

bool
SourceSurfaceSkia::InitWithBitmap(SkCanvas* aBitmap,
                                  SurfaceFormat aFormat)
{
  if (aBitmap->readPixels(&mBitmap)) {
    mStride = mBitmap.rowBytes();
    mSize = IntSize(mBitmap.width(), mBitmap.height());
    mFormat = aFormat;
    return true;
  }
  return false;
}

unsigned char*
SourceSurfaceSkia::GetData()
{
  mBitmap.lockPixels();
  unsigned char *pixels = (unsigned char *)mBitmap.getPixels();
  mBitmap.unlockPixels();
  return pixels;

}


}
}
