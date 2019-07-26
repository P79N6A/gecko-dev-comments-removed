




#include "VectorImage.h"

#include "imgDecoderObserver.h"
#include "SVGDocumentWrapper.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "nsPresContext.h"
#include "nsRect.h"
#include "nsIObserverService.h"
#include "nsIPresShell.h"
#include "nsIStreamListener.h"
#include "nsMimeTypes.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsSVGUtils.h"  
#include "nsSVGEffects.h" 
#include "gfxDrawable.h"
#include "gfxUtils.h"
#include "mozilla/dom/SVGSVGElement.h"
#include <algorithm>

namespace mozilla {

using namespace dom;
using namespace layers;

namespace image {


class SVGRootRenderingObserver : public nsSVGRenderingObserver {
public:
  SVGRootRenderingObserver(SVGDocumentWrapper* aDocWrapper,
                           VectorImage*        aVectorImage)
    : nsSVGRenderingObserver(),
      mDocWrapper(aDocWrapper),
      mVectorImage(aVectorImage)
  {
    StartListening();
    Element* elem = GetTarget();
    if (elem) {
      nsSVGEffects::AddRenderingObserver(elem, this);
      mInObserverList = true;
    }
#ifdef DEBUG
    else {
      NS_ABORT_IF_FALSE(!mInObserverList,
                        "Have no target, so we can't be in "
                        "target's observer list...");
    }
#endif
  }

  virtual ~SVGRootRenderingObserver()
  {
    StopListening();
  }

protected:
  virtual Element* GetTarget()
  {
    return mDocWrapper->GetRootSVGElem();
  }

  virtual void DoUpdate()
  {
    Element* elem = GetTarget();
    if (!elem)
      return;

    if (!mDocWrapper->ShouldIgnoreInvalidation()) {
      nsIFrame* frame = elem->GetPrimaryFrame();
      if (!frame || frame->PresContext()->PresShell()->IsDestroying()) {
        
        return;
      }

      mVectorImage->InvalidateObserver();
    }

    
    
    if (!mInObserverList) {
      nsSVGEffects::AddRenderingObserver(elem, this);
      mInObserverList = true;
    }
  }

  
  nsRefPtr<SVGDocumentWrapper> mDocWrapper;
  VectorImage* mVectorImage;   
};

class SVGParseCompleteListener MOZ_FINAL : public nsStubDocumentObserver {
public:
  NS_DECL_ISUPPORTS

  SVGParseCompleteListener(nsIDocument* aDocument,
                           VectorImage* aImage)
    : mDocument(aDocument)
    , mImage(aImage)
  {
    NS_ABORT_IF_FALSE(mDocument, "Need an SVG document");
    NS_ABORT_IF_FALSE(mImage, "Need an image");

    mDocument->AddObserver(this);
  }

  ~SVGParseCompleteListener()
  { 
    if (mDocument) {
      
      
      
      Cancel();
    }
  }

  void EndLoad(nsIDocument* aDocument) MOZ_OVERRIDE
  {
    NS_ABORT_IF_FALSE(aDocument == mDocument, "Got EndLoad for wrong document?");

    
    
    nsRefPtr<SVGParseCompleteListener> kungFuDeathGroup(this);

    mImage->OnSVGDocumentParsed();
  }

  void Cancel()
  {
    NS_ABORT_IF_FALSE(mDocument, "Duplicate call to Cancel");
    mDocument->RemoveObserver(this);
    mDocument = nullptr;
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
  VectorImage* mImage; 
};

NS_IMPL_ISUPPORTS1(SVGParseCompleteListener, nsIDocumentObserver)

class SVGLoadEventListener MOZ_FINAL : public nsIDOMEventListener {
public:
  NS_DECL_ISUPPORTS

  SVGLoadEventListener(nsIDocument* aDocument,
                       VectorImage* aImage)
    : mDocument(aDocument)
    , mImage(aImage)
  {
    NS_ABORT_IF_FALSE(mDocument, "Need an SVG document");
    NS_ABORT_IF_FALSE(mImage, "Need an image");

    mDocument->AddEventListener(NS_LITERAL_STRING("MozSVGAsImageDocumentLoad"), this, true, false);
    mDocument->AddEventListener(NS_LITERAL_STRING("SVGAbort"), this, true, false);
    mDocument->AddEventListener(NS_LITERAL_STRING("SVGError"), this, true, false);
  }

  ~SVGLoadEventListener()
  {
    if (mDocument) {
      
      
      
      Cancel();
    }
  }

