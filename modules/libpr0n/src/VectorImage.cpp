





































#include "VectorImage.h"
#include "imgIDecoderObserver.h"
#include "SVGDocumentWrapper.h"
#include "gfxContext.h"
#include "gfxPlatform.h"
#include "nsPresContext.h"
#include "nsRect.h"
#include "nsIDocumentViewer.h"
#include "nsIObserverService.h"
#include "nsIPresShell.h"
#include "nsIStreamListener.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsSVGUtils.h"  
#include "nsSVGEffects.h" 
#include "gfxDrawable.h"
#include "gfxUtils.h"
#include "nsSVGSVGElement.h"

using namespace mozilla::dom;

namespace mozilla {
namespace imagelib {


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
      mInObserverList = PR_TRUE;
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
      mInObserverList = PR_TRUE;
    }
  }

  
  nsRefPtr<SVGDocumentWrapper> mDocWrapper;
  VectorImage* mVectorImage;   
};


class SVGDrawingCallback : public gfxDrawingCallback {
public:
  SVGDrawingCallback(SVGDocumentWrapper* aSVGDocumentWrapper,
                     const nsIntRect& aViewport,
                     PRUint32 aImageFlags) :
    mSVGDocumentWrapper(aSVGDocumentWrapper),
    mViewport(aViewport),
    mImageFlags(aImageFlags)
  {}
  virtual PRBool operator()(gfxContext* aContext,
                            const gfxRect& aFillRect,
                            const gfxPattern::GraphicsFilter& aFilter,
                            const gfxMatrix& aTransform);
private:
  nsRefPtr<SVGDocumentWrapper> mSVGDocumentWrapper;
  const nsIntRect mViewport;
  PRUint32        mImageFlags;
};


