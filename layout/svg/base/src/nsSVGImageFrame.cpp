



































#include "nsSVGPathGeometryFrame.h"
#include "imgIContainer.h"
#include "nsStubImageDecoderObserver.h"
#include "nsImageLoadingContent.h"
#include "nsIDOMSVGImageElement.h"
#include "nsLayoutUtils.h"
#include "nsSVGImageElement.h"
#include "nsSVGUtils.h"
#include "gfxContext.h"
#include "gfxMatrix.h"
#include "nsIInterfaceRequestorUtils.h"
#include "gfxPlatform.h"
#include "nsSVGSVGElement.h"

class nsSVGImageFrame;

class nsSVGImageListener : public nsStubImageDecoderObserver
{
public:
  nsSVGImageListener(nsSVGImageFrame *aFrame);

  NS_DECL_ISUPPORTS
  
  NS_IMETHOD OnStopDecode(imgIRequest *aRequest, nsresult status,
                          const PRUnichar *statusArg);
  
  NS_IMETHOD FrameChanged(imgIContainer *aContainer,
                          const nsIntRect *aDirtyRect);
  
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
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD PaintSVG(nsSVGRenderState *aContext, const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);

  
  NS_IMETHOD UpdateCoveredRegion();
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
  gfxMatrix GetRasterImageTransform(PRInt32 aNativeWidth,
                                    PRInt32 aNativeHeight);
  gfxMatrix GetVectorImageTransform();
  PRBool    TransformContextForPainting(gfxContext* aGfxContext);

  nsCOMPtr<imgIDecoderObserver> mListener;

  nsCOMPtr<imgIContainer> mImageContainer;

  friend class nsSVGImageListener;
};




nsIFrame*
NS_NewSVGImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGImageFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGImageFrame)

