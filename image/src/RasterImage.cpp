






#include "ImageLogging.h"

#include "RasterImage.h"

#include "base/histogram.h"
#include "gfxPlatform.h"
#include "nsComponentManagerUtils.h"
#include "nsError.h"
#include "Decoder.h"
#include "nsAutoPtr.h"
#include "prenv.h"
#include "prsystem.h"
#include "ImageContainer.h"
#include "ImageRegion.h"
#include "Layers.h"
#include "nsPresContext.h"
#include "SourceBuffer.h"
#include "SurfaceCache.h"
#include "FrameAnimator.h"

#include "nsPNGDecoder.h"
#include "nsGIFDecoder2.h"
#include "nsJPEGDecoder.h"
#include "nsBMPDecoder.h"
#include "nsICODecoder.h"
#include "nsIconDecoder.h"

#include "gfxContext.h"

#include "mozilla/gfx/2D.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Likely.h"
#include "mozilla/RefPtr.h"
#include "mozilla/Move.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/Services.h"
#include <stdint.h>
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/gfx/Scale.h"

#include "GeckoProfiler.h"
#include "gfx2DGlue.h"
#include "gfxPrefs.h"
#include <algorithm>

namespace mozilla {

using namespace gfx;
using namespace layers;

namespace image {

using std::ceil;
using std::min;



static int32_t sMaxDecodeCount = 0;
















#define LOG_CONTAINER_ERROR                      \
  PR_BEGIN_MACRO                                 \
  PR_LOG (GetImgLog(), PR_LOG_ERROR,             \
          ("RasterImage: [this=%p] Error "      \
           "detected at line %u for image of "   \
           "type %s\n", this, __LINE__,          \
           mSourceDataMimeType.get()));          \
  PR_END_MACRO

#define CONTAINER_ENSURE_SUCCESS(status)      \
  PR_BEGIN_MACRO                              \
  nsresult _status = status; /* eval once */  \
  if (NS_FAILED(_status)) {                   \
    LOG_CONTAINER_ERROR;                      \
    DoError();                                \
    return _status;                           \
  }                                           \
 PR_END_MACRO

#define CONTAINER_ENSURE_TRUE(arg, rv)  \
  PR_BEGIN_MACRO                        \
  if (!(arg)) {                         \
    LOG_CONTAINER_ERROR;                \
    DoError();                          \
    return rv;                          \
  }                                     \
  PR_END_MACRO

class ScaleRunner : public nsRunnable
{
  enum ScaleState
  {
    eNew,
    eReady,
    eFinish,
    eFinishWithError
  };

public:
  ScaleRunner(RasterImage* aImage,
              uint32_t aImageFlags,
              const IntSize& aSize,
              RawAccessFrameRef&& aSrcRef)
    : mImage(aImage)
    , mSrcRef(Move(aSrcRef))
    , mDstSize(aSize)
    , mImageFlags(aImageFlags)
    , mState(eNew)
  {
    MOZ_ASSERT(!mSrcRef->GetIsPaletted());
    MOZ_ASSERT(aSize.width > 0 && aSize.height > 0);
  }

  bool Init()
  {
    MOZ_ASSERT(NS_IsMainThread());
    MOZ_ASSERT(mState == eNew, "Calling Init() twice?");

    
    
    nsRefPtr<imgFrame> tentativeDstFrame = new imgFrame();
    nsresult rv =
      tentativeDstFrame->InitForDecoder(mDstSize, SurfaceFormat::B8G8R8A8);
    if (NS_FAILED(rv)) {
      return false;
    }

    
    
    RawAccessFrameRef tentativeDstRef = tentativeDstFrame->RawAccessRef();
    if (!tentativeDstRef) {
      return false;
    }

    
    mDstRef = Move(tentativeDstRef);
    mState = eReady;

    
    
    SurfaceCache::Insert(mDstRef.get(), ImageKey(mImage.get()),
                         RasterSurfaceKey(mDstSize, mImageFlags, 0),
                         Lifetime::Transient);

    return true;
  }

  NS_IMETHOD Run() override
  {
    if (mState == eReady) {
      
      ScalingData srcData = mSrcRef->GetScalingData();
      ScalingData dstData = mDstRef->GetScalingData();

      
      bool succeeded =
        gfx::Scale(srcData.mRawData, srcData.mSize.width, srcData.mSize.height,
                   srcData.mBytesPerRow, dstData.mRawData, mDstSize.width,
                   mDstSize.height, dstData.mBytesPerRow, srcData.mFormat);

      if (succeeded) {
        
        mDstRef->ImageUpdated(mDstRef->GetRect());
        MOZ_ASSERT(mDstRef->IsImageComplete(),
                   "Incomplete, but just updated the entire frame");
      }

      
      
      mState = succeeded ? eFinish : eFinishWithError;
      NS_DispatchToMainThread(this);
    } else if (mState == eFinish) {
      MOZ_ASSERT(NS_IsMainThread());
      MOZ_ASSERT(mDstRef, "Should have a valid scaled frame");

      
      nsRefPtr<RasterImage> image = mImage.get();
      if (image) {
        image->NotifyNewScaledFrame();
      }

      
      mSrcRef.reset();
      mDstRef.reset();
    } else if (mState == eFinishWithError) {
      MOZ_ASSERT(NS_IsMainThread());
      NS_WARNING("HQ scaling failed");

      
      SurfaceCache::RemoveSurface(ImageKey(mImage.get()),
                                  RasterSurfaceKey(mDstSize,
                                                   mImageFlags, 0));

      
      mSrcRef.reset();
      mDstRef.reset();
    } else {
      
      MOZ_ASSERT(false, "Need to call Init() before dispatching");
    }

    return NS_OK;
  }

private:
  virtual ~ScaleRunner()
  {
    MOZ_ASSERT(!mSrcRef && !mDstRef,
               "Should have released strong refs in Run()");
  }

