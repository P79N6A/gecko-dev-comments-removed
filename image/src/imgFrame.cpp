





#include "imgFrame.h"
#include "ImageRegion.h"
#include "ShutdownTracker.h"

#include "prenv.h"

#include "gfx2DGlue.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "gfxAlphaRecovery.h"

static bool gDisableOptimize = false;

#include "GeckoProfiler.h"
#include "mozilla/Likely.h"
#include "MainThreadUtils.h"
#include "mozilla/MemoryReporting.h"
#include "nsMargin.h"
#include "mozilla/CheckedInt.h"
#include "mozilla/gfx/Tools.h"


namespace mozilla {

using namespace gfx;

namespace image {

static UserDataKey kVolatileBuffer;

static void
VolatileBufferRelease(void *vbuf)
{
  delete static_cast<VolatileBufferPtr<unsigned char>*>(vbuf);
}

static int32_t
VolatileSurfaceStride(const IntSize& size, SurfaceFormat format)
{
  
  return (size.width * BytesPerPixel(format) + 0x3) & ~0x3;
}

static TemporaryRef<DataSourceSurface>
CreateLockedSurface(VolatileBuffer *vbuf,
                    const IntSize& size,
                    SurfaceFormat format)
{
  VolatileBufferPtr<unsigned char> *vbufptr =
    new VolatileBufferPtr<unsigned char>(vbuf);
  MOZ_ASSERT(!vbufptr->WasBufferPurged(), "Expected image data!");

  int32_t stride = VolatileSurfaceStride(size, format);
  RefPtr<DataSourceSurface> surf =
    Factory::CreateWrappingDataSourceSurface(*vbufptr, stride, size, format);
  if (!surf) {
    delete vbufptr;
    return nullptr;
  }

  surf->AddUserData(&kVolatileBuffer, vbufptr, VolatileBufferRelease);
  return surf;
}

static TemporaryRef<VolatileBuffer>
AllocateBufferForImage(const IntSize& size, SurfaceFormat format)
{
  int32_t stride = VolatileSurfaceStride(size, format);
  RefPtr<VolatileBuffer> buf = new VolatileBuffer();
  if (buf->Init(stride * size.height,
                1 << gfxAlphaRecovery::GoodAlignmentLog2()))
    return buf;

  return nullptr;
}


static bool AllowedImageSize(int32_t aWidth, int32_t aHeight)
{
  
  const int32_t k64KLimit = 0x0000FFFF;
  if (MOZ_UNLIKELY(aWidth > k64KLimit || aHeight > k64KLimit )) {
    NS_WARNING("image too big");
    return false;
  }

  
  if (MOZ_UNLIKELY(aHeight <= 0 || aWidth <= 0)) {
    return false;
  }

  
  CheckedInt32 requiredBytes = CheckedInt32(aWidth) * CheckedInt32(aHeight) * 4;
  if (MOZ_UNLIKELY(!requiredBytes.isValid())) {
    NS_WARNING("width or height too large");
    return false;
  }
#if defined(XP_MACOSX)
  
  if (MOZ_UNLIKELY(aHeight > SHRT_MAX)) {
    NS_WARNING("image too big");
    return false;
  }
#endif
  return true;
}

static bool AllowedImageAndFrameDimensions(const nsIntSize& aImageSize,
                                           const nsIntRect& aFrameRect)
{
  if (!AllowedImageSize(aImageSize.width, aImageSize.height)) {
    return false;
  }
  if (!AllowedImageSize(aFrameRect.width, aFrameRect.height)) {
    return false;
  }
  nsIntRect imageRect(0, 0, aImageSize.width, aImageSize.height);
  if (!imageRect.Contains(aFrameRect)) {
    return false;
  }
  return true;
}


imgFrame::imgFrame() :
  mDecoded(0, 0, 0, 0),
  mDecodedMutex("imgFrame::mDecoded"),
  mPalettedImageData(nullptr),
  mTimeout(100),
  mLockCount(0),
  mDisposalMethod(DisposalMethod::NOT_SPECIFIED),
  mBlendMethod(BlendMethod::OVER),
  mSinglePixel(false),
  mCompositingFailed(false),
  mHasNoAlpha(false),
  mNonPremult(false),
  mOptimizable(false)
{
  static bool hasCheckedOptimize = false;
  if (!hasCheckedOptimize) {
    if (PR_GetEnv("MOZ_DISABLE_IMAGE_OPTIMIZE")) {
      gDisableOptimize = true;
    }
    hasCheckedOptimize = true;
  }
}

imgFrame::~imgFrame()
{
  moz_free(mPalettedImageData);
  mPalettedImageData = nullptr;
}

nsresult
imgFrame::InitForDecoder(const nsIntSize& aImageSize,
                         const nsIntRect& aRect,
                         SurfaceFormat aFormat,
                         uint8_t aPaletteDepth )
{
  
  
  if (!AllowedImageAndFrameDimensions(aImageSize, aRect)) {
    NS_WARNING("Should have legal image size");
    return NS_ERROR_FAILURE;
  }

  mImageSize = aImageSize.ToIntSize();
  mOffset.MoveTo(aRect.x, aRect.y);
  mSize.SizeTo(aRect.width, aRect.height);

  mFormat = aFormat;
  mPaletteDepth = aPaletteDepth;

  if (aPaletteDepth != 0) {
    
    if (aPaletteDepth > 8) {
      NS_WARNING("Should have legal palette depth");
      NS_ERROR("This Depth is not supported");
      return NS_ERROR_FAILURE;
    }

    
    mPalettedImageData = (uint8_t*)moz_malloc(PaletteDataLength() + GetImageDataLength());
    if (!mPalettedImageData)
      NS_WARNING("moz_malloc for paletted image data should succeed");
    NS_ENSURE_TRUE(mPalettedImageData, NS_ERROR_OUT_OF_MEMORY);
  } else {
    MOZ_ASSERT(!mImageSurface, "Called imgFrame::InitForDecoder() twice?");

    mVBuf = AllocateBufferForImage(mSize, mFormat);
    if (!mVBuf) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (mVBuf->OnHeap()) {
      int32_t stride = VolatileSurfaceStride(mSize, mFormat);
      VolatileBufferPtr<uint8_t> ptr(mVBuf);
      memset(ptr, 0, stride * mSize.height);
    }
    mImageSurface = CreateLockedSurface(mVBuf, mSize, mFormat);

    if (!mImageSurface) {
      NS_WARNING("Failed to create VolatileDataSourceSurface");
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  return NS_OK;
}

nsresult
imgFrame::InitWithDrawable(gfxDrawable* aDrawable,
                           const nsIntSize& aSize,
                           const SurfaceFormat aFormat,
                           GraphicsFilter aFilter,
                           uint32_t aImageFlags)
{
  
  
  if (!AllowedImageSize(aSize.width, aSize.height)) {
    NS_WARNING("Should have legal image size");
    return NS_ERROR_FAILURE;
  }

  mImageSize = aSize.ToIntSize();
  mOffset.MoveTo(0, 0);
  mSize.SizeTo(aSize.width, aSize.height);

  mFormat = aFormat;
  mPaletteDepth = 0;

  RefPtr<DrawTarget> target;

  bool canUseDataSurface =
    gfxPlatform::GetPlatform()->CanRenderContentToDataSurface();

  if (canUseDataSurface) {
    
    
    MOZ_ASSERT(!mImageSurface, "Called imgFrame::InitWithDrawable() twice?");

    mVBuf = AllocateBufferForImage(mSize, mFormat);
    if (!mVBuf) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    int32_t stride = VolatileSurfaceStride(mSize, mFormat);
    VolatileBufferPtr<uint8_t> ptr(mVBuf);
    if (!ptr) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    if (mVBuf->OnHeap()) {
      memset(ptr, 0, stride * mSize.height);
    }
    mImageSurface = CreateLockedSurface(mVBuf, mSize, mFormat);

    target = gfxPlatform::GetPlatform()->
      CreateDrawTargetForData(ptr, mSize, stride, mFormat);
  } else {
    
    
    
    
    MOZ_ASSERT(!mOptSurface, "Called imgFrame::InitWithDrawable() twice?");

    target = gfxPlatform::GetPlatform()->
        CreateOffscreenContentDrawTarget(mSize, mFormat);
  }

  if (!target) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  
  nsIntRect imageRect(0, 0, mSize.width, mSize.height);
  nsRefPtr<gfxContext> ctx = new gfxContext(target);
  gfxUtils::DrawPixelSnapped(ctx, aDrawable, ThebesIntSize(mSize),
                             ImageRegion::Create(imageRect),
                             mFormat, aFilter, aImageFlags);

  if (canUseDataSurface && !mImageSurface) {
    NS_WARNING("Failed to create VolatileDataSourceSurface");
    return NS_ERROR_OUT_OF_MEMORY;
  }

  if (!canUseDataSurface) {
    
    
    mOptSurface = target->Snapshot();
  }

  return NS_OK;
}

nsresult imgFrame::Optimize()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mLockCount == 1,
             "Should only optimize when holding the lock exclusively");

  
  if (ShutdownTracker::ShutdownHasStarted())
    return NS_OK;

  if (!mOptimizable || gDisableOptimize)
    return NS_OK;

  if (mPalettedImageData || mOptSurface || mSinglePixel)
    return NS_OK;

  
  
  if (mNonPremult)
    return NS_OK;

  

  
  if (mImageSurface->Stride() == mSize.width * 4) {
    uint32_t *imgData = (uint32_t*) ((uint8_t *)mVBufPtr);
    uint32_t firstPixel = * (uint32_t*) imgData;
    uint32_t pixelCount = mSize.width * mSize.height + 1;

    while (--pixelCount && *imgData++ == firstPixel)
      ;

    if (pixelCount == 0) {
      
      if (mFormat == SurfaceFormat::B8G8R8A8 ||
          mFormat == SurfaceFormat::B8G8R8X8) {
        mSinglePixel = true;
        mSinglePixelColor.a = ((firstPixel >> 24) & 0xFF) * (1.0f / 255.0f);
        mSinglePixelColor.r = ((firstPixel >> 16) & 0xFF) * (1.0f / 255.0f);
        mSinglePixelColor.g = ((firstPixel >>  8) & 0xFF) * (1.0f / 255.0f);
        mSinglePixelColor.b = ((firstPixel >>  0) & 0xFF) * (1.0f / 255.0f);
        mSinglePixelColor.r /= mSinglePixelColor.a;
        mSinglePixelColor.g /= mSinglePixelColor.a;
        mSinglePixelColor.b /= mSinglePixelColor.a;

        
        mVBuf = nullptr;
        mVBufPtr = nullptr;
        mImageSurface = nullptr;
        mOptSurface = nullptr;

        return NS_OK;
      }
    }

    
  }

#ifdef ANDROID
  SurfaceFormat optFormat =
    gfxPlatform::GetPlatform()->Optimal2DFormatForContent(gfxContentType::COLOR);

  if (!GetHasAlpha() && optFormat == SurfaceFormat::R5G6B5) {
    RefPtr<VolatileBuffer> buf =
      AllocateBufferForImage(mSize, optFormat);
    if (!buf)
      return NS_OK;

    RefPtr<DataSourceSurface> surf =
      CreateLockedSurface(buf, mSize, optFormat);
    if (!surf)
      return NS_ERROR_OUT_OF_MEMORY;

    DataSourceSurface::MappedSurface mapping;
    DebugOnly<bool> success =
      surf->Map(DataSourceSurface::MapType::WRITE, &mapping);
    NS_ASSERTION(success, "Failed to map surface");
    RefPtr<DrawTarget> target =
      Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                       mapping.mData,
                                       mSize,
                                       mapping.mStride,
                                       optFormat);

    Rect rect(0, 0, mSize.width, mSize.height);
    target->DrawSurface(mImageSurface, rect, rect);
    target->Flush();
    surf->Unmap();

    mImageSurface = surf;
    mVBuf = buf;
    mFormat = optFormat;
  }
#else
  mOptSurface = gfxPlatform::GetPlatform()->ScreenReferenceDrawTarget()->OptimizeSourceSurface(mImageSurface);
  if (mOptSurface == mImageSurface)
    mOptSurface = nullptr;
#endif

