





#include "imgFrame.h"
#include "DiscardTracker.h"

#include <limits.h>

#include "prenv.h"

#include "gfxPlatform.h"
#include "gfxUtils.h"

static bool gDisableOptimize = false;

#include "cairo.h"
#include "GeckoProfiler.h"
#include "mozilla/Likely.h"

#if defined(XP_WIN)

#include "gfxWindowsPlatform.h"


#define USE_WIN_SURFACE 1

static uint32_t gTotalDDBs = 0;
static uint32_t gTotalDDBSize = 0;

#define kMaxDDBSize (64*1024*1024)

#define kMaxSingleDDBSize (4*1024*1024)

#endif

using namespace mozilla::image;


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

  
  int32_t tmp = aWidth * aHeight;
  if (MOZ_UNLIKELY(tmp / aHeight != aWidth)) {
    NS_WARNING("width or height too large");
    return false;
  }
  tmp = tmp * 4;
  if (MOZ_UNLIKELY(tmp / 4 != aWidth * aHeight)) {
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



static bool ShouldUseImageSurfaces()
{
#if defined(USE_WIN_SURFACE)
  static const DWORD kGDIObjectsHighWaterMark = 7000;

  if (gfxWindowsPlatform::GetPlatform()->GetRenderMode() ==
      gfxWindowsPlatform::RENDER_DIRECT2D) {
    return true;
  }

  
  
  
  DWORD count = GetGuiResources(GetCurrentProcess(), GR_GDIOBJECTS);
  if (count == 0 ||
      count > kGDIObjectsHighWaterMark)
  {
    
    
    
    return true;
  }
#endif

  return false;
}

imgFrame::imgFrame() :
  mDecoded(0, 0, 0, 0),
  mPalettedImageData(nullptr),
  mSinglePixelColor(0),
  mTimeout(100),
  mDisposalMethod(0), 
  mLockCount(0),
  mBlendMethod(1), 
  mSinglePixel(false),
  mNeverUseDeviceSurface(false),
  mFormatChanged(false),
  mCompositingFailed(false),
  mNonPremult(false),
#ifdef USE_WIN_SURFACE
  mIsDDBSurface(false),
#endif
  mInformedDiscardTracker(false)
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
#ifdef USE_WIN_SURFACE
  if (mIsDDBSurface) {
      gTotalDDBs--;
      gTotalDDBSize -= mSize.width * mSize.height * 4;
  }
#endif

  if (mInformedDiscardTracker) {
    DiscardTracker::InformAllocation(-4 * mSize.height * mSize.width);
  }
}

nsresult imgFrame::Init(int32_t aX, int32_t aY, int32_t aWidth, int32_t aHeight,
                        gfxASurface::gfxImageFormat aFormat, uint8_t aPaletteDepth )
{
  
  if (!AllowedImageSize(aWidth, aHeight))
    return NS_ERROR_FAILURE;

  mOffset.MoveTo(aX, aY);
  mSize.SizeTo(aWidth, aHeight);

  mFormat = aFormat;
  mPaletteDepth = aPaletteDepth;

  if (aPaletteDepth != 0) {
    
    if (aPaletteDepth > 8) {
      NS_ERROR("This Depth is not supported");
      return NS_ERROR_FAILURE;
    }

    
    mPalettedImageData = (uint8_t*)moz_malloc(PaletteDataLength() + GetImageDataLength());
    NS_ENSURE_TRUE(mPalettedImageData, NS_ERROR_OUT_OF_MEMORY);
  } else {
    
    
    
#ifdef USE_WIN_SURFACE
    if (!mNeverUseDeviceSurface && !ShouldUseImageSurfaces()) {
      mWinSurface = new gfxWindowsSurface(gfxIntSize(mSize.width, mSize.height), mFormat);
      if (mWinSurface && mWinSurface->CairoStatus() == 0) {
        
        mImageSurface = mWinSurface->GetAsImageSurface();
      } else {
        mWinSurface = nullptr;
      }
    }
#endif

    
    
    
    
    if (!mImageSurface)
      mImageSurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height), mFormat);

    if (!mImageSurface || mImageSurface->CairoStatus()) {
      mImageSurface = nullptr;
      
      return NS_ERROR_OUT_OF_MEMORY;
    }

#ifdef XP_MACOSX
    if (!mNeverUseDeviceSurface && !ShouldUseImageSurfaces()) {
      mQuartzSurface = new gfxQuartzImageSurface(mImageSurface);
    }
