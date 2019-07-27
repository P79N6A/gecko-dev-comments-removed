





#include "ImageLogging.h"

#include "RasterImage.h"

#include "base/histogram.h"
#include "gfxPlatform.h"
#include "nsComponentManagerUtils.h"
#include "imgDecoderObserver.h"
#include "nsError.h"
#include "Decoder.h"
#include "nsAutoPtr.h"
#include "prenv.h"
#include "prsystem.h"
#include "ImageContainer.h"
#include "ImageRegion.h"
#include "Layers.h"
#include "nsPresContext.h"
#include "nsIThreadPool.h"
#include "nsXPCOMCIDInternal.h"
#include "nsIObserverService.h"
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
#include "mozilla/MemoryReporting.h"
#include "mozilla/Services.h"
#include "mozilla/Preferences.h"
#include <stdint.h>
#include "mozilla/Telemetry.h"
#include "mozilla/TimeStamp.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/gfx/Scale.h"

#include "GeckoProfiler.h"
#include "gfx2DGlue.h"
#include <algorithm>

#ifdef MOZ_NUWA_PROCESS
#include "ipc/Nuwa.h"
#endif

namespace mozilla {

using namespace gfx;
using namespace layers;

namespace image {
using std::ceil;


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
static bool gMultithreadedDecoding = true;
static int32_t gDecodingThreadLimit = -1;


static uint32_t gHQUpscalingMaxSize = 20971520;



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
  Preferences::AddBoolVarCache(&gMultithreadedDecoding,
                               "image.multithreaded_decoding.enabled", true);
  Preferences::AddIntVarCache(&gDecodingThreadLimit,
                              "image.multithreaded_decoding.limit", -1);
  Preferences::AddUintVarCache(&gHQUpscalingMaxSize,
                               "image.high_quality_upscaling.max_size", 20971520);
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
  ScaleRequest(RasterImage* aImage,
               const nsIntSize& aSize,
               imgFrame* aSrcFrame)
    : dstSize(aSize)
    , dstLocked(false)
    , done(false)
    , stopped(false)
  {
    MOZ_ASSERT(!aSrcFrame->GetIsPaletted());
    MOZ_ASSERT(aSize.width > 0 && aSize.height > 0);

    weakImage = aImage;
    srcRect = aSrcFrame->GetRect();
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
      
      
      bool imgLocked = NS_SUCCEEDED(image->LockImage());
      bool srcLocked = NS_SUCCEEDED(srcFrame->LockImageData());
      srcSurface = srcFrame->GetSurface();

      dstLocked = NS_SUCCEEDED(dstFrame->LockImageData());
      dstSurface = dstFrame->GetSurface();

      success = imgLocked && srcLocked && dstLocked && srcSurface && dstSurface;

      if (success) {
        srcData = srcFrame->GetImageData();
        dstData = dstFrame->GetImageData();
        srcStride = srcFrame->GetImageBytesPerRow();
        dstStride = dstFrame->GetImageBytesPerRow();
        srcFormat = srcFrame->GetFormat();
      }

      
      
      
      if (srcLocked) {
        success = NS_SUCCEEDED(srcFrame->UnlockImageData()) && success;
      }
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
      if (DiscardingEnabled())
        dstFrame->SetDiscardable();
      success = NS_SUCCEEDED(dstFrame->UnlockImageData());
      success = success && NS_SUCCEEDED(image->UnlockImage());

      dstLocked = false;
      srcData = nullptr;
      dstData = nullptr;
      srcSurface = nullptr;
      dstSurface = nullptr;
    }
    return success;
  }

  
  WeakPtr<RasterImage> weakImage;
  nsRefPtr<imgFrame> dstFrame;
  RefPtr<SourceSurface> srcSurface;
  RefPtr<SourceSurface> dstSurface;

  
  uint8_t* srcData;
  uint8_t* dstData;
  nsIntRect srcRect;
  nsIntSize dstSize;
  uint32_t srcStride;
  uint32_t dstStride;
  SurfaceFormat srcFormat;
  bool dstLocked;
  bool done;
  
  
  
  bool stopped;
};

class DrawRunner : public nsRunnable
{
public:
  explicit DrawRunner(ScaleRequest* request)
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
  ScaleRunner(RasterImage* aImage,
              const nsIntSize& aSize,
              imgFrame* aSrcFrame)
  {
    nsAutoPtr<ScaleRequest> request(new ScaleRequest(aImage, aSize, aSrcFrame));

    
    
    request->dstFrame = new imgFrame();
    nsresult rv = request->dstFrame->Init(0, 0, request->dstSize.width, request->dstSize.height,
                                          SurfaceFormat::B8G8R8A8);

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
      request->done = gfx::Scale(request->srcData, request->srcRect.width, request->srcRect.height, request->srcStride,
                                 request->dstData, request->dstSize.width, request->dstSize.height, request->dstStride,
                                 request->srcFormat);
    } else {
      request->done = false;
    }

    
    
    nsRefPtr<DrawRunner> runner = new DrawRunner(mScaleRequest.forget());
    NS_DispatchToMainThread(runner);

    return NS_OK;
  }

  bool IsOK() const { return !!mScaleRequest; }

private:
  nsAutoPtr<ScaleRequest> mScaleRequest;
};

 StaticRefPtr<RasterImage::DecodePool> RasterImage::DecodePool::sSingleton;
static nsCOMPtr<nsIThread> sScaleWorkerThread = nullptr;

#ifndef DEBUG
NS_IMPL_ISUPPORTS(RasterImage, imgIContainer, nsIProperties)
#else
NS_IMPL_ISUPPORTS(RasterImage, imgIContainer, nsIProperties,
                  imgIContainerDebug)
#endif


RasterImage::RasterImage(imgStatusTracker* aStatusTracker,
                         ImageURL* aURI ) :
  ImageResource(aURI), 
  mSize(0,0),
  mFrameDecodeFlags(DECODE_FLAGS_DEFAULT),
  mAnim(nullptr),
  mLockCount(0),
  mDecodeCount(0),
  mRequestedSampleSize(0),
#ifdef DEBUG
  mFramesNotified(0),
#endif
  mDecodingMonitor("RasterImage Decoding Monitor"),
  mDecoder(nullptr),
  mBytesDecoded(0),
  mInDecoder(false),
  mStatusDiff(ImageStatusDiff::NoChange()),
  mNotifying(false),
  mHasSize(false),
  mDecodeOnDraw(false),
  mMultipart(false),
  mDiscardable(false),
  mHasSourceData(false),
  mDecoded(false),
  mHasBeenDecoded(false),
  mAnimationFinished(false),
  mFinishing(false),
  mInUpdateImageContainer(false),
  mWantFullDecode(false),
  mPendingError(false),
  mScaleRequest(nullptr)
{
  mStatusTrackerInit = new imgStatusTrackerInit(this, aStatusTracker);

  
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
    
    
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);
    DecodePool::StopDecoding(this);
    mDecoder = nullptr;

    
    
    
    
    if (GetNumFrames() > 0) {
      nsRefPtr<imgFrame> curframe = mFrameBlender.RawGetFrame(GetNumFrames() - 1);
      curframe->UnlockImageData();
    }
  }

  delete mAnim;
  mAnim = nullptr;

  
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

    
    
    if (mStatusTracker)
      mStatusTracker->FrameChanged(&res.dirtyRect);
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

already_AddRefed<imgFrame>
RasterImage::GetImgFrameNoDecode(uint32_t framenum)
{
  if (!mAnim) {
    NS_ASSERTION(framenum == 0, "Don't ask for a frame > 0 if we're not animated!");
    return mFrameBlender.GetFrame(0);
  }
  return mFrameBlender.GetFrame(framenum);
}

already_AddRefed<imgFrame>
RasterImage::GetImgFrame(uint32_t framenum)
{
  nsresult rv = WantDecodedFrames();
  CONTAINER_ENSURE_TRUE(NS_SUCCEEDED(rv), nullptr);
  return GetImgFrameNoDecode(framenum);
}