  if (mOptSurface) {
    mVBuf = nullptr;
    mVBufPtr = nullptr;
    mImageSurface = nullptr;
  }

#ifdef MOZ_WIDGET_ANDROID
  
  
  
  
  mImageSurface = nullptr;
#endif

  return NS_OK;
}

DrawableFrameRef
imgFrame::DrawableRef()
{
  return DrawableFrameRef(this);
}

RawAccessFrameRef
imgFrame::RawAccessRef()
{
  return RawAccessFrameRef(this);
}

void
imgFrame::SetRawAccessOnly()
{
  MOZ_ASSERT(mLockCount > 0, "Must hold a RawAccessFrameRef");
  
  LockImageData();
}


imgFrame::SurfaceWithFormat
imgFrame::SurfaceForDrawing(bool               aDoPadding,
                            bool               aDoPartialDecode,
                            bool               aDoTile,
                            gfxContext*        aContext,
                            const nsIntMargin& aPadding,
                            gfxRect&           aImageRect,
                            ImageRegion&       aRegion,
                            SourceSurface*     aSurface)
{
  IntSize size(int32_t(aImageRect.Width()), int32_t(aImageRect.Height()));
  if (!aDoPadding && !aDoPartialDecode) {
    NS_ASSERTION(!mSinglePixel, "This should already have been handled");
    return SurfaceWithFormat(new gfxSurfaceDrawable(aSurface, ThebesIntSize(size)), mFormat);
  }

  gfxRect available = gfxRect(mDecoded.x, mDecoded.y, mDecoded.width, mDecoded.height);

  if (aDoTile || mSinglePixel) {
    
    
    
    RefPtr<DrawTarget> target =
      gfxPlatform::GetPlatform()->
        CreateOffscreenContentDrawTarget(size, SurfaceFormat::B8G8R8A8);
    if (!target)
      return SurfaceWithFormat();

    
    if (mSinglePixel) {
      target->FillRect(ToRect(aRegion.Intersect(available).Rect()),
                       ColorPattern(mSinglePixelColor),
                       DrawOptions(1.0f, CompositionOp::OP_SOURCE));
    } else {
      SurfacePattern pattern(aSurface,
                             ExtendMode::REPEAT,
                             Matrix::Translation(mDecoded.x, mDecoded.y));
      target->FillRect(ToRect(aRegion.Intersect(available).Rect()), pattern);
    }

    RefPtr<SourceSurface> newsurf = target->Snapshot();
    return SurfaceWithFormat(new gfxSurfaceDrawable(newsurf, ThebesIntSize(size)), target->GetFormat());
  }

  
  
  gfxPoint paddingTopLeft(aPadding.left, aPadding.top);
  aRegion = aRegion.Intersect(available) - paddingTopLeft;
  aContext->Multiply(gfxMatrix::Translation(paddingTopLeft));
  aImageRect = gfxRect(0, 0, mSize.width, mSize.height);

  gfxIntSize availableSize(mDecoded.width, mDecoded.height);
  return SurfaceWithFormat(new gfxSurfaceDrawable(aSurface, availableSize),
                           mFormat);
}

bool imgFrame::Draw(gfxContext* aContext, const ImageRegion& aRegion,
                    GraphicsFilter aFilter, uint32_t aImageFlags)
{
  PROFILER_LABEL("imgFrame", "Draw",
    js::ProfileEntry::Category::GRAPHICS);

  NS_ASSERTION(!aRegion.Rect().IsEmpty(), "Drawing empty region!");
  NS_ASSERTION(!aRegion.IsRestricted() ||
               !aRegion.Rect().Intersect(aRegion.Restriction()).IsEmpty(),
               "We must be allowed to sample *some* source pixels!");
  NS_ASSERTION(!mPalettedImageData, "Directly drawing a paletted image!");

  nsIntMargin padding(mOffset.y,
                      mImageSize.width - (mOffset.x + mSize.width),
                      mImageSize.height - (mOffset.y + mSize.height),
                      mOffset.x);

  bool doPadding = padding != nsIntMargin(0,0,0,0);
  bool doPartialDecode = !ImageComplete();

  if (mSinglePixel && !doPadding && !doPartialDecode) {
    if (mSinglePixelColor.a == 0.0) {
      return true;
    }
    RefPtr<DrawTarget> dt = aContext->GetDrawTarget();
    dt->FillRect(ToRect(aRegion.Rect()),
                 ColorPattern(mSinglePixelColor),
                 DrawOptions(1.0f,
                             CompositionOpForOp(aContext->CurrentOperator())));
    return true;
  }

  RefPtr<SourceSurface> surf = GetSurface();
  if (!surf && !mSinglePixel) {
    return false;
  }

  gfxRect imageRect(0, 0, mImageSize.width, mImageSize.height);
  bool doTile = !imageRect.Contains(aRegion.Rect()) &&
                !(aImageFlags & imgIContainer::FLAG_CLAMP);
  ImageRegion region(aRegion);
  
  
  
  
  
  
  gfxContextMatrixAutoSaveRestore autoSR(aContext);
  SurfaceWithFormat surfaceResult =
    SurfaceForDrawing(doPadding, doPartialDecode, doTile, aContext,
                      padding, imageRect, region, surf);

  if (surfaceResult.IsValid()) {
    gfxUtils::DrawPixelSnapped(aContext, surfaceResult.mDrawable,
                               imageRect.Size(), region, surfaceResult.mFormat,
                               aFilter, aImageFlags);
  }
  return true;
}


nsresult imgFrame::ImageUpdated(const nsIntRect &aUpdateRect)
{
  MutexAutoLock lock(mDecodedMutex);

  mDecoded.UnionRect(mDecoded, aUpdateRect);

  
  
  nsIntRect boundsRect(mOffset, nsIntSize(mSize.width, mSize.height));
  mDecoded.IntersectRect(mDecoded, boundsRect);

  return NS_OK;
}

nsIntRect imgFrame::GetRect() const
{
  return nsIntRect(mOffset, nsIntSize(mSize.width, mSize.height));
}

int32_t
imgFrame::GetStride() const
{
  if (mImageSurface) {
    return mImageSurface->Stride();
  }

  return VolatileSurfaceStride(mSize, mFormat);
}

SurfaceFormat imgFrame::GetFormat() const
{
  return mFormat;
}

uint32_t imgFrame::GetImageBytesPerRow() const
{
  if (mVBuf)
    return mSize.width * BytesPerPixel(mFormat);

  if (mPaletteDepth)
    return mSize.width;

  return 0;
}

uint32_t imgFrame::GetImageDataLength() const
{
  return GetImageBytesPerRow() * mSize.height;
}

void imgFrame::GetImageData(uint8_t **aData, uint32_t *length) const
{
  NS_ABORT_IF_FALSE(mLockCount != 0, "Can't GetImageData unless frame is locked");

  if (mImageSurface)
    *aData = mVBufPtr;
  else if (mPalettedImageData)
    *aData = mPalettedImageData + PaletteDataLength();
  else
    *aData = nullptr;

  *length = GetImageDataLength();
}

uint8_t* imgFrame::GetImageData() const
{
  uint8_t *data;
  uint32_t length;
  GetImageData(&data, &length);
  return data;
}

bool imgFrame::GetIsPaletted() const
{
  return mPalettedImageData != nullptr;
}

bool imgFrame::GetHasAlpha() const
{
  return mFormat == SurfaceFormat::B8G8R8A8;
}

void imgFrame::GetPaletteData(uint32_t **aPalette, uint32_t *length) const
{
  NS_ABORT_IF_FALSE(mLockCount != 0, "Can't GetPaletteData unless frame is locked");

  if (!mPalettedImageData) {
    *aPalette = nullptr;
    *length = 0;
  } else {
    *aPalette = (uint32_t *) mPalettedImageData;
    *length = PaletteDataLength();
  }
}

uint32_t* imgFrame::GetPaletteData() const
{
  uint32_t* data;
  uint32_t length;
  GetPaletteData(&data, &length);
  return data;
}

uint8_t*
imgFrame::GetRawData() const
{
  MOZ_ASSERT(mLockCount, "Should be locked to call GetRawData()");
  if (mPalettedImageData) {
    return mPalettedImageData;
  }
  return GetImageData();
}

nsresult imgFrame::LockImageData()
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ABORT_IF_FALSE(mLockCount >= 0, "Unbalanced locks and unlocks");
  if (mLockCount < 0) {
    return NS_ERROR_FAILURE;
  }

