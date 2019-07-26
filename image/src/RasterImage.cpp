




#include "base/histogram.h"
#include "ImageLogging.h"
#include "nsComponentManagerUtils.h"
#include "imgDecoderObserver.h"
#include "nsError.h"
#include "Decoder.h"
#include "RasterImage.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsAutoPtr.h"
#include "nsStringStream.h"
#include "prenv.h"
#include "ImageContainer.h"
#include "Layers.h"
#include "nsPresContext.h"

#include "nsPNGDecoder.h"
#include "nsGIFDecoder2.h"
#include "nsJPEGDecoder.h"
#include "nsBMPDecoder.h"
#include "nsICODecoder.h"
#include "nsIconDecoder.h"

#include "gfxContext.h"

#include "mozilla/Preferences.h"
#include "mozilla/StandardInteger.h"
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/gfx/Scale.h"

#include "sampler.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::image;
using namespace mozilla::layers;


#define DECODE_FLAGS_MASK (imgIContainer::FLAG_DECODE_NO_PREMULTIPLY_ALPHA | imgIContainer::FLAG_DECODE_NO_COLORSPACE_CONVERSION)
#define DECODE_FLAGS_DEFAULT 0


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




static uint32_t gDecodeBytesAtATime = 0;
static uint32_t gMaxMSBeforeYield = 0;
static bool gHQDownscaling = false;

static uint32_t gHQDownscalingMinFactor = 1000;



static int32_t sMaxDecodeCount = 0;

static void
InitPrefCaches()
{
  Preferences::AddUintVarCache(&gDecodeBytesAtATime,
                               "image.mem.decode_bytes_at_a_time", 200000);
  Preferences::AddUintVarCache(&gMaxMSBeforeYield,
                               "image.mem.max_ms_before_yield", 400);
  Preferences::AddBoolVarCache(&gHQDownscaling,
                               "image.high_quality_downscaling.enabled", false);
  Preferences::AddUintVarCache(&gHQDownscalingMinFactor,
                               "image.high_quality_downscaling.min_factor", 1000);
}
















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



static int num_containers;
static int num_discardable_containers;
static int64_t total_source_bytes;
static int64_t discardable_source_bytes;


static bool
DiscardingEnabled()
{
  static bool inited;
  static bool enabled;

  if (!inited) {
    inited = true;

    enabled = (PR_GetEnv("MOZ_DISABLE_IMAGE_DISCARD") == nullptr);
  }

  return enabled;
}

class ScaleRequest
{
public:
  ScaleRequest(RasterImage* aImage, const gfxSize& aScale, imgFrame* aSrcFrame)
    : scale(aScale)
    , dstLocked(false)
    , done(false)
    , stopped(false)
  {
    MOZ_ASSERT(!aSrcFrame->GetIsPaletted());
    MOZ_ASSERT(aScale.width > 0 && aScale.height > 0);

    weakImage = aImage->asWeakPtr();
    srcRect = aSrcFrame->GetRect();

    nsIntRect dstRect = srcRect;
    dstRect.ScaleRoundOut(scale.width, scale.height);
    dstSize = dstRect.Size();
  }

  
  bool GetSurfaces(imgFrame* srcFrame)
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsRefPtr<RasterImage> image = weakImage.get();
    if (!image) {
      return false;
    }

    bool success = false;
    if (!dstLocked) {
      bool srcLocked = NS_SUCCEEDED(srcFrame->LockImageData());
      dstLocked = NS_SUCCEEDED(dstFrame->LockImageData());

      nsRefPtr<gfxASurface> dstASurf;
      nsRefPtr<gfxASurface> srcASurf;
      success = srcLocked && NS_SUCCEEDED(srcFrame->GetSurface(getter_AddRefs(srcASurf)));
      success = success && dstLocked && NS_SUCCEEDED(dstFrame->GetSurface(getter_AddRefs(dstASurf)));

      success = success && srcLocked && dstLocked && srcASurf && dstASurf;

      if (success) {
        srcSurface = srcASurf->GetAsImageSurface();
        dstSurface = dstASurf->GetAsImageSurface();
        srcData = srcSurface->Data();
        dstData = dstSurface->Data();
        srcStride = srcSurface->Stride();
        dstStride = dstSurface->Stride();
        srcFormat = mozilla::gfx::ImageFormatToSurfaceFormat(srcFrame->GetFormat());
      }

      
      
      
      if (srcLocked) {
        success = NS_SUCCEEDED(srcFrame->UnlockImageData()) && success;
      }

      success = success && srcSurface && dstSurface;
    }

    return success;
  }

  
  bool ReleaseSurfaces()
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsRefPtr<RasterImage> image = weakImage.get();
    if (!image) {
      return false;
    }

    bool success = false;
    if (dstLocked) {
      success = NS_SUCCEEDED(dstFrame->UnlockImageData());

      dstLocked = false;
      srcData = nullptr;
      dstData = nullptr;
      srcSurface = nullptr;
      dstSurface = nullptr;
    }
    return success;
  }

  
  WeakPtr<RasterImage> weakImage;
  nsAutoPtr<imgFrame> dstFrame;
  nsRefPtr<gfxImageSurface> srcSurface;
  nsRefPtr<gfxImageSurface> dstSurface;

  
  gfxSize scale;
  uint8_t* srcData;
  uint8_t* dstData;
  nsIntRect srcRect;
  gfxIntSize dstSize;
  uint32_t srcStride;
  uint32_t dstStride;
  mozilla::gfx::SurfaceFormat srcFormat;
  bool dstLocked;
  bool done;
  
  
  
  bool stopped;
};

class DrawRunner : public nsRunnable
{
public:
  DrawRunner(ScaleRequest* request)
   : mScaleRequest(request)
  {}

  NS_IMETHOD Run()
  {
    
    mScaleRequest->ReleaseSurfaces();

    nsRefPtr<RasterImage> image = mScaleRequest->weakImage.get();

    if (image) {
      RasterImage::ScaleStatus status;
      if (mScaleRequest->done) {
        status = RasterImage::SCALE_DONE;
      } else {
        status = RasterImage::SCALE_INVALID;
      }

      image->ScalingDone(mScaleRequest, status);
    }

    return NS_OK;
  }

private: 
  nsAutoPtr<ScaleRequest> mScaleRequest;
};

class ScaleRunner : public nsRunnable
{
public:
  ScaleRunner(RasterImage* aImage, const gfxSize& aScale, imgFrame* aSrcFrame)
  {
    nsAutoPtr<ScaleRequest> request(new ScaleRequest(aImage, aScale, aSrcFrame));

    
    
    request->dstFrame = new imgFrame();
    nsresult rv = request->dstFrame->Init(0, 0, request->dstSize.width, request->dstSize.height,
                                          gfxASurface::ImageFormatARGB32);

    if (NS_FAILED(rv) || !request->GetSurfaces(aSrcFrame)) {
      return;
    }

    aImage->ScalingStart(request);

    mScaleRequest = request;
  }

  NS_IMETHOD Run()
  {
    
    ScaleRequest* request = mScaleRequest;

    if (!request->stopped) {
      request->done = mozilla::gfx::Scale(request->srcData, request->srcRect.width, request->srcRect.height, request->srcStride,
                                          request->dstData, request->dstSize.width, request->dstSize.height, request->dstStride,
                                          request->srcFormat);
    } else {
      request->done = false;
    }

    
    
    nsRefPtr<DrawRunner> runner = new DrawRunner(mScaleRequest.forget());
    NS_DispatchToMainThread(runner, NS_DISPATCH_NORMAL);

    return NS_OK;
  }

  bool IsOK() const { return !!mScaleRequest; }

private:
  nsAutoPtr<ScaleRequest> mScaleRequest;
};

namespace mozilla {
namespace image {

 StaticRefPtr<RasterImage::DecodeWorker> RasterImage::DecodeWorker::sSingleton;
static nsCOMPtr<nsIThread> sScaleWorkerThread = nullptr;

#ifndef DEBUG
NS_IMPL_ISUPPORTS2(RasterImage, imgIContainer, nsIProperties)
#else
NS_IMPL_ISUPPORTS3(RasterImage, imgIContainer, nsIProperties,
                   imgIContainerDebug)
#endif


RasterImage::RasterImage(imgStatusTracker* aStatusTracker,
                         nsIURI* aURI ) :
  ImageResource(aStatusTracker, aURI), 
  mSize(0,0),
  mFrameDecodeFlags(DECODE_FLAGS_DEFAULT),
  mAnim(nullptr),
  mLoopCount(-1),
  mLockCount(0),
  mDecoder(nullptr),


#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable : 4355)
#endif
  mDecodeRequest(this),
#ifdef _MSC_VER
#pragma warning(pop)
#endif
  mBytesDecoded(0),
  mDecodeCount(0),
#ifdef DEBUG
  mFramesNotified(0),
#endif
  mHasSize(false),
  mDecodeOnDraw(false),
  mMultipart(false),
  mDiscardable(false),
  mHasSourceData(false),
  mDecoded(false),
  mHasBeenDecoded(false),
  mInDecoder(false),
  mAnimationFinished(false),
  mFinishing(false),
  mInUpdateImageContainer(false),
  mScaleRequest(nullptr)
{
  
  mDiscardTrackerNode.img = this;
  Telemetry::GetHistogramById(Telemetry::IMAGE_DECODE_COUNT)->Add(0);

  
  num_containers++;
}


RasterImage::~RasterImage()
{
  
  if (mDiscardable) {
    num_discardable_containers--;
    discardable_source_bytes -= mSourceData.Length();

    PR_LOG (GetCompressedImageAccountingLog(), PR_LOG_DEBUG,
            ("CompressedImageAccounting: destroying RasterImage %p.  "
             "Total Containers: %d, Discardable containers: %d, "
             "Total source bytes: %lld, Source bytes for discardable containers %lld",
             this,
             num_containers,
             num_discardable_containers,
             total_source_bytes,
             discardable_source_bytes));
  }

  if (mDecoder) {
    
    
    DecodeWorker::Singleton()->StopDecoding(this);
    mDecoder = nullptr;
  }

  delete mAnim;

  for (unsigned int i = 0; i < mFrames.Length(); ++i)
    delete mFrames[i];

  
  num_containers--;
  total_source_bytes -= mSourceData.Length();

  if (NS_IsMainThread()) {
    DiscardTracker::Remove(&mDiscardTrackerNode);
  }
}

