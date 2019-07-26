





#include "gfxContext.h"
#include "gfxMatrix.h"
#include "gfxPlatform.h"
#include "imgIContainer.h"
#include "nsIDOMSVGImageElement.h"
#include "nsLayoutUtils.h"
#include "nsRenderingContext.h"
#include "imgINotificationObserver.h"
#include "nsSVGEffects.h"
#include "nsSVGImageElement.h"
#include "nsSVGPathGeometryFrame.h"
#include "nsSVGSVGElement.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"

using namespace mozilla;

class nsSVGImageFrame;

class nsSVGImageListener MOZ_FINAL : public imgINotificationObserver
{
public:
  nsSVGImageListener(nsSVGImageFrame *aFrame);

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  void SetFrame(nsSVGImageFrame *frame) { mFrame = frame; }

private:
  nsSVGImageFrame *mFrame;
};

typedef nsSVGPathGeometryFrame nsSVGImageFrameBase;

class nsSVGImageFrame : public nsSVGImageFrameBase
{
  friend nsIFrame*
  NS_NewSVGImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

protected:
  nsSVGImageFrame(nsStyleContext* aContext) : nsSVGImageFrameBase(aContext) {}
  virtual ~nsSVGImageFrame();

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  NS_IMETHOD PaintSVG(nsRenderingContext *aContext, const nsIntRect *aDirtyRect);
  NS_IMETHOD_(nsIFrame*) GetFrameForPoint(const nsPoint &aPoint);
  virtual void ReflowSVG();

  
  virtual uint16_t GetHitTestFlags();

  
  NS_IMETHOD  AttributeChanged(int32_t         aNameSpaceID,
                               nsIAtom*        aAttribute,
                               int32_t         aModType);
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  




  virtual nsIAtom* GetType() const;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGImage"), aResult);
  }
#endif

private:
  gfxMatrix GetRasterImageTransform(int32_t aNativeWidth,
                                    int32_t aNativeHeight,
                                    uint32_t aFor);
  gfxMatrix GetVectorImageTransform(uint32_t aFor);
  bool      TransformContextForPainting(gfxContext* aGfxContext);

  nsCOMPtr<imgINotificationObserver> mListener;

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
    reinterpret_cast<nsSVGImageListener*>(mListener.get())->SetFrame(nullptr);
  }
  mListener = nullptr;
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

  nsresult rv = nsSVGImageFrameBase::Init(aContent, aParent, aPrevInFlow);
  if (NS_FAILED(rv)) return rv;
  
  mListener = new nsSVGImageListener(this);
  if (!mListener) return NS_ERROR_OUT_OF_MEMORY;
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(imageLoader, NS_ERROR_UNEXPECTED);

  
  
  imageLoader->FrameCreated(this);

  
  
  
  nsCxPusher pusher;
  pusher.PushNull();

  imageLoader->AddObserver(mListener);

  return NS_OK; 
}

 void
nsSVGImageFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsCOMPtr<nsIImageLoadingContent> imageLoader =
    do_QueryInterface(nsFrame::mContent);

  if (imageLoader) {
    imageLoader->FrameDestroyed(this);
  }

  nsFrame::DestroyFrom(aDestructRoot);
}




NS_IMETHODIMP
nsSVGImageFrame::AttributeChanged(int32_t         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::x ||
        aAttribute == nsGkAtoms::y ||
        aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {
      nsSVGUtils::InvalidateAndScheduleReflowSVG(this);
      return NS_OK;
    }
    else if (aAttribute == nsGkAtoms::preserveAspectRatio) {
      nsSVGUtils::InvalidateBounds(this);
      return NS_OK;
    }
  }
  if (aNameSpaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {

    
    if (nsContentUtils::IsImageSrcSetDisabled()) {
      return NS_OK;
    }
    nsSVGImageElement *element = static_cast<nsSVGImageElement*>(mContent);

    if (element->mStringAttributes[nsSVGImageElement::HREF].IsExplicitlySet()) {
      element->LoadSVGImage(true, true);
    } else {
      element->CancelImageRequests(true);
    }
  }

  return nsSVGImageFrameBase::AttributeChanged(aNameSpaceID,
                                               aAttribute, aModType);
}