  WeakPtr<RasterImage> mImage;
  RawAccessFrameRef    mSrcRef;
  RawAccessFrameRef    mDstRef;
  const IntSize      mDstSize;
  uint32_t             mImageFlags;
  ScaleState           mState;
};

static nsCOMPtr<nsIThread> sScaleWorkerThread = nullptr;

#ifndef DEBUG
NS_IMPL_ISUPPORTS(RasterImage, imgIContainer, nsIProperties)
#else
NS_IMPL_ISUPPORTS(RasterImage, imgIContainer, nsIProperties,
                  imgIContainerDebug)
#endif


RasterImage::RasterImage(ImageURL* aURI ) :
  ImageResource(aURI), 
  mSize(0,0),
  mLockCount(0),
  mDecodeCount(0),
  mRequestedSampleSize(0),
  mLastImageContainerDrawResult(DrawResult::NOT_READY),
#ifdef DEBUG
  mFramesNotified(0),
#endif
  mSourceBuffer(new SourceBuffer()),
  mFrameCount(0),
  mHasSize(false),
  mDecodeOnlyOnDraw(false),
  mTransient(false),
  mDiscardable(false),
  mHasSourceData(false),
  mHasBeenDecoded(false),
  mPendingAnimation(false),
  mAnimationFinished(false),
  mWantFullDecode(false)
{
  Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Add(0);
}


RasterImage::~RasterImage()
{
  
  
  if (!mSourceBuffer->IsComplete()) {
    mSourceBuffer->Complete(NS_ERROR_ABORT);
  }

  
  SurfaceCache::RemoveImage(ImageKey(this));
}

nsresult
RasterImage::Init(const char* aMimeType,
                  uint32_t aFlags)
{
  
  if (mInitialized) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_ARG_POINTER(aMimeType);

  
  
  MOZ_ASSERT(!(aFlags & INIT_FLAG_TRANSIENT) ||
               (!(aFlags & INIT_FLAG_DISCARDABLE) &&
                !(aFlags & INIT_FLAG_DECODE_ONLY_ON_DRAW) &&
                !(aFlags & INIT_FLAG_DOWNSCALE_DURING_DECODE)),
             "Illegal init flags for transient image");

  
  mSourceDataMimeType.Assign(aMimeType);
  mDiscardable = !!(aFlags & INIT_FLAG_DISCARDABLE);
  mDecodeOnlyOnDraw = !!(aFlags & INIT_FLAG_DECODE_ONLY_ON_DRAW);
  mWantFullDecode = !!(aFlags & INIT_FLAG_DECODE_IMMEDIATELY);
  mTransient = !!(aFlags & INIT_FLAG_TRANSIENT);
  mDownscaleDuringDecode = !!(aFlags & INIT_FLAG_DOWNSCALE_DURING_DECODE);

#ifndef MOZ_ENABLE_SKIA
  
  mDownscaleDuringDecode = false;
#endif

  
  if (!mDiscardable) {
    mLockCount++;
    SurfaceCache::LockImage(ImageKey(this));
  }

  
  nsresult rv = Decode(Nothing(), DECODE_FLAGS_DEFAULT);
  if (NS_FAILED(rv)) {
    return NS_ERROR_FAILURE;
  }

  
  mInitialized = true;

  return NS_OK;
}



NS_IMETHODIMP_(void)
RasterImage::RequestRefresh(const TimeStamp& aTime)
{
  if (HadRecentRefresh(aTime)) {
    return;
  }

  EvaluateAnimation();

  if (!mAnimating) {
    return;
  }

  FrameAnimator::RefreshResult res;
  if (mAnim) {
    res = mAnim->RequestRefresh(aTime);
  }

  if (res.frameAdvanced) {
    
    
    
    #ifdef DEBUG
      mFramesNotified++;
    #endif

    NotifyProgress(NoProgress, res.dirtyRect);
  }

  if (res.animationFinished) {
    mAnimationFinished = true;
    EvaluateAnimation();
  }
}



NS_IMETHODIMP
RasterImage::GetWidth(int32_t* aWidth)
{
  NS_ENSURE_ARG_POINTER(aWidth);

  if (mError) {
    *aWidth = 0;
    return NS_ERROR_FAILURE;
  }

  *aWidth = mSize.width;
  return NS_OK;
}



NS_IMETHODIMP
RasterImage::GetHeight(int32_t* aHeight)
{
  NS_ENSURE_ARG_POINTER(aHeight);

  if (mError) {
    *aHeight = 0;
    return NS_ERROR_FAILURE;
  }

  *aHeight = mSize.height;
  return NS_OK;
}



NS_IMETHODIMP
RasterImage::GetIntrinsicSize(nsSize* aSize)
{
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  *aSize = nsSize(nsPresContext::CSSPixelsToAppUnits(mSize.width),
                  nsPresContext::CSSPixelsToAppUnits(mSize.height));
  return NS_OK;
}



NS_IMETHODIMP
RasterImage::GetIntrinsicRatio(nsSize* aRatio)
{
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  *aRatio = nsSize(mSize.width, mSize.height);
  return NS_OK;
}

NS_IMETHODIMP_(Orientation)
RasterImage::GetOrientation()
{
  return mOrientation;
}



NS_IMETHODIMP
RasterImage::GetType(uint16_t* aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = imgIContainer::TYPE_RASTER;
  return NS_OK;
}

DrawableFrameRef
RasterImage::LookupFrameInternal(uint32_t aFrameNum,
                                 const IntSize& aSize,
                                 uint32_t aFlags)
{
  if (!mAnim) {
    NS_ASSERTION(aFrameNum == 0,
                 "Don't ask for a frame > 0 if we're not animated!");
    aFrameNum = 0;
  }

  if (mAnim && aFrameNum > 0) {
    MOZ_ASSERT(DecodeFlags(aFlags) == DECODE_FLAGS_DEFAULT,
               "Can't composite frames with non-default decode flags");
    return mAnim->GetCompositedFrame(aFrameNum);
  }

  Maybe<uint32_t> alternateFlags;
  if (IsOpaque()) {
    
    
    
    alternateFlags = Some(aFlags ^ FLAG_DECODE_NO_PREMULTIPLY_ALPHA);
  }

  
  
  if (aFlags & FLAG_SYNC_DECODE) {
    return SurfaceCache::Lookup(ImageKey(this),
                                RasterSurfaceKey(aSize,
                                                 DecodeFlags(aFlags),
                                                 aFrameNum),
                                alternateFlags);
  }

  
  return SurfaceCache::LookupBestMatch(ImageKey(this),
                                       RasterSurfaceKey(aSize,
                                                        DecodeFlags(aFlags),
                                                        aFrameNum),
                                       alternateFlags);
}

DrawableFrameRef
RasterImage::LookupFrame(uint32_t aFrameNum,
                         const IntSize& aSize,
                         uint32_t aFlags)
{
  MOZ_ASSERT(NS_IsMainThread());

  IntSize requestedSize = CanDownscaleDuringDecode(aSize, aFlags)
                        ? aSize : mSize;

  DrawableFrameRef ref = LookupFrameInternal(aFrameNum, requestedSize, aFlags);

  if (!ref && !mHasSize) {
    
    return DrawableFrameRef();
  }

  if (!ref || ref->GetImageSize() != requestedSize) {
    
    MOZ_ASSERT(!mAnim, "Animated frames should be locked");

    Decode(Some(requestedSize), aFlags);

    
    if (aFlags & FLAG_SYNC_DECODE) {
      ref = LookupFrameInternal(aFrameNum, requestedSize, aFlags);
    }
  }

  if (!ref) {
    
    return DrawableFrameRef();
  }

  if (ref->GetCompositingFailed()) {
    return DrawableFrameRef();
  }

  MOZ_ASSERT(!ref || !ref->GetIsPaletted(), "Should not have paletted frame");

  
  
  
  if (ref && mHasSourceData && (aFlags & FLAG_SYNC_DECODE)) {
    ref->WaitUntilComplete();
  }

  return ref;
}

uint32_t
RasterImage::GetCurrentFrameIndex() const
{
  if (mAnim) {
    return mAnim->GetCurrentAnimationFrameIndex();
  }

  return 0;
}

uint32_t
RasterImage::GetRequestedFrameIndex(uint32_t aWhichFrame) const
{
  return aWhichFrame == FRAME_FIRST ? 0 : GetCurrentFrameIndex();
}

IntRect
RasterImage::GetFirstFrameRect()
{
  if (mAnim) {
    return mAnim->GetFirstFrameRefreshArea();
  }

  
  return IntRect(IntPoint(0,0), mSize);
}

NS_IMETHODIMP_(bool)
RasterImage::IsOpaque()
{
  if (mError) {
    return false;
  }

  Progress progress = mProgressTracker->GetProgress();

  
  if (!(progress & FLAG_DECODE_COMPLETE)) {
    return false;
  }

  
  return !(progress & FLAG_HAS_TRANSPARENCY);
}

void
RasterImage::OnSurfaceDiscarded()
{
  MOZ_ASSERT(mProgressTracker);

  nsCOMPtr<nsIRunnable> runnable =
    NS_NewRunnableMethod(mProgressTracker, &ProgressTracker::OnDiscard);
  NS_DispatchToMainThread(runnable);
}



NS_IMETHODIMP
RasterImage::GetAnimated(bool* aAnimated)
{
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  NS_ENSURE_ARG_POINTER(aAnimated);

  
  if (mAnim) {
    *aAnimated = true;
    return NS_OK;
  }

  
  
  if (!mHasBeenDecoded) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  
  *aAnimated = false;

  return NS_OK;
}



NS_IMETHODIMP_(int32_t)
RasterImage::GetFirstFrameDelay()
{
  if (mError) {
    return -1;
  }

  bool animated = false;
  if (NS_FAILED(GetAnimated(&animated)) || !animated) {
    return -1;
  }

  MOZ_ASSERT(mAnim, "Animated images should have a FrameAnimator");
  return mAnim->GetTimeoutForFrame(0);
}

TemporaryRef<SourceSurface>
RasterImage::CopyFrame(uint32_t aWhichFrame, uint32_t aFlags)
{
  if (aWhichFrame > FRAME_MAX_VALUE) {
    return nullptr;
  }

  if (mError) {
    return nullptr;
  }

  
  
  
  DrawableFrameRef frameRef =
    LookupFrame(GetRequestedFrameIndex(aWhichFrame), mSize, aFlags);
  if (!frameRef) {
    
    return nullptr;
  }

  
  

  IntSize size(mSize.width, mSize.height);
  RefPtr<DataSourceSurface> surf =
    Factory::CreateDataSourceSurface(size,
                                     SurfaceFormat::B8G8R8A8,
                                      true);
  if (NS_WARN_IF(!surf)) {
    return nullptr;
  }

  DataSourceSurface::MappedSurface mapping;
  DebugOnly<bool> success =
    surf->Map(DataSourceSurface::MapType::WRITE, &mapping);
  NS_ASSERTION(success, "Failed to map surface");
  RefPtr<DrawTarget> target =
    Factory::CreateDrawTargetForData(BackendType::CAIRO,
                                     mapping.mData,
                                     size,
                                     mapping.mStride,
                                     SurfaceFormat::B8G8R8A8);
  if (!target) {
    gfxWarning() << "RasterImage::CopyFrame failed in CreateDrawTargetForData";
    return nullptr;
  }

  IntRect intFrameRect = frameRef->GetRect();
  Rect rect(intFrameRect.x, intFrameRect.y,
            intFrameRect.width, intFrameRect.height);
  if (frameRef->IsSinglePixel()) {
    target->FillRect(rect, ColorPattern(frameRef->SinglePixelColor()),
                     DrawOptions(1.0f, CompositionOp::OP_SOURCE));
  } else {
    RefPtr<SourceSurface> srcSurf = frameRef->GetSurface();
    if (!srcSurf) {
      RecoverFromLossOfFrames(mSize, aFlags);
      return nullptr;
    }

    Rect srcRect(0, 0, intFrameRect.width, intFrameRect.height);
    target->DrawSurface(srcSurf, srcRect, rect);
  }

  target->Flush();
  surf->Unmap();

  return surf;
}




NS_IMETHODIMP_(TemporaryRef<SourceSurface>)
RasterImage::GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags)
{
  return GetFrameInternal(aWhichFrame, aFlags).second().forget();
}

Pair<DrawResult, RefPtr<SourceSurface>>
RasterImage::GetFrameInternal(uint32_t aWhichFrame, uint32_t aFlags)
{
  MOZ_ASSERT(aWhichFrame <= FRAME_MAX_VALUE);

  if (aWhichFrame > FRAME_MAX_VALUE) {
    return MakePair(DrawResult::BAD_ARGS, RefPtr<SourceSurface>());
  }

  if (mError) {
    return MakePair(DrawResult::BAD_IMAGE, RefPtr<SourceSurface>());
  }

  
  
  
  DrawableFrameRef frameRef =
    LookupFrame(GetRequestedFrameIndex(aWhichFrame), mSize, aFlags);
  if (!frameRef) {
    
    return MakePair(DrawResult::TEMPORARY_ERROR, RefPtr<SourceSurface>());
  }

  
  
  RefPtr<SourceSurface> frameSurf;
  IntRect frameRect = frameRef->GetRect();
  if (frameRect.x == 0 && frameRect.y == 0 &&
      frameRect.width == mSize.width &&
      frameRect.height == mSize.height) {
    frameSurf = frameRef->GetSurface();
  }

  
  
  if (!frameSurf) {
    frameSurf = CopyFrame(aWhichFrame, aFlags);
  }

  if (!frameRef->IsImageComplete()) {
    return MakePair(DrawResult::INCOMPLETE, Move(frameSurf));
  }

  return MakePair(DrawResult::SUCCESS, Move(frameSurf));
}

Pair<DrawResult, nsRefPtr<layers::Image>>
RasterImage::GetCurrentImage(ImageContainer* aContainer, uint32_t aFlags)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aContainer);

  auto result = GetFrameInternal(FRAME_CURRENT, aFlags | FLAG_ASYNC_NOTIFY);
  if (!result.second()) {
    
    
    return MakePair(result.first(), nsRefPtr<layers::Image>());
  }

  CairoImage::Data cairoData;
  GetWidth(&cairoData.mSize.width);
  GetHeight(&cairoData.mSize.height);
  cairoData.mSourceSurface = result.second();

  nsRefPtr<layers::Image> image =
    aContainer->CreateImage(ImageFormat::CAIRO_SURFACE);
  MOZ_ASSERT(image);

  static_cast<CairoImage*>(image.get())->SetData(cairoData);

  return MakePair(result.first(), Move(image));
}