nsSVGImageFrame::~nsSVGImageFrame()
{
  
  if (mListener) {
    nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
    if (imageLoader) {
      
      
      
      nsCxPusher pusher;
      pusher.PushNull();

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

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

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

gfxMatrix
nsSVGImageFrame::GetRasterImageTransform(PRInt32 aNativeWidth, PRInt32 aNativeHeight)
{
  float x, y, width, height;
  nsSVGImageElement *element = static_cast<nsSVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

  gfxMatrix viewBoxTM =
    nsSVGUtils::GetViewBoxTransform(element,
                                    width, height,
                                    0, 0, aNativeWidth, aNativeHeight,
                                    element->mPreserveAspectRatio);

  return viewBoxTM * gfxMatrix().Translate(gfxPoint(x, y)) * GetCanvasTM();
}

gfxMatrix
nsSVGImageFrame::GetVectorImageTransform()
{
  float x, y, width, height;
  nsSVGImageElement *element = static_cast<nsSVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

  
  
  

  return gfxMatrix().Translate(gfxPoint(x, y)) * GetCanvasTM();
}

PRBool
nsSVGImageFrame::TransformContextForPainting(gfxContext* aGfxContext)
{
  gfxMatrix imageTransform;
  if (mImageContainer->GetType() == imgIContainer::TYPE_VECTOR) {
    imageTransform = GetVectorImageTransform();
  } else {
    PRInt32 nativeWidth, nativeHeight;
    if (NS_FAILED(mImageContainer->GetWidth(&nativeWidth)) ||
        NS_FAILED(mImageContainer->GetHeight(&nativeHeight)) ||
        nativeWidth == 0 || nativeHeight == 0) {
      return PR_FALSE;
    }
    imageTransform = GetRasterImageTransform(nativeWidth, nativeHeight);
  }

  if (imageTransform.IsSingular()) {
    return PR_FALSE;
  }

  
  
  nscoord appUnitsPerDevPx = PresContext()->AppUnitsPerDevPixel();
  gfxFloat pageZoomFactor =
    nsPresContext::AppUnitsToFloatCSSPixels(appUnitsPerDevPx);
  aGfxContext->Multiply(imageTransform.Scale(pageZoomFactor, pageZoomFactor));

  return PR_TRUE;
}



NS_IMETHODIMP
nsSVGImageFrame::PaintSVG(nsSVGRenderState *aContext,
                          const nsIntRect *aDirtyRect)
{
  nsresult rv = NS_OK;

  if (!GetStyleVisibility()->IsVisible())
    return NS_OK;

  float x, y, width, height;
  nsSVGImageElement *imgElem = static_cast<nsSVGImageElement*>(mContent);
  imgElem->GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);
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

  if (mImageContainer) {
    gfxContext* ctx = aContext->GetGfxContext();
    gfxContextAutoSaveRestore autoRestorer(ctx);

    if (GetStyleDisplay()->IsScrollableOverflow()) {
      gfxRect clipRect = nsSVGUtils::GetClipRectForFrame(this, x, y,
                                                         width, height);
      nsSVGUtils::SetClipRect(ctx, GetCanvasTM(), clipRect);
    }

    if (!TransformContextForPainting(ctx)) {
      return NS_ERROR_FAILURE;
    }

    
    
    
    float opacity = 1.0f;
    if (nsSVGUtils::CanOptimizeOpacity(this)) {
      opacity = GetStyleDisplay()->mOpacity;
    }

    if (opacity != 1.0f) {
      ctx->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
    }

    nscoord appUnitsPerDevPx = PresContext()->AppUnitsPerDevPixel();
    nsRect dirtyRect; 
    if (aDirtyRect) {
      dirtyRect = aDirtyRect->ToAppUnits(appUnitsPerDevPx);
      
      dirtyRect.MoveBy(-mRect.TopLeft());
    }

    
    
    
    
    PRUint32 drawFlags = imgIContainer::FLAG_SYNC_DECODE;

    if (mImageContainer->GetType() == imgIContainer::TYPE_VECTOR) {
      nsIFrame* imgRootFrame = mImageContainer->GetRootLayoutFrame();
      if (!imgRootFrame) {
        
        return NS_OK;
      }

      
      nsSVGSVGElement* rootSVGElem =
        static_cast<nsSVGSVGElement*>(imgRootFrame->GetContent());
      if (!rootSVGElem || rootSVGElem->GetNameSpaceID() != kNameSpaceID_SVG ||
          rootSVGElem->Tag() != nsGkAtoms::svg) {
        NS_ABORT_IF_FALSE(PR_FALSE, "missing or non-<svg> root node!!");
        return PR_FALSE;
      }

      
      
      
      rootSVGElem->SetImageOverridePreserveAspectRatio(
        imgElem->mPreserveAspectRatio.GetAnimValue());
      nsRect destRect(0, 0,
                      appUnitsPerDevPx * width,
                      appUnitsPerDevPx * height);

      
      
      
      nsLayoutUtils::DrawSingleImage(
        aContext->GetRenderingContext(this),
        mImageContainer,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        destRect,
        aDirtyRect ? dirtyRect : destRect,
        drawFlags);

      rootSVGElem->ClearImageOverridePreserveAspectRatio();
    } else { 
      nsLayoutUtils::DrawSingleUnscaledImage(
        aContext->GetRenderingContext(this),
        mImageContainer,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        nsPoint(0, 0),
        aDirtyRect ? &dirtyRect : nsnull,
        drawFlags);
    }

    if (opacity != 1.0f) {
      ctx->PopGroupToSource();
      ctx->SetOperator(gfxContext::OPERATOR_OVER);
      ctx->Paint(opacity);
    }
    
  }

  return rv;
}

NS_IMETHODIMP_(nsIFrame*)
nsSVGImageFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  
  
  
  
  
  if (GetStyleDisplay()->IsScrollableOverflow() && mImageContainer) {
    if (mImageContainer->GetType() == imgIContainer::TYPE_RASTER) {
      PRInt32 nativeWidth, nativeHeight;
      if (NS_FAILED(mImageContainer->GetWidth(&nativeWidth)) ||
          NS_FAILED(mImageContainer->GetHeight(&nativeHeight)) ||
          nativeWidth == 0 || nativeHeight == 0) {
        return nsnull;
      }

      if (!nsSVGUtils::HitTestRect(
               GetRasterImageTransform(nativeWidth, nativeHeight),
               0, 0, nativeWidth, nativeHeight,
               PresContext()->AppUnitsToDevPixels(aPoint.x),
               PresContext()->AppUnitsToDevPixels(aPoint.y))) {
        return nsnull;
      }
    }
    
    
    
    
  }

  return nsSVGPathGeometryFrame::GetFrameForPoint(aPoint);
}

nsIAtom *
nsSVGImageFrame::GetType() const
{
  return nsGkAtoms::svgImageFrame;
}






NS_IMETHODIMP
nsSVGImageFrame::UpdateCoveredRegion()
{
  mRect.SetEmpty();

  gfxContext context(gfxPlatform::GetPlatform()->ScreenReferenceSurface());

  GeneratePath(&context);
  context.IdentityMatrix();

  gfxRect extent = context.GetUserPathExtent();

  if (!extent.IsEmpty()) {
    mRect = nsSVGUtils::ToAppPixelRect(PresContext(), extent);
  }

  return NS_OK;
}

PRUint16
nsSVGImageFrame::GetHittestMask()
{
  PRUint16 mask = 0;

  switch(GetStyleVisibility()->mPointerEvents) {
    case NS_STYLE_POINTER_EVENTS_NONE:
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEPAINTED:
    case NS_STYLE_POINTER_EVENTS_AUTO:
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
                                               const nsIntRect *aDirtyRect)
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

