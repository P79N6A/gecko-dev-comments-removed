





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


#define DECODE_FLAGS_MASK (imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA | imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION)
#define DECODE_FLAGS_DEFAULT 0

static uint32_t
DecodeFlags(uint32_t aFlags)
{
  return aFlags & DECODE_FLAGS_MASK;
}


#if defined(PR_LOGGING)
static PRLogModuleInfo *
GetCompressedImageAccountingLog()
{
  static PRLogModuleInfo *sLog;
  if (!sLog)
    sLog = PR_NewLogModule("CompressedImageAccounting");
  return sLog;
}
#else
#define GetCompressedImageAccountingLog()
#endif



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
              const nsIntSize& aSize,
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
                         RasterSurfaceKey(mDstSize.ToIntSize(), mImageFlags, 0),
                         Lifetime::Transient);

    return true;
  }

  NS_IMETHOD Run() MOZ_OVERRIDE
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
                                  RasterSurfaceKey(mDstSize.ToIntSize(),
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
  const nsIntSize      mDstSize;
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


RasterImage::RasterImage(ProgressTracker* aProgressTracker,
                         ImageURL* aURI ) :
  ImageResource(aURI), 
  mSize(0,0),
  mFrameDecodeFlags(DECODE_FLAGS_DEFAULT),
  mLockCount(0),
  mDecodeCount(0),
  mRequestedSampleSize(0),
#ifdef DEBUG
  mFramesNotified(0),
#endif
  mDecodingMonitor("RasterImage Decoding Monitor"),
  mDecoder(nullptr),
  mDecodeStatus(DecodeStatus::INACTIVE),
  mFrameCount(0),
  mNotifyProgress(NoProgress),
  mNotifying(false),
  mHasSize(false),
  mDecodeOnDraw(false),
  mTransient(false),
  mDiscardable(false),
  mHasSourceData(false),
  mDecoded(false),
  mHasBeenDecoded(false),
  mPendingAnimation(false),
  mAnimationFinished(false),
  mWantFullDecode(false),
  mPendingError(false)
{
  mProgressTrackerInit = new ProgressTrackerInit(this, aProgressTracker);

  Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Add(0);
}


RasterImage::~RasterImage()
{
  if (mDecoder) {
    
    
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);
    DecodePool::StopDecoding(this);
    mDecoder = nullptr;
  }

  
  SurfaceCache::RemoveImage(ImageKey(this));

  mAnim = nullptr;
}

 void
RasterImage::Initialize()
{
  
  
  DecodePool::Singleton();
}

nsresult
RasterImage::Init(const char* aMimeType,
                  uint32_t aFlags)
{
  
  if (mInitialized)
    return NS_ERROR_ILLEGAL_VALUE;

  
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aMimeType);

  
  
  MOZ_ASSERT(!(aFlags & INIT_FLAG_TRANSIENT) ||
               (!(aFlags & INIT_FLAG_DISCARDABLE) &&
                !(aFlags & INIT_FLAG_DECODE_ON_DRAW)),
             "Transient images can't be discardable or decode-on-draw");

  
  mSourceDataMimeType.Assign(aMimeType);
  mDiscardable = !!(aFlags & INIT_FLAG_DISCARDABLE);
  mDecodeOnDraw = !!(aFlags & INIT_FLAG_DECODE_ON_DRAW);
  mTransient = !!(aFlags & INIT_FLAG_TRANSIENT);

  
  if (!mDiscardable) {
    SurfaceCache::LockImage(ImageKey(this));
  }

  
  nsresult rv = InitDecoder( true);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  
  if (!StoringSourceData()) {
    mWantFullDecode = true;
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

    UpdateImageContainer();

    if (mProgressTracker) {
      mProgressTracker->SyncNotifyProgress(NoProgress, res.dirtyRect);
    }
  }

  if (res.animationFinished) {
    mAnimationFinished = true;
    EvaluateAnimation();
  }
}



NS_IMETHODIMP
RasterImage::GetWidth(int32_t *aWidth)
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
RasterImage::GetHeight(int32_t *aHeight)
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
  if (mError)
    return NS_ERROR_FAILURE;

  *aSize = nsSize(nsPresContext::CSSPixelsToAppUnits(mSize.width),
                  nsPresContext::CSSPixelsToAppUnits(mSize.height));
  return NS_OK;
}



NS_IMETHODIMP
RasterImage::GetIntrinsicRatio(nsSize* aRatio)
{
  if (mError)
    return NS_ERROR_FAILURE;

  *aRatio = nsSize(mSize.width, mSize.height);
  return NS_OK;
}

NS_IMETHODIMP_(Orientation)
RasterImage::GetOrientation()
{
  return mOrientation;
}



NS_IMETHODIMP
RasterImage::GetType(uint16_t *aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = GetType();
  return NS_OK;
}



NS_IMETHODIMP_(uint16_t)
RasterImage::GetType()
{
  return imgIContainer::TYPE_RASTER;
}

DrawableFrameRef
RasterImage::LookupFrameInternal(uint32_t aFrameNum,
                                 const nsIntSize& aSize,
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

  return SurfaceCache::Lookup(ImageKey(this),
                              RasterSurfaceKey(aSize.ToIntSize(),
                                               DecodeFlags(aFlags),
                                               aFrameNum));
}