#endif
  }

  
  
  
  if (!mPalettedImageData) {
    DiscardTracker::InformAllocation(4 * mSize.width * mSize.height);
    mInformedDiscardTracker = true;
  }

  return NS_OK;
}

nsresult imgFrame::Optimize()
{
  if (gDisableOptimize)
    return NS_OK;

  if (mPalettedImageData || mOptSurface || mSinglePixel)
    return NS_OK;

  
  
  if (mNonPremult)
    return NS_OK;

  

  
  if (mImageSurface->Stride() == mSize.width * 4) {
    uint32_t *imgData = (uint32_t*) mImageSurface->Data();
    uint32_t firstPixel = * (uint32_t*) imgData;
    uint32_t pixelCount = mSize.width * mSize.height + 1;

    while (--pixelCount && *imgData++ == firstPixel)
      ;

    if (pixelCount == 0) {
      
      if (mFormat == gfxASurface::ImageFormatARGB32 ||
          mFormat == gfxASurface::ImageFormatRGB24)
      {
        
        gfxRGBA::PackedColorType inputType = gfxRGBA::PACKED_XRGB;
        if (mFormat == gfxASurface::ImageFormatARGB32)
          inputType = gfxRGBA::PACKED_ARGB_PREMULTIPLIED;

        mSinglePixelColor = gfxRGBA(firstPixel, inputType);

        mSinglePixel = true;

        
        mImageSurface = nullptr;
        mOptSurface = nullptr;
#ifdef USE_WIN_SURFACE
        mWinSurface = nullptr;
#endif
#ifdef XP_MACOSX
        mQuartzSurface = nullptr;
#endif

        
        
        if (mInformedDiscardTracker) {
          DiscardTracker::InformAllocation(-4 * mSize.width * mSize.height);
          mInformedDiscardTracker = false;
        }

        return NS_OK;
      }
    }

    
  }

  
  
  if (mNeverUseDeviceSurface || ShouldUseImageSurfaces())
    return NS_OK;

  mOptSurface = nullptr;

#ifdef USE_WIN_SURFACE
  
  
  
  if (mWinSurface) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    

    
    uint32_t ddbSize = mSize.width * mSize.height * 4;
    if (ddbSize <= kMaxSingleDDBSize &&
        ddbSize + gTotalDDBSize <= kMaxDDBSize)
    {
      nsRefPtr<gfxWindowsSurface> wsurf = mWinSurface->OptimizeToDDB(nullptr, gfxIntSize(mSize.width, mSize.height), mFormat);
      if (wsurf) {
        gTotalDDBs++;
        gTotalDDBSize += ddbSize;
        mIsDDBSurface = true;
        mOptSurface = wsurf;
      }
    }
    if (!mOptSurface && !mFormatChanged) {
      
      mOptSurface = mWinSurface;
    }
  }
#endif

#ifdef XP_MACOSX
  if (mQuartzSurface) {
    mQuartzSurface->Flush();
    mOptSurface = mQuartzSurface;
  }
#endif

  if (mOptSurface == nullptr)
    mOptSurface = gfxPlatform::GetPlatform()->OptimizeImage(mImageSurface, mFormat);

  if (mOptSurface) {
    mImageSurface = nullptr;
#ifdef USE_WIN_SURFACE
    mWinSurface = nullptr;
#endif
#ifdef XP_MACOSX
    mQuartzSurface = nullptr;
#endif
  }

  return NS_OK;
}

static void
DoSingleColorFastPath(gfxContext*    aContext,
                      const gfxRGBA& aSinglePixelColor,
                      const gfxRect& aFill)
{
  
  if (aSinglePixelColor.a == 0.0)
    return;

  gfxContext::GraphicsOperator op = aContext->CurrentOperator();
  if (op == gfxContext::OPERATOR_OVER && aSinglePixelColor.a == 1.0) {
    aContext->SetOperator(gfxContext::OPERATOR_SOURCE);
  }

  aContext->SetDeviceColor(aSinglePixelColor);
  aContext->NewPath();
  aContext->Rectangle(aFill);
  aContext->Fill();
  aContext->SetOperator(op);
  aContext->SetDeviceColor(gfxRGBA(0,0,0,0));
}