 void
RasterImage::Initialize()
{
  InitPrefCaches();

  
  
  DecodeWorker::Singleton();
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

  
  
  NS_ABORT_IF_FALSE(!(aFlags & INIT_FLAG_MULTIPART) ||
                    (!(aFlags & INIT_FLAG_DISCARDABLE) &&
                     !(aFlags & INIT_FLAG_DECODE_ON_DRAW)),
                    "Can't be discardable or decode-on-draw for multipart");

  
  mSourceDataMimeType.Assign(aMimeType);
  mDiscardable = !!(aFlags & INIT_FLAG_DISCARDABLE);
  mDecodeOnDraw = !!(aFlags & INIT_FLAG_DECODE_ON_DRAW);
  mMultipart = !!(aFlags & INIT_FLAG_MULTIPART);

  
  if (mDiscardable) {
    num_discardable_containers++;
    discardable_source_bytes += mSourceData.Length();
  }

  
  
  
  if (mSourceDataMimeType.Length() == 0) {
    mInitialized = true;
    return NS_OK;
  }

  
  
  
  
  
  nsresult rv = InitDecoder( mDecodeOnDraw);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  mInitialized = true;

  return NS_OK;
}

bool
RasterImage::AdvanceFrame(TimeStamp aTime, nsIntRect* aDirtyRect)
{
  NS_ASSERTION(aTime <= TimeStamp::Now(),
               "Given time appears to be in the future");

  imgFrame* nextFrame = nullptr;
  uint32_t currentFrameIndex = mAnim->currentAnimationFrameIndex;
  uint32_t nextFrameIndex = mAnim->currentAnimationFrameIndex + 1;
  uint32_t timeout = 0;

  
  
  
  NS_ABORT_IF_FALSE(mDecoder || nextFrameIndex <= mFrames.Length(),
                    "How did we get 2 indices too far by incrementing?");

  
  
  
  bool haveFullNextFrame = (mMultipart && mBytesDecoded == 0) || !mDecoder ||
                           nextFrameIndex < mDecoder->GetCompleteFrameCount();

  
  
  if (haveFullNextFrame) {
    if (mFrames.Length() == nextFrameIndex) {
      

      
      if (mAnimationMode == kLoopOnceAnimMode || mLoopCount == 0) {
        mAnimationFinished = true;
        EvaluateAnimation();
      }

      
      
      if (mAnim->compositingFrame && mAnim->lastCompositedFrameIndex == -1) {
        mAnim->compositingFrame = nullptr;
      }

      nextFrameIndex = 0;

      if (mLoopCount > 0) {
        mLoopCount--;
      }

      if (!mAnimating) {
        
        return false;
      }
    }

    if (!(nextFrame = mFrames[nextFrameIndex])) {
      
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      return false;
    }

    timeout = nextFrame->GetTimeout();

  } else {
    
    
    return false;
  }

  if (!(timeout > 0)) {
    mAnimationFinished = true;
    EvaluateAnimation();
  }

  if (nextFrameIndex == 0) {
    *aDirtyRect = mAnim->firstFrameRefreshArea;
  } else {
    imgFrame *curFrame = mFrames[currentFrameIndex];

    if (!curFrame) {
      return false;
    }

    
    if (NS_FAILED(DoComposite(aDirtyRect, curFrame,
                              nextFrame, nextFrameIndex))) {
      
      NS_WARNING("RasterImage::AdvanceFrame(): Compositing of frame failed");
      nextFrame->SetCompositingFailed(true);
      mAnim->currentAnimationFrameIndex = nextFrameIndex;
      mAnim->currentAnimationFrameTime = aTime;
      return false;
    }

    nextFrame->SetCompositingFailed(false);
  }

  
  mAnim->currentAnimationFrameIndex = nextFrameIndex;
  mAnim->currentAnimationFrameTime = aTime;

  return true;
}



NS_IMETHODIMP_(void)
RasterImage::RequestRefresh(const mozilla::TimeStamp& aTime)
{
  if (!mAnimating || !ShouldAnimate()) {
    return;
  }

  EnsureAnimExists();

  
  
  TimeStamp currentFrameEndTime = GetCurrentImgFrameEndTime();
  bool frameAdvanced = false;

  
  
  nsIntRect dirtyRect;

  while (currentFrameEndTime <= aTime) {
    TimeStamp oldFrameEndTime = currentFrameEndTime;
    nsIntRect frameDirtyRect;
    bool didAdvance = AdvanceFrame(aTime, &frameDirtyRect);
    frameAdvanced = frameAdvanced || didAdvance;
    currentFrameEndTime = GetCurrentImgFrameEndTime();

    
    dirtyRect = dirtyRect.Union(frameDirtyRect);

    
    
    
    if (!frameAdvanced && (currentFrameEndTime == oldFrameEndTime)) {
      break;
    }
  }

  if (frameAdvanced) {
    
    
    
    #ifdef DEBUG
      mFramesNotified++;
    #endif

    UpdateImageContainer();
    if (mStatusTracker)
      mStatusTracker->GetDecoderObserver()->FrameChanged(&dirtyRect);
  }
}





NS_IMETHODIMP
RasterImage::ExtractFrame(uint32_t aWhichFrame,
                          const nsIntRect &aRegion,
                          uint32_t aFlags,
                          imgIContainer **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);

  nsresult rv;

  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  
  nsRefPtr<RasterImage> img(new RasterImage());

  
  
  
  img->Init("", INIT_FLAG_NONE);
  img->SetSize(aRegion.width, aRegion.height);
  img->mDecoded = true; 
  img->mHasBeenDecoded = true;
  img->mFrameDecodeFlags = aFlags & DECODE_FLAGS_MASK;

  if (!ApplyDecodeFlags(aFlags))
    return NS_ERROR_NOT_AVAILABLE;
  
  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nullptr;
    return NS_ERROR_FAILURE;
  }

  
  
  nsIntRect framerect = frame->GetRect();
  framerect.IntersectRect(framerect, aRegion);

  if (framerect.IsEmpty())
    return NS_ERROR_NOT_AVAILABLE;

  nsAutoPtr<imgFrame> subframe;
  rv = frame->Extract(framerect, getter_Transfers(subframe));
  if (NS_FAILED(rv))
    return rv;

  img->mFrames.AppendElement(subframe.forget());

  img->mStatusTracker->RecordLoaded();
  img->mStatusTracker->RecordDecoded();

  *_retval = img.forget().get();

  return NS_OK;
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

imgFrame*
RasterImage::GetImgFrameNoDecode(uint32_t framenum)
{
  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrames.SafeElementAt(0, nullptr);
  }
  if (mAnim->lastCompositedFrameIndex == int32_t(framenum))
    return mAnim->compositingFrame;
  return mFrames.SafeElementAt(framenum, nullptr);
}

imgFrame*
RasterImage::GetImgFrame(uint32_t framenum)
{
  nsresult rv = WantDecodedFrames();
  CONTAINER_ENSURE_TRUE(NS_SUCCEEDED(rv), nullptr);
  return GetImgFrameNoDecode(framenum);
}

imgFrame*
RasterImage::GetDrawableImgFrame(uint32_t framenum)
{
  imgFrame *frame = GetImgFrame(framenum);

  
  
  if (frame && frame->GetCompositingFailed())
    return nullptr;
  return frame;
}

uint32_t
RasterImage::GetCurrentImgFrameIndex() const
{
  if (mAnim)
    return mAnim->currentAnimationFrameIndex;

  return 0;
}

TimeStamp
RasterImage::GetCurrentImgFrameEndTime() const
{
  imgFrame* currentFrame = mFrames[mAnim->currentAnimationFrameIndex];
  TimeStamp currentFrameTime = mAnim->currentAnimationFrameTime;
  int64_t timeout = currentFrame->GetTimeout();

  if (timeout < 0) {
    
    
    
    
    return TimeStamp() +
           TimeDuration::FromMilliseconds(static_cast<double>(UINT64_MAX));
  }

  TimeDuration durationOfTimeout =
    TimeDuration::FromMilliseconds(static_cast<double>(timeout));
  TimeStamp currentFrameEndTime = currentFrameTime + durationOfTimeout;

  return currentFrameEndTime;
}

imgFrame*
RasterImage::GetCurrentImgFrame()
{
  return GetImgFrame(GetCurrentImgFrameIndex());
}

imgFrame*
RasterImage::GetCurrentDrawableImgFrame()
{
  return GetDrawableImgFrame(GetCurrentImgFrameIndex());
}