PRBool
SVGDrawingCallback::operator()(gfxContext* aContext,
                               const gfxRect& aFillRect,
                               const gfxPattern::GraphicsFilter& aFilter,
                               const gfxMatrix& aTransform)
{
  NS_ABORT_IF_FALSE(mSVGDocumentWrapper, "need an SVGDocumentWrapper");

  
  nsCOMPtr<nsIPresShell> presShell;
  if (NS_FAILED(mSVGDocumentWrapper->GetPresShell(getter_AddRefs(presShell)))) {
    NS_WARNING("Unable to draw -- presShell lookup failed");
    return NS_ERROR_FAILURE;
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

  PRUint32 renderDocFlags = nsIPresShell::RENDER_IGNORE_VIEWPORT_SCROLLING;
  if (!(mImageFlags & imgIContainer::FLAG_SYNC_DECODE)) {
    renderDocFlags |= nsIPresShell::RENDER_ASYNC_DECODE_IMAGES;
  }

  presShell->RenderDocument(svgRect, renderDocFlags,
                            NS_RGBA(0, 0, 0, 0), 
                            aContext);

  return PR_TRUE;
}


NS_IMPL_ISUPPORTS3(VectorImage,
                   imgIContainer,
                   nsIStreamListener,
                   nsIRequestObserver)




VectorImage::VectorImage(imgStatusTracker* aStatusTracker) :
  Image(aStatusTracker), 
  mRestrictedRegion(0, 0, 0, 0),
  mLastRenderedSize(0, 0),
  mIsInitialized(PR_FALSE),
  mIsFullyLoaded(PR_FALSE),
  mIsDrawing(PR_FALSE),
  mHaveAnimations(PR_FALSE),
  mHaveRestrictedRegion(PR_FALSE)
{
}

VectorImage::~VectorImage()
{
}




nsresult
VectorImage::Init(imgIDecoderObserver* aObserver,
                  const char* aMimeType,
                  const char* aURIString,
                  PRUint32 aFlags)
{
  
  if (mIsInitialized)
    return NS_ERROR_ILLEGAL_VALUE;

  NS_ABORT_IF_FALSE(!mIsFullyLoaded && !mHaveAnimations &&
                    !mHaveRestrictedRegion && !mError,
                    "Flags unexpectedly set before initialization");

  mObserver = do_GetWeakReference(aObserver);
  NS_ABORT_IF_FALSE(!strcmp(aMimeType, SVG_MIMETYPE), "Unexpected mimetype");

  mIsInitialized = PR_TRUE;

  return NS_OK;
}

void
VectorImage::GetCurrentFrameRect(nsIntRect& aRect)
{
  aRect = nsIntRect::GetMaxSizedIntRect();
}

PRUint32
VectorImage::GetDecodedDataSize()
{
  
  return sizeof(*this);
}

PRUint32
VectorImage::GetSourceDataSize()
{
  
  
  
  return 0;
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

PRBool
VectorImage::ShouldAnimate()
{
  return Image::ShouldAnimate() && mIsFullyLoaded && mHaveAnimations;
}






NS_IMETHODIMP
VectorImage::GetWidth(PRInt32* aWidth)
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



NS_IMETHODIMP
VectorImage::GetHeight(PRInt32* aHeight)
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
VectorImage::GetType(PRUint16* aType)
{
  NS_ENSURE_ARG_POINTER(aType);

  *aType = GetType();
  return NS_OK;
}



NS_IMETHODIMP_(PRUint16)
VectorImage::GetType()
{
  return imgIContainer::TYPE_VECTOR;
}



NS_IMETHODIMP
VectorImage::GetAnimated(PRBool* aAnimated)
{
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  *aAnimated = mSVGDocumentWrapper->IsAnimated();
  return NS_OK;
}



NS_IMETHODIMP
VectorImage::GetCurrentFrameIsOpaque(PRBool* aIsOpaque)
{
  NS_ENSURE_ARG_POINTER(aIsOpaque);
  *aIsOpaque = PR_FALSE;   
  return NS_OK;
}




NS_IMETHODIMP
VectorImage::GetFrame(PRUint32 aWhichFrame,
                      PRUint32 aFlags,
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
VectorImage::CopyFrame(PRUint32 aWhichFrame,
                       PRUint32 aFlags,
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
VectorImage::ExtractFrame(PRUint32 aWhichFrame,
                          const nsIntRect& aRegion,
                          PRUint32 aFlags,
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

  
  extractedImg->mRestrictedRegion.width  = NS_MAX(aRegion.width,  0);
  extractedImg->mRestrictedRegion.height = NS_MAX(aRegion.height, 0);

  extractedImg->mIsInitialized = PR_TRUE;
  extractedImg->mIsFullyLoaded = PR_TRUE;
  extractedImg->mHaveRestrictedRegion = PR_TRUE;

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
                  PRUint32 aFlags)
{
  NS_ENSURE_ARG_POINTER(aContext);
  if (mError || !mIsFullyLoaded)
    return NS_ERROR_FAILURE;

  if (mIsDrawing) {
    NS_WARNING("Refusing to make re-entrant call to VectorImage::Draw");
    return NS_ERROR_FAILURE;
  }
  mIsDrawing = PR_TRUE;

  if (aViewportSize != mLastRenderedSize) {
    mSVGDocumentWrapper->UpdateViewportBounds(aViewportSize);
    mLastRenderedSize = aViewportSize;
  }
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

  mIsDrawing = PR_FALSE;
  return NS_OK;
}



nsIFrame*
VectorImage::GetRootLayoutFrame()
{
  return mSVGDocumentWrapper->GetRootLayoutFrame();
}



NS_IMETHODIMP
VectorImage::RequestDecode()
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
    mSVGDocumentWrapper = nsnull;
    mError = PR_TRUE;
  }

  return rv;
}




NS_IMETHODIMP
VectorImage::OnStopRequest(nsIRequest* aRequest, nsISupports* aCtxt,
                           nsresult aStatus)
{
  if (mError)
    return NS_ERROR_FAILURE;

  NS_ABORT_IF_FALSE(!mIsFullyLoaded && !mHaveAnimations,
                    "these flags shouldn't get set until OnStopRequest. "
                    "Duplicate calls to OnStopRequest?");

  nsresult rv = mSVGDocumentWrapper->OnStopRequest(aRequest, aCtxt, aStatus);
  if (!mSVGDocumentWrapper->ParsedSuccessfully()) {
    
    
    
    mError = PR_TRUE;
    return rv;
  }

  mIsFullyLoaded = PR_TRUE;
  mHaveAnimations = mSVGDocumentWrapper->IsAnimated();

  
  mRenderingObserver = new SVGRootRenderingObserver(mSVGDocumentWrapper, this);

  
  nsCOMPtr<imgIDecoderObserver> observer = do_QueryReferent(mObserver);
  if (observer) {
    
    observer->OnStartContainer(nsnull, this);

    observer->FrameChanged(this, &nsIntRect::GetMaxSizedIntRect());
    observer->OnStopFrame(nsnull, 0);
    observer->OnStopDecode(nsnull, NS_OK, nsnull);
  }
  EvaluateAnimation();

  return rv;
}








NS_IMETHODIMP
VectorImage::OnDataAvailable(nsIRequest* aRequest, nsISupports* aCtxt,
                             nsIInputStream* aInStr, PRUint32 aSourceOffset,
                             PRUint32 aCount)
{
  return mSVGDocumentWrapper->OnDataAvailable(aRequest, aCtxt, aInStr,
                                              aSourceOffset, aCount);
}




void
VectorImage::InvalidateObserver()
{
  if (!mObserver)
    return;

  nsCOMPtr<imgIContainerObserver> containerObs(do_QueryReferent(mObserver));
  if (containerObs) {
    containerObs->FrameChanged(this, &nsIntRect::GetMaxSizedIntRect());
  }

  nsCOMPtr<imgIDecoderObserver> decoderObs(do_QueryReferent(mObserver));
  if (decoderObs) {
    decoderObs->OnStopFrame(nsnull, imgIContainer::FRAME_CURRENT);
  }
}

} 
} 
