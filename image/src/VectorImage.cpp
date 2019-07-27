




#include "VectorImage.h"

#include "gfx2DGlue.h"
#include "gfxContext.h"
#include "gfxDrawable.h"
#include "gfxPlatform.h"
#include "gfxUtils.h"
#include "imgDecoderObserver.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/MemoryReporting.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/gfx/2D.h"
#include "mozilla/RefPtr.h"
#include "nsIDOMEvent.h"
#include "nsIPresShell.h"
#include "nsIStreamListener.h"
#include "nsMimeTypes.h"
#include "nsPresContext.h"
#include "nsRect.h"
#include "nsString.h"
#include "nsStubDocumentObserver.h"
#include "nsSVGEffects.h" 
#include "nsWindowMemoryReporter.h"
#include "ImageRegion.h"
#include "Orientation.h"
#include "SVGDocumentWrapper.h"
#include "nsIDOMEventListener.h"
#include "SurfaceCache.h"


#undef GetCurrentTime

namespace mozilla {

using namespace dom;
using namespace gfx;
using namespace layers;

namespace image {


class SVGRootRenderingObserver MOZ_FINAL : public nsSVGRenderingObserver {
public:
  SVGRootRenderingObserver(SVGDocumentWrapper* aDocWrapper,
                           VectorImage*        aVectorImage)
    : nsSVGRenderingObserver()
    , mDocWrapper(aDocWrapper)
    , mVectorImage(aVectorImage)
    , mHonoringInvalidations(true)
  {
    MOZ_ASSERT(mDocWrapper, "Need a non-null SVG document wrapper");
    MOZ_ASSERT(mVectorImage, "Need a non-null VectorImage");

    StartListening();
    Element* elem = GetTarget();
    MOZ_ASSERT(elem, "no root SVG node for us to observe");

    nsSVGEffects::AddRenderingObserver(elem, this);
    mInObserverList = true;
  }

  virtual ~SVGRootRenderingObserver()
  {
    StopListening();
  }

  void ResumeHonoringInvalidations()
  {
    mHonoringInvalidations = true;
  }

protected:
  virtual Element* GetTarget() MOZ_OVERRIDE
  {
    return mDocWrapper->GetRootSVGElem();
  }

  virtual void DoUpdate() MOZ_OVERRIDE
  {
    Element* elem = GetTarget();
    MOZ_ASSERT(elem, "missing root SVG node");

    if (mHonoringInvalidations && !mDocWrapper->ShouldIgnoreInvalidation()) {
      nsIFrame* frame = elem->GetPrimaryFrame();
      if (!frame || frame->PresContext()->PresShell()->IsDestroying()) {
        
        return;
      }

      
      mHonoringInvalidations = false;

      mVectorImage->InvalidateObserversOnNextRefreshDriverTick();
    }

    
    
    if (!mInObserverList) {
      nsSVGEffects::AddRenderingObserver(elem, this);
      mInObserverList = true;
    } 
  }

  
  const nsRefPtr<SVGDocumentWrapper> mDocWrapper;
  VectorImage* const mVectorImage;   
  bool mHonoringInvalidations;
};

class SVGParseCompleteListener MOZ_FINAL : public nsStubDocumentObserver {
public:
  NS_DECL_ISUPPORTS

  SVGParseCompleteListener(nsIDocument* aDocument,
                           VectorImage* aImage)
    : mDocument(aDocument)
    , mImage(aImage)
  {
    MOZ_ASSERT(mDocument, "Need an SVG document");
    MOZ_ASSERT(mImage, "Need an image");

    mDocument->AddObserver(this);
  }

private:
  ~SVGParseCompleteListener()
  {
    if (mDocument) {
      
      
      
      Cancel();
    }
  }

public:
  void EndLoad(nsIDocument* aDocument) MOZ_OVERRIDE
  {
    MOZ_ASSERT(aDocument == mDocument, "Got EndLoad for wrong document?");

    
    
    nsRefPtr<SVGParseCompleteListener> kungFuDeathGroup(this);

    mImage->OnSVGDocumentParsed();
  }