NS_IMETHODIMP_(already_AddRefed<ImageContainer>)
RasterImage::GetImageContainer(LayerManager* aManager, uint32_t aFlags)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aManager);
  MOZ_ASSERT((aFlags & ~(FLAG_SYNC_DECODE |
                         FLAG_SYNC_DECODE_IF_FAST |
                         FLAG_ASYNC_NOTIFY))
               == FLAG_NONE,
             "Unsupported flag passed to GetImageContainer");

  int32_t maxTextureSize = aManager->GetMaxTextureSize();
  if (!mHasSize ||
      mSize.width > maxTextureSize ||
      mSize.height > maxTextureSize) {
    return nullptr;
  }

  if (IsUnlocked() && mProgressTracker) {
    mProgressTracker->OnUnlockedDraw();
  }

  nsRefPtr<layers::ImageContainer> container = mImageContainer.get();

  bool mustRedecode =
    (aFlags & (FLAG_SYNC_DECODE | FLAG_SYNC_DECODE_IF_FAST)) &&
    mLastImageContainerDrawResult != DrawResult::SUCCESS &&
    mLastImageContainerDrawResult != DrawResult::BAD_IMAGE;

  if (container && !mustRedecode) {
    return container.forget();
  }

  
  container = LayerManager::CreateImageContainer();

  auto result = GetCurrentImage(container, aFlags);
  if (!result.second()) {
    
    return nullptr;
  }

  
  
  
  container->SetCurrentImageInTransaction(result.second());

  mLastImageContainerDrawResult = result.first();
  mImageContainer = container;

  return container.forget();
}