DrawableFrameRef
RasterImage::LookupFrame(uint32_t aFrameNum,
                         const nsIntSize& aSize,
                         uint32_t aFlags,
                         bool aShouldSyncNotify )
{
  MOZ_ASSERT(NS_IsMainThread());

  DrawableFrameRef ref = LookupFrameInternal(aFrameNum, aSize, aFlags);

  if (!ref && IsOpaque() && aFrameNum == 0) {
    
    
    
    ref = LookupFrameInternal(aFrameNum, aSize,
                              aFlags ^ FLAG_DECODE_NO_PREMULTIPLY_ALPHA);
  }

  if (!ref) {
    
    MOZ_ASSERT(!mAnim, "Animated frames should be locked");

    
    mFrameDecodeFlags = aFlags & DECODE_FLAGS_MASK;
    mDecoded = false;
    mFrameCount = 0;
    WantDecodedFrames(aFlags, aShouldSyncNotify);

    
    
    ref = LookupFrameInternal(aFrameNum, aSize, aFlags);

    if (!ref) {
      
      return DrawableFrameRef();
    }
  }

  if (ref->GetCompositingFailed()) {
    return DrawableFrameRef();
  }

  MOZ_ASSERT(!ref || !ref->GetIsPaletted(), "Should not have paletted frame");

  
  
  
  if (ref && mHasSourceData && aShouldSyncNotify &&
      (aFlags & FLAG_SYNC_DECODE)) {
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

nsIntRect
RasterImage::GetFirstFrameRect()
{
  if (mAnim) {
    return mAnim->GetFirstFrameRefreshArea();
  }

  
  return nsIntRect(nsIntPoint(0,0), mSize);
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
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> runnable =
      NS_NewRunnableMethod(this, &RasterImage::OnSurfaceDiscarded);
    NS_DispatchToMainThread(runnable);
    return;
  }

  if (mProgressTracker) {
    mProgressTracker->OnDiscard();
  }
}



NS_IMETHODIMP
RasterImage::GetAnimated(bool *aAnimated)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aAnimated);

  
  if (mAnim) {
    *aAnimated = true;
    return NS_OK;
  }

  
  
  if (!mHasBeenDecoded)
    return NS_ERROR_NOT_AVAILABLE;

  
  *aAnimated = false;

  return NS_OK;
}



NS_IMETHODIMP_(int32_t)
RasterImage::GetFirstFrameDelay()
{
  if (mError)
    return -1;

  bool animated = false;
  if (NS_FAILED(GetAnimated(&animated)) || !animated)
    return -1;

  MOZ_ASSERT(mAnim, "Animated images should have a FrameAnimator");
  return mAnim->GetTimeoutForFrame(0);
}

TemporaryRef<SourceSurface>
RasterImage::CopyFrame(uint32_t aWhichFrame,
                       uint32_t aFlags,
                       bool aShouldSyncNotify )
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return nullptr;

  if (mError)
    return nullptr;

  
  
  
  DrawableFrameRef frameRef = LookupFrame(GetRequestedFrameIndex(aWhichFrame),
                                          mSize, aFlags, aShouldSyncNotify);
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

  nsIntRect intFrameRect = frameRef->GetRect();
  Rect rect(intFrameRect.x, intFrameRect.y,
            intFrameRect.width, intFrameRect.height);
  if (frameRef->IsSinglePixel()) {
    target->FillRect(rect, ColorPattern(frameRef->SinglePixelColor()),
                     DrawOptions(1.0f, CompositionOp::OP_SOURCE));
  } else {
    RefPtr<SourceSurface> srcSurf = frameRef->GetSurface();
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
  return GetFrameInternal(aWhichFrame, aFlags);
}

TemporaryRef<SourceSurface>
RasterImage::GetFrameInternal(uint32_t aWhichFrame,
                              uint32_t aFlags,
                              bool aShouldSyncNotify )
{
  MOZ_ASSERT(aWhichFrame <= FRAME_MAX_VALUE);

  if (aWhichFrame > FRAME_MAX_VALUE)
    return nullptr;

  if (mError)
    return nullptr;

  
  
  
  DrawableFrameRef frameRef = LookupFrame(GetRequestedFrameIndex(aWhichFrame),
                                          mSize, aFlags, aShouldSyncNotify);
  if (!frameRef) {
    
    return nullptr;
  }

  
  
  RefPtr<SourceSurface> frameSurf;
  nsIntRect frameRect = frameRef->GetRect();
  if (frameRect.x == 0 && frameRect.y == 0 &&
      frameRect.width == mSize.width &&
      frameRect.height == mSize.height) {
    frameSurf = frameRef->GetSurface();
  }

  
  
  if (!frameSurf) {
    frameSurf = CopyFrame(aWhichFrame, aFlags, aShouldSyncNotify);
  }

  return frameSurf;
}

already_AddRefed<layers::Image>
RasterImage::GetCurrentImage()
{
  RefPtr<SourceSurface> surface =
    GetFrameInternal(FRAME_CURRENT, FLAG_NONE,  false);
  if (!surface) {
    
    
    return nullptr;
  }

  if (!mImageContainer) {
    mImageContainer = LayerManager::CreateImageContainer();
  }

  CairoImage::Data cairoData;
  GetWidth(&cairoData.mSize.width);
  GetHeight(&cairoData.mSize.height);
  cairoData.mSourceSurface = surface;

  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(ImageFormat::CAIRO_SURFACE);
  NS_ASSERTION(image, "Failed to create Image");

  static_cast<CairoImage*>(image.get())->SetData(cairoData);

  return image.forget();
}