gfxMatrix
nsSVGImageFrame::GetRasterImageTransform(int32_t aNativeWidth,
                                         int32_t aNativeHeight,
                                         uint32_t aFor)
{
  float x, y, width, height;
  nsSVGImageElement *element = static_cast<nsSVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

  gfxMatrix viewBoxTM =
    SVGContentUtils::GetViewBoxTransform(element,
                                         width, height,
                                         0, 0, aNativeWidth, aNativeHeight,
                                         element->mPreserveAspectRatio);

  return viewBoxTM * gfxMatrix().Translate(gfxPoint(x, y)) * GetCanvasTM(aFor);
}

gfxMatrix
nsSVGImageFrame::GetVectorImageTransform(uint32_t aFor)
{
  float x, y, width, height;
  nsSVGImageElement *element = static_cast<nsSVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

  
  
  

  return gfxMatrix().Translate(gfxPoint(x, y)) * GetCanvasTM(aFor);
}

bool
nsSVGImageFrame::TransformContextForPainting(gfxContext* aGfxContext)
{
  gfxMatrix imageTransform;
  if (mImageContainer->GetType() == imgIContainer::TYPE_VECTOR) {
    imageTransform = GetVectorImageTransform(FOR_PAINTING);
  } else {
    int32_t nativeWidth, nativeHeight;
    if (NS_FAILED(mImageContainer->GetWidth(&nativeWidth)) ||
        NS_FAILED(mImageContainer->GetHeight(&nativeHeight)) ||
        nativeWidth == 0 || nativeHeight == 0) {
      return false;
    }
    imageTransform =
      GetRasterImageTransform(nativeWidth, nativeHeight, FOR_PAINTING);
  }

  if (imageTransform.IsSingular()) {
    return false;
  }

  
  
  nscoord appUnitsPerDevPx = PresContext()->AppUnitsPerDevPixel();
  gfxFloat pageZoomFactor =
    nsPresContext::AppUnitsToFloatCSSPixels(appUnitsPerDevPx);
  aGfxContext->Multiply(imageTransform.Scale(pageZoomFactor, pageZoomFactor));

  return true;
}



NS_IMETHODIMP
nsSVGImageFrame::PaintSVG(nsRenderingContext *aContext,
                          const nsIntRect *aDirtyRect)
{
  nsresult rv = NS_OK;

  if (!GetStyleVisibility()->IsVisible())
    return NS_OK;

  float x, y, width, height;
  nsSVGImageElement *imgElem = static_cast<nsSVGImageElement*>(mContent);
  imgElem->GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);
  NS_ASSERTION(width > 0 && height > 0,
               "Should only be painting things with valid width/height");

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
    gfxContext* ctx = aContext->ThebesContext();
    gfxContextAutoSaveRestore autoRestorer(ctx);

    if (GetStyleDisplay()->IsScrollableOverflow()) {
      gfxRect clipRect = nsSVGUtils::GetClipRectForFrame(this, x, y,
                                                         width, height);
      nsSVGUtils::SetClipRect(ctx, GetCanvasTM(FOR_PAINTING), clipRect);
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
      NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
                   (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
                   "Display lists handle dirty rect intersection test");
      dirtyRect = aDirtyRect->ToAppUnits(appUnitsPerDevPx);
      
      nsRect rootRect =
        nsSVGUtils::TransformFrameRectToOuterSVG(mRect,
                      GetCanvasTM(FOR_PAINTING), PresContext());
      dirtyRect.MoveBy(-rootRect.TopLeft());
    }

    
    
    
    
    uint32_t drawFlags = imgIContainer::FLAG_SYNC_DECODE;

    if (mImageContainer->GetType() == imgIContainer::TYPE_VECTOR) {
      nsIFrame* imgRootFrame = mImageContainer->GetRootLayoutFrame();
      if (!imgRootFrame) {
        
        return NS_OK;
      }

      
      nsSVGSVGElement* rootSVGElem =
        static_cast<nsSVGSVGElement*>(imgRootFrame->GetContent());
      if (!rootSVGElem || !rootSVGElem->IsSVG(nsGkAtoms::svg)) {
        NS_ABORT_IF_FALSE(false, "missing or non-<svg> root node!!");
        return NS_OK;
      }

      
      
      
      rootSVGElem->SetImageOverridePreserveAspectRatio(
        imgElem->mPreserveAspectRatio.GetAnimValue());
      nsRect destRect(0, 0,
                      appUnitsPerDevPx * width,
                      appUnitsPerDevPx * height);

      
      
      
      nsLayoutUtils::DrawSingleImage(
        aContext,
        mImageContainer,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        destRect,
        aDirtyRect ? dirtyRect : destRect,
        drawFlags);

      rootSVGElem->ClearImageOverridePreserveAspectRatio();
    } else { 
      nsLayoutUtils::DrawSingleUnscaledImage(
        aContext,
        mImageContainer,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        nsPoint(0, 0),
        aDirtyRect ? &dirtyRect : nullptr,
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
      int32_t nativeWidth, nativeHeight;
      if (NS_FAILED(mImageContainer->GetWidth(&nativeWidth)) ||
          NS_FAILED(mImageContainer->GetHeight(&nativeHeight)) ||
          nativeWidth == 0 || nativeHeight == 0) {
        return nullptr;
      }

      if (!nsSVGUtils::HitTestRect(
               GetRasterImageTransform(nativeWidth, nativeHeight,
                                       FOR_HIT_TESTING),
               0, 0, nativeWidth, nativeHeight,
               PresContext()->AppUnitsToDevPixels(aPoint.x),
               PresContext()->AppUnitsToDevPixels(aPoint.y))) {
        return nullptr;
      }
    }
    
    
    
    
  }

  return nsSVGImageFrameBase::GetFrameForPoint(aPoint);
}

