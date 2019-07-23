



































#include "nsSVGPathGeometryFrame.h"
#include "nsIDOMSVGMatrix.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsStubImageDecoderObserver.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMSVGImageElement.h"
#include "nsSVGImageElement.h"
#include "nsSVGUtils.h"
#include "nsSVGMatrix.h"
#include "gfxContext.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIImage.h"

class nsSVGImageFrame;

class nsSVGImageListener : public nsStubImageDecoderObserver
{
public:
  nsSVGImageListener(nsSVGImageFrame *aFrame);

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest, nsresult status,
                          const PRUnichar *statusArg);
  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer, gfxIImageFrame *newframe,
                          nsRect * dirtyRect);
  
  NS_IMETHOD OnStartContainer(imgIRequest *aRequest,
                              imgIContainer *aContainer);

  void SetFrame(nsSVGImageFrame *frame) { mFrame = frame; }

private:
  nsSVGImageFrame *mFrame;
};


class nsSVGImageFrame : public nsSVGPathGeometryFrame
{
  friend nsIFrame*
  NS_NewSVGImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

protected:
  nsSVGImageFrame(nsStyleContext* aContext) : nsSVGPathGeometryFrame(aContext) {}
  virtual ~nsSVGImageFrame();

public:
  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);

  
  virtual PRUint16 GetHittestMask();

  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGImage"), aResult);
  }
#endif

private:
  already_AddRefed<nsIDOMSVGMatrix> GetImageTransform();

  nsCOMPtr<imgIDecoderObserver> mListener;

  nsCOMPtr<imgIContainer> mImageContainer;

  friend class nsSVGImageListener;
};




nsIFrame*
NS_NewSVGImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGImageFrame(aContext);
}

nsSVGImageFrame::~nsSVGImageFrame()
{
  
  if (mListener) {
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
    if (imageLoader) {
      imageLoader->RemoveObserver(mListener);
    }
    reinterpret_cast<nsSVGImageListener*>(mListener.get())->SetFrame(nsnull);
  }
  mListener = nsnull;
}

NS_IMETHODIMP
nsSVGImageFrame::Init(nsIContent* aContent,
                      nsIFrame* aParent,
                      nsIFrame* aPrevInFlow)
{
#ifdef DEBUG
  nsCOMPtr<nsIDOMSVGImageElement> image = do_QueryInterface(aContent);
  NS_ASSERTION(image, "Content is not an SVG image!");
#endif

  nsresult rv = nsSVGPathGeometryFrame::Init(aContent, aParent, aPrevInFlow);
  if (NS_FAILED(rv)) return rv;
  
  mListener = new nsSVGImageListener(this);
  if (!mListener) return NS_ERROR_OUT_OF_MEMORY;
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(imageLoader, NS_ERROR_UNEXPECTED);
  imageLoader->AddObserver(mListener);

  return NS_OK; 
}




NS_IMETHODIMP
nsSVGImageFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  PRInt32         aModType)
{
   if (aNameSpaceID == kNameSpaceID_None &&
       (aAttribute == nsGkAtoms::x ||
        aAttribute == nsGkAtoms::y ||
        aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height ||
        aAttribute == nsGkAtoms::preserveAspectRatio)) {
     nsSVGUtils::UpdateGraphic(this);
     return NS_OK;
   }

   return nsSVGPathGeometryFrame::AttributeChanged(aNameSpaceID,
                                                   aAttribute, aModType);
}

already_AddRefed<nsIDOMSVGMatrix>
nsSVGImageFrame::GetImageTransform()
{
  nsCOMPtr<nsIDOMSVGMatrix> ctm;
  GetCanvasTM(getter_AddRefs(ctm));

  float x, y, width, height;
  nsSVGImageElement *element = static_cast<nsSVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

  PRInt32 nativeWidth, nativeHeight;
  mImageContainer->GetWidth(&nativeWidth);
  mImageContainer->GetHeight(&nativeHeight);

  nsCOMPtr<nsIDOMSVGMatrix> trans, ctmXY, fini;
  trans = nsSVGUtils::GetViewBoxTransform(width, height,
                                          0, 0,
                                          nativeWidth, nativeHeight,
                                          element->mPreserveAspectRatio);
  ctm->Translate(x, y, getter_AddRefs(ctmXY));
  ctmXY->Multiply(trans, getter_AddRefs(fini));

  nsIDOMSVGMatrix *retval = nsnull;
  fini.swap(retval);
  return retval;
}