NS_IMETHODIMP
RasterImage::GetImageContainer(LayerManager* aManager, ImageContainer **_retval)
{
  int32_t maxTextureSize = aManager->GetMaxTextureSize();
  if (mSize.width > maxTextureSize || mSize.height > maxTextureSize) {
    *_retval = nullptr;
    return NS_OK;
  }

  if (IsUnlocked() && mProgressTracker) {
    mProgressTracker->OnUnlockedDraw();
  }

  if (!mImageContainer) {
    mImageContainer = mImageContainerCache;
  }

  if (mImageContainer) {
    *_retval = mImageContainer;
    NS_ADDREF(*_retval);
    return NS_OK;
  }

  nsRefPtr<layers::Image> image = GetCurrentImage();
  if (!image) {
    return NS_ERROR_NOT_AVAILABLE;
  }
  mImageContainer->SetCurrentImageInTransaction(image);

  *_retval = mImageContainer;
  NS_ADDREF(*_retval);
  
  
  if (CanDiscard()) {
    mImageContainerCache = mImageContainer;
    mImageContainer = nullptr;
  }

  return NS_OK;
}

void
RasterImage::UpdateImageContainer()
{
  if (!mImageContainer) {
    return;
  }

  nsRefPtr<layers::Image> image = GetCurrentImage();
  if (!image) {
    return;
  }

  mImageContainer->SetCurrentImage(image);
}

size_t
RasterImage::SizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  
  size_t n = mSourceData.SizeOfExcludingThis(aMallocSizeOf);
  if (n == 0) {
    n = mSourceData.Length();
  }
  return n;
}

size_t
RasterImage::SizeOfDecoded(gfxMemoryLocation aLocation,
                           MallocSizeOf aMallocSizeOf) const
{
  size_t n = 0;
  n += SurfaceCache::SizeOfSurfaces(ImageKey(this), aLocation, aMallocSizeOf);
  if (mAnim) {
    n += mAnim->SizeOfCompositingFrames(aLocation, aMallocSizeOf);
  }
  return n;
}

class OnAddedFrameRunnable : public nsRunnable
{
public:
  OnAddedFrameRunnable(RasterImage* aImage,
                       uint32_t aNewFrameCount,
                       const nsIntRect& aNewRefreshArea)
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
  nsIntRect mNewRefreshArea;
};

void
RasterImage::OnAddedFrame(uint32_t aNewFrameCount,
                          const nsIntRect& aNewRefreshArea)
{
  if (!NS_IsMainThread()) {
    nsCOMPtr<nsIRunnable> runnable =
      new OnAddedFrameRunnable(this, aNewFrameCount, aNewRefreshArea);
    NS_DispatchToMainThread(runnable);
    return;
  }

  MOZ_ASSERT(aNewFrameCount <= mFrameCount ||
             aNewFrameCount == mFrameCount + 1,
             "Skipped a frame?");

  mFrameCount = aNewFrameCount;

  if (aNewFrameCount == 2) {
    
    MOZ_ASSERT(!mAnim, "Already have animation state?");
    mAnim = MakeUnique<FrameAnimator>(this, mSize.ToIntSize(), mAnimationMode);

    
    
    
    
    
    
    
    
    LockImage();

    if (mPendingAnimation && ShouldAnimate()) {
      StartAnimation();
    }
  }

  if (aNewFrameCount > 1) {
    mAnim->UnionFirstFrameRefreshArea(aNewRefreshArea);
  }
}

nsresult
RasterImage::SetSize(int32_t aWidth, int32_t aHeight, Orientation aOrientation)
{
  MOZ_ASSERT(NS_IsMainThread());
  mDecodingMonitor.AssertCurrentThreadIn();

  if (mError)
    return NS_ERROR_FAILURE;

  
  
  if ((aWidth < 0) || (aHeight < 0))
    return NS_ERROR_INVALID_ARG;

  
  if (mHasSize &&
      ((aWidth != mSize.width) ||
       (aHeight != mSize.height) ||
       (aOrientation != mOrientation))) {
    NS_WARNING("Image changed size on redecode! This should not happen!");

    
    
    if (mDecoder)
      mDecoder->PostResizeError();

    DoError();
    return NS_ERROR_UNEXPECTED;
  }

  
  mSize.SizeTo(aWidth, aHeight);
  mOrientation = aOrientation;
  mHasSize = true;

  return NS_OK;
}

void
RasterImage::DecodingComplete(imgFrame* aFinalFrame, bool aIsAnimated)
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mError) {
    return;
  }

  
  
  
  mDecoded = true;
  mHasBeenDecoded = true;

  
  
  if (!aIsAnimated && !mTransient && aFinalFrame) {
    aFinalFrame->SetOptimizable();
  }

  if (aIsAnimated) {
    if (mAnim) {
      mAnim->SetDoneDecoding(true);
    } else {
      NS_DispatchToMainThread(
        NS_NewRunnableMethod(this, &RasterImage::MarkAnimationDecoded));
    }
  }
}

void
RasterImage::MarkAnimationDecoded()
{
  NS_ASSERTION(mAnim, "No FrameAnimator in MarkAnimationDecoded - bad event order");
  if (!mAnim) {
    return;
  }

  mAnim->SetDoneDecoding(true);
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
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(ShouldAnimate(), "Should not animate!");

  
  
  
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
  NS_ABORT_IF_FALSE(mAnimating, "Should be animating!");

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
  if (mError)
    return NS_ERROR_FAILURE;

  mPendingAnimation = false;

  if (mAnimationMode == kDontAnimMode || !mAnim ||
      mAnim->GetCurrentAnimationFrameIndex() == 0) {
    return NS_OK;
  }

  mAnimationFinished = false;

  if (mAnimating)
    StopAnimation();

  MOZ_ASSERT(mAnim, "Should have a FrameAnimator");
  mAnim->ResetAnimation();

  UpdateImageContainer();

  
  

  
  if (mProgressTracker) {
    nsIntRect rect = mAnim->GetFirstFrameRefreshArea();
    mProgressTracker->SyncNotifyProgress(NoProgress, rect);
  }

  
  
  EvaluateAnimation();

  return NS_OK;
}