  void Cancel()
  {
    MOZ_ASSERT(mDocument, "Duplicate call to Cancel");
    if (mDocument) {
      mDocument->RemoveObserver(this);
      mDocument = nullptr;
    }
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
  VectorImage* const mImage; 
};

NS_IMPL_ISUPPORTS(SVGParseCompleteListener, nsIDocumentObserver)

class SVGLoadEventListener MOZ_FINAL : public nsIDOMEventListener {
public:
  NS_DECL_ISUPPORTS

  SVGLoadEventListener(nsIDocument* aDocument,
                       VectorImage* aImage)
    : mDocument(aDocument)
    , mImage(aImage)
  {
    MOZ_ASSERT(mDocument, "Need an SVG document");
    MOZ_ASSERT(mImage, "Need an image");

    mDocument->AddEventListener(NS_LITERAL_STRING("MozSVGAsImageDocumentLoad"), this, true, false);
    mDocument->AddEventListener(NS_LITERAL_STRING("SVGAbort"), this, true, false);
    mDocument->AddEventListener(NS_LITERAL_STRING("SVGError"), this, true, false);
  }

private:
  ~SVGLoadEventListener()
  {
    if (mDocument) {
      
      
      
      Cancel();
    }
  }

public:
  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) MOZ_OVERRIDE
  {
    MOZ_ASSERT(mDocument, "Need an SVG document. Received multiple events?");

    
    
    nsRefPtr<SVGLoadEventListener> kungFuDeathGroup(this);

    nsAutoString eventType;
    aEvent->GetType(eventType);
    MOZ_ASSERT(eventType.EqualsLiteral("MozSVGAsImageDocumentLoad")  ||
               eventType.EqualsLiteral("SVGAbort")                   ||
               eventType.EqualsLiteral("SVGError"),
               "Received unexpected event");

    if (eventType.EqualsLiteral("MozSVGAsImageDocumentLoad")) {
      mImage->OnSVGDocumentLoaded();
    } else {
      mImage->OnSVGDocumentError();
    }

    return NS_OK;
  }