already_AddRefed<imgFrame>
RasterImage::GetDrawableImgFrame(uint32_t framenum)
{
  nsRefPtr<imgFrame> frame;

  if (mMultipart && framenum == GetCurrentImgFrameIndex()) {
    
    
    
    frame = mMultipartDecodedFrame;
  }

  if (!frame) {
    frame = GetImgFrame(framenum);
  }

  
  
  if (frame && frame->GetCompositingFailed())
    return nullptr;

  return frame.forget();
}

uint32_t
RasterImage::GetCurrentImgFrameIndex() const
{
  if (mAnim)
    return mAnim->GetCurrentAnimationFrameIndex();

  return 0;
}

already_AddRefed<imgFrame>
RasterImage::GetCurrentImgFrame()
{
  return GetImgFrame(GetCurrentImgFrameIndex());
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

  
  nsRefPtr<imgFrame> frame = aWhichFrame == FRAME_FIRST
                           ? GetImgFrameNoDecode(0)
                           : GetImgFrameNoDecode(GetCurrentImgFrameIndex());

  
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

  
  nsRefPtr<imgFrame> frame = aWhichFrame == FRAME_FIRST
                           ? GetImgFrameNoDecode(0)
                           : GetImgFrameNoDecode(GetCurrentImgFrameIndex());

  
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
RasterImage::GetNumFrames() const
{
  return mFrameBlender.GetNumFrames();
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

  return mFrameBlender.GetTimeoutForFrame(0);
}

TemporaryRef<SourceSurface>
RasterImage::CopyFrame(uint32_t aWhichFrame,
                       uint32_t aFlags)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return nullptr;

  if (mError)
    return nullptr;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return nullptr;

  nsresult rv;

  if (!ApplyDecodeFlags(aFlags, aWhichFrame))
    return nullptr;

  
  if (aFlags & FLAG_SYNC_DECODE) {
    rv = SyncDecode();
    CONTAINER_ENSURE_TRUE(NS_SUCCEEDED(rv), nullptr);
  }

  
  
  
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                        0 : GetCurrentImgFrameIndex();
  nsRefPtr<imgFrame> frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    return nullptr;
  }

  
  

  IntSize size(mSize.width, mSize.height);
  RefPtr<DataSourceSurface> surf =
    Factory::CreateDataSourceSurface(size, SurfaceFormat::B8G8R8A8);
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

  nsIntRect intframerect = frame->GetRect();
  Rect rect(intframerect.x, intframerect.y,
            intframerect.width, intframerect.height);
  if (frame->IsSinglePixel()) {
    target->FillRect(rect, ColorPattern(frame->SinglePixelColor()),
                     DrawOptions(1.0f, CompositionOp::OP_SOURCE));
  } else {
    RefPtr<SourceSurface> srcsurf = frame->GetSurface();
    Rect srcrect(0, 0, intframerect.width, intframerect.height);
    target->DrawSurface(srcsurf, srcrect, rect);
  }

  target->Flush();
  surf->Unmap();

  return surf;
}




NS_IMETHODIMP_(TemporaryRef<SourceSurface>)
RasterImage::GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags)
{
  MOZ_ASSERT(aWhichFrame <= FRAME_MAX_VALUE);

  if (aWhichFrame > FRAME_MAX_VALUE)
    return nullptr;

  if (mError)
    return nullptr;

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return nullptr;

  if (!ApplyDecodeFlags(aFlags, aWhichFrame))
    return nullptr;

  
  if (aFlags & FLAG_SYNC_DECODE) {
    nsresult rv = SyncDecode();
    CONTAINER_ENSURE_TRUE(NS_SUCCEEDED(rv), nullptr);
  }

  
  
  
  uint32_t frameIndex = (aWhichFrame == FRAME_FIRST) ?
                          0 : GetCurrentImgFrameIndex();
  nsRefPtr<imgFrame> frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    return nullptr;
  }

  RefPtr<SourceSurface> framesurf;

  
  
  nsIntRect framerect = frame->GetRect();
  if (framerect.x == 0 && framerect.y == 0 &&
      framerect.width == mSize.width &&
      framerect.height == mSize.height) {
    framesurf = frame->GetSurface();
    if (!framesurf && !frame->IsSinglePixel()) {
      
      if (!(aFlags & FLAG_SYNC_DECODE))
        return nullptr;

      
      
      
      
      
      
      
      ForceDiscard();
      return GetFrame(aWhichFrame, aFlags);
    }
  }

  
  
  if (!framesurf) {
    framesurf = CopyFrame(aWhichFrame, aFlags);
  }

  return framesurf;
}

already_AddRefed<layers::Image>
RasterImage::GetCurrentImage()
{
  if (!mDecoded) {
    
    
    RequestDecodeCore(ASYNCHRONOUS);
    return nullptr;
  }

  RefPtr<SourceSurface> surface = GetFrame(FRAME_CURRENT, FLAG_NONE);
  if (!surface) {
    
    
    
    
    ForceDiscard();
    RequestDecodeCore(ASYNCHRONOUS);
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

  if (IsUnlocked() && mStatusTracker) {
    mStatusTracker->OnUnlockedDraw();
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
  
  
  if (CanForciblyDiscardAndRedecode()) {
    mImageContainerCache = mImageContainer;
    mImageContainer = nullptr;
  }

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
RasterImage::HeapSizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  
  size_t n = mSourceData.SizeOfExcludingThis(aMallocSizeOf);
  if (n == 0) {
    n = mSourceData.Length();
  }
  return n;
}

size_t
RasterImage::SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation aLocation,
                                                     MallocSizeOf aMallocSizeOf) const
{
  size_t n = mFrameBlender.SizeOfDecodedWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);

  if (mScaleResult.status == SCALE_DONE) {
    n += mScaleResult.frame->SizeOfExcludingThisWithComputedFallbackIfHeap(aLocation, aMallocSizeOf);
  }

  return n;
}

size_t
RasterImage::HeapSizeOfDecodedWithComputedFallback(MallocSizeOf aMallocSizeOf) const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation::IN_PROCESS_HEAP,
                                                 aMallocSizeOf);
}

size_t
RasterImage::NonHeapSizeOfDecoded() const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation::IN_PROCESS_NONHEAP,
                                                 nullptr);
}

size_t
RasterImage::OutOfProcessSizeOfDecoded() const
{
  return SizeOfDecodedWithComputedFallbackIfHeap(gfxMemoryLocation::OUT_OF_PROCESS,
                                                 nullptr);
}

void
RasterImage::EnsureAnimExists()
{
  if (!mAnim) {

    
    mAnim = new FrameAnimator(mFrameBlender, mAnimationMode);

    
    
    
    
    
    
    
    
    
    LockImage();

    
    nsRefPtr<imgStatusTracker> statusTracker = CurrentStatusTracker();
    statusTracker->RecordImageIsAnimated();
  }
}

nsresult
RasterImage::InternalAddFrameHelper(uint32_t framenum, imgFrame *aFrame,
                                    uint8_t **imageData, uint32_t *imageLength,
                                    uint32_t **paletteData, uint32_t *paletteLength,
                                    imgFrame** aRetFrame)
{
  NS_ABORT_IF_FALSE(framenum <= GetNumFrames(), "Invalid frame index!");
  if (framenum > GetNumFrames())
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<imgFrame> frame(aFrame);

  
  
  frame->LockImageData();

  if (paletteData && paletteLength)
    frame->GetPaletteData(paletteData, paletteLength);

  frame->GetImageData(imageData, imageLength);

  mFrameBlender.InsertFrame(framenum, frame);

  frame.forget(aRetFrame);
  return NS_OK;
}