NS_IMETHODIMP_(void)
RasterImage::SetAnimationStartTime(const TimeStamp& aTime)
{
  if (mError || mAnimationMode == kDontAnimMode || mAnimating || !mAnim)
    return;

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
  if (mError)
    return;

  
  if (mAnim) {
    mAnim->SetLoopCount(aLoopCount);
  }
}

NS_IMETHODIMP_(nsIntRect)
RasterImage::GetImageSpaceInvalidationRect(const nsIntRect& aRect)
{
  return aRect;
}

nsresult
RasterImage::AddSourceData(const char *aBuffer, uint32_t aCount)
{
  ReentrantMonitorAutoEnter lock(mDecodingMonitor);

  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aBuffer);
  nsresult rv = NS_OK;

  
  NS_ABORT_IF_FALSE(mInitialized, "Calling AddSourceData() on uninitialized "
                                  "RasterImage!");

  
  NS_ABORT_IF_FALSE(!mHasSourceData, "Calling AddSourceData() after calling "
                                     "sourceDataComplete()!");

  
  
  if (mDecoded) {
    return NS_OK;
  }

  
  
  
  if (!StoringSourceData() && mHasSize) {
    rv = WriteToDecoder(aBuffer, aCount);
    CONTAINER_ENSURE_SUCCESS(rv);

    rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  else {

    
    char *newElem = mSourceData.AppendElements(aBuffer, aCount);
    if (!newElem)
      return NS_ERROR_OUT_OF_MEMORY;

    if (mDecoder) {
      DecodePool::Singleton()->RequestDecode(this);
    }
  }

  return NS_OK;
}



static void
get_header_str (char *buf, char *data, size_t data_len)
{
  int i;
  int n;
  static char hex[] = "0123456789abcdef";

  n = data_len < 4 ? data_len : 4;

  for (i = 0; i < n; i++) {
    buf[i * 2]     = hex[(data[i] >> 4) & 0x0f];
    buf[i * 2 + 1] = hex[data[i] & 0x0f];
  }

  buf[i * 2] = 0;
}

nsresult
RasterImage::DoImageDataComplete()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mHasSourceData)
    return NS_OK;
  mHasSourceData = true;

  
  
  
  
  if (mDecoder) {
    nsresult rv = DecodePool::Singleton()->DecodeUntilSizeAvailable(this);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  {
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);

    
    mSourceData.Compact();
  }

  
  if (PR_LOG_TEST(GetCompressedImageAccountingLog(), PR_LOG_DEBUG)) {
    char buf[9];
    get_header_str(buf, mSourceData.Elements(), mSourceData.Length());
    PR_LOG (GetCompressedImageAccountingLog(), PR_LOG_DEBUG,
            ("CompressedImageAccounting: RasterImage::SourceDataComplete() - data "
             "is done for container %p (%s) - header %p is 0x%s (length %d)",
             this,
             mSourceDataMimeType.get(),
             mSourceData.Elements(),
             buf,
             mSourceData.Length()));
  }

  return NS_OK;
}

nsresult
RasterImage::OnImageDataComplete(nsIRequest*, nsISupports*, nsresult aStatus, bool aLastPart)
{
  nsresult finalStatus = DoImageDataComplete();

  
  if (NS_FAILED(aStatus))
    finalStatus = aStatus;

  
  {
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);
    FinishedSomeDecoding(ShutdownReason::DONE,
                         LoadCompleteProgress(aLastPart, mError, finalStatus));
  }

  return finalStatus;
}

nsresult
RasterImage::OnImageDataAvailable(nsIRequest*,
                                  nsISupports*,
                                  nsIInputStream* aInStr,
                                  uint64_t,
                                  uint32_t aCount)
{
  nsresult rv;

  
  
  uint32_t bytesRead;
  rv = aInStr->ReadSegments(WriteToRasterImage, this, aCount, &bytesRead);

  NS_ABORT_IF_FALSE(bytesRead == aCount || HasError() || NS_FAILED(rv),
    "WriteToRasterImage should consume everything if ReadSegments succeeds or "
    "the image must be in error!");

  return rv;
}

 already_AddRefed<nsIEventTarget>
RasterImage::GetEventTarget()
{
  return DecodePool::Singleton()->GetEventTarget();
}

nsresult
RasterImage::SetSourceSizeHint(uint32_t sizeHint)
{
  if (sizeHint && StoringSourceData())
    return mSourceData.SetCapacity(sizeHint) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}


NS_IMETHODIMP
RasterImage::Get(const char *prop, const nsIID & iid, void * *result)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Get(prop, iid, result);
}

NS_IMETHODIMP
RasterImage::Set(const char *prop, nsISupports *value)
{
  if (!mProperties)
    mProperties = do_CreateInstance("@mozilla.org/properties;1");
  if (!mProperties)
    return NS_ERROR_OUT_OF_MEMORY;
  return mProperties->Set(prop, value);
}

NS_IMETHODIMP
RasterImage::Has(const char *prop, bool *_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (!mProperties) {
    *_retval = false;
    return NS_OK;
  }
  return mProperties->Has(prop, _retval);
}

NS_IMETHODIMP
RasterImage::Undefine(const char *prop)
{
  if (!mProperties)
    return NS_ERROR_FAILURE;
  return mProperties->Undefine(prop);
}