NS_IMETHODIMP
nsSVGImageFrame::PaintSVG(nsSVGRenderState *aContext,
                          const nsIntRect *aDirtyRect)
{
  nsresult rv = NS_OK;

  if (!GetStyleVisibility()->IsVisible())
    return NS_OK;

  float x, y, width, height;
  nsSVGElement *element = static_cast<nsSVGElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);
  if (width <= 0 || height <= 0)
    return NS_OK;

  if (!mImageContainer) {
    nsCOMPtr<imgIRequest> currentRequest;
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
    if (imageLoader)
      imageLoader->GetRequest(nsIImageLoadingContent::CURRENT_REQUEST,
                              getter_AddRefs(currentRequest));

    if (currentRequest)
      currentRequest->GetImage(getter_AddRefs(mImageContainer));
  }

  nsCOMPtr<gfxIImageFrame> currentFrame;
  if (mImageContainer)
    mImageContainer->GetCurrentFrame(getter_AddRefs(currentFrame));

  nsRefPtr<gfxPattern> thebesPattern = nsnull;
  if (currentFrame) {
    nsCOMPtr<nsIImage> img(do_GetInterface(currentFrame));

    img->GetPattern(getter_AddRefs(thebesPattern));
  }

  if (thebesPattern) {

    switch (GetStyleSVG()->mImageRendering) {
    case NS_STYLE_IMAGE_RENDERING_OPTIMIZESPEED:
      thebesPattern->SetFilter(gfxPattern::FILTER_FAST);
      break;
    case NS_STYLE_IMAGE_RENDERING_OPTIMIZEQUALITY:
      thebesPattern->SetFilter(gfxPattern::FILTER_BEST);
      break;
    case NS_STYLE_IMAGE_RENDERING_DISABLE_RESAMPLING:
      thebesPattern->SetFilter(gfxPattern::FILTER_NEAREST);
      break;
    default:
      thebesPattern->SetFilter(gfxPattern::FILTER_GOOD);
      break;
    }

    gfxContext *gfx = aContext->GetGfxContext();

    if (GetStyleDisplay()->IsScrollableOverflow()) {
      gfx->Save();

      nsCOMPtr<nsIDOMSVGMatrix> ctm;
      GetCanvasTM(getter_AddRefs(ctm));

      if (ctm) {
        nsSVGUtils::SetClipRect(gfx, ctm, x, y, width, height);
      }
    }

    nsCOMPtr<nsIDOMSVGMatrix> fini = GetImageTransform();

    
    
    
    float opacity = 1.0f;
    if (nsSVGUtils::CanOptimizeOpacity(this)) {
      opacity = GetStyleDisplay()->mOpacity;
    }

    PRInt32 nativeWidth, nativeHeight;
    currentFrame->GetWidth(&nativeWidth);
    currentFrame->GetHeight(&nativeHeight);

    nsSVGUtils::CompositePatternMatrix(gfx, thebesPattern, fini, nativeWidth, nativeHeight, opacity);

    if (GetStyleDisplay()->IsScrollableOverflow())
      gfx->Restore();
  }

  return rv;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGImageFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  if (GetStyleDisplay()->IsScrollableOverflow() && mImageContainer) {
    PRInt32 nativeWidth, nativeHeight;
    mImageContainer->GetWidth(&nativeWidth);
    mImageContainer->GetHeight(&nativeHeight);

    nsCOMPtr<nsIDOMSVGMatrix> fini = GetImageTransform();

    if (!nsSVGUtils::HitTestRect(fini,
                                 0, 0, nativeWidth, nativeHeight,
                                 PresContext()->AppUnitsToDevPixels(aPoint.x),
                                 PresContext()->AppUnitsToDevPixels(aPoint.y))) {
      return nsnull;
    }
  }

  return nsSVGPathGeometryFrame::GetFrameForPoint(aPoint);
}

nsIAtom *
nsSVGImageFrame::GetType() const
{
  return nsGkAtoms::svgImageFrame;
}






PRUint16
nsSVGImageFrame::GetHittestMask()
{
  PRUint16 mask = 0;

  switch(GetStyleSVG()->mPointerEvents) {
    case NS_STYLE_POINTER_EVENTS_NONE:
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEPAINTED:
      if (GetStyleVisibility()->IsVisible()) {
        
        mask |= HITTEST_MASK_FILL;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEFILL:
    case NS_STYLE_POINTER_EVENTS_VISIBLESTROKE:
    case NS_STYLE_POINTER_EVENTS_VISIBLE:
      if (GetStyleVisibility()->IsVisible()) {
        mask |= HITTEST_MASK_FILL;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_PAINTED:
      
      mask |= HITTEST_MASK_FILL;
      break;
    case NS_STYLE_POINTER_EVENTS_FILL:
    case NS_STYLE_POINTER_EVENTS_STROKE:
    case NS_STYLE_POINTER_EVENTS_ALL:
      mask |= HITTEST_MASK_FILL;
      break;
    default:
      NS_ERROR("not reached");
      break;
  }

  return mask;
}




NS_IMPL_ISUPPORTS2(nsSVGImageListener,
                   imgIDecoderObserver,
                   imgIContainerObserver)

nsSVGImageListener::nsSVGImageListener(nsSVGImageFrame *aFrame) :  mFrame(aFrame)
{
}

NS_IMETHODIMP nsSVGImageListener::OnStopDecode(imgIRequest *aRequest,
                                               nsresult status,
                                               const PRUnichar *statusArg)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsSVGUtils::UpdateGraphic(mFrame);
  return NS_OK;
}

NS_IMETHODIMP nsSVGImageListener::FrameChanged(imgIContainer *aContainer,
                                               gfxIImageFrame *newframe,
                                               nsRect * dirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  nsSVGUtils::UpdateGraphic(mFrame);
  return NS_OK;
}

NS_IMETHODIMP nsSVGImageListener::OnStartContainer(imgIRequest *aRequest,
                                                   imgIContainer *aContainer)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  mFrame->mImageContainer = aContainer;
  nsSVGUtils::UpdateGraphic(mFrame);

  return NS_OK;
}