  mLockCount++;

  
  if (mLockCount != 1) {
    return NS_OK;
  }

  
  if (mPalettedImageData)
    return NS_OK;

  if (!mImageSurface) {
    if (mVBuf) {
      VolatileBufferPtr<uint8_t> ref(mVBuf);
      if (ref.WasBufferPurged())
        return NS_ERROR_FAILURE;

      mImageSurface = CreateLockedSurface(mVBuf, mSize, mFormat);
      if (!mImageSurface)
        return NS_ERROR_OUT_OF_MEMORY;
    }
    if (mOptSurface || mSinglePixel || mFormat == SurfaceFormat::R5G6B5) {
      SurfaceFormat format = mFormat;
      if (mFormat == SurfaceFormat::R5G6B5)
        format = SurfaceFormat::B8G8R8A8;

      
      RefPtr<VolatileBuffer> buf =
        AllocateBufferForImage(mSize, format);
      if (!buf) {
        return NS_ERROR_OUT_OF_MEMORY;
      }

      RefPtr<DataSourceSurface> surf =
        CreateLockedSurface(buf, mSize, format);
      if (!surf)
        return NS_ERROR_OUT_OF_MEMORY;

      DataSourceSurface::MappedSurface mapping;
      DebugOnly<bool> success =
        surf->Map(DataSourceSurface::MapType::WRITE, &mapping);
      NS_ASSERTION(success, "Failed to map surface");
      RefPtr<DrawTarget> target =
        Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                         mapping.mData,
                                         mSize,
                                         mapping.mStride,
                                         format);

      Rect rect(0, 0, mSize.width, mSize.height);
      if (mSinglePixel)
        target->FillRect(rect, ColorPattern(mSinglePixelColor),
                         DrawOptions(1.0f, CompositionOp::OP_SOURCE));
      else if (mFormat == SurfaceFormat::R5G6B5)
        target->DrawSurface(mImageSurface, rect, rect);
      else
        target->DrawSurface(mOptSurface, rect, rect,
                            DrawSurfaceOptions(),
                            DrawOptions(1.0f, CompositionOp::OP_SOURCE));
      target->Flush();
      surf->Unmap();

      mFormat = format;
      mVBuf = buf;
      mImageSurface = surf;
      mOptSurface = nullptr;
    }
  }

  mVBufPtr = mVBuf;
  return NS_OK;
}