NS_IMETHODIMP
RasterImage::GetKeys(uint32_t *count, char ***keys)
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

  
  NS_ABORT_IF_FALSE(!mDecoder, "Asked to discard with open decoder!");

  
  
  NS_ABORT_IF_FALSE(!mAnim, "Asked to discard for animated image!");

  
  SurfaceCache::RemoveImage(ImageKey(this));

  
  mDecoded = false;
  mFrameCount = 0;

  
  if (mProgressTracker) {
    mProgressTracker->OnDiscard();
  }

  mDecodeStatus = DecodeStatus::INACTIVE;
}

bool
RasterImage::CanDiscard() {
  return mHasSourceData &&       
         !mDecoder &&            
         !mAnim;                 
}



bool
RasterImage::StoringSourceData() const {
  return !mTransient;
}




nsresult
RasterImage::InitDecoder(bool aDoSizeDecode)
{
  
  NS_ABORT_IF_FALSE(!mDecoder, "Calling InitDecoder() while already decoding!");

  
  NS_ABORT_IF_FALSE(!mDecoded, "Calling InitDecoder() but already decoded!");

  
  if (!aDoSizeDecode) {
    NS_ABORT_IF_FALSE(mHasSize, "Must do a size decode before a full decode!");
  }

  
  eDecoderType type = GetDecoderType(mSourceDataMimeType.get());
  CONTAINER_ENSURE_TRUE(type != eDecoderType_unknown, NS_IMAGELIB_ERROR_NO_DECODER);

  
  switch (type) {
    case eDecoderType_png:
      mDecoder = new nsPNGDecoder(*this);
      break;
    case eDecoderType_gif:
      mDecoder = new nsGIFDecoder2(*this);
      break;
    case eDecoderType_jpeg:
      
      
      mDecoder = new nsJPEGDecoder(*this,
                                   mHasBeenDecoded ? Decoder::SEQUENTIAL :
                                                     Decoder::PROGRESSIVE);
      break;
    case eDecoderType_bmp:
      mDecoder = new nsBMPDecoder(*this);
      break;
    case eDecoderType_ico:
      mDecoder = new nsICODecoder(*this);
      break;
    case eDecoderType_icon:
      mDecoder = new nsIconDecoder(*this);
      break;
    default:
      NS_ABORT_IF_FALSE(0, "Shouldn't get here!");
  }

  
  mDecoder->SetSizeDecode(aDoSizeDecode);
  mDecoder->SetDecodeFlags(mFrameDecodeFlags);
  mDecoder->SetSendPartialInvalidations(!mHasBeenDecoded);
  mDecoder->Init();
  CONTAINER_ENSURE_SUCCESS(mDecoder->GetDecoderError());

  if (!aDoSizeDecode) {
    Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Subtract(mDecodeCount);
    mDecodeCount++;
    Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Add(mDecodeCount);

    if (mDecodeCount > sMaxDecodeCount) {
      
      
      if (sMaxDecodeCount > 0) {
        Telemetry::GetHistogramById(Telemetry::IMAGE_MAX_DECODE_COUNT)->Subtract(sMaxDecodeCount);
      }
      sMaxDecodeCount = mDecodeCount;
      Telemetry::GetHistogramById(Telemetry::IMAGE_MAX_DECODE_COUNT)->Add(sMaxDecodeCount);
    }
  }

  return NS_OK;
}










nsresult
RasterImage::ShutdownDecoder(ShutdownReason aReason)
{
  MOZ_ASSERT(NS_IsMainThread());
  mDecodingMonitor.AssertCurrentThreadIn();

  
  NS_ABORT_IF_FALSE(mDecoder, "Calling ShutdownDecoder() with no active decoder!");

  
  bool wasSizeDecode = mDecoder->IsSizeDecode();

  
  
  
  nsRefPtr<Decoder> decoder = mDecoder;
  mDecoder = nullptr;

  decoder->Finish(aReason);

  
  
  DecodePool::StopDecoding(this);

  nsresult decoderStatus = decoder->GetDecoderError();
  if (NS_FAILED(decoderStatus)) {
    DoError();
    return decoderStatus;
  }

  
  
  bool succeeded = wasSizeDecode ? mHasSize : mDecoded;
  if ((aReason == ShutdownReason::DONE) && !succeeded) {
    DoError();
    return NS_ERROR_FAILURE;
  }

  
  
  if (!wasSizeDecode && !StoringSourceData()) {
    mSourceData.Clear();
  }

  return NS_OK;
}


nsresult
RasterImage::WriteToDecoder(const char *aBuffer, uint32_t aCount)
{
  mDecodingMonitor.AssertCurrentThreadIn();

  
  NS_ABORT_IF_FALSE(mDecoder, "Trying to write to null decoder!");

  
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mDecoder->Write(aBuffer, aCount);

  CONTAINER_ENSURE_SUCCESS(mDecoder->GetDecoderError());

  return NS_OK;
}






nsresult
RasterImage::WantDecodedFrames(uint32_t aFlags, bool aShouldSyncNotify)
{
  
  if (aShouldSyncNotify) {
    
    if (aFlags & FLAG_SYNC_DECODE) {
      return SyncDecode();
    }
    return StartDecoding();
  }

  
  return RequestDecodeCore(ASYNCHRONOUS);
}



NS_IMETHODIMP
RasterImage::RequestDecode()
{
  return RequestDecodeCore(SYNCHRONOUS_NOTIFY);
}


NS_IMETHODIMP
RasterImage::StartDecoding()
{
  if (!NS_IsMainThread()) {
    return NS_DispatchToMainThread(
      NS_NewRunnableMethod(this, &RasterImage::StartDecoding));
  }
  
  
  return RequestDecodeCore(mHasBeenDecoded ?
    SYNCHRONOUS_NOTIFY : SYNCHRONOUS_NOTIFY_AND_SOME_DECODE);
}