imgFrame::SurfaceWithFormat
imgFrame::SurfaceForDrawing(bool               aDoPadding,
                            bool               aDoPartialDecode,
                            bool               aDoTile,
                            const nsIntMargin& aPadding,
                            gfxMatrix&         aUserSpaceToImageSpace,
                            gfxRect&           aFill,
                            gfxRect&           aSubimage,
                            gfxRect&           aSourceRect,
                            gfxRect&           aImageRect)
{
  gfxIntSize size(int32_t(aImageRect.Width()), int32_t(aImageRect.Height()));
  if (!aDoPadding && !aDoPartialDecode) {
    NS_ASSERTION(!mSinglePixel, "This should already have been handled");
    return SurfaceWithFormat(new gfxSurfaceDrawable(ThebesSurface(), size), mFormat);
  }

  gfxRect available = gfxRect(mDecoded.x, mDecoded.y, mDecoded.width, mDecoded.height);

  if (aDoTile || mSinglePixel) {
    
    
    
    gfxImageSurface::gfxImageFormat format = gfxASurface::ImageFormatARGB32;
    nsRefPtr<gfxASurface> surface =
      gfxPlatform::GetPlatform()->CreateOffscreenSurface(size, gfxImageSurface::ContentFromFormat(format));
    if (!surface || surface->CairoStatus())
      return SurfaceWithFormat();

    
    gfxContext tmpCtx(surface);
    tmpCtx.SetOperator(gfxContext::OPERATOR_SOURCE);
    if (mSinglePixel) {
      tmpCtx.SetDeviceColor(mSinglePixelColor);
    } else {
      tmpCtx.SetSource(ThebesSurface(), gfxPoint(aPadding.left, aPadding.top));
    }
    tmpCtx.Rectangle(available);
    tmpCtx.Fill();

    return SurfaceWithFormat(new gfxSurfaceDrawable(surface, size), format);
  }

  
  
  
  aSourceRect = aSourceRect.Intersect(available);
  gfxMatrix imageSpaceToUserSpace = aUserSpaceToImageSpace;
  imageSpaceToUserSpace.Invert();
  aFill = imageSpaceToUserSpace.Transform(aSourceRect);

  aSubimage = aSubimage.Intersect(available) - gfxPoint(aPadding.left, aPadding.top);
  aUserSpaceToImageSpace.Multiply(gfxMatrix().Translate(-gfxPoint(aPadding.left, aPadding.top)));
  aSourceRect = aSourceRect - gfxPoint(aPadding.left, aPadding.top);
  aImageRect = gfxRect(0, 0, mSize.width, mSize.height);

  gfxIntSize availableSize(mDecoded.width, mDecoded.height);
  return SurfaceWithFormat(new gfxSurfaceDrawable(ThebesSurface(),
                                                  availableSize),
                           mFormat);
}

void imgFrame::Draw(gfxContext *aContext, gfxPattern::GraphicsFilter aFilter,
                    const gfxMatrix &aUserSpaceToImageSpace, const gfxRect& aFill,
                    const nsIntMargin &aPadding, const nsIntRect &aSubimage,
                    uint32_t aImageFlags)
{
  PROFILER_LABEL("image", "imgFrame::Draw");
  NS_ASSERTION(!aFill.IsEmpty(), "zero dest size --- fix caller");
  NS_ASSERTION(!aSubimage.IsEmpty(), "zero source size --- fix caller");
  NS_ASSERTION(!mPalettedImageData, "Directly drawing a paletted image!");

  bool doPadding = aPadding != nsIntMargin(0,0,0,0);
  bool doPartialDecode = !ImageComplete();

  if (mSinglePixel && !doPadding && !doPartialDecode) {
    DoSingleColorFastPath(aContext, mSinglePixelColor, aFill);
    return;
  }

  gfxMatrix userSpaceToImageSpace = aUserSpaceToImageSpace;
  gfxRect sourceRect = userSpaceToImageSpace.Transform(aFill);
  gfxRect imageRect(0, 0, mSize.width + aPadding.LeftRight(),
                    mSize.height + aPadding.TopBottom());
  gfxRect subimage(aSubimage.x, aSubimage.y, aSubimage.width, aSubimage.height);
  gfxRect fill = aFill;

  NS_ASSERTION(!sourceRect.Intersect(subimage).IsEmpty(),
               "We must be allowed to sample *some* source pixels!");

  bool doTile = !imageRect.Contains(sourceRect) &&
                !(aImageFlags & imgIContainer::FLAG_CLAMP);
  SurfaceWithFormat surfaceResult =
    SurfaceForDrawing(doPadding, doPartialDecode, doTile, aPadding,
                      userSpaceToImageSpace, fill, subimage, sourceRect,
                      imageRect);

  if (surfaceResult.IsValid()) {
    gfxUtils::DrawPixelSnapped(aContext, surfaceResult.mDrawable,
                               userSpaceToImageSpace,
                               subimage, sourceRect, imageRect, fill,
                               surfaceResult.mFormat, aFilter, aImageFlags);
  }
}