  void Cancel()
  {
    MOZ_ASSERT(mDocument, "Duplicate call to Cancel");
    if (mDocument) {
      mDocument->RemoveEventListener(NS_LITERAL_STRING("MozSVGAsImageDocumentLoad"), this, true);
      mDocument->RemoveEventListener(NS_LITERAL_STRING("SVGAbort"), this, true);
      mDocument->RemoveEventListener(NS_LITERAL_STRING("SVGError"), this, true);
      mDocument = nullptr;
    }
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
  VectorImage* const mImage; 
};

NS_IMPL_ISUPPORTS(SVGLoadEventListener, nsIDOMEventListener)


class SVGDrawingCallback : public gfxDrawingCallback {
public:
  SVGDrawingCallback(SVGDocumentWrapper* aSVGDocumentWrapper,
                     const nsIntRect& aViewport,
                     const IntSize& aSize,
                     uint32_t aImageFlags)
    : mSVGDocumentWrapper(aSVGDocumentWrapper)
    , mViewport(aViewport)
    , mSize(aSize)
    , mImageFlags(aImageFlags)
  {}
  virtual bool operator()(gfxContext* aContext,
                            const gfxRect& aFillRect,
                            const GraphicsFilter& aFilter,
                            const gfxMatrix& aTransform);
private:
  nsRefPtr<SVGDocumentWrapper> mSVGDocumentWrapper;
  const nsIntRect              mViewport;
  const IntSize                mSize;
  uint32_t                     mImageFlags;
};


bool
SVGDrawingCallback::operator()(gfxContext* aContext,
                               const gfxRect& aFillRect,
                               const GraphicsFilter& aFilter,
                               const gfxMatrix& aTransform)
{
  MOZ_ASSERT(mSVGDocumentWrapper, "need an SVGDocumentWrapper");

  
  nsCOMPtr<nsIPresShell> presShell;
  if (NS_FAILED(mSVGDocumentWrapper->GetPresShell(getter_AddRefs(presShell)))) {
    NS_WARNING("Unable to draw -- presShell lookup failed");
    return false;
  }
  MOZ_ASSERT(presShell, "GetPresShell succeeded but returned null");

  gfxContextAutoSaveRestore contextRestorer(aContext);

  
  aContext->NewPath();
  aContext->Rectangle(aFillRect);
  aContext->Clip();

  gfxMatrix matrix = aTransform;
  if (!matrix.Invert()) {
    return false;
  }
  aContext->Multiply(matrix);
  aContext->Scale(double(mSize.width) / mViewport.width,
                  double(mSize.height) / mViewport.height);

  nsPresContext* presContext = presShell->GetPresContext();
  MOZ_ASSERT(presContext, "pres shell w/out pres context");

  nsRect svgRect(presContext->DevPixelsToAppUnits(mViewport.x),
                 presContext->DevPixelsToAppUnits(mViewport.y),
                 presContext->DevPixelsToAppUnits(mViewport.width),
                 presContext->DevPixelsToAppUnits(mViewport.height));

  uint32_t renderDocFlags = nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
  if (!(mImageFlags & imgIContainer::FLAG_SYNC_DECODE)) {
    renderDocFlags |= nsIPresShell::RENDER_ASYNC_DECODE_IMAGES;
  }

  presShell->RenderDocument(svgRect, renderDocFlags,
                            NS_RGBA(0, 0, 0, 0), 
                            aContext);

  return true;
}


NS_IMPL_ISUPPORTS(VectorImage,
                  imgIContainer,
                  nsIStreamListener,
                  nsIRequestObserver)




VectorImage::VectorImage(imgStatusTracker* aStatusTracker,
                         ImageURL* aURI ) :
  ImageResource(aURI), 
  mIsInitialized(false),
  mIsFullyLoaded(false),
  mIsDrawing(false),
  mHaveAnimations(false),
  mHasPendingInvalidation(false)
{
  mStatusTrackerInit = new imgStatusTrackerInit(this, aStatusTracker);
}

VectorImage::~VectorImage()
{
  CancelAllListeners();
  SurfaceCache::Discard(this);
}




nsresult
VectorImage::Init(const char* aMimeType,
                  uint32_t aFlags)
{
  
  if (mIsInitialized)
    return NS_ERROR_ILLEGAL_VALUE;

  MOZ_ASSERT(!mIsFullyLoaded && !mHaveAnimations && !mError,
             "Flags unexpectedly set before initialization");
  MOZ_ASSERT(!strcmp(aMimeType, IMAGE_SVG_XML), "Unexpected mimetype");

  mIsInitialized = true;
  return NS_OK;
}

nsIntRect
VectorImage::FrameRect(uint32_t aWhichFrame)
{
  return nsIntRect::GetMaxSizedIntRect();
}

size_t
VectorImage::HeapSizeOfSourceWithComputedFallback(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  
  
  
  return 0;
}

size_t
VectorImage::HeapSizeOfDecodedWithComputedFallback(MallocSizeOf aMallocSizeOf) const
{
  
  
  
  return 0;
}

size_t
VectorImage::NonHeapSizeOfDecoded() const
{
  
  
  
  return 0;
}

size_t
VectorImage::OutOfProcessSizeOfDecoded() const
{
  
  
  
  return 0;
}

MOZ_DEFINE_MALLOC_SIZE_OF(WindowsMallocSizeOf);

size_t
VectorImage::HeapSizeOfVectorImageDocument(nsACString* aDocURL) const
{
  nsIDocument* doc = mSVGDocumentWrapper->GetDocument();
  if (!doc) {
    if (aDocURL) {
      mURI->GetSpec(*aDocURL);
    }
    return 0; 
  }

  if (aDocURL) {
    doc->GetDocumentURI()->GetSpec(*aDocURL);
  }

  nsWindowSizes windowSizes(WindowsMallocSizeOf);
  doc->DocAddSizeOfIncludingThis(&windowSizes);
  return windowSizes.getTotalSize();
}

nsresult
VectorImage::OnImageDataComplete(nsIRequest* aRequest,
                                 nsISupports* aContext,
                                 nsresult aStatus,
                                 bool aLastPart)
{
  
  
  nsresult finalStatus = OnStopRequest(aRequest, aContext, aStatus);

  
  if (NS_FAILED(aStatus))
    finalStatus = aStatus;

  
  if (mStatusTracker) {
    
    nsRefPtr<imgStatusTracker> clone = mStatusTracker->CloneForRecording();
    imgDecoderObserver* observer = clone->GetDecoderObserver();
    observer->OnStopRequest(aLastPart, finalStatus);
    ImageStatusDiff diff = mStatusTracker->Difference(clone);
    mStatusTracker->ApplyDifference(diff);
    mStatusTracker->SyncNotifyDifference(diff);
  }
  return finalStatus;
}

nsresult
VectorImage::OnImageDataAvailable(nsIRequest* aRequest,
                                  nsISupports* aContext,
                                  nsIInputStream* aInStr,
                                  uint64_t aSourceOffset,
                                  uint32_t aCount)
{
  return OnDataAvailable(aRequest, aContext, aInStr, aSourceOffset, aCount);
}

nsresult
VectorImage::OnNewSourceData()
{
  return NS_OK;
}

nsresult
VectorImage::StartAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  MOZ_ASSERT(ShouldAnimate(), "Should not animate!");