nsresult imgFrame::UnlockImageData()
{
  MOZ_ASSERT(NS_IsMainThread());

  MOZ_ASSERT(mLockCount > 0, "Unlocking an unlocked image!");
  if (mLockCount <= 0) {
    return NS_ERROR_FAILURE;
  }

  
  
  
  if (mLockCount == 1 && !mPalettedImageData) {
    
    
    
    if (mHasNoAlpha && mFormat == SurfaceFormat::B8G8R8A8 && mImageSurface) {
      mFormat = SurfaceFormat::B8G8R8X8;
      mImageSurface = CreateLockedSurface(mVBuf, mSize, mFormat);
    }

    
    
    Optimize();
    
    
    mVBufPtr = nullptr;
  }

  mLockCount--;

  return NS_OK;
}

void
imgFrame::SetOptimizable()
{
  MOZ_ASSERT(mLockCount, "Expected to be locked when SetOptimizable is called");
  mOptimizable = true;
}

TemporaryRef<SourceSurface>
imgFrame::GetSurface()
{
  if (mOptSurface) {
    if (mOptSurface->IsValid())
      return mOptSurface;
    else
      mOptSurface = nullptr;
  }

  if (mImageSurface)
    return mImageSurface;

  if (!mVBuf)
    return nullptr;

  VolatileBufferPtr<char> buf(mVBuf);
  if (buf.WasBufferPurged())
    return nullptr;

  return CreateLockedSurface(mVBuf, mSize, mFormat);
}