nsresult imgFrame::Extract(const nsIntRect& aRegion, imgFrame** aResult)
{
  nsAutoPtr<imgFrame> subImage(new imgFrame());

  
  
  
  
  
  
  
  
  
  
  subImage->mNeverUseDeviceSurface = true;

  nsresult rv = subImage->Init(0, 0, aRegion.width, aRegion.height,
                               mFormat, mPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  subImage->SetAsNonPremult(mNonPremult);

  
  {
    gfxContext ctx(subImage->ThebesSurface());
    ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
    if (mSinglePixel) {
      ctx.SetDeviceColor(mSinglePixelColor);
    } else {
      
      
      
      
      ctx.SetSource(this->ThebesSurface(), gfxPoint(-aRegion.x, -aRegion.y));
    }
    ctx.Rectangle(gfxRect(0, 0, aRegion.width, aRegion.height));
    ctx.Fill();
  }

  nsIntRect filled(0, 0, aRegion.width, aRegion.height);

  rv = subImage->ImageUpdated(filled);
  NS_ENSURE_SUCCESS(rv, rv);

  subImage->Optimize();

  *aResult = subImage.forget();

  return NS_OK;
}

nsresult imgFrame::ImageUpdated(const nsIntRect &aUpdateRect)
{
  mDecoded.UnionRect(mDecoded, aUpdateRect);

  
  
  nsIntRect boundsRect(mOffset, mSize);
  mDecoded.IntersectRect(mDecoded, boundsRect);

#ifdef XP_MACOSX
  if (mQuartzSurface)
    mQuartzSurface->Flush();
#endif
  return NS_OK;
}

nsIntRect imgFrame::GetRect() const
{
  return nsIntRect(mOffset, mSize);
}

gfxASurface::gfxImageFormat imgFrame::GetFormat() const
{
  return mFormat;
}

bool imgFrame::GetNeedsBackground() const
{
  
  return (mFormat == gfxASurface::ImageFormatARGB32 || !ImageComplete());
}

uint32_t imgFrame::GetImageBytesPerRow() const
{
  if (mImageSurface)
    return mImageSurface->Stride();

  if (mPaletteDepth)
    return mSize.width;

  NS_ERROR("GetImageBytesPerRow called with mImageSurface == null and mPaletteDepth == 0");

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
    *aData = mImageSurface->Data();
  else if (mPalettedImageData)
    *aData = mPalettedImageData + PaletteDataLength();
  else
    *aData = nullptr;

  *length = GetImageDataLength();
}

bool imgFrame::GetIsPaletted() const
{
  return mPalettedImageData != nullptr;
}

bool imgFrame::GetHasAlpha() const
{
  return mFormat == gfxASurface::ImageFormatARGB32;
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

nsresult imgFrame::LockImageData()
{
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

  if ((mOptSurface || mSinglePixel) && !mImageSurface) {
    
    mImageSurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height),
                                        gfxImageSurface::ImageFormatARGB32);
    if (!mImageSurface || mImageSurface->CairoStatus())
      return NS_ERROR_OUT_OF_MEMORY;

    gfxContext context(mImageSurface);
    context.SetOperator(gfxContext::OPERATOR_SOURCE);
    if (mSinglePixel)
      context.SetDeviceColor(mSinglePixelColor);
    else
      context.SetSource(mOptSurface);
    context.Paint();

    mOptSurface = nullptr;
#ifdef USE_WIN_SURFACE
    mWinSurface = nullptr;
#endif
#ifdef XP_MACOSX
    mQuartzSurface = nullptr;
#endif
  }

  
  
  if (mImageSurface)
    mImageSurface->Flush();

#ifdef USE_WIN_SURFACE
  if (mWinSurface)
    mWinSurface->Flush();
#endif

  return NS_OK;
}