  mSVGDocumentWrapper->StartAnimation();
  return NS_OK;
}

nsresult
VectorImage::StopAnimation()
{
  nsresult rv = NS_OK;
  if (mError) {
    rv = NS_ERROR_FAILURE;
  } else {
    MOZ_ASSERT(mIsFullyLoaded && mHaveAnimations,
               "Should not have been animating!");

    mSVGDocumentWrapper->StopAnimation();
  }

  mAnimating = false;
  return rv;
}

bool
VectorImage::ShouldAnimate()
{
  return ImageResource::ShouldAnimate() && mIsFullyLoaded && mHaveAnimations;
}

NS_IMETHODIMP_(void)
VectorImage::SetAnimationStartTime(const TimeStamp& aTime)
{
  
}






NS_IMETHODIMP
VectorImage::GetWidth(int32_t* aWidth)
{
  if (mError || !mIsFullyLoaded) {
    *aWidth = 0;
    return NS_ERROR_FAILURE;
  }

  if (!mSVGDocumentWrapper->GetWidthOrHeight(SVGDocumentWrapper::eWidth,
                                             *aWidth)) {
    *aWidth = 0;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}



NS_IMETHODIMP_(void)
VectorImage::RequestRefresh(const TimeStamp& aTime)
{
  if (HadRecentRefresh(aTime)) {
    return;
  }

  EvaluateAnimation();

  mSVGDocumentWrapper->TickRefreshDriver();

  if (mHasPendingInvalidation) {
    SendInvalidationNotifications();
    mHasPendingInvalidation = false;
  }
}

void
VectorImage::SendInvalidationNotifications()
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  if (mStatusTracker) {
    SurfaceCache::Discard(this);
    mStatusTracker->FrameChanged(&nsIntRect::GetMaxSizedIntRect());
    mStatusTracker->OnStopFrame();
  }
}

NS_IMETHODIMP_(nsIntRect)
VectorImage::GetImageSpaceInvalidationRect(const nsIntRect& aRect)
{
  return aRect;
}



NS_IMETHODIMP
VectorImage::GetHeight(int32_t* aHeight)
{
  if (mError || !mIsFullyLoaded) {
    *aHeight = 0;
    return NS_ERROR_FAILURE;
  }

  if (!mSVGDocumentWrapper->GetWidthOrHeight(SVGDocumentWrapper::eHeight,
                                             *aHeight)) {
    *aHeight = 0;
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}



NS_IMETHODIMP
VectorImage::GetIntrinsicSize(nsSize* aSize)
{
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  nsIFrame* rootFrame = mSVGDocumentWrapper->GetRootLayoutFrame();
  if (!rootFrame)
    return NS_ERROR_FAILURE;

  *aSize = nsSize(-1, -1);
  IntrinsicSize rfSize = rootFrame->GetIntrinsicSize();
  if (rfSize.width.GetUnit() == eStyleUnit_Coord)
    aSize->width = rfSize.width.GetCoordValue();
  if (rfSize.height.GetUnit() == eStyleUnit_Coord)
    aSize->height = rfSize.height.GetCoordValue();

  return NS_OK;
}



NS_IMETHODIMP
VectorImage::GetIntrinsicRatio(nsSize* aRatio)
{
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  nsIFrame* rootFrame = mSVGDocumentWrapper->GetRootLayoutFrame();
  if (!rootFrame)
    return NS_ERROR_FAILURE;

  *aRatio = rootFrame->GetIntrinsicRatio();
  return NS_OK;
}

NS_IMETHODIMP_(Orientation)
VectorImage::GetOrientation()
{
  return Orientation();
}



NS_IMETHODIMP
VectorImage::GetType(uint16_t* aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = GetType();
  return NS_OK;
}



NS_IMETHODIMP_(uint16_t)
VectorImage::GetType()
{
  return imgIContainer::TYPE_VECTOR;
}



NS_IMETHODIMP
VectorImage::GetAnimated(bool* aAnimated)
{
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  *aAnimated = mSVGDocumentWrapper->IsAnimated();
  return NS_OK;
}



int32_t
VectorImage::GetFirstFrameDelay()
{
  if (mError)
    return -1;

  if (!mSVGDocumentWrapper->IsAnimated())
    return -1;

  
  
  return 0;
}




NS_IMETHODIMP_(bool)
VectorImage::FrameIsOpaque(uint32_t aWhichFrame)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    NS_WARNING("aWhichFrame outside valid range!");

  return false; 
}




NS_IMETHODIMP_(TemporaryRef<SourceSurface>)
VectorImage::GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags)
{
  MOZ_ASSERT(aWhichFrame <= FRAME_MAX_VALUE);

  if (aWhichFrame > FRAME_MAX_VALUE)
    return nullptr;

  if (mError)
    return nullptr;

  
  
  nsIntSize imageIntSize;
  if (!mSVGDocumentWrapper->GetWidthOrHeight(SVGDocumentWrapper::eWidth,
                                             imageIntSize.width) ||
      !mSVGDocumentWrapper->GetWidthOrHeight(SVGDocumentWrapper::eHeight,
                                             imageIntSize.height)) {
    
    return nullptr;
  }

  
  
  RefPtr<DrawTarget> dt = gfxPlatform::GetPlatform()->
    CreateOffscreenContentDrawTarget(IntSize(imageIntSize.width,
                                             imageIntSize.height),
                                     SurfaceFormat::B8G8R8A8);
  if (!dt) {
    NS_ERROR("Could not create a DrawTarget");
    return nullptr;
  }

  nsRefPtr<gfxContext> context = new gfxContext(dt);

  nsresult rv = Draw(context, imageIntSize,
                     ImageRegion::Create(imageIntSize),
                     aWhichFrame, GraphicsFilter::FILTER_NEAREST,
                     Nothing(), aFlags);

  NS_ENSURE_SUCCESS(rv, nullptr);
  return dt->Snapshot();
}



NS_IMETHODIMP
VectorImage::GetImageContainer(LayerManager* aManager,
                               layers::ImageContainer** _retval)
{
  *_retval = nullptr;
  return NS_OK;
}

struct SVGDrawingParameters
{
  SVGDrawingParameters(gfxContext* aContext,
                       const nsIntSize& aSize,
                       const ImageRegion& aRegion,
                       GraphicsFilter aFilter,
                       const Maybe<SVGImageContext>& aSVGContext,
                       float aAnimationTime,
                       uint32_t aFlags)
    : context(aContext)
    , size(aSize.width, aSize.height)
    , imageRect(0, 0, aSize.width, aSize.height)
    , region(aRegion)
    , filter(aFilter)
    , svgContext(aSVGContext)
    , viewportSize(aSVGContext ? aSVGContext->GetViewportSize() : aSize)
    , animationTime(aAnimationTime)
    , flags(aFlags)
    , opacity(aSVGContext ? aSVGContext->GetGlobalOpacity() : 1.0)
  { }