NS_IMETHODIMP_(bool)
RasterImage::FrameIsOpaque(uint32_t aWhichFrame)
{
  if (aWhichFrame > FRAME_MAX_VALUE) {
    NS_WARNING("aWhichFrame outside valid range!");
    return false;
  }

  if (mError)
    return false;

  
  imgFrame* frame = aWhichFrame == FRAME_FIRST ? GetImgFrame(0)
                                               : GetCurrentImgFrame();

  
  if (!frame)
    return false;

  
  
  
  nsIntRect framerect = frame->GetRect();
  return !frame->GetNeedsBackground() &&
         framerect.IsEqualInterior(nsIntRect(0, 0, mSize.width, mSize.height));
}

nsIntRect
RasterImage::FrameRect(uint32_t aWhichFrame)
{
  if (aWhichFrame > FRAME_MAX_VALUE) {
    NS_WARNING("aWhichFrame outside valid range!");
    return nsIntRect();
  }

  
  imgFrame* frame = aWhichFrame == FRAME_FIRST ? GetImgFrame(0)
                                               : GetCurrentImgFrame();

  
  if (frame) {
    return frame->GetRect();
  }

  
  
  
  
  
  return nsIntRect();
}

uint32_t
RasterImage::GetCurrentFrameIndex()
{
  return GetCurrentImgFrameIndex();
}

uint32_t
RasterImage::GetNumFrames()
{
  return mFrames.Length();
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





NS_IMETHODIMP
RasterImage::CopyFrame(uint32_t aWhichFrame,
                       uint32_t aFlags,
                       gfxImageSurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  nsresult rv;

  if (!ApplyDecodeFlags(aFlags))
    return NS_ERROR_NOT_AVAILABLE;

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  NS_ENSURE_ARG_POINTER(_retval);

  
  
  
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nullptr;
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxPattern> pattern;
  frame->GetPattern(getter_AddRefs(pattern));
  nsIntRect intframerect = frame->GetRect();
  gfxRect framerect(intframerect.x, intframerect.y, intframerect.width, intframerect.height);

  
  
  nsRefPtr<gfxImageSurface> imgsurface = new gfxImageSurface(gfxIntSize(mSize.width, mSize.height),
                                                             gfxASurface::ImageFormatARGB32);
  gfxContext ctx(imgsurface);
  ctx.SetOperator(gfxContext::OPERATOR_SOURCE);
  ctx.Rectangle(framerect);
  ctx.Translate(framerect.TopLeft());
  ctx.SetPattern(pattern);
  ctx.Fill();

  *_retval = imgsurface.forget().get();
  return NS_OK;
}




NS_IMETHODIMP
RasterImage::GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags,
                      gfxASurface **_retval)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  nsresult rv = NS_OK;

  if (!ApplyDecodeFlags(aFlags))
    return NS_ERROR_NOT_AVAILABLE;

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                          0 : GetCurrentImgFrameIndex();
  imgFrame *frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    *_retval = nullptr;
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<gfxASurface> framesurf;

  
  
  nsIntRect framerect = frame->GetRect();
  if (framerect.x == 0 && framerect.y == 0 &&
      framerect.width == mSize.width &&
      framerect.height == mSize.height)
    rv = frame->GetSurface(getter_AddRefs(framesurf));

  
  
  if (!framesurf) {
    nsRefPtr<gfxImageSurface> imgsurf;
    rv = CopyFrame(aWhichFrame, aFlags, getter_AddRefs(imgsurf));
    framesurf = imgsurf;
  }

  *_retval = framesurf.forget().get();

  return rv;
}

already_AddRefed<layers::Image>
RasterImage::GetCurrentImage()
{
  if (!mDecoded) {
    
    
    RequestDecode();
    return nullptr;
  }

  nsRefPtr<gfxASurface> imageSurface;
  nsresult rv = GetFrame(FRAME_CURRENT, FLAG_NONE, getter_AddRefs(imageSurface));
  NS_ENSURE_SUCCESS(rv, nullptr);

  if (!imageSurface) {
    return nullptr;
  }

  if (!mImageContainer) {
    mImageContainer = LayerManager::CreateImageContainer();
  }

  CairoImage::Data cairoData;
  cairoData.mSurface = imageSurface;
  GetWidth(&cairoData.mSize.width);
  GetHeight(&cairoData.mSize.height);

  ImageFormat cairoFormat = CAIRO_SURFACE;
  nsRefPtr<layers::Image> image = mImageContainer->CreateImage(&cairoFormat, 1);
  NS_ASSERTION(image, "Failed to create Image");
  
  NS_ASSERTION(image->GetFormat() == cairoFormat, "Wrong format");
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
  return NS_OK;
}

void
RasterImage::UpdateImageContainer()
{
  if (!mImageContainer || IsInUpdateImageContainer()) {
    return;
  }

  SetInUpdateImageContainer(true);

  nsRefPtr<layers::Image> image = GetCurrentImage();
  if (!image) {
    return;
  }
  mImageContainer->SetCurrentImage(image);
  SetInUpdateImageContainer(false);
}

size_t
RasterImage::HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const
{
  
  
  
  
  size_t n = mSourceData.SizeOfExcludingThis(aMallocSizeOf);
  if (n == 0) {
    n = mSourceData.Length();
    NS_ABORT_IF_FALSE(StoringSourceData() || (n == 0),
                      "Non-zero source data size when we aren't storing it?");
  }
  return n;
}

size_t
RasterImage::SizeOfDecodedWithComputedFallbackIfHeap(gfxASurface::MemoryLocation aLocation,
                                                     nsMallocSizeOfFun aMallocSizeOf) const
{
  size_t n = 0;
  for (uint32_t i = 0; i < mFrames.Length(); ++i) {
    imgFrame* frame = mFrames.SafeElementAt(i, nullptr);
    NS_ABORT_IF_FALSE(frame, "Null frame in frame array!");
    n += frame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
  }

  if (mScaleResult.status == SCALE_DONE) {
    n += mScaleResult.frame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
  }

  return n;
}

size_t
RasterImage::HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(gfxASurface::MEMORY_IN_PROCESS_HEAP,
                                                 aMallocSizeOf);
}

size_t
RasterImage::NonHeapSizeOfDecoded() const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(gfxASurface::MEMORY_IN_PROCESS_NONHEAP,
                                                 NULL);
}

size_t
RasterImage::OutOfProcessSizeOfDecoded() const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(gfxASurface::MEMORY_OUT_OF_PROCESS,
                                                 NULL);
}

void
RasterImage::DeleteImgFrame(uint32_t framenum)
{
  NS_ABORT_IF_FALSE(framenum < mFrames.Length(), "Deleting invalid frame!");

  delete mFrames[framenum];
  mFrames[framenum] = nullptr;
}

nsresult
RasterImage::InternalAddFrameHelper(uint32_t framenum, imgFrame *aFrame,
                                    uint8_t **imageData, uint32_t *imageLength,
                                    uint32_t **paletteData, uint32_t *paletteLength)
{
  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Invalid frame index!");
  if (framenum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(aFrame);

  
  
  frame->LockImageData();

  if (paletteData && paletteLength)
    frame->GetPaletteData(paletteData, paletteLength);

  frame->GetImageData(imageData, imageLength);

  mFrames.InsertElementAt(framenum, frame.forget());

  return NS_OK;
}
                                  
nsresult
RasterImage::InternalAddFrame(uint32_t framenum,
                              int32_t aX, int32_t aY,
                              int32_t aWidth, int32_t aHeight,
                              gfxASurface::gfxImageFormat aFormat,
                              uint8_t aPaletteDepth,
                              uint8_t **imageData,
                              uint32_t *imageLength,
                              uint32_t **paletteData,
                              uint32_t *paletteLength)
{
  
  
  
  NS_ABORT_IF_FALSE(mInDecoder, "Only decoders may add frames!");

  NS_ABORT_IF_FALSE(framenum <= mFrames.Length(), "Invalid frame index!");
  if (framenum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  nsAutoPtr<imgFrame> frame(new imgFrame());

  nsresult rv = frame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (mFrames.Length() > 0) {
    imgFrame *prevframe = mFrames.ElementAt(mFrames.Length() - 1);
    prevframe->UnlockImageData();
  }

  if (mFrames.Length() == 0) {
    return InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength, 
                                  paletteData, paletteLength);
  }

  if (mFrames.Length() == 1) {
    
    EnsureAnimExists();
    
    
    
    
    int32_t frameDisposalMethod = mFrames[0]->GetFrameDisposalMethod();
    if (frameDisposalMethod == kDisposeClear ||
        frameDisposalMethod == kDisposeRestorePrevious)
      mAnim->firstFrameRefreshArea = mFrames[0]->GetRect();
  }

  
  
  
  nsIntRect frameRect = frame->GetRect();
  mAnim->firstFrameRefreshArea.UnionRect(mAnim->firstFrameRefreshArea, 
                                         frameRect);
  
  rv = InternalAddFrameHelper(framenum, frame.forget(), imageData, imageLength,
                              paletteData, paletteLength);
  
  
  EvaluateAnimation();
  
  return rv;
}

bool
RasterImage::ApplyDecodeFlags(uint32_t aNewFlags)
{
  if (mFrameDecodeFlags == (aNewFlags & DECODE_FLAGS_MASK))
    return true; 

  if (mDecoded) {
    
    
    
    if (!(aNewFlags & FLAG_SYNC_DECODE))
      return false;
    if (!CanForciblyDiscard() || mDecoder || mAnim)
      return false;
    ForceDiscard();
  }

  mFrameDecodeFlags = aNewFlags & DECODE_FLAGS_MASK;
  return true;
}

nsresult
RasterImage::SetSize(int32_t aWidth, int32_t aHeight)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  
  if ((aWidth < 0) || (aHeight < 0))
    return NS_ERROR_INVALID_ARG;

  
  if (!mMultipart && mHasSize &&
      ((aWidth != mSize.width) || (aHeight != mSize.height))) {
    NS_WARNING("Image changed size on redecode! This should not happen!");

    
    
    if (mDecoder)
      mDecoder->PostResizeError();

    DoError();
    return NS_ERROR_UNEXPECTED;
  }

  
  mSize.SizeTo(aWidth, aHeight);
  mHasSize = true;

  return NS_OK;
}