nsresult imgFrame::UnlockImageData()
{
  NS_ABORT_IF_FALSE(mLockCount != 0, "Unlocking an unlocked image!");
  if (mLockCount == 0) {
    return NS_ERROR_FAILURE;
  }

  mLockCount--;

  NS_ABORT_IF_FALSE(mLockCount >= 0, "Unbalanced locks and unlocks");
  if (mLockCount < 0) {
    return NS_ERROR_FAILURE;
  }

  
  if (mLockCount != 0) {
    return NS_OK;
  }

  
  if (mPalettedImageData)
    return NS_OK;

  
  if (mImageSurface)
    mImageSurface->MarkDirty();

#ifdef USE_WIN_SURFACE
  if (mWinSurface)
    mWinSurface->MarkDirty();
#endif

#ifdef XP_MACOSX
  
  
  if (mQuartzSurface)
    mQuartzSurface->Flush();
#endif

  return NS_OK;
}

void imgFrame::MarkImageDataDirty()
{
  if (mImageSurface)
    mImageSurface->Flush();

#ifdef USE_WIN_SURFACE
  if (mWinSurface)
    mWinSurface->Flush();
#endif

  if (mImageSurface)
    mImageSurface->MarkDirty();

#ifdef USE_WIN_SURFACE
  if (mWinSurface)
    mWinSurface->MarkDirty();
#endif

#ifdef XP_MACOSX
  
  
  if (mQuartzSurface)
    mQuartzSurface->Flush();
#endif
}

int32_t imgFrame::GetTimeout() const
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  if (mTimeout >= 0 && mTimeout <= 10)
    return 100;
  else
    return mTimeout;
}

void imgFrame::SetTimeout(int32_t aTimeout)
{
  mTimeout = aTimeout;
}

int32_t imgFrame::GetFrameDisposalMethod() const
{
  return mDisposalMethod;
}

void imgFrame::SetFrameDisposalMethod(int32_t aFrameDisposalMethod)
{
  mDisposalMethod = aFrameDisposalMethod;
}

int32_t imgFrame::GetBlendMethod() const
{
  return mBlendMethod;
}

void imgFrame::SetBlendMethod(int32_t aBlendMethod)
{
  mBlendMethod = (int8_t)aBlendMethod;
}

bool imgFrame::ImageComplete() const
{
  return mDecoded.IsEqualInterior(nsIntRect(mOffset, mSize));
}







void imgFrame::SetHasNoAlpha()
{
  if (mFormat == gfxASurface::ImageFormatARGB32) {
      mFormat = gfxASurface::ImageFormatRGB24;
      mFormatChanged = true;
  }
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
imgFrame::SizeOfExcludingThisWithComputedFallbackIfHeap(gfxASurface::MemoryLocation aLocation, nsMallocSizeOfFun aMallocSizeOf) const
{
  
  
  NS_ABORT_IF_FALSE(
    (aLocation == gfxASurface::MEMORY_IN_PROCESS_HEAP &&  aMallocSizeOf) ||
    (aLocation != gfxASurface::MEMORY_IN_PROCESS_HEAP && !aMallocSizeOf),
    "mismatch between aLocation and aMallocSizeOf");

  size_t n = 0;

  if (mPalettedImageData && aLocation == gfxASurface::MEMORY_IN_PROCESS_HEAP) {
    size_t n2 = aMallocSizeOf(mPalettedImageData);
    if (n2 == 0) {
      n2 = GetImageDataLength() + PaletteDataLength();
    }
    n += n2;
  }

#ifdef USE_WIN_SURFACE
  if (mWinSurface && aLocation == mWinSurface->GetMemoryLocation()) {
    n += mWinSurface->KnownMemoryUsed();
  } else
#endif
#ifdef XP_MACOSX
  if (mQuartzSurface && aLocation == gfxASurface::MEMORY_IN_PROCESS_HEAP) {
    n += mSize.width * mSize.height * 4;
  } else
#endif
  if (mImageSurface && aLocation == mImageSurface->GetMemoryLocation()) {
    size_t n2 = 0;
    if (aLocation == gfxASurface::MEMORY_IN_PROCESS_HEAP) { 
      n2 = mImageSurface->SizeOfIncludingThis(aMallocSizeOf);
    }
    if (n2 == 0) {  
      n2 = mImageSurface->KnownMemoryUsed();
    }
    n += n2;
  }

  if (mOptSurface && aLocation == mOptSurface->GetMemoryLocation()) {
    size_t n2 = 0;
    if (aLocation == gfxASurface::MEMORY_IN_PROCESS_HEAP &&
        mOptSurface->SizeOfIsMeasured()) {
      
      n2 = mOptSurface->SizeOfIncludingThis(aMallocSizeOf);
    }
    if (n2 == 0) {  
      n2 = mOptSurface->KnownMemoryUsed();
    }
    n += n2;
  }

  return n;
}