nsresult
RasterImage::InternalAddFrame(uint32_t framenum,
                              int32_t aX, int32_t aY,
                              int32_t aWidth, int32_t aHeight,
                              SurfaceFormat aFormat,
                              uint8_t aPaletteDepth,
                              uint8_t **imageData,
                              uint32_t *imageLength,
                              uint32_t **paletteData,
                              uint32_t *paletteLength,
                              imgFrame** aRetFrame)
{
  
  
  
  NS_ABORT_IF_FALSE(mDecoder, "Only decoders may add frames!");

  NS_ABORT_IF_FALSE(framenum <= GetNumFrames(), "Invalid frame index!");
  if (framenum > GetNumFrames())
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<imgFrame> frame(new imgFrame());

  nsresult rv = frame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  if (!(mSize.width > 0 && mSize.height > 0))
    NS_WARNING("Shouldn't call InternalAddFrame with zero size");
  if (!NS_SUCCEEDED(rv))
    NS_WARNING("imgFrame::Init should succeed");
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  if (GetNumFrames() > 0) {
    nsRefPtr<imgFrame> prevframe = mFrameBlender.RawGetFrame(GetNumFrames() - 1);
    prevframe->UnlockImageData();
  }

  if (GetNumFrames() == 0) {
    return InternalAddFrameHelper(framenum, frame, imageData, imageLength,
                                  paletteData, paletteLength, aRetFrame);
  }

  if (GetNumFrames() == 1) {
    
    EnsureAnimExists();

    
    
    
    nsRefPtr<imgFrame> firstFrame = mFrameBlender.RawGetFrame(0);
    int32_t frameDisposalMethod = firstFrame->GetFrameDisposalMethod();
    if (frameDisposalMethod == FrameBlender::kDisposeClear ||
        frameDisposalMethod == FrameBlender::kDisposeRestorePrevious)
      mAnim->SetFirstFrameRefreshArea(firstFrame->GetRect());
  }

  
  
  
  mAnim->UnionFirstFrameRefreshArea(frame->GetRect());

  rv = InternalAddFrameHelper(framenum, frame, imageData, imageLength,
                              paletteData, paletteLength, aRetFrame);

  return rv;
}

bool
RasterImage::ApplyDecodeFlags(uint32_t aNewFlags, uint32_t aWhichFrame)
{
  if (mFrameDecodeFlags == (aNewFlags & DECODE_FLAGS_MASK))
    return true; 

  if (mDecoded) {
    
    
    
    uint32_t currentNonAlphaFlags =
      (mFrameDecodeFlags & DECODE_FLAGS_MASK) & ~FLAG_DECODE_NO_PREMULTIPLY_ALPHA;
    uint32_t newNonAlphaFlags =
      (aNewFlags & DECODE_FLAGS_MASK) & ~FLAG_DECODE_NO_PREMULTIPLY_ALPHA;
    if (currentNonAlphaFlags == newNonAlphaFlags && FrameIsOpaque(aWhichFrame)) {
      return true;
    }

    
    
    
    if (!(aNewFlags & FLAG_SYNC_DECODE))
      return false;
    if (!CanForciblyDiscardAndRedecode())
      return false;
    ForceDiscard();
  }

  mFrameDecodeFlags = aNewFlags & DECODE_FLAGS_MASK;
  return true;
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

  
  if (!mMultipart && mHasSize &&
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

  mFrameBlender.SetSize(mSize);

  return NS_OK;
}

nsresult
RasterImage::EnsureFrame(uint32_t aFrameNum, int32_t aX, int32_t aY,
                         int32_t aWidth, int32_t aHeight,
                         SurfaceFormat aFormat,
                         uint8_t aPaletteDepth,
                         uint8_t **imageData, uint32_t *imageLength,
                         uint32_t **paletteData, uint32_t *paletteLength,
                         imgFrame** aRetFrame)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(imageData);
  NS_ENSURE_ARG_POINTER(imageLength);
  NS_ENSURE_ARG_POINTER(aRetFrame);
  NS_ABORT_IF_FALSE(aFrameNum <= GetNumFrames(), "Invalid frame index!");

  if (aPaletteDepth > 0) {
    NS_ENSURE_ARG_POINTER(paletteData);
    NS_ENSURE_ARG_POINTER(paletteLength);
  }

  if (aFrameNum > GetNumFrames())
    return NS_ERROR_INVALID_ARG;

  
  if (aFrameNum == GetNumFrames()) {
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat,
                            aPaletteDepth, imageData, imageLength,
                            paletteData, paletteLength, aRetFrame);
  }

  nsRefPtr<imgFrame> frame = mFrameBlender.RawGetFrame(aFrameNum);
  if (!frame) {
    return InternalAddFrame(aFrameNum, aX, aY, aWidth, aHeight, aFormat,
                            aPaletteDepth, imageData, imageLength,
                            paletteData, paletteLength, aRetFrame);
  }

  
  nsIntRect rect = frame->GetRect();
  if (rect.x == aX && rect.y == aY && rect.width == aWidth &&
      rect.height == aHeight && frame->GetFormat() == aFormat &&
      frame->GetPaletteDepth() == aPaletteDepth) {
    frame->GetImageData(imageData, imageLength);
    if (paletteData) {
      frame->GetPaletteData(paletteData, paletteLength);
    }

    
    if (*imageData && paletteData && *paletteData) {
      frame.forget(aRetFrame);
      return NS_OK;
    }
    if (*imageData && !paletteData) {
      frame.forget(aRetFrame);
      return NS_OK;
    }
  }

  

  
  
  frame->UnlockImageData();

  mFrameBlender.RemoveFrame(aFrameNum);
  nsRefPtr<imgFrame> newFrame(new imgFrame());
  nsresult rv = newFrame->Init(aX, aY, aWidth, aHeight, aFormat, aPaletteDepth);
  NS_ENSURE_SUCCESS(rv, rv);
  return InternalAddFrameHelper(aFrameNum, newFrame, imageData, imageLength,
                                paletteData, paletteLength, aRetFrame);
}

nsresult
RasterImage::EnsureFrame(uint32_t aFramenum, int32_t aX, int32_t aY,
                         int32_t aWidth, int32_t aHeight,
                         SurfaceFormat aFormat,
                         uint8_t** imageData, uint32_t* imageLength,
                         imgFrame** aFrame)
{
  return EnsureFrame(aFramenum, aX, aY, aWidth, aHeight, aFormat,
                      0, imageData, imageLength,
                      nullptr,
                      nullptr,
                     aFrame);
}

nsresult
RasterImage::SetFrameAsNonPremult(uint32_t aFrameNum, bool aIsNonPremult)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(aFrameNum < GetNumFrames(), "Invalid frame index!");
  if (aFrameNum >= GetNumFrames())
    return NS_ERROR_INVALID_ARG;

  nsRefPtr<imgFrame> frame = mFrameBlender.RawGetFrame(aFrameNum);
  NS_ABORT_IF_FALSE(frame, "Calling SetFrameAsNonPremult on frame that doesn't exist!");
  NS_ENSURE_TRUE(frame, NS_ERROR_FAILURE);

  frame->SetAsNonPremult(aIsNonPremult);

  return NS_OK;
}