nsIAtom *
nsSVGImageFrame::GetType() const
{
  return nsGkAtoms::svgImageFrame;
}






void
nsSVGImageFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
                    "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    return;
  }

  gfxContext tmpCtx(gfxPlatform::GetPlatform()->ScreenReferenceSurface());

  gfxMatrix identity;
  GeneratePath(&tmpCtx, identity);

  gfxRect extent = tmpCtx.GetUserPathExtent();

  if (!extent.IsEmpty()) {
    mRect = nsLayoutUtils::RoundGfxRectToAppRect(extent, 
              PresContext()->AppUnitsPerCSSPixel());
  } else {
    mRect.SetEmpty();
  }

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  
  
  
  
  
  bool invalidate = (mState & NS_FRAME_IS_DIRTY) &&
    !(GetParent()->GetStateBits() &
       (NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY));

  nsRect overflow = nsRect(nsPoint(0,0), mRect.Size());
  nsOverflowAreas overflowAreas(overflow, overflow);
  FinishAndStoreOverflow(overflowAreas, mRect.Size());

  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);

  if (invalidate) {
    
    nsSVGUtils::InvalidateBounds(this, true);
  }
}

uint16_t
nsSVGImageFrame::GetHitTestFlags()
{
  uint16_t flags = 0;

  switch(GetStyleVisibility()->mPointerEvents) {
    case NS_STYLE_POINTER_EVENTS_NONE:
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEPAINTED:
    case NS_STYLE_POINTER_EVENTS_AUTO:
      if (GetStyleVisibility()->IsVisible()) {
        
        flags |= SVG_HIT_TEST_FILL;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEFILL:
    case NS_STYLE_POINTER_EVENTS_VISIBLESTROKE:
    case NS_STYLE_POINTER_EVENTS_VISIBLE:
      if (GetStyleVisibility()->IsVisible()) {
        flags |= SVG_HIT_TEST_FILL;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_PAINTED:
      
      flags |= SVG_HIT_TEST_FILL;
      break;
    case NS_STYLE_POINTER_EVENTS_FILL:
    case NS_STYLE_POINTER_EVENTS_STROKE:
    case NS_STYLE_POINTER_EVENTS_ALL:
      flags |= SVG_HIT_TEST_FILL;
      break;
    default:
      NS_ERROR("not reached");
      break;
  }

  return flags;
}




NS_IMPL_ISUPPORTS1(nsSVGImageListener, imgINotificationObserver)

nsSVGImageListener::nsSVGImageListener(nsSVGImageFrame *aFrame) :  mFrame(aFrame)
{
}

NS_IMETHODIMP
nsSVGImageListener::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (aType == imgINotificationObserver::STOP_DECODE) {
    nsSVGUtils::InvalidateAndScheduleReflowSVG(mFrame);
  }

  if (aType == imgINotificationObserver::FRAME_CHANGED) {
    
    
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
    nsSVGUtils::InvalidateBounds(mFrame);
  }

  if (aType == imgINotificationObserver::START_CONTAINER) {
    
    aRequest->GetImage(getter_AddRefs(mFrame->mImageContainer));
    nsSVGUtils::InvalidateAndScheduleReflowSVG(mFrame);
  }

  return NS_OK;
}