bool
RasterImage::IsDecoded()
{
  return mDecoded || mError;
}

NS_IMETHODIMP
RasterImage::RequestDecodeCore(RequestDecodeType aDecodeType)
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mDecoded)
    return NS_OK;

  
  if (mDecoder && mDecoder->IsSizeDecode()) {
    nsresult rv = DecodePool::Singleton()->DecodeUntilSizeAvailable(this);
    CONTAINER_ENSURE_SUCCESS(rv);

    
    
    if (!mHasSize) {
      mWantFullDecode = true;
      return NS_OK;
    }
  }

  
  if (mDecodeStatus == DecodeStatus::WORK_DONE &&
      aDecodeType == SYNCHRONOUS_NOTIFY) {
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);
    nsresult rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  if (mDecoded) {
    return NS_OK;
  }

  
  
  
  if (mDecoder && !mDecoder->IsSizeDecode() && mDecoder->BytesDecoded() > 0 &&
      aDecodeType != SYNCHRONOUS_NOTIFY_AND_SOME_DECODE) {
    return NS_OK;
  }

  ReentrantMonitorAutoEnter lock(mDecodingMonitor);

  
  
  
  if (mDecoder && mDecoder->BytesDecoded() > mSourceData.Length()) {
    return NS_OK;
  }

  
  

  
  if (mDecodeStatus == DecodeStatus::WORK_DONE && aDecodeType != ASYNCHRONOUS) {
    nsresult rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  if (mDecoded) {
    return NS_OK;
  }

  
  
  if (mDecoder && !mDecoder->IsSizeDecode() && mDecoder->BytesDecoded() > 0) {
    return NS_OK;
  }

  
  
  if (mDecoder && mDecoder->GetDecodeFlags() != mFrameDecodeFlags) {
    nsresult rv = FinishedSomeDecoding(ShutdownReason::NOT_NEEDED);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);

    rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  MOZ_ASSERT(mDecoder);

  
  if (mHasSourceData && mDecoder->BytesDecoded() == mSourceData.Length()) {
    return NS_OK;
  }

  
  
  
  if (!mDecoded && mHasSourceData && aDecodeType == SYNCHRONOUS_NOTIFY_AND_SOME_DECODE) {
    PROFILER_LABEL_PRINTF("RasterImage", "DecodeABitOf",
      js::ProfileEntry::Category::GRAPHICS, "%s", GetURIString().get());

    DecodePool::Singleton()->DecodeABitOf(this);
    return NS_OK;
  }

  if (!mDecoded) {
    
    
    
    DecodePool::Singleton()->RequestDecode(this);
  }

  return NS_OK;
}