nsresult
RasterImage::EnsureFrame(uint32_t aFrameNum, int32_t aX, int32_t aY,
                         int32_t aWidth, int32_t aHeight,
                         gfxASurface::gfxImageFormat aFormat,
                         uint8_t aPaletteDepth,
                         uint8_t **imageData, uint32_t *imageLength,
                         uint32_t **paletteData, uint32_t *paletteLength)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);
  NS_ABORT_IF_FALSE(aFrameNum <= mFrames.Length(), "Invalid frame index!");

  if (aPaletteDepth > 0) {
    NS_ENSURE_ARG_POINTER(paletteData);
    NS_ENSURE_ARG_POINTER(paletteLength);
  }

  if (aFrameNum > mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  
  if (aFrameNum == mFrames.Length())
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                            aPaletteDepth, imageData, imageLength,
                            paletteData, paletteLength);

  imgFrame *frame = GetImgFrame(aFrameNum);
  if (!frame)
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat, 
                            aPaletteDepth, imageData, imageLength,
                            paletteData, paletteLength);

  
  nsIntRect rect = frame->GetRect();
  if (rect.x == aX && rect.y == aY && rect.width == aWidth &&
      rect.height == aHeight && frame->GetFormat() == aFormat &&
      frame->GetPaletteDepth() == aPaletteDepth) {
    frame->GetImageData(imageData, imageLength);
    if (paletteData) {
      frame->GetPaletteData(paletteData, paletteLength);
    }

    
    if (*imageData && paletteData && *paletteData) {
      return NS_OK;
    }
    if (*imageData && !paletteData) {
      return NS_OK;
    }
  }

  

  
  
  frame->UnlockImageData();

  DeleteImgFrame(aFrameNum);
  mFrames.RemoveElementAt(aFrameNum);
  nsAutoPtr<imgFrame> newFrame(new imgFrame());
  nsresult rv = newFrame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);
  return InternalAddFrameHelper(aFrameNum, newFrame.forget(), imageData,
                                imageLength, paletteData, paletteLength);
}

nsresult
RasterImage::EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                         int32_t aWidth, int32_t aHeight,
                         gfxASurface::gfxImageFormat aFormat,
                         uint8_t** imageData, uint32_t* imageLength)
{
  return EnsureFrame(aFramenum, aX, aY, aWidth, aHeight, aFormat,
                      0, imageData, imageLength,
                      nullptr,
                      nullptr);
}

void
RasterImage::FrameUpdated(uint32_t aFrameNum, nsIntRect &aUpdatedRect)
{
  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");

  imgFrame *frame = GetImgFrameNoDecode(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling FrameUpdated on frame that doesn't exist!");

  frame->ImageUpdated(aUpdatedRect);
    
  if (aFrameNum == GetCurrentImgFrameIndex() &&
      !IsInUpdateImageContainer()) {
    mImageContainer = nullptr;
  }
}

nsresult
RasterImage::SetFrameDisposalMethod(uint32_t aFrameNum,
                                    int32_t aDisposalMethod)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrameNoDecode(aFrameNum);
  NS_ABORT_IF_FALSE(frame,
                    "Calling SetFrameDisposalMethod on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetFrameDisposalMethod(aDisposalMethod);

  return NS_OK;
}

nsresult
RasterImage::SetFrameTimeout(uint32_t aFrameNum, int32_t aTimeout)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrameNoDecode(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameTimeout on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetTimeout(aTimeout);

  return NS_OK;
}

nsresult
RasterImage::SetFrameBlendMethod(uint32_t aFrameNum, int32_t aBlendMethod)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrameNoDecode(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameBlendMethod on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetBlendMethod(aBlendMethod);

  return NS_OK;
}

nsresult
RasterImage::SetFrameHasNoAlpha(uint32_t aFrameNum)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame *frame = GetImgFrameNoDecode(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameHasNoAlpha on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetHasNoAlpha();

  return NS_OK;
}

nsresult
RasterImage::SetFrameAsNonPremult(uint32_t aFrameNum, bool aIsNonPremult)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < mFrames.Length(), "Invalid frame index!");
  if (aFrameNum >= mFrames.Length())
    return NS_ERROR_INVALID_ARG;

  imgFrame* frame = GetImgFrameNoDecode(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameAsNonPremult on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetAsNonPremult(aIsNonPremult);

  return NS_OK;
}

nsresult
RasterImage::DecodingComplete()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  
  
  mDecoded = true;
  mHasBeenDecoded = true;

  nsresult rv;

  
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(!DiscardingActive(),
                      "We shouldn't have been discardable before this");
    rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  
  if ((mFrames.Length() == 1) && !mMultipart) {
    rv = mFrames[0]->Optimize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}



nsresult
RasterImage::StartAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(ShouldAnimate(), "Should not animate!");

  EnsureAnimExists();

  imgFrame* currentFrame = GetCurrentImgFrame();
  if (currentFrame) {
    if (currentFrame->GetTimeout() < 0) { 
      mAnimationFinished = true;
      return NS_ERROR_ABORT;
    }

    
    
    mAnim->currentAnimationFrameTime = TimeStamp::Now();
  }
  
  return NS_OK;
}



nsresult
RasterImage::StopAnimation()
{
  NS_ABORT_IF_FALSE(mAnimating, "Should be animating!");

  if (mError)
    return NS_ERROR_FAILURE;

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::ResetAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  if (mAnimationMode == kDontAnimMode || 
      !mAnim || mAnim->currentAnimationFrameIndex == 0)
    return NS_OK;

  mAnimationFinished = false;

  if (mAnimating)
    StopAnimation();

  mAnim->lastCompositedFrameIndex = -1;
  mAnim->currentAnimationFrameIndex = 0;
  UpdateImageContainer();

  
  

  
  if (mAnimating && mStatusTracker)
    mStatusTracker->GetDecoderObserver()->FrameChanged(&(mAnim->firstFrameRefreshArea));

  if (ShouldAnimate()) {
    StartAnimation();
    
    
    
    mAnimating = true;
  }

  return NS_OK;
}

void
RasterImage::SetLoopCount(int32_t aLoopCount)
{
  if (mError)
    return;

  
  
  
  
  mLoopCount = aLoopCount;
}

nsresult
RasterImage::AddSourceData(const char *aBuffer, uint32_t aCount)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aBuffer);
  nsresult rv = NS_OK;

  
  NS_ABORT_IF_FALSE(mInitialized, "Calling AddSourceData() on uninitialized "
                                  "RasterImage!");

  
  NS_ABORT_IF_FALSE(!mHasSourceData, "Calling AddSourceData() after calling "
                                     "sourceDataComplete()!");

  
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  
  
  if (mDecoded) {
    return NS_OK;
  }

  
  
  
  if (mMultipart && mBytesDecoded == 0) {
    
    if (mAnimating) {
      StopAnimation();
      mAnimating = false;
    }
    mAnimationFinished = false;
    if (mAnim) {
      delete mAnim;
      mAnim = nullptr;
    }
    
    int old_frame_count = mFrames.Length();
    if (old_frame_count > 1) {
      for (int i = 0; i < old_frame_count; ++i) {
        DeleteImgFrame(i);
      }
      mFrames.Clear();
    }
  }

  
  if (!StoringSourceData()) {
    rv = WriteToDecoder(aBuffer, aCount);
    CONTAINER_ENSURE_SUCCESS(rv);

    
    
    
    nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
    mInDecoder = true;
    mDecoder->FlushInvalidations();
    mInDecoder = false;
  }

  
  else {

    
    char *newElem = mSourceData.AppendElements(aBuffer, aCount);
    if (!newElem)
      return NS_ERROR_OUT_OF_MEMORY;

    
    
    if (mDecoder) {
      DecodeWorker::Singleton()->RequestDecode(this);
    }
  }

  
  total_source_bytes += aCount;
  if (mDiscardable)
    discardable_source_bytes += aCount;
  PR_LOG (GetCompressedImageAccountingLog(), PR_LOG_DEBUG,
          ("CompressedImageAccounting: Added compressed data to RasterImage %p (%s). "
           "Total Containers: %d, Discardable containers: %d, "
           "Total source bytes: %lld, Source bytes for discardable containers %lld",
           this,
           mSourceDataMimeType.get(),
           num_containers,
           num_discardable_containers,
           total_source_bytes,
           discardable_source_bytes));

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
  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mHasSourceData)
    return NS_OK;
  mHasSourceData = true;

  
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  
  
  
  if (!StoringSourceData()) {
    nsresult rv = ShutdownDecoder(eShutdownIntent_Done);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  if (mDecoder) {
    nsresult rv = DecodeWorker::Singleton()->DecodeUntilSizeAvailable(this);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  if (mDecoder) {
    DecodeWorker::Singleton()->RequestDecode(this);
  }

  
  mSourceData.Compact();

  
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

  
  if (CanDiscard()) {
    nsresult rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }
  return NS_OK;
}

nsresult
RasterImage::OnImageDataComplete(nsIRequest*, nsISupports*, nsresult aStatus, bool aLastPart)
{
  nsresult finalStatus = DoImageDataComplete();

  
  if (NS_FAILED(aStatus))
    finalStatus = aStatus;

  GetStatusTracker().OnStopRequest(aLastPart, finalStatus);
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

  NS_ABORT_IF_FALSE(bytesRead == aCount || HasError(),
    "WriteToRasterImage should consume everything or the image must be in error!");

  return rv;
}

nsresult
RasterImage::OnNewSourceData()
{
  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mHasSourceData,
                    "Calling NewSourceData before SourceDataComplete!");
  if (!mHasSourceData)
    return NS_ERROR_ILLEGAL_VALUE;

  
  
  
  NS_ABORT_IF_FALSE(mMultipart, "NewSourceData not supported for multipart");
  if (!mMultipart)
    return NS_ERROR_ILLEGAL_VALUE;

  
  NS_ABORT_IF_FALSE(!StoringSourceData(),
                    "Shouldn't be storing source data for multipart");

  
  
  NS_ABORT_IF_FALSE(!mDecoder, "Shouldn't have a decoder in NewSourceData");

  
  NS_ABORT_IF_FALSE(mDecoded, "Should be decoded in NewSourceData");

  
  mDecoded = false;
  mHasSourceData = false;

  
  
  rv = InitDecoder( false);
  CONTAINER_ENSURE_SUCCESS(rv);

  return NS_OK;
}