void
RasterImage::UpdateImageContainer()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsRefPtr<layers::ImageContainer> container = mImageContainer.get();
  if (!container) {
    return;
  }

  auto result = GetCurrentImage(container, FLAG_NONE);
  if (!result.second()) {
    
    return;
  }

  mLastImageContainerDrawResult = result.first();
  container->SetCurrentImage(result.second());
}

size_t
RasterImage::SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const
{
  return mSourceBuffer->SizeOfIncludingThisWithComputedFallback(aMallocSizeOf);
}

void
RasterImage::CollectSizeOfSurfaces(nsTArray<SurfaceMemoryCounter>& aCounters,
                                   MallocSizeOf aMallocSizeOf) const
{
  SurfaceCache::CollectSizeOfSurfaces(ImageKey(this), aCounters, aMallocSizeOf);
  if (mAnim) {
    mAnim->CollectSizeOfCompositingSurfaces(aCounters, aMallocSizeOf);
  }
}

class OnAddedFrameRunnable : public nsRunnable
{
public:
  OnAddedFrameRunnable(RasterImage* aImage,
                       uint32_t aNewFrameCount,
                       const IntRect& aNewRefreshArea)
    : mImage(aImage)
    , mNewFrameCount(aNewFrameCount)
    , mNewRefreshArea(aNewRefreshArea)
  {
    MOZ_ASSERT(aImage);
  }

  NS_IMETHOD Run()
  {
    mImage->OnAddedFrame(mNewFrameCount, mNewRefreshArea);
    return NS_OK;
  }

private:
  nsRefPtr<RasterImage> mImage;
  uint32_t mNewFrameCount;
  IntRect mNewRefreshArea;
};

void
RasterImage::OnAddedFrame(uint32_t aNewFrameCount,
                          const IntRect& aNewRefreshArea)
{
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> runnable =
      new OnAddedFrameRunnable(this, aNewFrameCount, aNewRefreshArea);
    NS_DispatchToMainThread(runnable);
    return;
  }

  MOZ_ASSERT((mFrameCount == 1 && aNewFrameCount == 1) ||
             mFrameCount < aNewFrameCount,
             "Frame count running backwards");

  if (aNewFrameCount > mFrameCount) {
    mFrameCount = aNewFrameCount;

    if (aNewFrameCount == 2) {
      
      MOZ_ASSERT(!mAnim, "Already have animation state?");
      mAnim = MakeUnique<FrameAnimator>(this, mSize, mAnimationMode);

      
      
      
      
      
      
      
      
      
      LockImage();

      if (mPendingAnimation && ShouldAnimate()) {
        StartAnimation();
      }
    }
    if (aNewFrameCount > 1) {
      mAnim->UnionFirstFrameRefreshArea(aNewRefreshArea);
    }
  }
}

nsresult
RasterImage::SetSize(int32_t aWidth, int32_t aHeight, Orientation aOrientation)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mError) {
    return NS_ERROR_FAILURE;
  }

  
  
  if ((aWidth < 0) || (aHeight < 0)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (mHasSize &&
      ((aWidth != mSize.width) ||
       (aHeight != mSize.height) ||
       (aOrientation != mOrientation))) {
    NS_WARNING("Image changed size on redecode! This should not happen!");
    DoError();
    return NS_ERROR_UNEXPECTED;
  }

  
  mSize.SizeTo(aWidth, aHeight);
  mOrientation = aOrientation;
  mHasSize = true;

  return NS_OK;
}

void
RasterImage::OnDecodingComplete()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mError) {
    return;
  }

  
  mHasBeenDecoded = true;

  
  if (mAnim) {
    mAnim->SetDoneDecoding(true);
  }
}

NS_IMETHODIMP
RasterImage::SetAnimationMode(uint16_t aAnimationMode)
{
  if (mAnim) {
    mAnim->SetAnimationMode(aAnimationMode);
  }
  return SetAnimationModeInternal(aAnimationMode);
}



nsresult
RasterImage::StartAnimation()
{
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(ShouldAnimate(), "Should not animate!");

  
  
  
  mPendingAnimation = !mAnim;
  if (mPendingAnimation) {
    return NS_OK;
  }

  
  if (mAnim->GetTimeoutForFrame(GetCurrentFrameIndex()) < 0) {
    mAnimationFinished = true;
    return NS_ERROR_ABORT;
  }

  
  
  mAnim->InitAnimationFrameTimeIfNecessary();

  return NS_OK;
}



nsresult
RasterImage::StopAnimation()
{
  MOZ_ASSERT(mAnimating, "Should be animating!");

  nsresult rv = NS_OK;
  if (mError) {
    rv = NS_ERROR_FAILURE;
  } else {
    mAnim->SetAnimationFrameTime(TimeStamp());
  }

  mAnimating = false;
  return rv;
}