  gfxContext*                   context;
  IntSize                       size;
  IntRect                       imageRect;
  ImageRegion                   region;
  GraphicsFilter                filter;
  const Maybe<SVGImageContext>& svgContext;
  nsIntSize                     viewportSize;
  float                         animationTime;
  uint32_t                      flags;
  gfxFloat                      opacity;
};











NS_IMETHODIMP
VectorImage::Draw(gfxContext* aContext,
                  const nsIntSize& aSize,
                  const ImageRegion& aRegion,
                  uint32_t aWhichFrame,
                  GraphicsFilter aFilter,
                  const Maybe<SVGImageContext>& aSVGContext,
                  uint32_t aFlags)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  NS_ENSURE_ARG_POINTER(aContext);
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  if (mIsDrawing) {
    NS_WARNING("Refusing to make re-entrant call to VectorImage::Draw");
    return NS_ERROR_FAILURE;
  }

  if (mAnimationConsumers == 0 && mStatusTracker) {
    mStatusTracker->OnUnlockedDraw();
  }

  AutoRestore<bool> autoRestoreIsDrawing(mIsDrawing);
  mIsDrawing = true;

  float animTime = (aWhichFrame == FRAME_FIRST) ? 0.0f
                                                : mSVGDocumentWrapper->GetCurrentTime();
  AutoSVGRenderingState autoSVGState(aSVGContext, animTime,
                                     mSVGDocumentWrapper->GetRootSVGElem());

  
  SVGDrawingParameters params(aContext, aSize, aRegion, aFilter,
                              aSVGContext, animTime, aFlags);