nsresult
RasterImage::SetSourceSizeHint(uint32_t sizeHint)
{
  if (sizeHint && StoringSourceData())
    return mSourceData.SetCapacity(sizeHint) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
  return NS_OK;
}




nsresult
RasterImage::DoComposite(nsIntRect* aDirtyRect,
                         imgFrame* aPrevFrame,
                         imgFrame* aNextFrame,
                         int32_t aNextFrameIndex)
{
  NS_ENSURE_ARG_POINTER(aDirtyRect);
  NS_ENSURE_ARG_POINTER(aPrevFrame);
  NS_ENSURE_ARG_POINTER(aNextFrame);

  int32_t prevFrameDisposalMethod = aPrevFrame->GetFrameDisposalMethod();
  if (prevFrameDisposalMethod == kDisposeRestorePrevious &&
      !mAnim->compositingPrevFrame)
    prevFrameDisposalMethod = kDisposeClear;

  nsIntRect prevFrameRect = aPrevFrame->GetRect();
  bool isFullPrevFrame = (prevFrameRect.x == 0 && prevFrameRect.y == 0 &&
                          prevFrameRect.width == mSize.width &&
                          prevFrameRect.height == mSize.height);

  
  
  if (isFullPrevFrame && 
      (prevFrameDisposalMethod == kDisposeClear))
    prevFrameDisposalMethod = kDisposeClearAll;

  int32_t nextFrameDisposalMethod = aNextFrame->GetFrameDisposalMethod();
  nsIntRect nextFrameRect = aNextFrame->GetRect();
  bool isFullNextFrame = (nextFrameRect.x == 0 && nextFrameRect.y == 0 &&
                          nextFrameRect.width == mSize.width &&
                          nextFrameRect.height == mSize.height);

  if (!aNextFrame->GetIsPaletted()) {
    
    
    if (prevFrameDisposalMethod == kDisposeClearAll) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return NS_OK;
    }
  
    
    
    if (isFullNextFrame &&
        (nextFrameDisposalMethod != kDisposeRestorePrevious) &&
        !aNextFrame->GetHasAlpha()) {
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      return NS_OK;
    }
  }

  
  switch (prevFrameDisposalMethod) {
    default:
    case kDisposeNotSpecified:
    case kDisposeKeep:
      *aDirtyRect = nextFrameRect;
      break;

    case kDisposeClearAll:
      
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;

    case kDisposeClear:
      
      
      
      
      
      
      
      aDirtyRect->UnionRect(nextFrameRect, prevFrameRect);
      break;

    case kDisposeRestorePrevious:
      aDirtyRect->SetRect(0, 0, mSize.width, mSize.height);
      break;
  }

  
  
  
  
  
  
  if (mAnim->lastCompositedFrameIndex == aNextFrameIndex) {
    return NS_OK;
  }

  bool needToBlankComposite = false;

  
  if (!mAnim->compositingFrame) {
    mAnim->compositingFrame = new imgFrame();
    nsresult rv = mAnim->compositingFrame->Init(0, 0, mSize.width, mSize.height,
                                                gfxASurface::ImageFormatARGB32);
    if (NS_FAILED(rv)) {
      mAnim->compositingFrame = nullptr;
      return rv;
    }
    needToBlankComposite = true;
  } else if (aNextFrameIndex != mAnim->lastCompositedFrameIndex+1) {

    
    
    needToBlankComposite = true;
  }

  
  
  
  
  
  
  bool doDisposal = true;
  if (!aNextFrame->GetHasAlpha() &&
      nextFrameDisposalMethod != kDisposeRestorePrevious) {
    if (isFullNextFrame) {
      
      
      doDisposal = false;
      
      needToBlankComposite = false;
    } else {
      if ((prevFrameRect.x >= nextFrameRect.x) &&
          (prevFrameRect.y >= nextFrameRect.y) &&
          (prevFrameRect.x + prevFrameRect.width <= nextFrameRect.x + nextFrameRect.width) &&
          (prevFrameRect.y + prevFrameRect.height <= nextFrameRect.y + nextFrameRect.height)) {
        
        
        doDisposal = false;
      }
    }      
  }

  if (doDisposal) {
    
    switch (prevFrameDisposalMethod) {
      case kDisposeClear:
        if (needToBlankComposite) {
          
          
          ClearFrame(mAnim->compositingFrame);
        } else {
          
          ClearFrame(mAnim->compositingFrame, prevFrameRect);
        }
        break;
  
      case kDisposeClearAll:
        ClearFrame(mAnim->compositingFrame);
        break;
  
      case kDisposeRestorePrevious:
        
        
        if (mAnim->compositingPrevFrame) {
          CopyFrameImage(mAnim->compositingPrevFrame, mAnim->compositingFrame);
  
          
          if (nextFrameDisposalMethod != kDisposeRestorePrevious)
            mAnim->compositingPrevFrame = nullptr;
        } else {
          ClearFrame(mAnim->compositingFrame);
        }
        break;
      
      default:
        
        
        
        
        
        
        if (mAnim->lastCompositedFrameIndex != aNextFrameIndex - 1) {
          if (isFullPrevFrame && !aPrevFrame->GetIsPaletted()) {
            
            CopyFrameImage(aPrevFrame, mAnim->compositingFrame);
          } else {
            if (needToBlankComposite) {
              
              if (aPrevFrame->GetHasAlpha() || !isFullPrevFrame) {
                ClearFrame(mAnim->compositingFrame);
              }
            }
            DrawFrameTo(aPrevFrame, mAnim->compositingFrame, prevFrameRect);
          }
        }
    }
  } else if (needToBlankComposite) {
    
    
    ClearFrame(mAnim->compositingFrame);
  }

  
  
  
  if ((nextFrameDisposalMethod == kDisposeRestorePrevious) &&
      (prevFrameDisposalMethod != kDisposeRestorePrevious)) {
    
    
    
    if (!mAnim->compositingPrevFrame) {
      mAnim->compositingPrevFrame = new imgFrame();
      nsresult rv = mAnim->compositingPrevFrame->Init(0, 0, mSize.width, mSize.height,
                                                      gfxASurface::ImageFormatARGB32);
      if (NS_FAILED(rv)) {
        mAnim->compositingPrevFrame = nullptr;
        return rv;
      }
    }

    CopyFrameImage(mAnim->compositingFrame, mAnim->compositingPrevFrame);
  }

  
  DrawFrameTo(aNextFrame, mAnim->compositingFrame, nextFrameRect);

  
  
  int32_t timeout = aNextFrame->GetTimeout();
  mAnim->compositingFrame->SetTimeout(timeout);

  
  nsresult rv = mAnim->compositingFrame->ImageUpdated(mAnim->compositingFrame->GetRect());
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  
  
  if (isFullNextFrame && mAnimationMode == kNormalAnimMode && mLoopCount != 0 &&
      nextFrameDisposalMethod != kDisposeRestorePrevious &&
      !aNextFrame->GetIsPaletted()) {
    
    
    
    
    
    if (CopyFrameImage(mAnim->compositingFrame, aNextFrame)) {
      aPrevFrame->SetFrameDisposalMethod(kDisposeClearAll);
      mAnim->lastCompositedFrameIndex = -1;
      return NS_OK;
    }
  }

  mAnim->lastCompositedFrameIndex = aNextFrameIndex;

  return NS_OK;
}



void
RasterImage::ClearFrame(imgFrame *aFrame)
{
  if (!aFrame)
    return;

  nsresult rv = aFrame->LockImageData();
  if (NS_FAILED(rv))
    return;

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Paint();

  aFrame->UnlockImageData();
}


void
RasterImage::ClearFrame(imgFrame *aFrame, nsIntRect &aRect)
{
  if (!aFrame || aRect.width <= 0 || aRect.height <= 0)
    return;

  nsresult rv = aFrame->LockImageData();
  if (NS_FAILED(rv))
    return;

  nsRefPtr<gfxASurface> surf;
  aFrame->GetSurface(getter_AddRefs(surf));

  
  gfxContext ctx(surf);
  ctx.SetOperator(gfxContext::OPERATOR_CLEAR);
  ctx.Rectangle(gfxRect(aRect.x, aRect.y, aRect.width, aRect.height));
  ctx.Fill();

  aFrame->UnlockImageData();
}





