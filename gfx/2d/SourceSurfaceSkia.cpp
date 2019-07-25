





#include "Logging.h"
#include "SourceSurfaceSkia.h"
#include "skia/SkBitmap.h"
#include "skia/SkDevice.h"
#include "HelpersSkia.h"
#include "DrawTargetSkia.h"

namespace mozilla {
namespace gfx {

SourceSurfaceSkia::SourceSurfaceSkia()
  : mDrawTarget(nullptr)
{
}

SourceSurfaceSkia::~SourceSurfaceSkia()
{
  MarkIndependent();
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
  SkBitmap temp;
  temp.setConfig(GfxFormatToSkiaConfig(aFormat), aSize.width, aSize.height, aStride);
  temp.setPixels(aData);

  if (!temp.copyTo(&mBitmap, GfxFormatToSkiaConfig(aFormat))) {
    return false;
  }
  
  mSize = aSize;
  mFormat = aFormat;
  mStride = aStride;
  return true;
}

bool
SourceSurfaceSkia::InitWithBitmap(const SkBitmap& aBitmap,
                                  SurfaceFormat aFormat,
                                  DrawTargetSkia* aOwner)
{
  mFormat = aFormat;
  mSize = IntSize(aBitmap.width(), aBitmap.height());

  if (aOwner) {
    mBitmap = aBitmap;
    mStride = aBitmap.rowBytes();
    mDrawTarget = aOwner;
    return true;
  } else if (aBitmap.copyTo(&mBitmap, aBitmap.getConfig())) {
    mStride = mBitmap.rowBytes();
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

void
SourceSurfaceSkia::DrawTargetWillChange()
{
  if (mDrawTarget) {
    mDrawTarget = nullptr;
    SkBitmap temp = mBitmap;
    mBitmap.reset();
    temp.copyTo(&mBitmap, temp.getConfig());
  }
}

void
SourceSurfaceSkia::DrawTargetDestroyed()
{
  mDrawTarget = nullptr;
}

void
SourceSurfaceSkia::MarkIndependent()
{
  if (mDrawTarget) {
    mDrawTarget->RemoveSnapshot(this);
    mDrawTarget = nullptr;
  }
}

}
}