NS_IMETHODIMP
RasterImage::ResetAnimation()
{
  if (mError) {
    return NS_ERROR_FAILURE;
    }

  mPendingAnimation = false;

  if (mAnimationMode == kDontAnimMode || !mAnim ||
      mAnim->GetCurrentAnimationFrameIndex() == 0) {
    return NS_OK;
  }

  mAnimationFinished = false;

  if (mAnimating) {
    StopAnimation();
  }

  MOZ_ASSERT(mAnim, "Should have a FrameAnimator");
  mAnim->ResetAnimation();

  NotifyProgress(NoProgress, mAnim->GetFirstFrameRefreshArea());

  
  
  EvaluateAnimation();

  return NS_OK;
}



NS_IMETHODIMP_(void)
RasterImage::SetAnimationStartTime(const TimeStamp& aTime)
{
  if (mError || mAnimationMode == kDontAnimMode || mAnimating || !mAnim) {
    return;
  }

  mAnim->SetAnimationFrameTime(aTime);
}

NS_IMETHODIMP_(float)
RasterImage::GetFrameIndex(uint32_t aWhichFrame)
{
  MOZ_ASSERT(aWhichFrame <= FRAME_MAX_VALUE, "Invalid argument");
  return (aWhichFrame == FRAME_FIRST || !mAnim)
         ? 0.0f
         : mAnim->GetCurrentAnimationFrameIndex();
}

void
RasterImage::SetLoopCount(int32_t aLoopCount)
{
  if (mError) {
    return;
  }

  
  if (mAnim) {
    mAnim->SetLoopCount(aLoopCount);
  }
}

NS_IMETHODIMP_(IntRect)
RasterImage::GetImageSpaceInvalidationRect(const IntRect& aRect)
{
  return aRect;
}

nsresult
RasterImage::OnImageDataComplete(nsIRequest*, nsISupports*, nsresult aStatus,
                                 bool aLastPart)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  mHasSourceData = true;

  
  mSourceBuffer->Complete(aStatus);

  if (!mHasSize) {
    
    
    
    Decode(Nothing(), FLAG_SYNC_DECODE);
  }

  
  
  nsresult finalStatus = mError ? NS_ERROR_FAILURE : NS_OK;
  if (NS_FAILED(aStatus)) {
    finalStatus = aStatus;
  }

  
  if (NS_FAILED(finalStatus)) {
    DoError();
  }

  MOZ_ASSERT(mHasSize || mError, "Need to know size before firing load event");
  MOZ_ASSERT(!mHasSize ||
             (mProgressTracker->GetProgress() & FLAG_SIZE_AVAILABLE),
             "Should have notified that the size is available if we have it");

  Progress loadProgress = LoadCompleteProgress(aLastPart, mError, finalStatus);

  if (mDecodeOnlyOnDraw) {
    
    
    
    
    
    loadProgress |= FLAG_ONLOAD_BLOCKED |
                    FLAG_DECODE_STARTED |
                    FLAG_FRAME_COMPLETE |
                    FLAG_DECODE_COMPLETE |
                    FLAG_ONLOAD_UNBLOCKED;
  }

  
  NotifyProgress(loadProgress);

  return finalStatus;
}

void
RasterImage::NotifyForDecodeOnlyOnDraw()
{
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &RasterImage::NotifyForDecodeOnlyOnDraw);
    NS_DispatchToMainThread(runnable);
    return;
  }

  NotifyProgress(FLAG_DECODE_STARTED | FLAG_ONLOAD_BLOCKED);
}

nsresult
RasterImage::OnImageDataAvailable(nsIRequest*,
                                  nsISupports*,
                                  nsIInputStream* aInStr,
                                  uint64_t aOffset,
                                  uint32_t aCount)
{
  nsresult rv;

  if (MOZ_UNLIKELY(mDecodeOnlyOnDraw && aOffset == 0)) {
    
    
    NotifyForDecodeOnlyOnDraw();
  }

  
  
  uint32_t bytesRead;
  rv = aInStr->ReadSegments(WriteToSourceBuffer, this, aCount, &bytesRead);

  MOZ_ASSERT(bytesRead == aCount || HasError() || NS_FAILED(rv),
    "WriteToSourceBuffer should consume everything if ReadSegments succeeds or "
    "the image must be in error!");

  return rv;
}

nsresult
RasterImage::SetSourceSizeHint(uint32_t aSizeHint)
{
  return mSourceBuffer->ExpectLength(aSizeHint);
}


NS_IMETHODIMP
RasterImage::Get(const char* prop, const nsIID& iid, void** result)
{
  if (!mProperties) {
    return NS_ERROR_FAILURE;
  }
  return mProperties->Get(prop, iid, result);
}

NS_IMETHODIMP
RasterImage::Set(const char* prop, nsISupports* value)
{
  if (!mProperties) {
    mProperties = do_CreateInstance("@mozilla.org/properties;1");
  }
  if (!mProperties) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return mProperties->Set(prop, value);
}

NS_IMETHODIMP
RasterImage::Has(const char* prop, bool* _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mProperties) {
    *_retval = false;
    return NS_OK;
  }
  return mProperties->Has(prop, _retval);
}

NS_IMETHODIMP
RasterImage::Undefine(const char* prop)
{
  if (!mProperties) {
    return NS_ERROR_FAILURE;
  }
  return mProperties->Undefine(prop);
}

NS_IMETHODIMP
RasterImage::GetKeys(uint32_t* count, char*** keys)
{
  if (!mProperties) {
    *count = 0;
    *keys = nullptr;
    return NS_OK;
  }
  return mProperties->GetKeys(count, keys);
}

void
RasterImage::Discard()
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(CanDiscard(), "Asked to discard but can't");
  MOZ_ASSERT(!mAnim, "Asked to discard for animated image");

  
  SurfaceCache::RemoveImage(ImageKey(this));

  
  if (mProgressTracker) {
    mProgressTracker->OnDiscard();
  }
}

bool
RasterImage::CanDiscard() {
  return mHasSourceData &&       
         !mAnim;                 
}