TemporaryRef<DrawTarget>
imgFrame::GetDrawTarget()
{
  MOZ_ASSERT(mLockCount >= 1, "Should lock before requesting a DrawTarget");

  uint8_t* data = GetImageData();
  if (!data) {
    return nullptr;
  }

  int32_t stride = GetStride();
  return gfxPlatform::GetPlatform()->
    CreateDrawTargetForData(data, mSize, stride, mFormat);
}

int32_t imgFrame::GetRawTimeout() const
{
  return mTimeout;
}

void imgFrame::SetRawTimeout(int32_t aTimeout)
{
  mTimeout = aTimeout;
}


bool imgFrame::ImageComplete() const
{
  MutexAutoLock lock(mDecodedMutex);

  return mDecoded.IsEqualInterior(nsIntRect(mOffset.x, mOffset.y,
                                            mSize.width, mSize.height));
}



void imgFrame::SetHasNoAlpha()
{
  MOZ_ASSERT(mLockCount, "Expected to be locked when SetHasNoAlpha is called");
  mHasNoAlpha = true;
}

void imgFrame::SetAsNonPremult(bool aIsNonPremult)
{
  mNonPremult = aIsNonPremult;
}

bool imgFrame::GetCompositingFailed() const
{
  return mCompositingFailed;
}

