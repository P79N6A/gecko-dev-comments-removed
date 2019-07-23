



































#include "nsSVGPathGeometryFrame.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGAnimPresAspRatio.h"
#include "nsIDOMSVGPresAspectRatio.h"
#include "imgIContainer.h"
#include "gfxIImageFrame.h"
#include "nsStubImageDecoderObserver.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMSVGImageElement.h"
#include "nsSVGElement.h"
#include "nsSVGUtils.h"
#include "nsSVGMatrix.h"
#include "gfxContext.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsThebesImage.h"

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
protected:
  friend nsIFrame*
  NS_NewSVGImageFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext);

  virtual ~nsSVGImageFrame();
  NS_IMETHOD InitSVG();

public:
  nsSVGImageFrame(nsStyleContext* aContext) : nsSVGPathGeometryFrame(aContext) {}

  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, nsRect *aDirtyRect);
  NS_IMETHOD GetFrameForPointSVG(float x, float y, nsIFrame** hit);

  
  
  virtual nsresult GetStrokePaintType() { return eStyleSVGPaintType_None; }
  virtual nsresult GetFillPaintType() { return eStyleSVGPaintType_Color; }

  
  virtual PRUint16 GetHittestMask();

  
  NS_IMETHOD  AttributeChanged(PRInt32         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               PRInt32         aModType);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGImage"), aResult);
  }
#endif

private:
  already_AddRefed<nsIDOMSVGMatrix> GetImageTransform();

  nsCOMPtr<nsIDOMSVGPreserveAspectRatio> mPreserveAspectRatio;

  nsCOMPtr<imgIDecoderObserver> mListener;

  nsCOMPtr<imgIContainer> mImageContainer;

  friend class nsSVGImageListener;
};




nsIFrame*
NS_NewSVGImageFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGImageElement> Rect = do_QueryInterface(aContent);
  if (!Rect) {
#ifdef DEBUG
    printf("warning: trying to construct an SVGImageFrame for a content element that doesn't support the right interfaces\n");
#endif
    return nsnull;
  }

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
nsSVGImageFrame::InitSVG()
{
  nsresult rv = nsSVGPathGeometryFrame::InitSVG();
  if (NS_FAILED(rv)) return rv;
  
  nsCOMPtr<nsIDOMSVGImageElement> Rect = do_QueryInterface(mContent);
  NS_ASSERTION(Rect,"wrong content element");

  {
    nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> ratio;
    Rect->GetPreserveAspectRatio(getter_AddRefs(ratio));
    ratio->GetAnimVal(getter_AddRefs(mPreserveAspectRatio));
    NS_ASSERTION(mPreserveAspectRatio, "no preserveAspectRatio");
    if (!mPreserveAspectRatio) return NS_ERROR_FAILURE;
  }

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
     UpdateGraphic();
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
  nsSVGElement *element = static_cast<nsSVGElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

  PRInt32 nativeWidth, nativeHeight;
  mImageContainer->GetWidth(&nativeWidth);
  mImageContainer->GetHeight(&nativeHeight);

  nsCOMPtr<nsIDOMSVGImageElement> image = do_QueryInterface(mContent);
  nsCOMPtr<nsIDOMSVGAnimatedPreserveAspectRatio> ratio;
  image->GetPreserveAspectRatio(getter_AddRefs(ratio));

  nsCOMPtr<nsIDOMSVGMatrix> trans, ctmXY, fini;
  trans = nsSVGUtils::GetViewBoxTransform(width, height,
                                          0, 0,
                                          nativeWidth, nativeHeight,
                                          ratio);
  ctm->Translate(x, y, getter_AddRefs(ctmXY));
  ctmXY->Multiply(trans, getter_AddRefs(fini));

  nsIDOMSVGMatrix *retval = nsnull;
  fini.swap(retval);
  return retval;
}



NS_IMETHODIMP
nsSVGImageFrame::PaintSVG(nsSVGRenderState *aContext, nsRect *aDirtyRect)
{
  nsresult rv = NS_OK;

  if (!GetStyleVisibility()->IsVisible())
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

  gfxASurface *thebesSurface = nsnull;
  if (currentFrame) {
    nsCOMPtr<nsIImage> img(do_GetInterface(currentFrame));

    nsThebesImage *thebesImage = nsnull;
    if (img)
      thebesImage = static_cast<nsThebesImage*>(img.get());

    if (thebesImage)
      thebesSurface = thebesImage->ThebesSurface();
  }

  if (thebesSurface) {
    gfxContext *gfx = aContext->GetGfxContext();

    if (GetStyleDisplay()->IsScrollableOverflow()) {
      gfx->Save();

      nsCOMPtr<nsIDOMSVGMatrix> ctm;
      GetCanvasTM(getter_AddRefs(ctm));

      if (ctm) {
        float x, y, width, height;
        nsSVGElement *element = static_cast<nsSVGElement*>(mContent);
        element->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

        nsSVGUtils::SetClipRect(gfx, ctm, x, y, width, height);
      }
    }

    nsCOMPtr<nsIDOMSVGMatrix> fini = GetImageTransform();

    
    
    
    float opacity = 1.0f;
    if (nsSVGUtils::CanOptimizeOpacity(this)) {
      opacity = GetStyleDisplay()->mOpacity;
    }

    nsSVGUtils::CompositeSurfaceMatrix(gfx, thebesSurface, fini, opacity);

    if (GetStyleDisplay()->IsScrollableOverflow())
      gfx->Restore();
  }

  return rv;
}

NS_IMETHODIMP
nsSVGImageFrame::GetFrameForPointSVG(float x, float y, nsIFrame** hit)
{
  if (GetStyleDisplay()->IsScrollableOverflow() && mImageContainer) {
    PRInt32 nativeWidth, nativeHeight;
    mImageContainer->GetWidth(&nativeWidth);
    mImageContainer->GetHeight(&nativeHeight);

    nsCOMPtr<nsIDOMSVGMatrix> fini = GetImageTransform();

    if (!nsSVGUtils::HitTestRect(fini,
                                 0, 0, nativeWidth, nativeHeight,
                                 x, y)) {
      *hit = nsnull;
      return NS_OK;
    }
  }

  return nsSVGPathGeometryFrame::GetFrameForPointSVG(x, y, hit);
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

  mFrame->UpdateGraphic();
  return NS_OK;
}

NS_IMETHODIMP nsSVGImageListener::FrameChanged(imgIContainer *aContainer,
                                               gfxIImageFrame *newframe,
                                               nsRect * dirtyRect)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  mFrame->UpdateGraphic();
  return NS_OK;
}

NS_IMETHODIMP nsSVGImageListener::OnStartContainer(imgIRequest *aRequest,
                                                   imgIContainer *aContainer)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  mFrame->mImageContainer = aContainer;
  mFrame->UpdateGraphic();

  return NS_OK;
}