nsresult
RasterImage::SyncDecode()
{
  PROFILER_LABEL_PRINTF("RasterImage", "SyncDecode",
    js::ProfileEntry::Category::GRAPHICS, "%s", GetURIString().get());

  
  if (mDecoder && mDecoder->IsSizeDecode()) {
    nsresult rv = DecodePool::Singleton()->DecodeUntilSizeAvailable(this);
    CONTAINER_ENSURE_SUCCESS(rv);

    
    
    if (!mHasSize) {
      mWantFullDecode = true;
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  ReentrantMonitorAutoEnter lock(mDecodingMonitor);

  
  if (mDecodeStatus == DecodeStatus::WORK_DONE) {
    nsresult rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  nsresult rv;

  
  
  if (mDecoded)
    return NS_OK;

  
  
  
  if (mDecoder && mDecoder->BytesDecoded() > mSourceData.Length()) {
    return NS_OK;
  }

  
  
  if (mDecoder && mDecoder->GetDecodeFlags() != mFrameDecodeFlags) {
    nsresult rv = FinishedSomeDecoding(ShutdownReason::NOT_NEEDED);
    CONTAINER_ENSURE_SUCCESS(rv);

    if (mDecoded && mAnim) {
      
      return NS_ERROR_NOT_AVAILABLE;
    }
  }

  
  if (!mDecoder) {
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  MOZ_ASSERT(mDecoder);

  
  rv = DecodeSomeData(mSourceData.Length() - mDecoder->BytesDecoded());
  CONTAINER_ENSURE_SUCCESS(rv);

  rv = FinishedSomeDecoding();
  CONTAINER_ENSURE_SUCCESS(rv);
  
  
  if (mDecoder) {
    DecodePool::Singleton()->RequestDecode(this);
  }

  
  return mError ? NS_ERROR_FAILURE : NS_OK;
}

bool
RasterImage::CanScale(GraphicsFilter aFilter,
                      const nsIntSize& aSize,
                      uint32_t aFlags)
{
#ifndef MOZ_ENABLE_SKIA
  
  return false;
#else
  
  
  
  
  
  if (!gfxPrefs::ImageHQDownscalingEnabled() || !mDecoded ||
      !(aFlags & imgIContainer::FLAG_HIGH_QUALITY_SCALING) ||
      aFilter != GraphicsFilter::FILTER_GOOD) {
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

  
  if (!SurfaceCache::CanHold(aSize.ToIntSize())) {
    return false;
  }

  
  
  
  
  gfx::Size scale(double(aSize.width) / mSize.width,
                  double(aSize.height) / mSize.height);
  gfxFloat minFactor = gfxPrefs::ImageHQDownscalingMinFactor() / 1000.0;
  return (scale.width < minFactor || scale.height < minFactor);
#endif
}

void
RasterImage::NotifyNewScaledFrame()
{
  if (mProgressTracker) {
    
    
    nsIntRect rect(0, 0, mSize.width, mSize.height);
    mProgressTracker->SyncNotifyProgress(NoProgress, rect);
  }
}

void
RasterImage::RequestScale(imgFrame* aFrame,
                          uint32_t aFlags,
                          const nsIntSize& aSize)
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

void
RasterImage::DrawWithPreDownscaleIfNeeded(DrawableFrameRef&& aFrameRef,
                                          gfxContext* aContext,
                                          const nsIntSize& aSize,
                                          const ImageRegion& aRegion,
                                          GraphicsFilter aFilter,
                                          uint32_t aFlags)
{
  DrawableFrameRef frameRef;

  if (CanScale(aFilter, aSize, aFlags)) {
    frameRef =
      SurfaceCache::Lookup(ImageKey(this),
                           RasterSurfaceKey(aSize.ToIntSize(),
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
  if (!frameRef) {
    frameRef = Move(aFrameRef);
  }

  
  
  IntSize finalSize = frameRef->GetImageSize();
  if (ThebesIntSize(finalSize) != aSize) {
    gfx::Size scale(double(aSize.width) / finalSize.width,
                    double(aSize.height) / finalSize.height);
    aContext->Multiply(gfxMatrix::Scaling(scale.width, scale.height));
    region.Scale(1.0 / scale.width, 1.0 / scale.height);
  }

  frameRef->Draw(aContext, region, aFilter, aFlags);
}











NS_IMETHODIMP
RasterImage::Draw(gfxContext* aContext,
                  const nsIntSize& aSize,
                  const ImageRegion& aRegion,
                  uint32_t aWhichFrame,
                  GraphicsFilter aFilter,
                  const Maybe<SVGImageContext>& ,
                  uint32_t aFlags)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  
  
  if ((aFlags & DECODE_FLAGS_MASK) != DECODE_FLAGS_DEFAULT)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aContext);

  if (IsUnlocked() && mProgressTracker) {
    mProgressTracker->OnUnlockedDraw();
  }

  
  if (!mDecoded && mHasSourceData) {
    mDrawStartTime = TimeStamp::Now();
  }

  
  if (aFlags & FLAG_SYNC_DECODE) {
    nsresult rv = SyncDecode();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  
  
  
  DrawableFrameRef ref = LookupFrame(GetRequestedFrameIndex(aWhichFrame),
                                     mSize, aFlags);
  if (!ref) {
    
    return NS_OK;
  }

  DrawWithPreDownscaleIfNeeded(Move(ref), aContext, aSize,
                               aRegion, aFilter, aFlags);

  if (mDecoded && !mDrawStartTime.IsNull()) {
      TimeDuration drawLatency = TimeStamp::Now() - mDrawStartTime;
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_ON_DRAW_LATENCY, int32_t(drawLatency.ToMicroseconds()));
      
      mDrawStartTime = TimeStamp();
  }

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::LockImage()
{
  MOZ_ASSERT(NS_IsMainThread(),
             "Main thread to encourage serialization with UnlockImage");
  if (mError)
    return NS_ERROR_FAILURE;

  
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
  if (mError)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mLockCount > 0,
                    "Calling UnlockImage with mLockCount == 0!");
  if (mLockCount == 0)
    return NS_ERROR_ABORT;

  
  mLockCount--;

  
  if (mLockCount == 0 ) {
    SurfaceCache::UnlockImage(ImageKey(this));
  }

  
  
  
  
  if (mHasBeenDecoded && mDecoder && mLockCount == 0 && !mAnim) {
    PR_LOG(GetCompressedImageAccountingLog(), PR_LOG_DEBUG,
           ("RasterImage[0x%p] canceling decode because image "
            "is now unlocked.", this));
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);
    FinishedSomeDecoding(ShutdownReason::NOT_NEEDED);
    return NS_OK;
  }

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::RequestDiscard()
{
  if (mDiscardable &&      
      mLockCount == 0 &&   
      mDecoded &&          
      CanDiscard()) {
    Discard();
  }

  return NS_OK;
}


nsresult
RasterImage::DecodeSomeData(size_t aMaxBytes)
{
  MOZ_ASSERT(mDecoder, "Should have a decoder");

  mDecodingMonitor.AssertCurrentThreadIn();

  
  if (mDecoder->BytesDecoded() == mSourceData.Length()) {
    return NS_OK;
  }

  MOZ_ASSERT(mDecoder->BytesDecoded() < mSourceData.Length());

  
  size_t bytesToDecode = min(aMaxBytes,
                             mSourceData.Length() - mDecoder->BytesDecoded());
  return WriteToDecoder(mSourceData.Elements() + mDecoder->BytesDecoded(),
                        bytesToDecode);

}





bool
RasterImage::IsDecodeFinished()
{
  
  mDecodingMonitor.AssertCurrentThreadIn();
  MOZ_ASSERT(mDecoder, "Should have a decoder");

  
  if (mDecoder->IsSizeDecode()) {
    if (mDecoder->HasSize()) {
      return true;
    }
  } else if (mDecoder->GetDecodeDone()) {
    return true;
  }

  
  
  
  
  
  
  if (mHasSourceData && (mDecoder->BytesDecoded() == mSourceData.Length())) {
    return true;
  }

  
  return false;
}


void
RasterImage::DoError()
{
  
  if (mError)
    return;

  
  if (!NS_IsMainThread()) {
    HandleErrorWorker::DispatchIfNeeded(this);
    return;
  }

  
  ReentrantMonitorAutoEnter lock(mDecodingMonitor);

  
  if (mDecoder) {
    FinishedSomeDecoding(ShutdownReason::FATAL_ERROR);
  }

  
  mError = true;

  
  LOG_CONTAINER_ERROR;
}

 void
RasterImage::HandleErrorWorker::DispatchIfNeeded(RasterImage* aImage)
{
  if (!aImage->mPendingError) {
    aImage->mPendingError = true;
    nsRefPtr<HandleErrorWorker> worker = new HandleErrorWorker(aImage);
    NS_DispatchToMainThread(worker);
  }
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
RasterImage::WriteToRasterImage(nsIInputStream* ,
                                void*          aClosure,
                                const char*    aFromRawSegment,
                                uint32_t       ,
                                uint32_t       aCount,
                                uint32_t*      aWriteCount)
{
  
  RasterImage* image = static_cast<RasterImage*>(aClosure);

  
  
  
  
  nsresult rv = image->AddSourceData(aFromRawSegment, aCount);
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
RasterImage::GetFramesNotified(uint32_t *aFramesNotified)
{
  NS_ENSURE_ARG_POINTER(aFramesNotified);

  *aFramesNotified = mFramesNotified;

  return NS_OK;
}
#endif

nsresult
RasterImage::RequestDecodeIfNeeded(nsresult aStatus,
                                   ShutdownReason aReason,
                                   bool aDone,
                                   bool aWasSize)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (NS_SUCCEEDED(aStatus) &&
      aReason == ShutdownReason::DONE &&
      aDone &&
      aWasSize &&
      mWantFullDecode) {
    mWantFullDecode = false;

    
    
    
    return StoringSourceData() ? RequestDecode()
                               : SyncDecode();
  }

  
  return aStatus;
}

nsresult
RasterImage::FinishedSomeDecoding(ShutdownReason aReason ,
                                  Progress aProgress )
{
  MOZ_ASSERT(NS_IsMainThread());

  mDecodingMonitor.AssertCurrentThreadIn();

  
  
  nsRefPtr<RasterImage> image(this);

  bool done = false;
  bool wasSize = false;
  bool wasDefaultFlags = false;
  nsIntRect invalidRect;
  nsresult rv = NS_OK;
  Progress progress = aProgress;

  if (image->mDecoder) {
    invalidRect = image->mDecoder->TakeInvalidRect();
    progress |= image->mDecoder->TakeProgress();
    wasDefaultFlags = image->mDecoder->GetDecodeFlags() == DECODE_FLAGS_DEFAULT;

    if (!image->mDecoder->IsSizeDecode() && image->mDecoder->ChunkCount()) {
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_CHUNKS,
                            image->mDecoder->ChunkCount());
    }

    if (!image->mHasSize && image->mDecoder->HasSize()) {
      image->mDecoder->SetSizeOnImage();
    }

    
    
    if (image->IsDecodeFinished() || aReason != ShutdownReason::DONE) {
      done = true;

      
      nsRefPtr<Decoder> decoder = image->mDecoder;

      wasSize = decoder->IsSizeDecode();

      
      if (!wasSize) {
        Telemetry::Accumulate(Telemetry::IMAGE_DECODE_TIME,
                              int32_t(decoder->DecodeTime().ToMicroseconds()));

        
        
        Telemetry::ID id = decoder->SpeedHistogram();
        if (id < Telemetry::HistogramCount) {
          int32_t KBps = int32_t(decoder->BytesDecoded() /
                                 (1024 * decoder->DecodeTime().ToSeconds()));
          Telemetry::Accumulate(id, KBps);
        }
      }

      
      
      rv = image->ShutdownDecoder(aReason);
      if (NS_FAILED(rv)) {
        image->DoError();
      }

      
      invalidRect.Union(decoder->TakeInvalidRect());
      progress |= decoder->TakeProgress();
    }
  }

  if (!invalidRect.IsEmpty() && wasDefaultFlags) {
    
    UpdateImageContainer();
  }

  if (mNotifying) {
    
    
    
    
    mNotifyProgress |= progress;
    mNotifyInvalidRect.Union(invalidRect);
  } else {
    MOZ_ASSERT(mNotifyProgress == NoProgress && mNotifyInvalidRect.IsEmpty(),
               "Shouldn't have an accumulated change at this point");

    progress = image->mProgressTracker->Difference(progress);

    while (progress != NoProgress || !invalidRect.IsEmpty()) {
      
      mNotifying = true;
      image->mProgressTracker->SyncNotifyProgress(progress, invalidRect);
      mNotifying = false;

      
      
      
      progress = image->mProgressTracker->Difference(mNotifyProgress);
      mNotifyProgress = NoProgress;

      invalidRect = mNotifyInvalidRect;
      mNotifyInvalidRect = nsIntRect();
    }
  }

  return RequestDecodeIfNeeded(rv, aReason, done, wasSize);
}

already_AddRefed<imgIContainer>
RasterImage::Unwrap()
{
  nsCOMPtr<imgIContainer> self(this);
  return self.forget();
}

nsIntSize
RasterImage::OptimalImageSizeForDest(const gfxSize& aDest, uint32_t aWhichFrame,
                                     GraphicsFilter aFilter, uint32_t aFlags)
{
  MOZ_ASSERT(aDest.width >= 0 || ceil(aDest.width) <= INT32_MAX ||
             aDest.height >= 0 || ceil(aDest.height) <= INT32_MAX,
             "Unexpected destination size");

  if (mSize.IsEmpty() || aDest.IsEmpty()) {
    return nsIntSize(0, 0);
  }

  nsIntSize destSize(ceil(aDest.width), ceil(aDest.height));

  if (CanScale(aFilter, destSize, aFlags)) {
    DrawableFrameRef frameRef =
      SurfaceCache::Lookup(ImageKey(this),
                           RasterSurfaceKey(destSize.ToIntSize(),
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