  if (aFlags & FLAG_BYPASS_SURFACE_CACHE) {
    CreateDrawableAndShow(params);
    return NS_OK;
  }

  nsRefPtr<gfxDrawable> drawable =
    SurfaceCache::Lookup(ImageKey(this),
                         SurfaceKey(params.size, aSVGContext,
                                    animTime, aFlags));

  
  if (drawable) {
    Show(drawable, params);
  } else {
    CreateDrawableAndShow(params);
  }

  return NS_OK;
}

void
VectorImage::CreateDrawableAndShow(const SVGDrawingParameters& aParams)
{
  mSVGDocumentWrapper->UpdateViewportBounds(aParams.viewportSize);
  mSVGDocumentWrapper->FlushImageTransformInvalidation();

  nsRefPtr<gfxDrawingCallback> cb =
    new SVGDrawingCallback(mSVGDocumentWrapper,
                           nsIntRect(nsIntPoint(0, 0), aParams.viewportSize),
                           aParams.size,
                           aParams.flags);

  nsRefPtr<gfxDrawable> svgDrawable =
    new gfxCallbackDrawable(cb, ThebesIntSize(aParams.size));

  bool bypassCache = bool(aParams.flags & FLAG_BYPASS_SURFACE_CACHE) ||
                     
                     
                     mHaveAnimations ||
                     
                     !SurfaceCache::CanHold(aParams.size);
  if (bypassCache)
    return Show(svgDrawable, aParams);

  
  RefPtr<gfx::DrawTarget> target =
    gfxPlatform::GetPlatform()->
      CreateOffscreenContentDrawTarget(aParams.size,
                                       gfx::SurfaceFormat::B8G8R8A8);

  
  
  
  if (!target)
    return Show(svgDrawable, aParams);

  nsRefPtr<gfxContext> ctx = new gfxContext(target);

  
  nsIntRect imageRect(ThebesIntRect(aParams.imageRect));
  gfxUtils::DrawPixelSnapped(ctx, svgDrawable,
                             ThebesIntSize(aParams.size),
                             ImageRegion::Create(imageRect),
                             SurfaceFormat::B8G8R8A8,
                             GraphicsFilter::FILTER_NEAREST, aParams.flags);

  RefPtr<SourceSurface> surface = target->Snapshot();

  
  SurfaceCache::Insert(surface, ImageKey(this),
                       SurfaceKey(aParams.size, aParams.svgContext,
                                  aParams.animationTime, aParams.flags));

  
  
  
  nsRefPtr<gfxDrawable> drawable =
    new gfxSurfaceDrawable(surface, ThebesIntSize(aParams.size));
  Show(drawable, aParams);
}