already_AddRefed<Decoder>
RasterImage::CreateDecoder(const Maybe<IntSize>& aSize, uint32_t aFlags)
{
  
  if (aSize) {
    MOZ_ASSERT(mHasSize, "Must do a size decode before a full decode!");
    MOZ_ASSERT(mDownscaleDuringDecode || *aSize == mSize,
               "Can only decode to our intrinsic size if we're not allowed to "
               "downscale-during-decode");
  } else {
    MOZ_ASSERT(!mHasSize, "Should not do unnecessary size decodes");
  }

  
  eDecoderType type = GetDecoderType(mSourceDataMimeType.get());
  if (type == eDecoderType_unknown) {
    return nullptr;
  }

  
  nsRefPtr<Decoder> decoder;
  switch (type) {
    case eDecoderType_png:
      decoder = new nsPNGDecoder(this);
      break;
    case eDecoderType_gif:
      decoder = new nsGIFDecoder2(this);
      break;
    case eDecoderType_jpeg:
      
      
      decoder = new nsJPEGDecoder(this,
                                  mHasBeenDecoded ? Decoder::SEQUENTIAL :
                                                    Decoder::PROGRESSIVE);
      break;
    case eDecoderType_bmp:
      decoder = new nsBMPDecoder(this);
      break;
    case eDecoderType_ico:
      decoder = new nsICODecoder(this);
      break;
    case eDecoderType_icon:
      decoder = new nsIconDecoder(this);
      break;
    default:
      MOZ_ASSERT_UNREACHABLE("Unknown decoder type");
  }

  MOZ_ASSERT(decoder, "Should have a decoder now");

  
  decoder->SetSizeDecode(!aSize);
  decoder->SetSendPartialInvalidations(!mHasBeenDecoded);
  decoder->SetImageIsTransient(mTransient);
  decoder->SetFlags(aFlags);

  if (!mHasBeenDecoded && aSize) {
    
    
    
    LockImage();
    decoder->SetImageIsLocked();
  }

  if (aSize) {
    
    
    
    
    
    decoder->SetSize(mSize, mOrientation);
    decoder->NeedNewFrame(0, 0, 0, aSize->width, aSize->height,
                          SurfaceFormat::B8G8R8A8);
    decoder->AllocateFrame(*aSize);
  }
  decoder->SetIterator(mSourceBuffer->Iterator());

  
  if (mDownscaleDuringDecode && aSize && *aSize != mSize) {
    DebugOnly<nsresult> rv = decoder->SetTargetSize(*aSize);
    MOZ_ASSERT(nsresult(rv) != NS_ERROR_NOT_AVAILABLE,
               "We're downscale-during-decode but decoder doesn't support it?");
    MOZ_ASSERT(NS_SUCCEEDED(rv), "Bad downscale-during-decode target size?");
  }

  decoder->Init();

  if (NS_FAILED(decoder->GetDecoderError())) {
    return nullptr;
  }

  if (!aSize) {
    Telemetry::GetHistogramById(
      Telemetry::IMAGE_DECODE_COUNT)->Subtract(mDecodeCount);
    mDecodeCount++;
    Telemetry::GetHistogramById(
      Telemetry::IMAGE_DECODE_COUNT)->Add(mDecodeCount);

    if (mDecodeCount > sMaxDecodeCount) {
      
      
      if (sMaxDecodeCount > 0) {
        Telemetry::GetHistogramById(
          Telemetry::IMAGE_MAX_DECODE_COUNT)->Subtract(sMaxDecodeCount);
      }
      sMaxDecodeCount = mDecodeCount;
      Telemetry::GetHistogramById(
        Telemetry::IMAGE_MAX_DECODE_COUNT)->Add(sMaxDecodeCount);
    }
  }

  return decoder.forget();
}



NS_IMETHODIMP
RasterImage::RequestDecode()
{
  
  if (mDecodeOnlyOnDraw) {
    return NS_OK;
  }

  return RequestDecodeForSize(mSize, DECODE_FLAGS_DEFAULT);
}


NS_IMETHODIMP
RasterImage::StartDecoding()
{
  
  if (mDecodeOnlyOnDraw) {
    return NS_OK;
  }

  return RequestDecodeForSize(mSize, FLAG_SYNC_DECODE_IF_FAST);
}

NS_IMETHODIMP
RasterImage::RequestDecodeForSize(const IntSize& aSize, uint32_t aFlags)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mError) {
    return NS_ERROR_FAILURE;
  }

  if (!mHasSize) {
    mWantFullDecode = true;
    return NS_OK;
  }

  
  
  IntSize targetSize = mDownscaleDuringDecode ? aSize : mSize;

  
  
  
  bool shouldSyncDecodeIfFast =
    !mHasBeenDecoded && (aFlags & FLAG_SYNC_DECODE_IF_FAST);

  uint32_t flags = shouldSyncDecodeIfFast
                 ? aFlags
                 : aFlags & ~FLAG_SYNC_DECODE_IF_FAST;

  
  
  LookupFrame(0, targetSize, flags);

  return NS_OK;
}

NS_IMETHODIMP
RasterImage::Decode(const Maybe<IntSize>& aSize, uint32_t aFlags)
{
  MOZ_ASSERT(!aSize || NS_IsMainThread());

  if (mError) {
    return NS_ERROR_FAILURE;
  }

  
  if (!mHasSize && aSize) {
    mWantFullDecode = true;
    return NS_OK;
  }

  if (mDownscaleDuringDecode && aSize) {
    
    
    
    
    
    
    
    SurfaceCache::UnlockSurfaces(ImageKey(this));
  }

  
  nsRefPtr<Decoder> decoder = CreateDecoder(aSize, aFlags);
  if (!decoder) {
    return NS_ERROR_FAILURE;
  }

  if (aSize) {
    
    
    NotifyProgress(decoder->TakeProgress(),
                   decoder->TakeInvalidRect(),
                   decoder->GetDecodeFlags());
  }

  if (mHasSourceData) {
    
    if (aFlags & FLAG_SYNC_DECODE) {
      PROFILER_LABEL_PRINTF("DecodePool", "SyncDecodeIfPossible",
        js::ProfileEntry::Category::GRAPHICS, "%s", GetURIString().get());
      DecodePool::Singleton()->SyncDecodeIfPossible(decoder);
      return NS_OK;
    }

    if (aFlags & FLAG_SYNC_DECODE_IF_FAST) {
      PROFILER_LABEL_PRINTF("DecodePool", "SyncDecodeIfSmall",
        js::ProfileEntry::Category::GRAPHICS, "%s", GetURIString().get());
      DecodePool::Singleton()->SyncDecodeIfSmall(decoder);
      return NS_OK;
    }
  }

  
  
  DecodePool::Singleton()->AsyncDecode(decoder);
  return NS_OK;
}

void
RasterImage::RecoverFromLossOfFrames(const IntSize& aSize, uint32_t aFlags)
{
  if (!mHasSize) {
    return;
  }

  NS_WARNING("An imgFrame became invalid. Attempting to recover...");

  
  SurfaceCache::RemoveImage(ImageKey(this));

  
  
  if (mAnim) {
    Decode(Some(mSize), aFlags | FLAG_SYNC_DECODE);
    ResetAnimation();
    return;
  }

  
  Decode(Some(aSize), aFlags);
}