bool
RasterImage::CopyFrameImage(imgFrame *aSrcFrame,
                            imgFrame *aDstFrame)
{
  uint8_t* aDataSrc;
  uint8_t* aDataDest;
  uint32_t aDataLengthSrc;
  uint32_t aDataLengthDest;

  if (!aSrcFrame || !aDstFrame)
    return false;

  AutoFrameLocker dstLock(aDstFrame);
  AutoFrameLocker srcLock(aSrcFrame);

  if (!srcLock.Succeeded() || !dstLock.Succeeded()) {
    return false;
  }

  
  aSrcFrame->GetImageData(&aDataSrc, &aDataLengthSrc);
  aDstFrame->GetImageData(&aDataDest, &aDataLengthDest);
  if (!aDataDest || !aDataSrc || aDataLengthDest != aDataLengthSrc) {
    return false;
  }

  memcpy(aDataDest, aDataSrc, aDataLengthSrc);

  return true;
}








nsresult
RasterImage::DrawFrameTo(imgFrame *aSrc,
                         imgFrame *aDst,
                         nsIntRect& aSrcRect)
{
  NS_ENSURE_ARG_POINTER(aSrc);
  NS_ENSURE_ARG_POINTER(aDst);

  AutoFrameLocker srcLock(aSrc);
  AutoFrameLocker dstLock(aDst);

  nsIntRect dstRect = aDst->GetRect();

  
  if (aSrcRect.x < 0 || aSrcRect.y < 0) {
    NS_WARNING("RasterImage::DrawFrameTo: negative offsets not allowed");
    return NS_ERROR_FAILURE;
  }
  
  if ((aSrcRect.x > dstRect.width) || (aSrcRect.y > dstRect.height)) {
    return NS_OK;
  }

  if (aSrc->GetIsPaletted()) {
    
    int32_t width = std::min(aSrcRect.width, dstRect.width - aSrcRect.x);
    int32_t height = std::min(aSrcRect.height, dstRect.height - aSrcRect.y);

    
    NS_ASSERTION((aSrcRect.x >= 0) && (aSrcRect.y >= 0) &&
                 (aSrcRect.x + width <= dstRect.width) &&
                 (aSrcRect.y + height <= dstRect.height),
                "RasterImage::DrawFrameTo: Invalid aSrcRect");

    
    NS_ASSERTION((width <= aSrcRect.width) && (height <= aSrcRect.height),
                 "RasterImage::DrawFrameTo: source must be smaller than dest");

    if (!srcLock.Succeeded() || !dstLock.Succeeded())
      return NS_ERROR_FAILURE;

    
    uint32_t size;
    uint8_t *srcPixels;
    uint32_t *colormap;
    uint32_t *dstPixels;

    aSrc->GetImageData(&srcPixels, &size);
    aSrc->GetPaletteData(&colormap, &size);
    aDst->GetImageData((uint8_t **)&dstPixels, &size);
    if (!srcPixels || !dstPixels || !colormap) {
      return NS_ERROR_FAILURE;
    }

    
    dstPixels += aSrcRect.x + (aSrcRect.y * dstRect.width);
    if (!aSrc->GetHasAlpha()) {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          dstPixels[c] = colormap[srcPixels[c]];
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    } else {
      for (int32_t r = height; r > 0; --r) {
        for (int32_t c = 0; c < width; c++) {
          const uint32_t color = colormap[srcPixels[c]];
          if (color)
            dstPixels[c] = color;
        }
        
        srcPixels += aSrcRect.width;
        dstPixels += dstRect.width;
      }
    }

    return NS_OK;
  }

  nsRefPtr<gfxPattern> srcPatt;
  aSrc->GetPattern(getter_AddRefs(srcPatt));

  nsRefPtr<gfxASurface> dstSurf;
  aDst->GetSurface(getter_AddRefs(dstSurf));

  gfxContext dst(dstSurf);
  dst.Translate(gfxPoint(aSrcRect.x, aSrcRect.y));
  dst.Rectangle(gfxRect(0, 0, aSrcRect.width, aSrcRect.height), true);
  
  
  int32_t blendMethod = aSrc->GetBlendMethod();
  if (blendMethod == kBlendSource) {
    gfxContext::GraphicsOperator defaultOperator = dst.CurrentOperator();
    dst.SetOperator(gfxContext::OPERATOR_CLEAR);
    dst.Fill();
    dst.SetOperator(defaultOperator);
  }
  dst.SetPattern(srcPatt);
  dst.Paint();

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
RasterImage::Discard(bool force)
{
  
  NS_ABORT_IF_FALSE(force ? CanForciblyDiscard() : CanDiscard(), "Asked to discard but can't!");

  
  NS_ABORT_IF_FALSE(!mDecoder, "Asked to discard with open decoder!");

  
  
  NS_ABORT_IF_FALSE(!mAnim, "Asked to discard for animated image!");

  
  int old_frame_count = mFrames.Length();

  
  for (int i = 0; i < old_frame_count; ++i)
    delete mFrames[i];
  mFrames.Clear();

  
  mScaleResult.status = SCALE_INVALID;
  mScaleResult.frame = nullptr;

  
  mDecoded = false;

  
  if (mStatusTracker)
    mStatusTracker->GetDecoderObserver()->OnDiscard();

  if (force)
    DiscardTracker::Remove(&mDiscardTrackerNode);

  
  PR_LOG(GetCompressedImageAccountingLog(), PR_LOG_DEBUG,
         ("CompressedImageAccounting: discarded uncompressed image "
          "data from RasterImage %p (%s) - %d frames (cached count: %d); "
          "Total Containers: %d, Discardable containers: %d, "
          "Total source bytes: %lld, Source bytes for discardable containers %lld",
          this,
          mSourceDataMimeType.get(),
          old_frame_count,
          mFrames.Length(),
          num_containers,
          num_discardable_containers,
          total_source_bytes,
          discardable_source_bytes));
}


bool
RasterImage::CanDiscard() {
  return (DiscardingEnabled() && 
          mDiscardable &&        
          (mLockCount == 0) &&   
          mHasSourceData &&      
          mDecoded);             
}

bool
RasterImage::CanForciblyDiscard() {
  return mDiscardable &&         
         mHasSourceData;         
}



bool
RasterImage::DiscardingActive() {
  return mDiscardTrackerNode.isInList();
}



bool
RasterImage::StoringSourceData() const {
  return (mDecodeOnDraw || mDiscardable);
}




nsresult
RasterImage::InitDecoder(bool aDoSizeDecode)
{
  
  NS_ABORT_IF_FALSE(!mDecoder, "Calling InitDecoder() while already decoding!");
  
  
  NS_ABORT_IF_FALSE(!mDecoded, "Calling InitDecoder() but already decoded!");

  
  NS_ABORT_IF_FALSE(!DiscardingActive(), "Discard Timer active in InitDecoder()!");

  
  eDecoderType type = GetDecoderType(mSourceDataMimeType.get());
  CONTAINER_ENSURE_TRUE(type != eDecoderType_unknown, NS_IMAGELIB_ERROR_NO_DECODER);

  
  imgDecoderObserver* observer = mStatusTracker ? mStatusTracker->GetDecoderObserver()
                                                : nullptr;
  switch (type) {
    case eDecoderType_png:
      mDecoder = new nsPNGDecoder(*this, observer);
      break;
    case eDecoderType_gif:
      mDecoder = new nsGIFDecoder2(*this, observer);
      break;
    case eDecoderType_jpeg:
      
      
      mDecoder = new nsJPEGDecoder(*this, observer,
                                   mHasBeenDecoded ? Decoder::SEQUENTIAL :
                                                     Decoder::PROGRESSIVE);
      break;
    case eDecoderType_bmp:
      mDecoder = new nsBMPDecoder(*this, observer);
      break;
    case eDecoderType_ico:
      mDecoder = new nsICODecoder(*this, observer);
      break;
    case eDecoderType_icon:
      mDecoder = new nsIconDecoder(*this, observer);
      break;
    default:
      NS_ABORT_IF_FALSE(0, "Shouldn't get here!");
  }

  
  mDecoder->SetSizeDecode(aDoSizeDecode);
  mDecoder->SetDecodeFlags(mFrameDecodeFlags);
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
RasterImage::ShutdownDecoder(eShutdownIntent aIntent)
{
  
  NS_ABORT_IF_FALSE((aIntent >= 0) && (aIntent < eShutdownIntent_AllCount),
                    "Invalid shutdown intent");

  
  NS_ABORT_IF_FALSE(mDecoder, "Calling ShutdownDecoder() with no active decoder!");

  
  bool wasSizeDecode = mDecoder->IsSizeDecode();

  
  
  
  nsRefPtr<Decoder> decoder = mDecoder;
  mDecoder = nullptr;

  mFinishing = true;
  mInDecoder = true;
  decoder->Finish(aIntent);
  mInDecoder = false;
  mFinishing = false;

  
  
  DecodeWorker::Singleton()->StopDecoding(this);

  nsresult decoderStatus = decoder->GetDecoderError();
  if (NS_FAILED(decoderStatus)) {
    DoError();
    return decoderStatus;
  }

  
  
  bool failed = false;
  if (wasSizeDecode && !mHasSize)
    failed = true;
  if (!wasSizeDecode && !mDecoded)
    failed = true;
  if ((aIntent == eShutdownIntent_Done) && failed) {
    DoError();
    return NS_ERROR_FAILURE;
  }

  
  mBytesDecoded = 0;

  return NS_OK;
}


nsresult
RasterImage::WriteToDecoder(const char *aBuffer, uint32_t aCount)
{
  
  NS_ABORT_IF_FALSE(mDecoder, "Trying to write to null decoder!");

  
  
  
  
  
  if (mFrames.Length() > 0) {
    imgFrame *curframe = mFrames.ElementAt(mFrames.Length() - 1);
    curframe->LockImageData();
  }

  
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = true;
  mDecoder->Write(aBuffer, aCount);
  mInDecoder = false;

  
  
  if (mFrames.Length() > 0) {
    imgFrame *curframe = mFrames.ElementAt(mFrames.Length() - 1);
    curframe->UnlockImageData();
  }

  if (!mDecoder)
    return NS_ERROR_FAILURE;
    
  CONTAINER_ENSURE_SUCCESS(mDecoder->GetDecoderError());

  
  
  mBytesDecoded += aCount;

  return NS_OK;
}







nsresult
RasterImage::WantDecodedFrames()
{
  nsresult rv;

  
  if (CanDiscard()) {
    NS_ABORT_IF_FALSE(DiscardingActive(),
                      "Decoded and discardable but discarding not activated!");
    rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  return StartDecoding();
}



NS_IMETHODIMP
RasterImage::RequestDecode()
{
  return RequestDecodeCore(ASYNCHRONOUS);
}


NS_IMETHODIMP
RasterImage::StartDecoding()
{
  return RequestDecodeCore(SOMEWHAT_SYNCHRONOUS);
}


NS_IMETHODIMP
RasterImage::RequestDecodeCore(RequestDecodeType aDecodeType)
{
  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mDecoded)
    return NS_OK;

  
  if (!StoringSourceData())
    return NS_OK;

  
  
  if (mDecoder && !mDecoder->IsSizeDecode() && mBytesDecoded) {
    return NS_OK;
  }

  
  
  
  if (mFinishing)
    return NS_OK;

  
  
  
  
  
  
  if (mInDecoder) {
    nsRefPtr<imgDecodeRequestor> requestor = new imgDecodeRequestor(*this);
    return NS_DispatchToCurrentThread(requestor);
  }


  
  
  if (mDecoder &&
      (mDecoder->IsSizeDecode() || mDecoder->GetDecodeFlags() != mFrameDecodeFlags))
  {
    rv = ShutdownDecoder(eShutdownIntent_NotNeeded);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;

  
  
  
  if (!mDecoded && !mInDecoder && mHasSourceData && aDecodeType == SOMEWHAT_SYNCHRONOUS) {
    SAMPLE_LABEL_PRINTF("RasterImage", "DecodeABitOf", "%s", GetURIString().get());
    DecodeWorker::Singleton()->DecodeABitOf(this);
    return NS_OK;
  }

  
  
  
  DecodeWorker::Singleton()->RequestDecode(this);

  return NS_OK;
}


nsresult
RasterImage::SyncDecode()
{
  nsresult rv;

  SAMPLE_LABEL_PRINTF("RasterImage", "SyncDecode", "%s", GetURIString().get());;

  
  if (mDecoded)
    return NS_OK;

  
  if (!StoringSourceData())
    return NS_OK;

  
  
  
  
  NS_ABORT_IF_FALSE(!mInDecoder, "Yikes, forcing sync in reentrant call!");

  
  
  if (mDecoder &&
      (mDecoder->IsSizeDecode() || mDecoder->GetDecodeFlags() != mFrameDecodeFlags))
  {
    rv = ShutdownDecoder(eShutdownIntent_NotNeeded);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    NS_ABORT_IF_FALSE(mFrames.IsEmpty(), "Trying to decode to non-empty frame-array");
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                      mSourceData.Length() - mBytesDecoded);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  
  
  
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = true;
  mDecoder->FlushInvalidations();
  mInDecoder = false;

  
  if (mDecoder && IsDecodeFinished()) {
    rv = ShutdownDecoder(eShutdownIntent_Done);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  return mError ? NS_ERROR_FAILURE : NS_OK;
}

static inline bool
IsDownscale(const gfxSize& scale)
{
  if (scale.width > 1.0)
    return false;
  if (scale.height > 1.0)
    return false;
  if (scale.width == 1.0 && scale.height == 1.0)
    return false;

  return true;
}

bool
RasterImage::CanScale(gfxPattern::GraphicsFilter aFilter,
                      gfxSize aScale)
{

#ifdef MOZ_ENABLE_SKIA
  
  
  if (gHQDownscaling && aFilter == gfxPattern::FILTER_GOOD &&
      !mAnim && mDecoded && !mMultipart && IsDownscale(aScale)) {
    gfxFloat factor = gHQDownscalingMinFactor / 1000.0;
    return (aScale.width < factor || aScale.height < factor);
  }
#endif

  return false;
}

void
RasterImage::ScalingStart(ScaleRequest* request)
{
  MOZ_ASSERT(request);
  mScaleResult.scale = request->scale;
  mScaleResult.status = SCALE_PENDING;
  mScaleRequest = request;
}

void
RasterImage::ScalingDone(ScaleRequest* request, ScaleStatus status)
{
  MOZ_ASSERT(status == SCALE_DONE || status == SCALE_INVALID);
  MOZ_ASSERT(request);

  if (status == SCALE_DONE) {
    MOZ_ASSERT(request->done);

    if (mStatusTracker) {
      imgFrame *scaledFrame = request->dstFrame.get();
      scaledFrame->ImageUpdated(scaledFrame->GetRect());
      mStatusTracker->GetDecoderObserver()->FrameChanged(&request->srcRect);
    }

    mScaleResult.status = SCALE_DONE;
    mScaleResult.frame = request->dstFrame;
    mScaleResult.scale = request->scale;
  } else {
    mScaleResult.status = SCALE_INVALID;
    mScaleResult.frame = nullptr;
  }

  
  
  
  if (mScaleRequest == request) {
    mScaleRequest = nullptr;
  }
}

void
RasterImage::DrawWithPreDownscaleIfNeeded(imgFrame *aFrame,
                                          gfxContext *aContext,
                                          gfxPattern::GraphicsFilter aFilter,
                                          const gfxMatrix &aUserSpaceToImageSpace,
                                          const gfxRect &aFill,
                                          const nsIntRect &aSubimage)
{
  imgFrame *frame = aFrame;
  nsIntRect framerect = frame->GetRect();
  gfxMatrix userSpaceToImageSpace = aUserSpaceToImageSpace;
  gfxMatrix imageSpaceToUserSpace = aUserSpaceToImageSpace;
  imageSpaceToUserSpace.Invert();
  gfxSize scale = imageSpaceToUserSpace.ScaleFactors(true);
  nsIntRect subimage = aSubimage;

  if (CanScale(aFilter, scale)) {
    
    
    
    
    
    
    
    
    if (mScaleResult.status == SCALE_DONE && mScaleResult.scale == scale) {
      frame = mScaleResult.frame;
      userSpaceToImageSpace.Multiply(gfxMatrix().Scale(scale.width, scale.height));

      
      
      
      subimage.ScaleRoundOut(scale.width, scale.height);
    }

    
    
    else if (!(mScaleResult.status == SCALE_PENDING && mScaleResult.scale == scale) &&
             mLockCount == 1) {
      
      if (mScaleRequest) {
        mScaleRequest->stopped = true;
      }

      nsRefPtr<ScaleRunner> runner = new ScaleRunner(this, scale, frame);
      if (runner->IsOK()) {
        if (!sScaleWorkerThread) {
          NS_NewNamedThread("Image Scaler", getter_AddRefs(sScaleWorkerThread));
          ClearOnShutdown(&sScaleWorkerThread);
        }

        sScaleWorkerThread->Dispatch(runner, NS_DISPATCH_NORMAL);
      }
    }
  }

  nsIntMargin padding(framerect.x, framerect.y,
                      mSize.width - framerect.XMost(),
                      mSize.height - framerect.YMost());

  frame->Draw(aContext, aFilter, userSpaceToImageSpace, aFill, padding, subimage);
}










NS_IMETHODIMP
RasterImage::Draw(gfxContext *aContext,
                  gfxPattern::GraphicsFilter aFilter,
                  const gfxMatrix &aUserSpaceToImageSpace,
                  const gfxRect &aFill,
                  const nsIntRect &aSubimage,
                  const nsIntSize& ,
                  const SVGImageContext* ,
                  uint32_t aFlags)
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  
  
  
  if ((aFlags & DECODE_FLAGS_MASK) != DECODE_FLAGS_DEFAULT)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aContext);

  
  if (mFrameDecodeFlags != DECODE_FLAGS_DEFAULT) {
    if (!CanForciblyDiscard())
      return NS_ERROR_NOT_AVAILABLE;
    ForceDiscard();

    mFrameDecodeFlags = DECODE_FLAGS_DEFAULT;
  }

  
  
  
  
  
  
  if (DiscardingActive()) {
    DiscardTracker::Reset(&mDiscardTrackerNode);
  }

  
  
  
  
  if (mLockCount == 0 || (mAnim && mAnimationConsumers == 0)) {
    if (mStatusTracker)
      mStatusTracker->GetDecoderObserver()->OnUnlockedDraw();
  }

  
  if (!mDecoded && mHasSourceData) {
      mDrawStartTime = TimeStamp::Now();

      
      
      DecodeWorker::Singleton()->MarkAsASAP(this);
  }

  
  if (aFlags & FLAG_SYNC_DECODE) {
    nsresult rv = SyncDecode();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  imgFrame *frame = GetCurrentDrawableImgFrame();
  if (!frame) {
    return NS_OK; 
  }

  DrawWithPreDownscaleIfNeeded(frame, aContext, aFilter, aUserSpaceToImageSpace, aFill, aSubimage);

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
  if (mError)
    return NS_ERROR_FAILURE;

  
  DiscardTracker::Remove(&mDiscardTrackerNode);

  
  mLockCount++;

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::UnlockImage()
{
  if (mError)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mLockCount > 0,
                    "Calling UnlockImage with mLockCount == 0!");
  if (mLockCount == 0)
    return NS_ERROR_ABORT;

  
  NS_ABORT_IF_FALSE(!DiscardingActive(), "Locked, but discarding activated");

  
  mLockCount--;

  
  
  
  
  if (mHasBeenDecoded && mDecoder &&
      mLockCount == 0 && CanForciblyDiscard()) {
    PR_LOG(GetCompressedImageAccountingLog(), PR_LOG_DEBUG,
           ("RasterImage[0x%p] canceling decode because image "
            "is now unlocked.", this));
    ShutdownDecoder(eShutdownIntent_NotNeeded);
    ForceDiscard();
    return NS_OK;
  }

  
  
  if (CanDiscard()) {
    nsresult rv = DiscardTracker::Reset(&mDiscardTrackerNode);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  return NS_OK;
}



NS_IMETHODIMP
RasterImage::RequestDiscard()
{
  if (CanDiscard()) {
    ForceDiscard();
  }

  return NS_OK;
}


nsresult
RasterImage::DecodeSomeData(uint32_t aMaxBytes)
{
  
  NS_ABORT_IF_FALSE(mDecoder, "trying to decode without decoder!");

  
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;


  
  uint32_t bytesToDecode = std::min(aMaxBytes,
                                  mSourceData.Length() - mBytesDecoded);
  nsresult rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                               bytesToDecode);

  return rv;
}





bool
RasterImage::IsDecodeFinished()
{
  
  NS_ABORT_IF_FALSE(mDecoder, "Can't call IsDecodeFinished() without decoder!");

  
  bool decodeFinished = false;

  
  
  NS_ABORT_IF_FALSE(StoringSourceData(),
                    "just shut down on SourceDataComplete!");

  
  if (mDecoder->IsSizeDecode()) {
    if (mHasSize)
      decodeFinished = true;
  }
  else {
    if (mDecoded)
      decodeFinished = true;
  }

  
  
  
  
  
  if (mHasSourceData && (mBytesDecoded == mSourceData.Length()))
    decodeFinished = true;

  return decodeFinished;
}


void
RasterImage::DoError()
{
  
  if (mError)
    return;

  
  if (mDecoder)
    ShutdownDecoder(eShutdownIntent_Error);

  
  mError = true;

  
  LOG_CONTAINER_ERROR;
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
  return ImageResource::ShouldAnimate() && mFrames.Length() >= 2 &&
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

 RasterImage::DecodeWorker*
RasterImage::DecodeWorker::Singleton()
{
  if (!sSingleton) {
    sSingleton = new DecodeWorker();
    ClearOnShutdown(&sSingleton);
  }

  return sSingleton;
}

void
RasterImage::DecodeWorker::MarkAsASAP(RasterImage* aImg)
{
  DecodeRequest* request = &aImg->mDecodeRequest;

  
  if (request->mIsASAP) {
    return;
  }

  request->mIsASAP = true;

  if (request->isInList()) {
    
    
    
    request->removeFrom(mNormalDecodeRequests);
    mASAPDecodeRequests.insertBack(request);

    
    
    
    
    
    
    
    
    MOZ_ASSERT(mPendingInEventLoop);
  }
}

void
RasterImage::DecodeWorker::AddDecodeRequest(DecodeRequest* aRequest)
{
  if (aRequest->isInList()) {
    
    
    return;
  }

  if (aRequest->mIsASAP) {
    mASAPDecodeRequests.insertBack(aRequest);
  } else {
    mNormalDecodeRequests.insertBack(aRequest);
  }
}

void
RasterImage::DecodeWorker::RequestDecode(RasterImage* aImg)
{
  AddDecodeRequest(&aImg->mDecodeRequest);
  EnsurePendingInEventLoop();
}

void
RasterImage::DecodeWorker::DecodeABitOf(RasterImage* aImg)
{
  DecodeSomeOfImage(aImg);

  
  
  if (aImg->mDecoder &&
      !aImg->mError &&
      !aImg->IsDecodeFinished() &&
      aImg->mSourceData.Length() > aImg->mBytesDecoded) {
    RequestDecode(aImg);
  }
}

void
RasterImage::DecodeWorker::EnsurePendingInEventLoop()
{
  if (!mPendingInEventLoop) {
    mPendingInEventLoop = true;
    NS_DispatchToCurrentThread(this);
  }
}

void
RasterImage::DecodeWorker::StopDecoding(RasterImage* aImg)
{
  DecodeRequest* request = &aImg->mDecodeRequest;
  if (request->isInList()) {
    request->remove();
  }
  request->mDecodeTime = TimeDuration(0);
  request->mIsASAP = false;
}

NS_IMETHODIMP
RasterImage::DecodeWorker::Run()
{
  
  
  mPendingInEventLoop = false;

  TimeStamp eventStart = TimeStamp::Now();

  
  do {
    
    
    
    DecodeRequest* request = mASAPDecodeRequests.popFirst();
    if (!request)
      request = mNormalDecodeRequests.popFirst();
    if (!request)
      break;

    
    
    nsRefPtr<RasterImage> image = request->mImage;
    DecodeSomeOfImage(image);

    
    
    if (image->mDecoder &&
        !image->mError &&
        !image->IsDecodeFinished() &&
        image->mSourceData.Length() > image->mBytesDecoded) {
      AddDecodeRequest(request);
    }

  } while ((TimeStamp::Now() - eventStart).ToMilliseconds() <= gMaxMSBeforeYield);

  
  if (!mASAPDecodeRequests.isEmpty() || !mNormalDecodeRequests.isEmpty()) {
    EnsurePendingInEventLoop();
  }

  Telemetry::Accumulate(Telemetry::IMAGE_DECODE_LATENCY_US,
                        uint32_t((TimeStamp::Now() - eventStart).ToMicroseconds()));

  return NS_OK;
}

nsresult
RasterImage::DecodeWorker::DecodeUntilSizeAvailable(RasterImage* aImg)
{
  return DecodeSomeOfImage(aImg, DECODE_TYPE_UNTIL_SIZE);
}

nsresult
RasterImage::DecodeWorker::DecodeSomeOfImage(
  RasterImage* aImg,
  DecodeType aDecodeType )
{
  NS_ABORT_IF_FALSE(aImg->mInitialized,
                    "Worker active for uninitialized container!");

  
  
  if (aImg->mError)
    return NS_OK;

  
  
  if (!aImg->mDecoder || aImg->mDecoded)
    return NS_OK;

  nsRefPtr<Decoder> decoderKungFuDeathGrip = aImg->mDecoder;

  uint32_t maxBytes;
  if (aImg->mDecoder->IsSizeDecode()) {
    
    
    maxBytes = aImg->mSourceData.Length();
  } else {
    
    
    
    maxBytes = gDecodeBytesAtATime;
  }

  int32_t chunkCount = 0;
  TimeStamp start = TimeStamp::Now();
  TimeStamp deadline = start + TimeDuration::FromMilliseconds(gMaxMSBeforeYield);

  
  
  
  
  
  while (aImg->mSourceData.Length() > aImg->mBytesDecoded &&
         !aImg->IsDecodeFinished() &&
         !(aDecodeType == DECODE_TYPE_UNTIL_SIZE && aImg->mHasSize)) {
    chunkCount++;
    nsresult rv = aImg->DecodeSomeData(maxBytes);
    if (NS_FAILED(rv)) {
      aImg->DoError();
      return rv;
    }

    
    
    if (TimeStamp::Now() >= deadline)
      break;
  }

  aImg->mDecodeRequest.mDecodeTime += (TimeStamp::Now() - start);

  if (chunkCount && !aImg->mDecoder->IsSizeDecode()) {
    Telemetry::Accumulate(Telemetry::IMAGE_DECODE_CHUNKS, chunkCount);
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (aDecodeType != DECODE_TYPE_UNTIL_SIZE &&
      !aImg->mDecoder->HasError() &&
      !aImg->mHasSourceData) {
    aImg->mInDecoder = true;
    aImg->mDecoder->FlushInvalidations();
    aImg->mInDecoder = false;
  }

  
  if (aImg->mDecoder && aImg->IsDecodeFinished()) {

    
    DecodeRequest* request = &aImg->mDecodeRequest;
    if (!aImg->mDecoder->IsSizeDecode()) {
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_TIME,
                            int32_t(request->mDecodeTime.ToMicroseconds()));

      
      
      Telemetry::ID id = aImg->mDecoder->SpeedHistogram();
      if (id < Telemetry::HistogramCount) {
          int32_t KBps = int32_t(request->mImage->mBytesDecoded /
                                 (1024 * request->mDecodeTime.ToSeconds()));
          Telemetry::Accumulate(id, KBps);
      }
    }

    nsresult rv = aImg->ShutdownDecoder(RasterImage::eShutdownIntent_Done);
    if (NS_FAILED(rv)) {
      aImg->DoError();
      return rv;
    }
  }

  return NS_OK;
}

} 
} 