  NS_IMETHOD HandleEvent(nsIDOMEvent* aEvent) MOZ_OVERRIDE
  {
    NS_ABORT_IF_FALSE(mDocument, "Need an SVG document. Received multiple events?");

    
    
    nsRefPtr<SVGLoadEventListener> kungFuDeathGroup(this);

    nsAutoString eventType;
    aEvent->GetType(eventType);
    NS_ABORT_IF_FALSE(eventType.EqualsLiteral("MozSVGAsImageDocumentLoad")  ||
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
    NS_ABORT_IF_FALSE(mDocument, "Duplicate call to Cancel");
    mDocument->RemoveEventListener(NS_LITERAL_STRING("MozSVGAsImageDocumentLoad"), this, true);
    mDocument->RemoveEventListener(NS_LITERAL_STRING("SVGAbort"), this, true);
    mDocument->RemoveEventListener(NS_LITERAL_STRING("SVGError"), this, true);
    mDocument = nullptr;
  }

private:
  nsCOMPtr<nsIDocument> mDocument;
  VectorImage* mImage; 
};

NS_IMPL_ISUPPORTS1(SVGLoadEventListener, nsIDOMEventListener)


class SVGDrawingCallback : public gfxDrawingCallback {
public:
  SVGDrawingCallback(SVGDocumentWrapper* aSVGDocumentWrapper,
                     const nsIntRect& aViewport,
                     uint32_t aImageFlags) :
    mSVGDocumentWrapper(aSVGDocumentWrapper),
    mViewport(aViewport),
    mImageFlags(aImageFlags)
  {}
  virtual bool operator()(gfxContext* aContext,
                            const gfxRect& aFillRect,
                            const gfxPattern::GraphicsFilter& aFilter,
                            const gfxMatrix& aTransform);
private:
  nsRefPtr<SVGDocumentWrapper> mSVGDocumentWrapper;
  const nsIntRect mViewport;
  uint32_t        mImageFlags;
};


bool
SVGDrawingCallback::operator()(gfxContext* aContext,
                               const gfxRect& aFillRect,
                               const gfxPattern::GraphicsFilter& aFilter,
                               const gfxMatrix& aTransform)
{
  NS_ABORT_IF_FALSE(mSVGDocumentWrapper, "need an SVGDocumentWrapper");

  
  nsCOMPtr<nsIPresShell> presShell;
  if (NS_FAILED(mSVGDocumentWrapper->GetPresShell(getter_AddRefs(presShell)))) {
    NS_WARNING("Unable to draw -- presShell lookup failed");
    return false;
  }
  NS_ABORT_IF_FALSE(presShell, "GetPresShell succeeded but returned null");

  gfxContextAutoSaveRestore contextRestorer(aContext);

  
  aContext->NewPath();
  aContext->Rectangle(aFillRect);
  aContext->Clip();

  gfxContextMatrixAutoSaveRestore contextMatrixRestorer(aContext);
  aContext->Multiply(gfxMatrix(aTransform).Invert());

  nsPresContext* presContext = presShell->GetPresContext();
  NS_ABORT_IF_FALSE(presContext, "pres shell w/out pres context");

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


NS_IMPL_ISUPPORTS3(VectorImage,
                   imgIContainer,
                   nsIStreamListener,
                   nsIRequestObserver)




VectorImage::VectorImage(imgStatusTracker* aStatusTracker,
                         nsIURI* aURI ) :
  ImageResource(aStatusTracker, aURI), 
  mRestrictedRegion(0, 0, 0, 0),
  mIsInitialized(false),
  mIsFullyLoaded(false),
  mIsDrawing(false),
  mHaveAnimations(false),
  mHaveRestrictedRegion(false)
{
}

VectorImage::~VectorImage()
{
}




nsresult
VectorImage::Init(const char* aMimeType,
                  uint32_t aFlags)
{
  
  if (mIsInitialized)
    return NS_ERROR_ILLEGAL_VALUE;

  NS_ABORT_IF_FALSE(!mIsFullyLoaded && !mHaveAnimations &&
                    !mHaveRestrictedRegion && !mError,
                    "Flags unexpectedly set before initialization");
  NS_ABORT_IF_FALSE(!strcmp(aMimeType, IMAGE_SVG_XML), "Unexpected mimetype");

  mIsInitialized = true;
  return NS_OK;
}

nsIntRect
VectorImage::FrameRect(uint32_t aWhichFrame)
{
  return nsIntRect::GetMaxSizedIntRect();
}

size_t
VectorImage::HeapSizeOfSourceWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const
{
  
  
  
  return 0;
}

size_t
VectorImage::HeapSizeOfDecodedWithComputedFallback(nsMallocSizeOfFun aMallocSizeOf) const
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

nsresult
VectorImage::OnImageDataComplete(nsIRequest* aRequest,
                                 nsISupports* aContext,
                                 nsresult aStatus)
{
  return OnStopRequest(aRequest, aContext, aStatus);
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

  NS_ABORT_IF_FALSE(ShouldAnimate(), "Should not animate!");

  mSVGDocumentWrapper->StartAnimation();
  return NS_OK;
}

nsresult
VectorImage::StopAnimation()
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(mIsFullyLoaded && mHaveAnimations,
                    "Should not have been animating!");

  mSVGDocumentWrapper->StopAnimation();
  return NS_OK;
}

bool
VectorImage::ShouldAnimate()
{
  return ImageResource::ShouldAnimate() && mIsFullyLoaded && mHaveAnimations;
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
VectorImage::RequestRefresh(const mozilla::TimeStamp& aTime)
{
  
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



NS_IMETHODIMP_(bool)
VectorImage::FrameIsOpaque(uint32_t aWhichFrame)
{
  if (aWhichFrame > FRAME_MAX_VALUE)
    NS_WARNING("aWhichFrame outside valid range!");

  return false; 
}




NS_IMETHODIMP
VectorImage::GetFrame(uint32_t aWhichFrame,
                      uint32_t aFlags,
                      gfxASurface** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  nsRefPtr<gfxImageSurface> surface;
  nsresult rv = CopyFrame(aWhichFrame, aFlags, getter_AddRefs(surface));
  if (NS_SUCCEEDED(rv)) {
    *_retval = surface.forget().get();
  }
  return rv;
}



NS_IMETHODIMP
VectorImage::GetImageContainer(LayerManager* aManager,
                               mozilla::layers::ImageContainer** _retval)
{
  *_retval = nullptr;
  return NS_OK;
}




NS_IMETHODIMP
VectorImage::CopyFrame(uint32_t aWhichFrame,
                       uint32_t aFlags,
                       gfxImageSurface** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  
  
  
  if (aWhichFrame > FRAME_MAX_VALUE)
    return NS_ERROR_INVALID_ARG;

  if (mError)
    return NS_ERROR_FAILURE;

  
  
  nsIntSize imageIntSize;
  if (!mSVGDocumentWrapper->GetWidthOrHeight(SVGDocumentWrapper::eWidth,
                                             imageIntSize.width) ||
      !mSVGDocumentWrapper->GetWidthOrHeight(SVGDocumentWrapper::eHeight,
                                             imageIntSize.height)) {
    
    return NS_ERROR_FAILURE;
  }

  
  
  
  
  gfxIntSize surfaceSize;
  if (mHaveRestrictedRegion) {
    surfaceSize.width = mRestrictedRegion.width;
    surfaceSize.height = mRestrictedRegion.height;
  } else {
    surfaceSize.width = imageIntSize.width;
    surfaceSize.height = imageIntSize.height;
  }

  nsRefPtr<gfxImageSurface> surface =
    new gfxImageSurface(surfaceSize, gfxASurface::ImageFormatARGB32);
  nsRefPtr<gfxContext> context = new gfxContext(surface);

  
  
  nsresult rv = Draw(context, gfxPattern::FILTER_NEAREST, gfxMatrix(),
                     gfxRect(gfxPoint(0,0), gfxIntSize(imageIntSize.width,
                                                       imageIntSize.height)),
                     nsIntRect(nsIntPoint(0,0), imageIntSize),
                     imageIntSize, aFlags);
  if (NS_SUCCEEDED(rv)) {
    *_retval = surface.forget().get();
  }

  return rv;
}





NS_IMETHODIMP
VectorImage::ExtractFrame(uint32_t aWhichFrame,
                          const nsIntRect& aRegion,
                          uint32_t aFlags,
                          imgIContainer** _retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  
  
  
  
  
  if (aWhichFrame != FRAME_CURRENT) {
    NS_WARNING("VectorImage::ExtractFrame with something other than "
               "FRAME_CURRENT isn't supported yet. Assuming FRAME_CURRENT.");
  }

  
  
  

  
  nsRefPtr<VectorImage> extractedImg = new VectorImage();
  extractedImg->mSVGDocumentWrapper = mSVGDocumentWrapper;
  extractedImg->mAnimationMode = kDontAnimMode;

  extractedImg->mRestrictedRegion.x = aRegion.x;
  extractedImg->mRestrictedRegion.y = aRegion.y;

  
  extractedImg->mRestrictedRegion.width  = std::max(aRegion.width,  0);
  extractedImg->mRestrictedRegion.height = std::max(aRegion.height, 0);

  extractedImg->mIsInitialized = true;
  extractedImg->mIsFullyLoaded = true;
  extractedImg->mHaveRestrictedRegion = true;

  *_retval = extractedImg.forget().get();
  return NS_OK;
}










NS_IMETHODIMP
VectorImage::Draw(gfxContext* aContext,
                  gfxPattern::GraphicsFilter aFilter,
                  const gfxMatrix& aUserSpaceToImageSpace,
                  const gfxRect& aFill,
                  const nsIntRect& aSubimage,
                  const nsIntSize& aViewportSize,
                  uint32_t aFlags)
{
  NS_ENSURE_ARG_POINTER(aContext);
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  if (mIsDrawing) {
    NS_WARNING("Refusing to make re-entrant call to VectorImage::Draw");
    return NS_ERROR_FAILURE;
  }
  mIsDrawing = true;

  mSVGDocumentWrapper->UpdateViewportBounds(aViewportSize);
  mSVGDocumentWrapper->FlushImageTransformInvalidation();

  nsIntSize imageSize = mHaveRestrictedRegion ?
    mRestrictedRegion.Size() : aViewportSize;

  
  
  
  gfxIntSize imageSizeGfx(imageSize.width, imageSize.height);

  
  gfxRect sourceRect = aUserSpaceToImageSpace.Transform(aFill);
  gfxRect imageRect(0, 0, imageSize.width, imageSize.height);
  gfxRect subimage(aSubimage.x, aSubimage.y, aSubimage.width, aSubimage.height);


  nsRefPtr<gfxDrawingCallback> cb =
    new SVGDrawingCallback(mSVGDocumentWrapper,
                           mHaveRestrictedRegion ?
                           mRestrictedRegion :
                           nsIntRect(nsIntPoint(0, 0), aViewportSize),
                           aFlags);

  nsRefPtr<gfxDrawable> drawable = new gfxCallbackDrawable(cb, imageSizeGfx);

  gfxUtils::DrawPixelSnapped(aContext, drawable,
                             aUserSpaceToImageSpace,
                             subimage, sourceRect, imageRect, aFill,
                             gfxASurface::ImageFormatARGB32, aFilter);

  mIsDrawing = false;
  return NS_OK;
}



nsIFrame*
VectorImage::GetRootLayoutFrame()
{
  if (mError)
    return nullptr;

  return mSVGDocumentWrapper->GetRootLayoutFrame();
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






NS_IMETHODIMP
VectorImage::OnStartRequest(nsIRequest* aRequest, nsISupports* aCtxt)
{
  NS_ABORT_IF_FALSE(!mSVGDocumentWrapper,
                    "Repeated call to OnStartRequest -- can this happen?");

  mSVGDocumentWrapper = new SVGDocumentWrapper();
  nsresult rv = mSVGDocumentWrapper->OnStartRequest(aRequest, aCtxt);
  if (NS_FAILED(rv)) {
    mSVGDocumentWrapper = nullptr;
    mError = true;
    return rv;
  }

  
  
  
  if (mStatusTracker) {
    mStatusTracker->GetDecoderObserver()->OnStartDecode();
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
  NS_ABORT_IF_FALSE(mParseCompleteListener, "Should have the parse complete listener");
  NS_ABORT_IF_FALSE(mLoadEventListener, "Should have the load event listener");

  if (!mSVGDocumentWrapper->GetRootSVGElem()) {
    
    
    
    
    OnSVGDocumentError();
  }
}

void
VectorImage::CancelAllListeners()
{
  NS_ABORT_IF_FALSE(mParseCompleteListener, "Should have the parse complete listener");
  NS_ABORT_IF_FALSE(mLoadEventListener, "Should have the load event listener");

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
  NS_ABORT_IF_FALSE(mSVGDocumentWrapper->GetRootSVGElem(), "Should have parsed successfully");
  NS_ABORT_IF_FALSE(!mIsFullyLoaded && !mHaveAnimations,
                    "These flags shouldn't get set until OnSVGDocumentLoaded. "
                    "Duplicate calls to OnSVGDocumentLoaded?");

  CancelAllListeners();

  
  mSVGDocumentWrapper->FlushLayout();

  mIsFullyLoaded = true;
  mHaveAnimations = mSVGDocumentWrapper->IsAnimated();

  
  mRenderingObserver = new SVGRootRenderingObserver(mSVGDocumentWrapper, this);

  
  if (mStatusTracker) {
    imgDecoderObserver* observer = mStatusTracker->GetDecoderObserver();

    observer->OnStartContainer(); 
    observer->FrameChanged(&nsIntRect::GetMaxSizedIntRect());
    observer->OnStopFrame();
    observer->OnStopDecode(NS_OK); 
  }

  EvaluateAnimation();
}

void
VectorImage::OnSVGDocumentError()
{
  CancelAllListeners();

  
  
  
  mError = true;

  if (mStatusTracker) {
    
    mStatusTracker->GetDecoderObserver()->OnStopDecode(NS_ERROR_FAILURE);
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
VectorImage::InvalidateObserver()
{
  if (mStatusTracker) {
    imgDecoderObserver* observer = mStatusTracker->GetDecoderObserver();
    observer->FrameChanged(&nsIntRect::GetMaxSizedIntRect());
    observer->OnStopFrame();
  }
}

} 
} 