bool
RasterImage::CanScale(GraphicsFilter aFilter,
                      const IntSize& aSize,
                      uint32_t aFlags)
{
#ifndef MOZ_ENABLE_SKIA
  
  return false;
#else
  
  
  
  
  
  
  if (!gfxPrefs::ImageHQDownscalingEnabled() || !mHasSize || !mHasSourceData ||
      !(aFlags & imgIContainer::FLAG_HIGH_QUALITY_SCALING) ||
      aFilter != GraphicsFilter::FILTER_GOOD) {
    return false;
  }

  
  if (mDownscaleDuringDecode) {
    return false;
  }

  
  
  if (mAnim || mTransient) {
    return false;
  }

  
  if (aSize == mSize) {
    return false;
  }

  
  if (aSize.width > mSize.width || aSize.height > mSize.height) {
    uint32_t scaledSize = static_cast<uint32_t>(aSize.width * aSize.height);
    if (scaledSize > gfxPrefs::ImageHQUpscalingMaxSize()) {
      return false;
    }
  }

  
  if (!SurfaceCache::CanHold(aSize)) {
    return false;
  }

  
  
  
  
  gfx::Size scale(double(aSize.width) / mSize.width,
                  double(aSize.height) / mSize.height);
  gfxFloat minFactor = gfxPrefs::ImageHQDownscalingMinFactor() / 1000.0;
  return (scale.width < minFactor || scale.height < minFactor);
#endif
}

bool
RasterImage::CanDownscaleDuringDecode(const IntSize& aSize, uint32_t aFlags)
{
  
  
  
  if (!mDownscaleDuringDecode || !mHasSize ||
      !(aFlags & imgIContainer::FLAG_HIGH_QUALITY_SCALING)) {
    return false;
  }

  
  if (mAnim) {
    return false;
  }

  
  if (aSize.width >= mSize.width || aSize.height >= mSize.height) {
    return false;
  }

  
  if (aSize.width < 1 || aSize.height < 1) {
    return false;
  }

  
  if (!SurfaceCache::CanHold(aSize)) {
    return false;
  }

  return true;
}

void
RasterImage::NotifyNewScaledFrame()
{
  
  
  NotifyProgress(NoProgress, IntRect(0, 0, mSize.width, mSize.height));
}

void
RasterImage::RequestScale(imgFrame* aFrame,
                          uint32_t aFlags,
                          const IntSize& aSize)
{
  
  if (!aFrame->IsImageComplete()) {
    return;
  }

  
  if (aFrame->NeedsPadding() || aFrame->IsSinglePixel()) {
    return;
  }

  
  RawAccessFrameRef frameRef = aFrame->RawAccessRef();
  if (!frameRef) {
    return;
  }

  nsRefPtr<ScaleRunner> runner =
    new ScaleRunner(this, DecodeFlags(aFlags), aSize, Move(frameRef));
  if (runner->Init()) {
    if (!sScaleWorkerThread) {
      NS_NewNamedThread("Image Scaler", getter_AddRefs(sScaleWorkerThread));
      ClearOnShutdown(&sScaleWorkerThread);
    }

    sScaleWorkerThread->Dispatch(runner, NS_DISPATCH_NORMAL);
  }
}

DrawResult
RasterImage::DrawWithPreDownscaleIfNeeded(DrawableFrameRef&& aFrameRef,
                                          gfxContext* aContext,
                                          const IntSize& aSize,
                                          const ImageRegion& aRegion,
                                          GraphicsFilter aFilter,
                                          uint32_t aFlags)
{
  DrawableFrameRef frameRef;

  if (CanScale(aFilter, aSize, aFlags)) {
    frameRef =
      SurfaceCache::Lookup(ImageKey(this),
                           RasterSurfaceKey(aSize,
                                            DecodeFlags(aFlags),
                                            0));
    if (!frameRef) {
      
      
      
      RequestScale(aFrameRef.get(), aFlags, aSize);
    }
    if (frameRef && !frameRef->IsImageComplete()) {
      frameRef.reset();  
    }
  }

  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  ImageRegion region(aRegion);
  bool frameIsComplete = true;  
  if (!frameRef) {
    
    
    frameRef = Move(aFrameRef);
    frameIsComplete = frameRef->IsImageComplete();
  }

  
  
  IntSize finalSize = frameRef->GetImageSize();
  bool couldRedecodeForBetterFrame = false;
  if (finalSize != aSize) {
    gfx::Size scale(double(aSize.width) / finalSize.width,
                    double(aSize.height) / finalSize.height);
    aContext->Multiply(gfxMatrix::Scaling(scale.width, scale.height));
    region.Scale(1.0 / scale.width, 1.0 / scale.height);

    couldRedecodeForBetterFrame = mDownscaleDuringDecode &&
                                  CanDownscaleDuringDecode(aSize, aFlags);
  }

  if (!frameRef->Draw(aContext, region, aFilter, aFlags)) {
    RecoverFromLossOfFrames(aSize, aFlags);
    return DrawResult::TEMPORARY_ERROR;
  }
  if (!frameIsComplete) {
    return DrawResult::INCOMPLETE;
  }
  if (couldRedecodeForBetterFrame) {
    return DrawResult::WRONG_SIZE;
  }
  return DrawResult::SUCCESS;
}











NS_IMETHODIMP_(DrawResult)
RasterImage::Draw(gfxContext* aContext,
                  const IntSize& aSize,
                  const ImageRegion& aRegion,
                  uint32_t aWhichFrame,
                  GraphicsFilter aFilter,
                  const Maybe<SVGImageContext>& ,
                  uint32_t aFlags)
{
  if (aWhichFrame > FRAME_MAX_VALUE) {
    return DrawResult::BAD_ARGS;
  }

  if (mError) {
    return DrawResult::BAD_IMAGE;
  }

  
  
  
  if (DecodeFlags(aFlags) != DECODE_FLAGS_DEFAULT) {
    return DrawResult::BAD_ARGS;
  }

  if (!aContext) {
    return DrawResult::BAD_ARGS;
  }

  if (IsUnlocked() && mProgressTracker) {
    mProgressTracker->OnUnlockedDraw();
  }

  
  
  uint32_t flags = aFilter == GraphicsFilter::FILTER_GOOD
                 ? aFlags
                 : aFlags & ~FLAG_HIGH_QUALITY_SCALING;

  DrawableFrameRef ref =
    LookupFrame(GetRequestedFrameIndex(aWhichFrame), aSize, flags);
  if (!ref) {
    
    if (mDrawStartTime.IsNull()) {
      mDrawStartTime = TimeStamp::Now();
    }
    return DrawResult::NOT_READY;
  }

  bool shouldRecordTelemetry = !mDrawStartTime.IsNull() &&
                               ref->IsImageComplete();

  auto result = DrawWithPreDownscaleIfNeeded(Move(ref), aContext, aSize,
                                             aRegion, aFilter, flags);

  if (shouldRecordTelemetry) {
      TimeDuration drawLatency = TimeStamp::Now() - mDrawStartTime;
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_ON_DRAW_LATENCY,
                            int32_t(drawLatency.ToMicroseconds()));
      mDrawStartTime = TimeStamp();
  }

  return result;
}