void
VectorImage::Show(gfxDrawable* aDrawable, const SVGDrawingParameters& aParams)
{
  MOZ_ASSERT(aDrawable, "Should have a gfxDrawable by now");
  gfxUtils::DrawPixelSnapped(aParams.context, aDrawable,
                             ThebesIntSize(aParams.size),
                             aParams.region,
                             SurfaceFormat::B8G8R8A8,
                             aParams.filter, aParams.flags, aParams.opacity);

  MOZ_ASSERT(mRenderingObserver, "Should have a rendering observer by now");
  mRenderingObserver->ResumeHonoringInvalidations();
}



NS_IMETHODIMP
VectorImage::RequestDecode()
{
  
  return NS_OK;
}

NS_IMETHODIMP
VectorImage::StartDecoding()
{
  
  return NS_OK;
}

bool
VectorImage::IsDecoded()
{
  return mIsFullyLoaded || mError;
}



NS_IMETHODIMP
VectorImage::LockImage()
{
  
  return NS_OK;
}



NS_IMETHODIMP
VectorImage::UnlockImage()
{
  
  return NS_OK;
}



NS_IMETHODIMP
VectorImage::RequestDiscard()
{
  SurfaceCache::Discard(this);
  return NS_OK;
}



NS_IMETHODIMP
VectorImage::ResetAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  if (!mIsFullyLoaded || !mHaveAnimations) {
    return NS_OK; 
  }

  mSVGDocumentWrapper->ResetAnimation();

  return NS_OK;
}

NS_IMETHODIMP_(float)
VectorImage::GetFrameIndex(uint32_t aWhichFrame)
{
  MOZ_ASSERT(aWhichFrame <= FRAME_MAX_VALUE, "Invalid argument");
  return aWhichFrame == FRAME_FIRST
         ? 0.0f
         : mSVGDocumentWrapper->GetCurrentTime();
}






NS_IMETHODIMP
VectorImage::OnStartRequest(nsIRequest* aRequest, nsISupports* aCtxt)
{
  MOZ_ASSERT(!mSVGDocumentWrapper,
             "Repeated call to OnStartRequest -- can this happen?");

  mSVGDocumentWrapper = new SVGDocumentWrapper();
  nsresult rv = mSVGDocumentWrapper->OnStartRequest(aRequest, aCtxt);
  if (NS_FAILED(rv)) {
    mSVGDocumentWrapper = nullptr;
    mError = true;
    return rv;
  }

  
  
  
  if (mStatusTracker) {
    nsRefPtr<imgStatusTracker> clone = mStatusTracker->CloneForRecording();
    imgDecoderObserver* observer = clone->GetDecoderObserver();
    observer->OnStartDecode();
    ImageStatusDiff diff = mStatusTracker->Difference(clone);
    mStatusTracker->ApplyDifference(diff);
    mStatusTracker->SyncNotifyDifference(diff);
  }

  
  
  
  
  
  
  nsIDocument* document = mSVGDocumentWrapper->GetDocument();
  mLoadEventListener = new SVGLoadEventListener(document, this);
  mParseCompleteListener = new SVGParseCompleteListener(document, this);

  return NS_OK;
}




NS_IMETHODIMP
VectorImage::OnStopRequest(nsIRequest* aRequest, nsISupports* aCtxt,
                           nsresult aStatus)
{
  if (mError)
    return NS_ERROR_FAILURE;

  return mSVGDocumentWrapper->OnStopRequest(aRequest, aCtxt, aStatus);
}