nsresult
RasterImage::DecodingComplete()
{
  MOZ_ASSERT(NS_IsMainThread());

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

  
  
  
  
  
  if ((GetNumFrames() == 1) && !mMultipart) {
    
    
    
    nsRefPtr<imgFrame> firstFrame = mFrameBlender.RawGetFrame(0);
    if (DiscardingEnabled() && CanForciblyDiscard()) {
      firstFrame->SetDiscardable();
    }
    rv = firstFrame->Optimize();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  if (mMultipart) {
    if (GetNumFrames() == 1) {
      mMultipartDecodedFrame = mFrameBlender.SwapFrame(GetCurrentFrameIndex(),
                                                       mMultipartDecodedFrame);
    } else {
      
      
      
      
      mMultipartDecodedFrame = nullptr;
    }
  }

  if (mAnim) {
    mAnim->SetDoneDecoding(true);
  }

  return NS_OK;
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

  EnsureAnimExists();

  nsRefPtr<imgFrame> currentFrame = GetCurrentImgFrame();
  
  if (currentFrame && mFrameBlender.GetTimeoutForFrame(GetCurrentImgFrameIndex()) < 0) {
    mAnimationFinished = true;
    return NS_ERROR_ABORT;
  }

  if (mAnim) {
    
    
    mAnim->InitAnimationFrameTimeIfNecessary();
  }

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

  if (mAnimationMode == kDontAnimMode ||
      !mAnim || mAnim->GetCurrentAnimationFrameIndex() == 0)
    return NS_OK;

  mAnimationFinished = false;

  if (mAnimating)
    StopAnimation();

  mFrameBlender.ResetAnimation();
  mAnim->ResetAnimation();

  UpdateImageContainer();

  
  

  
  if (mStatusTracker) {
    nsIntRect rect = mAnim->GetFirstFrameRefreshArea();
    mStatusTracker->FrameChanged(&rect);
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
    
    mFrameBlender.SetLoopCount(aLoopCount);
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

  
  NS_ABORT_IF_FALSE(!mInDecoder, "Re-entrant call to AddSourceData!");

  
  
  if (mDecoded) {
    return NS_OK;
  }

  
  
  
  if (mMultipart && mBytesDecoded == 0) {
    
    if (mAnimating)
      StopAnimation();
    mAnimationFinished = false;
    if (mAnim) {
      delete mAnim;
      mAnim = nullptr;
    }
    
    int old_frame_count = GetNumFrames();
    if (old_frame_count > 1) {
      mFrameBlender.ClearFrames();
    }
  }

  
  
  
  if (!StoringSourceData() && mHasSize) {
    rv = WriteToDecoder(aBuffer, aCount, DECODE_SYNC);
    CONTAINER_ENSURE_SUCCESS(rv);

    
    
    
    nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
    mInDecoder = true;
    mDecoder->FlushInvalidations();
    mInDecoder = false;

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

    
    
    if (!StoringSourceData() && mDecoder) {
      nsresult rv = ShutdownDecoder(eShutdownIntent_Done);
      CONTAINER_ENSURE_SUCCESS(rv);
    }

    
    
    if (mDecoder) {
      DecodePool::Singleton()->RequestDecode(this);
    }

    
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

  
  {
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);

    nsRefPtr<imgStatusTracker> statusTracker = CurrentStatusTracker();
    statusTracker->GetDecoderObserver()->OnStopRequest(aLastPart, finalStatus);

    FinishedSomeDecoding();
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

nsresult
RasterImage::OnNewSourceData()
{
  MOZ_ASSERT(NS_IsMainThread());

  nsresult rv;

  if (mError)
    return NS_ERROR_FAILURE;

  
  NS_ABORT_IF_FALSE(mHasSourceData,
                    "Calling NewSourceData before SourceDataComplete!");
  if (!mHasSourceData)
    return NS_ERROR_ILLEGAL_VALUE;

  
  
  
  NS_ABORT_IF_FALSE(mMultipart, "NewSourceData only supported for multipart");
  if (!mMultipart)
    return NS_ERROR_ILLEGAL_VALUE;

  
  NS_ABORT_IF_FALSE(!StoringSourceData(),
                    "Shouldn't be storing source data for multipart");

  
  
  NS_ABORT_IF_FALSE(!mDecoder, "Shouldn't have a decoder in NewSourceData");

  
  NS_ABORT_IF_FALSE(mDecoded, "Should be decoded in NewSourceData");

  
  mDecoded = false;
  mHasSourceData = false;
  mHasSize = false;
  mWantFullDecode = true;
  mDecodeRequest = nullptr;

  if (mAnim) {
    mAnim->SetDoneDecoding(false);
  }

  
  rv = InitDecoder( true);
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
  MOZ_ASSERT(NS_IsMainThread());

  
  NS_ABORT_IF_FALSE(force ? CanForciblyDiscard() : CanDiscard(), "Asked to discard but can't!");

  
  NS_ABORT_IF_FALSE(!mDecoder, "Asked to discard with open decoder!");

  
  
  NS_ABORT_IF_FALSE(!mAnim, "Asked to discard for animated image!");

  
  int old_frame_count = GetNumFrames();

  
  mFrameBlender.Discard();

  
  mScaleResult.status = SCALE_INVALID;
  mScaleResult.frame = nullptr;

  
  mMultipartDecodedFrame = nullptr;

  
  mDecoded = false;

  
  if (mStatusTracker)
    mStatusTracker->OnDiscard();

  mDecodeRequest = nullptr;

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
          GetNumFrames(),
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
RasterImage::CanForciblyDiscardAndRedecode() {
  return mDiscardable &&         
         mHasSourceData &&       
         !mDecoder &&            
         !mAnim;                 
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

  
  
  
  if (GetNumFrames() > 0) {
    nsRefPtr<imgFrame> curframe = mFrameBlender.RawGetFrame(GetNumFrames() - 1);
    curframe->LockImageData();
  }

  
  if (!mDecodeRequest) {
    mDecodeRequest = new DecodeRequest(this);
  }
  MOZ_ASSERT(mDecodeRequest->mStatusTracker);
  MOZ_ASSERT(mDecodeRequest->mStatusTracker->GetDecoderObserver());
  mDecoder->SetObserver(mDecodeRequest->mStatusTracker->GetDecoderObserver());
  mDecoder->SetSizeDecode(aDoSizeDecode);
  mDecoder->SetDecodeFlags(mFrameDecodeFlags);
  if (!aDoSizeDecode) {
    
    
    
    mDecoder->NeedNewFrame(0, 0, 0, mSize.width, mSize.height,
                           SurfaceFormat::B8G8R8A8);
    mDecoder->AllocateFrame();
  }
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
  MOZ_ASSERT(NS_IsMainThread());
  mDecodingMonitor.AssertCurrentThreadIn();

  
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

  
  
  if (GetNumFrames() > 0) {
    nsRefPtr<imgFrame> curframe = mFrameBlender.RawGetFrame(GetNumFrames() - 1);
    curframe->UnlockImageData();
  }

  
  
  DecodePool::StopDecoding(this);

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

  
  
  if (!wasSizeDecode && !StoringSourceData()) {
    mSourceData.Clear();
  }

  mBytesDecoded = 0;

  return NS_OK;
}


nsresult
RasterImage::WriteToDecoder(const char *aBuffer, uint32_t aCount, DecodeStrategy aStrategy)
{
  mDecodingMonitor.AssertCurrentThreadIn();

  
  NS_ABORT_IF_FALSE(mDecoder, "Trying to write to null decoder!");

  
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = true;
  mDecoder->Write(aBuffer, aCount, aStrategy);
  mInDecoder = false;

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

  
  
  
  if (mFinishing)
    return NS_OK;

  
  
  if (mDecoder && mDecoder->NeedsNewFrame())
    return NS_OK;

  
  
  
  
  
  
  if (mInDecoder) {
    nsRefPtr<imgDecodeRequestor> requestor = new imgDecodeRequestor(*this);
    return NS_DispatchToCurrentThread(requestor);
  }

  
  if (mDecoder && mDecoder->IsSizeDecode()) {
    nsresult rv = DecodePool::Singleton()->DecodeUntilSizeAvailable(this);
    CONTAINER_ENSURE_SUCCESS(rv);

    
    
    if (!mHasSize) {
      mWantFullDecode = true;
      return NS_OK;
    }
  }

  
  if (mDecodeRequest &&
      mDecodeRequest->mRequestStatus == DecodeRequest::REQUEST_WORK_DONE &&
      aDecodeType == SYNCHRONOUS_NOTIFY) {
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);
    nsresult rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  if (mDecoded) {
    return NS_OK;
  }

  
  
  
  if (mDecoder && !mDecoder->IsSizeDecode() && mBytesDecoded &&
      aDecodeType != SYNCHRONOUS_NOTIFY_AND_SOME_DECODE) {
    return NS_OK;
  }

  ReentrantMonitorAutoEnter lock(mDecodingMonitor);

  
  
  
  if (mBytesDecoded > mSourceData.Length())
    return NS_OK;

  
  

  
  if (mDecodeRequest &&
      mDecodeRequest->mRequestStatus == DecodeRequest::REQUEST_WORK_DONE &&
      aDecodeType != ASYNCHRONOUS) {
    nsresult rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  
  
  
  if (mDecoded) {
    return NS_OK;
  }

  
  
  if (mDecoder && !mDecoder->IsSizeDecode() && mBytesDecoded) {
    return NS_OK;
  }

  
  
  if (mDecoder && mDecoder->GetDecodeFlags() != mFrameDecodeFlags) {
    nsresult rv = FinishedSomeDecoding(eShutdownIntent_NotNeeded);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  if (!mDecoder) {
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);

    rv = FinishedSomeDecoding();
    CONTAINER_ENSURE_SUCCESS(rv);

    MOZ_ASSERT(mDecoder);
  }

  
  if (mHasSourceData && mBytesDecoded == mSourceData.Length())
    return NS_OK;

  
  
  
  if (!mDecoded && !mInDecoder && mHasSourceData && aDecodeType == SYNCHRONOUS_NOTIFY_AND_SOME_DECODE) {
    PROFILER_LABEL_PRINTF("RasterImage", "DecodeABitOf",
      js::ProfileEntry::Category::GRAPHICS, "%s", GetURIString().get());

    DecodePool::Singleton()->DecodeABitOf(this, DECODE_SYNC);
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

  
  
  
  
  NS_ABORT_IF_FALSE(!mInDecoder, "Yikes, forcing sync in reentrant call!");

  if (mDecodeRequest) {
    
    if (mDecodeRequest->mRequestStatus == DecodeRequest::REQUEST_WORK_DONE) {
      nsresult rv = FinishedSomeDecoding();
      CONTAINER_ENSURE_SUCCESS(rv);
    }
  }

  nsresult rv;

  
  
  if (mDecoded)
    return NS_OK;

  
  
  
  if (mBytesDecoded > mSourceData.Length())
    return NS_OK;

  
  
  if (mDecoder && mDecoder->GetDecodeFlags() != mFrameDecodeFlags) {
    nsresult rv = FinishedSomeDecoding(eShutdownIntent_NotNeeded);
    CONTAINER_ENSURE_SUCCESS(rv);

    if (mDecoded) {
      
      
      
      if (!CanForciblyDiscardAndRedecode())
        return NS_ERROR_NOT_AVAILABLE;
      ForceDiscard();
    }
  }

  
  
  if (mDecoder && mDecoder->NeedsNewFrame()) {
    mDecoder->AllocateFrame();
    mDecodeRequest->mAllocatedNewFrame = true;
  }

  
  if (!mDecoder) {
    rv = InitDecoder( false);
    CONTAINER_ENSURE_SUCCESS(rv);
  }

  
  rv = DecodeSomeData(mSourceData.Length() - mBytesDecoded, DECODE_SYNC);
  CONTAINER_ENSURE_SUCCESS(rv);

  
  
  
  
  nsRefPtr<Decoder> kungFuDeathGrip = mDecoder;
  mInDecoder = true;
  mDecoder->FlushInvalidations();
  mInDecoder = false;

  rv = FinishedSomeDecoding();
  CONTAINER_ENSURE_SUCCESS(rv);
  
  
  if (mDecoder) {
    DecodePool::Singleton()->RequestDecode(this);
  }

  
  return mError ? NS_ERROR_FAILURE : NS_OK;
}

bool
RasterImage::CanQualityScale(const gfx::Size& scale)
{
  
  if (scale.width == 1.0 && scale.height == 1.0)
    return false;

  
  if (scale.width > 1.0 || scale.height > 1.0) {
    uint32_t scaled_size = static_cast<uint32_t>(mSize.width * mSize.height * scale.width * scale.height);
    if (scaled_size > gHQUpscalingMaxSize)
      return false;
  }

  return true;
}

bool
RasterImage::CanScale(GraphicsFilter aFilter, gfx::Size aScale, uint32_t aFlags)
{

#ifdef MOZ_ENABLE_SKIA
  
  
  
  
  if (gHQDownscaling && aFilter == GraphicsFilter::FILTER_GOOD &&
      !mAnim && mDecoded && !mMultipart && CanQualityScale(aScale) &&
      (aFlags & imgIContainer::FLAG_HIGH_QUALITY_SCALING)) {
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
  mScaleResult.scaledSize = request->dstSize;
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

    mScaleResult.status = SCALE_DONE;
    mScaleResult.frame = request->dstFrame.forget();
    mScaleResult.scaledSize = request->dstSize;

    mScaleResult.frame->ImageUpdated(mScaleResult.frame->GetRect());

    if (mStatusTracker) {
      mStatusTracker->FrameChanged(&request->srcRect);
    }
  } else {
    mScaleResult.status = SCALE_INVALID;
    mScaleResult.frame = nullptr;
  }

  
  
  
  if (mScaleRequest == request) {
    mScaleRequest = nullptr;
  }
}

void
RasterImage::RequestScale(imgFrame* aFrame, nsIntSize aSize)
{
  
  
  if (mLockCount != 1) {
    return;
  }

  
  if (NS_FAILED(aFrame->LockImageData())) {
    aFrame->UnlockImageData();
    return;
  }

  
  if (mScaleRequest) {
    mScaleRequest->stopped = true;
  }

  nsRefPtr<ScaleRunner> runner = new ScaleRunner(this, aSize, aFrame);
  if (runner->IsOK()) {
    if (!sScaleWorkerThread) {
      NS_NewNamedThread("Image Scaler", getter_AddRefs(sScaleWorkerThread));
      ClearOnShutdown(&sScaleWorkerThread);
    }

    sScaleWorkerThread->Dispatch(runner, NS_DISPATCH_NORMAL);
  }

  aFrame->UnlockImageData();
}

bool
RasterImage::DrawWithPreDownscaleIfNeeded(imgFrame *aFrame,
                                          gfxContext *aContext,
                                          const nsIntSize& aSize,
                                          const ImageRegion& aRegion,
                                          GraphicsFilter aFilter,
                                          uint32_t aFlags)
{
  nsRefPtr<imgFrame> frame = aFrame;
  nsIntRect framerect = frame->GetRect();

  gfxContextMatrixAutoSaveRestore saveMatrix(aContext);
  RefPtr<SourceSurface> surf;
  gfx::Size scale(double(aSize.width) / mSize.width,
                  double(aSize.height) / mSize.height);

  if (CanScale(aFilter, scale, aFlags) && !frame->IsSinglePixel()) {
    
    
    
    
    
    
    
    
    bool needScaleReq;
    if (mScaleResult.status == SCALE_DONE && mScaleResult.scaledSize == aSize) {
      
      surf = mScaleResult.frame->GetSurface();
      needScaleReq = !surf;
      if (surf) {
        frame = mScaleResult.frame;
      }
    } else {
      needScaleReq = !(mScaleResult.status == SCALE_PENDING &&
                       mScaleResult.scaledSize == aSize);
    }

    
    
    if (needScaleReq) {
      RequestScale(frame, aSize);
    }
  }

  ImageRegion region(aRegion);

  
  
  nsIntSize finalFrameSize(frame->GetRect().Size());
  if (finalFrameSize != aSize) {
    aContext->Multiply(gfxMatrix::Scaling(scale.width, scale.height));
    region.Scale(1.0 / scale.width, 1.0 / scale.height);
  }

  nsIntMargin padding(framerect.y,
                      mSize.width - framerect.XMost(),
                      mSize.height - framerect.YMost(),
                      framerect.x);

  return frame->Draw(aContext, region, padding, aFilter, aFlags);
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

  
  if (mInDecoder && (aFlags & imgIContainer::FLAG_SYNC_DECODE))
    return NS_ERROR_FAILURE;

  
  
  
  if ((aFlags & DECODE_FLAGS_MASK) != DECODE_FLAGS_DEFAULT)
    return NS_ERROR_FAILURE;

  NS_ENSURE_ARG_POINTER(aContext);

  
  
  
  
  bool haveDefaultFlags = (mFrameDecodeFlags == DECODE_FLAGS_DEFAULT);
  bool haveSafeAlphaFlags =
    (mFrameDecodeFlags == FLAG_DECODE_NO_PREMULTIPLY_ALPHA) &&
    FrameIsOpaque(FRAME_CURRENT);

  if (!(haveDefaultFlags || haveSafeAlphaFlags)) {
    if (!CanForciblyDiscardAndRedecode())
      return NS_ERROR_NOT_AVAILABLE;
    ForceDiscard();

    mFrameDecodeFlags = DECODE_FLAGS_DEFAULT;
  }

  
  
  
  
  
  
  if (DiscardingActive()) {
    DiscardTracker::Reset(&mDiscardTrackerNode);
  }


  if (IsUnlocked() && mStatusTracker) {
    mStatusTracker->OnUnlockedDraw();
  }

  
  if (!mDecoded && mHasSourceData) {
    mDrawStartTime = TimeStamp::Now();
  }

  
  if (aFlags & FLAG_SYNC_DECODE) {
    nsresult rv = SyncDecode();
    NS_ENSURE_SUCCESS(rv, rv);
  }

  uint32_t frameIndex = aWhichFrame == FRAME_FIRST ? 0
                                                   : GetCurrentImgFrameIndex();
  nsRefPtr<imgFrame> frame = GetDrawableImgFrame(frameIndex);
  if (!frame) {
    return NS_OK; 
  }

  bool drawn = DrawWithPreDownscaleIfNeeded(frame, aContext, aSize,
                                            aRegion, aFilter, aFlags);
  if (!drawn) {
    
    ForceDiscard();
    WantDecodedFrames();
    return NS_OK;
  }

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

  
  DiscardTracker::Remove(&mDiscardTrackerNode);

  
  mLockCount++;

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

  
  NS_ABORT_IF_FALSE(!DiscardingActive(), "Locked, but discarding activated");

  
  mLockCount--;

  
  
  
  
  if (mHasBeenDecoded && mDecoder &&
      mLockCount == 0 && CanForciblyDiscard()) {
    PR_LOG(GetCompressedImageAccountingLog(), PR_LOG_DEBUG,
           ("RasterImage[0x%p] canceling decode because image "
            "is now unlocked.", this));
    ReentrantMonitorAutoEnter lock(mDecodingMonitor);
    FinishedSomeDecoding(eShutdownIntent_NotNeeded);
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
  if (CanDiscard() && CanForciblyDiscardAndRedecode()) {
    ForceDiscard();
  }

  return NS_OK;
}


nsresult
RasterImage::DecodeSomeData(size_t aMaxBytes, DecodeStrategy aStrategy)
{
  
  NS_ABORT_IF_FALSE(mDecoder, "trying to decode without decoder!");

  mDecodingMonitor.AssertCurrentThreadIn();

  
  
  
  if (mDecodeRequest->mAllocatedNewFrame) {
    mDecodeRequest->mAllocatedNewFrame = false;
    nsresult rv = WriteToDecoder(nullptr, 0, aStrategy);
    if (NS_FAILED(rv) || mDecoder->NeedsNewFrame()) {
      return rv;
    }
  }

  
  if (mBytesDecoded == mSourceData.Length())
    return NS_OK;

  MOZ_ASSERT(mBytesDecoded < mSourceData.Length());

  
  size_t bytesToDecode = std::min(aMaxBytes,
                                  mSourceData.Length() - mBytesDecoded);
  nsresult rv = WriteToDecoder(mSourceData.Elements() + mBytesDecoded,
                               bytesToDecode,
                               aStrategy);

  return rv;
}





bool
RasterImage::IsDecodeFinished()
{
  
  mDecodingMonitor.AssertCurrentThreadIn();
  NS_ABORT_IF_FALSE(mDecoder, "Can't call IsDecodeFinished() without decoder!");

  
  if (mDecoder->IsSizeDecode()) {
    if (mDecoder->HasSize()) {
      return true;
    }
  } else if (mDecoder->GetDecodeDone()) {
    return true;
  }

  
  
  
  if (mDecoder->NeedsNewFrame() ||
      (mDecodeRequest && mDecodeRequest->mAllocatedNewFrame)) {
    return false;
  }

  
  
  
  
  
  
  if (mHasSourceData && (mBytesDecoded == mSourceData.Length())) {
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
    FinishedSomeDecoding(eShutdownIntent_Error);
  }

  
  mError = true;

  nsRefPtr<imgStatusTracker> statusTracker = CurrentStatusTracker();
  statusTracker->GetDecoderObserver()->OnError();

  
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
                                   eShutdownIntent aIntent,
                                   bool aDone,
                                   bool aWasSize)
{
  MOZ_ASSERT(NS_IsMainThread());

  
  if (NS_SUCCEEDED(aStatus) &&
      aIntent == eShutdownIntent_Done &&
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
RasterImage::FinishedSomeDecoding(eShutdownIntent aIntent ,
                                  DecodeRequest* aRequest )
{
  MOZ_ASSERT(NS_IsMainThread());

  mDecodingMonitor.AssertCurrentThreadIn();

  nsRefPtr<DecodeRequest> request;
  if (aRequest) {
    request = aRequest;
  } else {
    request = mDecodeRequest;
  }

  
  
  nsRefPtr<RasterImage> image(this);

  bool done = false;
  bool wasSize = false;
  nsresult rv = NS_OK;

  if (image->mDecoder) {
    if (request && request->mChunkCount && !image->mDecoder->IsSizeDecode()) {
      Telemetry::Accumulate(Telemetry::IMAGE_DECODE_CHUNKS, request->mChunkCount);
    }

    if (!image->mHasSize && image->mDecoder->HasSize()) {
      image->mDecoder->SetSizeOnImage();
    }

    
    
    if (image->IsDecodeFinished() || aIntent != eShutdownIntent_Done) {
      done = true;

      
      nsRefPtr<Decoder> decoder = image->mDecoder;

      wasSize = decoder->IsSizeDecode();

      
      if (request && !wasSize) {
        Telemetry::Accumulate(Telemetry::IMAGE_DECODE_TIME,
                              int32_t(request->mDecodeTime.ToMicroseconds()));

        
        
        Telemetry::ID id = decoder->SpeedHistogram();
        if (id < Telemetry::HistogramCount) {
          int32_t KBps = int32_t(request->mImage->mBytesDecoded /
                                 (1024 * request->mDecodeTime.ToSeconds()));
          Telemetry::Accumulate(id, KBps);
        }
      }

      
      
      rv = image->ShutdownDecoder(aIntent);
      if (NS_FAILED(rv)) {
        image->DoError();
      }
    }
  }

  ImageStatusDiff diff =
    request ? image->mStatusTracker->Difference(request->mStatusTracker)
            : image->mStatusTracker->DecodeStateAsDifference();
  image->mStatusTracker->ApplyDifference(diff);

  if (mNotifying) {
    
    
    
    
    NS_WARNING("Recursively notifying in RasterImage::FinishedSomeDecoding!");
    mStatusDiff.Combine(diff);
  } else {
    MOZ_ASSERT(mStatusDiff.IsNoChange(), "Shouldn't have an accumulated change at this point");

    while (!diff.IsNoChange()) {
      
      mNotifying = true;
      image->mStatusTracker->SyncNotifyDifference(diff);
      mNotifying = false;

      
      
      
      diff = mStatusDiff;
      mStatusDiff = ImageStatusDiff::NoChange();
    }
  }

  return RequestDecodeIfNeeded(rv, aIntent, done, wasSize);
}

already_AddRefed<imgIContainer>
RasterImage::Unwrap()
{
  nsCOMPtr<imgIContainer> self(this);
  return self.forget();
}

NS_IMPL_ISUPPORTS(RasterImage::DecodePool,
                  nsIObserver)

 RasterImage::DecodePool*
RasterImage::DecodePool::Singleton()
{
  if (!sSingleton) {
    MOZ_ASSERT(NS_IsMainThread());
    sSingleton = new DecodePool();
    ClearOnShutdown(&sSingleton);
  }

  return sSingleton;
}

already_AddRefed<nsIEventTarget>
RasterImage::DecodePool::GetEventTarget()
{
  nsCOMPtr<nsIEventTarget> target = do_QueryInterface(mThreadPool);
  return target.forget();
}

#ifdef MOZ_NUWA_PROCESS

class RIDThreadPoolListener MOZ_FINAL : public nsIThreadPoolListener
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSITHREADPOOLLISTENER

    RIDThreadPoolListener() {}
    ~RIDThreadPoolListener() {}
};

NS_IMPL_ISUPPORTS(RIDThreadPoolListener, nsIThreadPoolListener)

NS_IMETHODIMP
RIDThreadPoolListener::OnThreadCreated()
{
    if (IsNuwaProcess()) {
        NuwaMarkCurrentThread((void (*)(void *))nullptr, nullptr);
    }
    return NS_OK;
}

NS_IMETHODIMP
RIDThreadPoolListener::OnThreadShuttingDown()
{
    return NS_OK;
}

#endif 

RasterImage::DecodePool::DecodePool()
 : mThreadPoolMutex("Thread Pool")
{
  if (gMultithreadedDecoding) {
    mThreadPool = do_CreateInstance(NS_THREADPOOL_CONTRACTID);
    if (mThreadPool) {
      mThreadPool->SetName(NS_LITERAL_CSTRING("ImageDecoder"));
      uint32_t limit;
      if (gDecodingThreadLimit <= 0) {
        limit = std::max(PR_GetNumberOfProcessors(), 2) - 1;
      } else {
        limit = static_cast<uint32_t>(gDecodingThreadLimit);
      }

      mThreadPool->SetThreadLimit(limit);
      mThreadPool->SetIdleThreadLimit(limit);

#ifdef MOZ_NUWA_PROCESS
      if (IsNuwaProcess()) {
        mThreadPool->SetListener(new RIDThreadPoolListener());
      }
#endif

      nsCOMPtr<nsIObserverService> obsSvc = services::GetObserverService();
      if (obsSvc) {
        obsSvc->AddObserver(this, "xpcom-shutdown-threads", false);
      }
    }
  }
}

RasterImage::DecodePool::~DecodePool()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must shut down DecodePool on main thread!");
}

NS_IMETHODIMP
RasterImage::DecodePool::Observe(nsISupports *subject, const char *topic,
                                 const char16_t *data)
{
  NS_ASSERTION(strcmp(topic, "xpcom-shutdown-threads") == 0, "oops");

  nsCOMPtr<nsIThreadPool> threadPool;

  {
    MutexAutoLock threadPoolLock(mThreadPoolMutex);
    threadPool = mThreadPool;
    mThreadPool = nullptr;
  }

  if (threadPool) {
    threadPool->Shutdown();
  }

  return NS_OK;
}

void
RasterImage::DecodePool::RequestDecode(RasterImage* aImg)
{
  MOZ_ASSERT(aImg->mDecoder);
  aImg->mDecodingMonitor.AssertCurrentThreadIn();

  
  
  if (!aImg->mDecoder->NeedsNewFrame()) {
    
    
    aImg->mDecodeRequest->mBytesToDecode = aImg->mSourceData.Length() - aImg->mBytesDecoded;

    if (aImg->mDecodeRequest->mRequestStatus == DecodeRequest::REQUEST_PENDING ||
        aImg->mDecodeRequest->mRequestStatus == DecodeRequest::REQUEST_ACTIVE) {
      
      
      return;
    }

    aImg->mDecodeRequest->mRequestStatus = DecodeRequest::REQUEST_PENDING;
    nsRefPtr<DecodeJob> job = new DecodeJob(aImg->mDecodeRequest, aImg);

    MutexAutoLock threadPoolLock(mThreadPoolMutex);
    if (!gMultithreadedDecoding || !mThreadPool) {
      NS_DispatchToMainThread(job);
    } else {
      mThreadPool->Dispatch(job, nsIEventTarget::DISPATCH_NORMAL);
    }
  }
}

void
RasterImage::DecodePool::DecodeABitOf(RasterImage* aImg, DecodeStrategy aStrategy)
{
  MOZ_ASSERT(NS_IsMainThread());
  aImg->mDecodingMonitor.AssertCurrentThreadIn();

  if (aImg->mDecodeRequest) {
    
    if (aImg->mDecodeRequest->mRequestStatus == DecodeRequest::REQUEST_WORK_DONE) {
      aImg->FinishedSomeDecoding();
    }
  }

  DecodeSomeOfImage(aImg, aStrategy);

  aImg->FinishedSomeDecoding();

  
  
  if (aImg->mDecoder && aImg->mDecoder->NeedsNewFrame()) {
    FrameNeededWorker::GetNewFrame(aImg);
  } else {
    
    
    if (aImg->mDecoder &&
        !aImg->mError &&
        !aImg->IsDecodeFinished() &&
        aImg->mSourceData.Length() > aImg->mBytesDecoded) {
      RequestDecode(aImg);
    }
  }
}

 void
RasterImage::DecodePool::StopDecoding(RasterImage* aImg)
{
  aImg->mDecodingMonitor.AssertCurrentThreadIn();

  
  
  if (aImg->mDecodeRequest) {
    aImg->mDecodeRequest->mRequestStatus = DecodeRequest::REQUEST_STOPPED;
  }
}

NS_IMETHODIMP
RasterImage::DecodePool::DecodeJob::Run()
{
  ReentrantMonitorAutoEnter lock(mImage->mDecodingMonitor);

  
  if (mRequest->mRequestStatus == DecodeRequest::REQUEST_STOPPED) {
    DecodeDoneWorker::NotifyFinishedSomeDecoding(mImage, mRequest);
    return NS_OK;
  }

  
  if (!mImage->mDecoder || mImage->IsDecodeFinished()) {
    DecodeDoneWorker::NotifyFinishedSomeDecoding(mImage, mRequest);
    return NS_OK;
  }

  
  
  
  if (mImage->mDecoder->NeedsNewFrame()) {
    return NS_OK;
  }

  mRequest->mRequestStatus = DecodeRequest::REQUEST_ACTIVE;

  size_t oldByteCount = mImage->mBytesDecoded;

  DecodeType type = DECODE_TYPE_UNTIL_DONE_BYTES;

  
  
  if (NS_IsMainThread()) {
    type = DECODE_TYPE_UNTIL_TIME;
  }

  DecodePool::Singleton()->DecodeSomeOfImage(mImage, DECODE_ASYNC, type, mRequest->mBytesToDecode);

  size_t bytesDecoded = mImage->mBytesDecoded - oldByteCount;

  mRequest->mRequestStatus = DecodeRequest::REQUEST_WORK_DONE;

  
  
  if (mImage->mDecoder && mImage->mDecoder->NeedsNewFrame()) {
    FrameNeededWorker::GetNewFrame(mImage);
  }
  
  
  else if (mImage->mDecoder &&
           !mImage->mError &&
           !mImage->mPendingError &&
           !mImage->IsDecodeFinished() &&
           bytesDecoded < mRequest->mBytesToDecode &&
           bytesDecoded > 0) {
    DecodePool::Singleton()->RequestDecode(mImage);
  } else {
    
    DecodeDoneWorker::NotifyFinishedSomeDecoding(mImage, mRequest);
  }

  return NS_OK;
}

RasterImage::DecodePool::DecodeJob::~DecodeJob()
{
  if (gMultithreadedDecoding) {
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    NS_WARN_IF_FALSE(mainThread, "Couldn't get the main thread!");
    if (mainThread) {
      
      RasterImage* rawImg = nullptr;
      mImage.swap(rawImg);
      DebugOnly<nsresult> rv = NS_ProxyRelease(mainThread, NS_ISUPPORTS_CAST(ImageResource*, rawImg));
      MOZ_ASSERT(NS_SUCCEEDED(rv), "Failed to proxy release to main thread");
    }
  }
}

nsresult
RasterImage::DecodePool::DecodeUntilSizeAvailable(RasterImage* aImg)
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter lock(aImg->mDecodingMonitor);

  if (aImg->mDecodeRequest) {
    
    if (aImg->mDecodeRequest->mRequestStatus == DecodeRequest::REQUEST_WORK_DONE) {
      nsresult rv = aImg->FinishedSomeDecoding();
      if (NS_FAILED(rv)) {
        aImg->DoError();
        return rv;
      }
    }
  }

  
  
  nsresult rv = DecodeSomeOfImage(aImg, DECODE_ASYNC, DECODE_TYPE_UNTIL_SIZE);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  if (aImg->mDecoder && aImg->mDecoder->NeedsNewFrame()) {
    FrameNeededWorker::GetNewFrame(aImg);
  } else {
    rv = aImg->FinishedSomeDecoding();
  }

  return rv;
}

nsresult
RasterImage::DecodePool::DecodeSomeOfImage(RasterImage* aImg,
                                           DecodeStrategy aStrategy,
                                           DecodeType aDecodeType ,
                                           uint32_t bytesToDecode )
{
  NS_ABORT_IF_FALSE(aImg->mInitialized,
                    "Worker active for uninitialized container!");
  aImg->mDecodingMonitor.AssertCurrentThreadIn();

  
  
  if (aImg->mError)
    return NS_OK;

  
  
  if (aImg->mPendingError)
    return NS_OK;

  
  
  if (!aImg->mDecoder || aImg->mDecoded)
    return NS_OK;

  
  
  if (aStrategy == DECODE_SYNC && aImg->mDecoder->NeedsNewFrame()) {
    MOZ_ASSERT(NS_IsMainThread());

    aImg->mDecoder->AllocateFrame();
    aImg->mDecodeRequest->mAllocatedNewFrame = true;
  }

  
  else if (aImg->mDecoder->NeedsNewFrame()) {
    return NS_OK;
  }

  nsRefPtr<Decoder> decoderKungFuDeathGrip = aImg->mDecoder;

  uint32_t maxBytes;
  if (aImg->mDecoder->IsSizeDecode()) {
    
    
    maxBytes = aImg->mSourceData.Length();
  } else {
    
    
    
    maxBytes = gDecodeBytesAtATime;
  }

  if (bytesToDecode == 0) {
    bytesToDecode = aImg->mSourceData.Length() - aImg->mBytesDecoded;
  }

  int32_t chunkCount = 0;
  TimeStamp start = TimeStamp::Now();
  TimeStamp deadline = start + TimeDuration::FromMilliseconds(gMaxMSBeforeYield);

  
  
  
  
  
  
  
  while ((aImg->mSourceData.Length() > aImg->mBytesDecoded &&
          bytesToDecode > 0 &&
          !aImg->IsDecodeFinished() &&
          !(aDecodeType == DECODE_TYPE_UNTIL_SIZE && aImg->mHasSize) &&
          !aImg->mDecoder->NeedsNewFrame()) ||
         (aImg->mDecodeRequest && aImg->mDecodeRequest->mAllocatedNewFrame)) {
    chunkCount++;
    uint32_t chunkSize = std::min(bytesToDecode, maxBytes);
    nsresult rv = aImg->DecodeSomeData(chunkSize, aStrategy);
    if (NS_FAILED(rv)) {
      aImg->DoError();
      return rv;
    }

    bytesToDecode -= chunkSize;

    
    
    if (aDecodeType == DECODE_TYPE_UNTIL_TIME && TimeStamp::Now() >= deadline)
      break;
  }

  if (aImg->mDecodeRequest) {
    aImg->mDecodeRequest->mDecodeTime += (TimeStamp::Now() - start);
    aImg->mDecodeRequest->mChunkCount += chunkCount;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (aDecodeType != DECODE_TYPE_UNTIL_SIZE &&
      !aImg->mDecoder->HasError() &&
      !aImg->mHasSourceData) {
    aImg->mInDecoder = true;
    aImg->mDecoder->FlushInvalidations();
    aImg->mInDecoder = false;
  }

  return NS_OK;
}

RasterImage::DecodeDoneWorker::DecodeDoneWorker(RasterImage* image, DecodeRequest* request)
 : mImage(image)
 , mRequest(request)
{}

void
RasterImage::DecodeDoneWorker::NotifyFinishedSomeDecoding(RasterImage* image, DecodeRequest* request)
{
  image->mDecodingMonitor.AssertCurrentThreadIn();

  nsCOMPtr<nsIRunnable> worker = new DecodeDoneWorker(image, request);
  NS_DispatchToMainThread(worker);
}

NS_IMETHODIMP
RasterImage::DecodeDoneWorker::Run()
{
  MOZ_ASSERT(NS_IsMainThread());
  ReentrantMonitorAutoEnter lock(mImage->mDecodingMonitor);

  mImage->FinishedSomeDecoding(eShutdownIntent_Done, mRequest);

  return NS_OK;
}

RasterImage::FrameNeededWorker::FrameNeededWorker(RasterImage* image)
 : mImage(image)
{}


void
RasterImage::FrameNeededWorker::GetNewFrame(RasterImage* image)
{
  nsCOMPtr<nsIRunnable> worker = new FrameNeededWorker(image);
  NS_DispatchToMainThread(worker);
}

NS_IMETHODIMP
RasterImage::FrameNeededWorker::Run()
{
  ReentrantMonitorAutoEnter lock(mImage->mDecodingMonitor);
  nsresult rv = NS_OK;

  
  
  if (mImage->mDecoder && mImage->mDecoder->NeedsNewFrame()) {
    rv = mImage->mDecoder->AllocateFrame();
    mImage->mDecodeRequest->mAllocatedNewFrame = true;
  }

  if (NS_SUCCEEDED(rv) && mImage->mDecoder) {
    
    DecodePool::Singleton()->RequestDecode(mImage);
  }

  return NS_OK;
}

nsIntSize
RasterImage::OptimalImageSizeForDest(const gfxSize& aDest, uint32_t aWhichFrame,
                                     GraphicsFilter aFilter, uint32_t aFlags)
{
  MOZ_ASSERT(aDest.width >= 0 || ceil(aDest.width) <= INT32_MAX ||
             aDest.height >= 0 || ceil(aDest.height) <= INT32_MAX,
             "Unexpected destination size");

  if (mSize.IsEmpty()) {
    return nsIntSize(0, 0);
  }

  nsIntSize destSize(ceil(aDest.width), ceil(aDest.height));
  gfx::Size scale(double(destSize.width) / mSize.width,
                  double(destSize.height) / mSize.height);

  if (CanScale(aFilter, scale, aFlags)) {
    if (mScaleResult.scaledSize == destSize) {
      if (mScaleResult.status == SCALE_DONE) {
        return destSize;  
      } else if (mScaleResult.status == SCALE_PENDING) {
        return mSize;     
      }
    }

    
    uint32_t frameIndex = aWhichFrame == FRAME_FIRST ? 0
                                                     : GetCurrentImgFrameIndex();

    nsRefPtr<imgFrame> frame = GetDrawableImgFrame(frameIndex);
    if (frame) {
      RequestScale(frame, destSize);
    }
  }

  
  
  return mSize;
}

} 
} 