NS_IMETHODIMP
RasterImage::LockImage()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "Main thread to encourage serialization with UnlockImage");
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  
  mLockCount++;

  
  if (mLockCount == 1) {
    SurfaceCache::LockImage(ImageKey(this));
  }

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::UnlockImage()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "Main thread to encourage serialization with LockImage");
  if (mError) {
    return NS_ERROR_FAILURE;
  }

  
  MOZ_ASSERT(mLockCount > 0,
             "Calling UnlockImage with mLockCount == 0!");
  if (mLockCount == 0) {
    return NS_ERROR_ABORT;
  }

  
  mLockCount--;

  
  if (mLockCount == 0 ) {
    SurfaceCache::UnlockImage(ImageKey(this));
  }

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::RequestDiscard()
{
  if (mDiscardable &&      
      mLockCount == 0 &&   
      CanDiscard()) {
    Discard();
  }

  return NS_OK;
}


void
RasterImage::DoError()
{
  
  if (mError) {
    return;
  }

  
  
  if (!NS_IsMainThread()) {
    HandleErrorWorker::DispatchIfNeeded(this);
    return;
  }

  
  mError = true;

  
  LOG_CONTAINER_ERROR;
}

 void
RasterImage::HandleErrorWorker::DispatchIfNeeded(RasterImage* aImage)
{
  nsRefPtr<HandleErrorWorker> worker = new HandleErrorWorker(aImage);
  NS_DispatchToMainThread(worker);
}

RasterImage::HandleErrorWorker::HandleErrorWorker(RasterImage* aImage)
  : mImage(aImage)
{
  MOZ_ASSERT(mImage, "Should have image");
}

NS_IMETHODIMP
RasterImage::HandleErrorWorker::Run()
{
  mImage->DoError();

  return NS_OK;
}




NS_METHOD
RasterImage::WriteToSourceBuffer(nsIInputStream* ,
                                 void*          aClosure,
                                 const char*    aFromRawSegment,
                                 uint32_t       ,
                                 uint32_t       aCount,
                                 uint32_t*      aWriteCount)
{
  
  RasterImage* image = static_cast<RasterImage*>(aClosure);

  
  
  
  
  nsresult rv = image->mSourceBuffer->Append(aFromRawSegment, aCount);
  if (rv == NS_ERROR_OUT_OF_MEMORY) {
    image->DoError();
    return rv;
  }

  
  *aWriteCount = aCount;

  return NS_OK;
}

bool
RasterImage::ShouldAnimate()
{
  return ImageResource::ShouldAnimate() && GetNumFrames() >= 2 &&
         !mAnimationFinished;
}


#ifdef DEBUG
NS_IMETHODIMP
RasterImage::GetFramesNotified(uint32_t* aFramesNotified)
{
  NS_ENSURE_ARG_POINTER(aFramesNotified);

  *aFramesNotified = mFramesNotified;

  return NS_OK;
}
#endif

void
RasterImage::NotifyProgress(Progress aProgress,
                            const IntRect& aInvalidRect ,
                            uint32_t aFlags )
{
  MOZ_ASSERT(NS_IsMainThread());

  
  nsRefPtr<RasterImage> image(this);

  bool wasDefaultFlags = aFlags == DECODE_FLAGS_DEFAULT;

  if (!aInvalidRect.IsEmpty() && wasDefaultFlags) {
    
    UpdateImageContainer();
  }

  
  image->mProgressTracker->SyncNotifyProgress(aProgress, aInvalidRect);
}

void
RasterImage::FinalizeDecoder(Decoder* aDecoder)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(aDecoder);
  MOZ_ASSERT(mError || mHasSize || !aDecoder->HasSize(),
             "Should have handed off size by now");

  
  NotifyProgress(aDecoder->TakeProgress(),
                 aDecoder->TakeInvalidRect(),
                 aDecoder->GetDecodeFlags());

  bool wasSize = aDecoder->IsSizeDecode();
  bool done = aDecoder->GetDecodeDone();

  if (!wasSize && aDecoder->ChunkCount()) {
    Telemetry::Accumulate(Telemetry::IMAGE_DECODE_CHUNKS,
                          aDecoder->ChunkCount());
  }

  if (done) {
    
    if (!wasSize) {
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_TIME,
                            int32_t(aDecoder->DecodeTime().ToMicroseconds()));

      
      
      Telemetry::ID id = aDecoder->SpeedHistogram();
      if (id < Telemetry::HistogramCount) {
        int32_t KBps = int32_t(aDecoder->BytesDecoded() /
                               (1024 * aDecoder->DecodeTime().ToSeconds()));
        Telemetry::Accumulate(id, KBps);
      }
    }

    
    if (aDecoder->HasError() && !aDecoder->WasAborted()) {
      DoError();
    } else if (wasSize && !mHasSize) {
      DoError();
    }
  }

  if (aDecoder->ImageIsLocked()) {
    
    UnlockImage();
  }

  
  if (done && wasSize && mWantFullDecode) {
    mWantFullDecode = false;
    RequestDecode();
  }
}

already_AddRefed<imgIContainer>
RasterImage::Unwrap()
{
  nsCOMPtr<imgIContainer> self(this);
  return self.forget();
}

IntSize
RasterImage::OptimalImageSizeForDest(const gfxSize& aDest, uint32_t aWhichFrame,
                                     GraphicsFilter aFilter, uint32_t aFlags)
{
  MOZ_ASSERT(aDest.width >= 0 || ceil(aDest.width) <= INT32_MAX ||
             aDest.height >= 0 || ceil(aDest.height) <= INT32_MAX,
             "Unexpected destination size");

  if (mSize.IsEmpty() || aDest.IsEmpty()) {
    return IntSize(0, 0);
  }

  IntSize destSize(ceil(aDest.width), ceil(aDest.height));

  if (aFilter == GraphicsFilter::FILTER_GOOD &&
      CanDownscaleDuringDecode(destSize, aFlags)) {
    return destSize;
  } else if (CanScale(aFilter, destSize, aFlags)) {
    DrawableFrameRef frameRef =
      SurfaceCache::Lookup(ImageKey(this),
                           RasterSurfaceKey(destSize,
                                            DecodeFlags(aFlags),
                                            0));

    if (frameRef && frameRef->IsImageComplete()) {
        return destSize;  
    }
    if (!frameRef) {
      
      frameRef = LookupFrame(GetRequestedFrameIndex(aWhichFrame),
                             mSize, aFlags);
      if (frameRef) {
        RequestScale(frameRef.get(), aFlags, destSize);
      }
    }
  }

  
  
  return mSize;
}

} 
} 