void
VectorImage::OnSVGDocumentParsed()
{
  MOZ_ASSERT(mParseCompleteListener, "Should have the parse complete listener");
  MOZ_ASSERT(mLoadEventListener, "Should have the load event listener");

  if (!mSVGDocumentWrapper->GetRootSVGElem()) {
    
    
    
    
    OnSVGDocumentError();
  }
}

void
VectorImage::CancelAllListeners()
{
  if (mParseCompleteListener) {
    mParseCompleteListener->Cancel();
    mParseCompleteListener = nullptr;
  }
  if (mLoadEventListener) {
    mLoadEventListener->Cancel();
    mLoadEventListener = nullptr;
  }
}

void
VectorImage::OnSVGDocumentLoaded()
{
  MOZ_ASSERT(mSVGDocumentWrapper->GetRootSVGElem(),
             "Should have parsed successfully");
  MOZ_ASSERT(!mIsFullyLoaded && !mHaveAnimations,
             "These flags shouldn't get set until OnSVGDocumentLoaded. "
             "Duplicate calls to OnSVGDocumentLoaded?");

  CancelAllListeners();

  
  mSVGDocumentWrapper->FlushLayout();

  mIsFullyLoaded = true;
  mHaveAnimations = mSVGDocumentWrapper->IsAnimated();

  
  mRenderingObserver = new SVGRootRenderingObserver(mSVGDocumentWrapper, this);

  
  if (mStatusTracker) {
    nsRefPtr<imgStatusTracker> clone = mStatusTracker->CloneForRecording();
    imgDecoderObserver* observer = clone->GetDecoderObserver();

    observer->OnStartContainer(); 
    observer->FrameChanged(&nsIntRect::GetMaxSizedIntRect());
    observer->OnStopFrame();
    observer->OnStopDecode(NS_OK); 

    ImageStatusDiff diff = mStatusTracker->Difference(clone);
    mStatusTracker->ApplyDifference(diff);
    mStatusTracker->SyncNotifyDifference(diff);
  }

  EvaluateAnimation();
}

void
VectorImage::OnSVGDocumentError()
{
  CancelAllListeners();

  
  
  
  mError = true;

  if (mStatusTracker) {
    nsRefPtr<imgStatusTracker> clone = mStatusTracker->CloneForRecording();
    imgDecoderObserver* observer = clone->GetDecoderObserver();

    
    observer->OnStopDecode(NS_ERROR_FAILURE);
    ImageStatusDiff diff = mStatusTracker->Difference(clone);
    mStatusTracker->ApplyDifference(diff);
    mStatusTracker->SyncNotifyDifference(diff);
  }
}








NS_IMETHODIMP
VectorImage::OnDataAvailable(nsIRequest* aRequest, nsISupports* aCtxt,
                             nsIInputStream* aInStr, uint64_t aSourceOffset,
                             uint32_t aCount)
{
  if (mError)
    return NS_ERROR_FAILURE;

  return mSVGDocumentWrapper->OnDataAvailable(aRequest, aCtxt, aInStr,
                                              aSourceOffset, aCount);
}




void
VectorImage::InvalidateObserversOnNextRefreshDriverTick()
{
  if (mHaveAnimations) {
    mHasPendingInvalidation = true;
  } else {
    SendInvalidationNotifications();
  }
}

nsIntSize
VectorImage::OptimalImageSizeForDest(const gfxSize& aDest, uint32_t aWhichFrame, GraphicsFilter aFilter, uint32_t aFlags)
{
  MOZ_ASSERT(aDest.width >= 0 || ceil(aDest.width) <= INT32_MAX ||
             aDest.height >= 0 || ceil(aDest.height) <= INT32_MAX,
             "Unexpected destination size");

  
  return nsIntSize(ceil(aDest.width), ceil(aDest.height));
}

already_AddRefed<imgIContainer>
VectorImage::Unwrap()
{
  nsCOMPtr<imgIContainer> self(this);
  return self.forget();
}

} 
} 