void imgFrame::SetCompositingFailed(bool val)
{
  mCompositingFailed = val;
}

size_t
imgFrame::SizeOfExcludingThis(gfxMemoryLocation aLocation,
                              MallocSizeOf aMallocSizeOf) const
{
  
  
  NS_ABORT_IF_FALSE(
    (aLocation == gfxMemoryLocation::IN_PROCESS_HEAP &&  aMallocSizeOf) ||
    (aLocation != gfxMemoryLocation::IN_PROCESS_HEAP && !aMallocSizeOf),
    "mismatch between aLocation and aMallocSizeOf");

  size_t n = 0;

  if (mPalettedImageData && aLocation == gfxMemoryLocation::IN_PROCESS_HEAP) {
    n += aMallocSizeOf(mPalettedImageData);
  }
  if (mImageSurface && aLocation == gfxMemoryLocation::IN_PROCESS_HEAP) {
    n += aMallocSizeOf(mImageSurface);
  }
  if (mOptSurface && aLocation == gfxMemoryLocation::IN_PROCESS_HEAP) {
    n += aMallocSizeOf(mOptSurface);
  }

  if (mVBuf && aLocation == gfxMemoryLocation::IN_PROCESS_HEAP) {
    n += aMallocSizeOf(mVBuf);
    n += mVBuf->HeapSizeOfExcludingThis(aMallocSizeOf);
  }

  if (mVBuf && aLocation == gfxMemoryLocation::IN_PROCESS_NONHEAP) {
    n += mVBuf->NonHeapSizeOfExcludingThis();
  }

  return n;
}

} 
} 
