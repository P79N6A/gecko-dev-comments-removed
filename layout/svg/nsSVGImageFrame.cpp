





#include "gfxContext.h"
#include "gfxPlatform.h"
#include "mozilla/gfx/2D.h"
#include "imgIContainer.h"
#include "nsContainerFrame.h"
#include "nsIImageLoadingContent.h"
#include "nsLayoutUtils.h"
#include "nsRenderingContext.h"
#include "imgINotificationObserver.h"
#include "nsSVGEffects.h"
#include "nsSVGPathGeometryFrame.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "nsSVGUtils.h"
#include "SVGContentUtils.h"
#include "SVGImageContext.h"
#include "mozilla/dom/SVGImageElement.h"
#include "nsContentUtils.h"
#include "nsIReflowCallback.h"

using namespace mozilla;
using namespace mozilla::dom;
using namespace mozilla::gfx;

class nsSVGImageFrame;

class nsSVGImageListener MOZ_FINAL : public imgINotificationObserver
{
public:
  explicit nsSVGImageListener(nsSVGImageFrame *aFrame);

  NS_DECL_ISUPPORTS
  NS_DECL_IMGINOTIFICATIONOBSERVER

  void SetFrame(nsSVGImageFrame *frame) { mFrame = frame; }

private:
  ~nsSVGImageListener() {}

  nsSVGImageFrame *mFrame;
};

typedef nsSVGPathGeometryFrame nsSVGImageFrameBase;

class nsSVGImageFrame : public nsSVGImageFrameBase,
                        public nsIReflowCallback
{
  friend nsIFrame*
  NS_NewSVGImageFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

protected:
  explicit nsSVGImageFrame(nsStyleContext* aContext) : nsSVGImageFrameBase(aContext),
                                                       mReflowCallbackPosted(false) {}
  virtual ~nsSVGImageFrame();

public:
  NS_DECL_FRAMEARENA_HELPERS

  
  virtual nsresult PaintSVG(nsRenderingContext *aContext,
                            const gfxMatrix& aTransform,
                            const nsIntRect* aDirtyRect = nullptr) MOZ_OVERRIDE;
  virtual nsIFrame* GetFrameForPoint(const gfxPoint& aPoint) MOZ_OVERRIDE;
  virtual void ReflowSVG() MOZ_OVERRIDE;

  
  virtual uint16_t GetHitTestFlags() MOZ_OVERRIDE;

  
  virtual nsresult  AttributeChanged(int32_t         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     int32_t         aModType) MOZ_OVERRIDE;
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("SVGImage"), aResult);
  }
#endif

  
  virtual bool ReflowFinished() MOZ_OVERRIDE;
  virtual void ReflowCallbackCanceled() MOZ_OVERRIDE;

private:
  gfx::Matrix GetRasterImageTransform(int32_t aNativeWidth,
                                      int32_t aNativeHeight);
  gfx::Matrix GetVectorImageTransform();
  bool TransformContextForPainting(gfxContext* aGfxContext,
                                   const gfxMatrix& aTransform);

  nsCOMPtr<imgINotificationObserver> mListener;

  nsCOMPtr<imgIContainer> mImageContainer;

  bool mReflowCallbackPosted;

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
      imageLoader->RemoveObserver(mListener);
    }
    reinterpret_cast<nsSVGImageListener*>(mListener.get())->SetFrame(nullptr);
  }
  mListener = nullptr;
}

void
nsSVGImageFrame::Init(nsIContent*       aContent,
                      nsContainerFrame* aParent,
                      nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::image),
               "Content is not an SVG image!");

  nsSVGImageFrameBase::Init(aContent, aParent, aPrevInFlow);

  mListener = new nsSVGImageListener(this);
  nsCOMPtr<nsIImageLoadingContent> imageLoader = do_QueryInterface(mContent);
  if (!imageLoader) {
    NS_RUNTIMEABORT("Why is this not an image loading content?");
  }

  
  
  imageLoader->FrameCreated(this);

  imageLoader->AddObserver(mListener);
}

 void
nsSVGImageFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  if (mReflowCallbackPosted) {
    PresContext()->PresShell()->CancelReflowCallback(this);
    mReflowCallbackPosted = false;
  }

  nsCOMPtr<nsIImageLoadingContent> imageLoader =
    do_QueryInterface(nsFrame::mContent);

  if (imageLoader) {
    imageLoader->FrameDestroyed(this);
  }

  nsFrame::DestroyFrom(aDestructRoot);
}




nsresult
nsSVGImageFrame::AttributeChanged(int32_t         aNameSpaceID,
                                  nsIAtom*        aAttribute,
                                  int32_t         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::x ||
        aAttribute == nsGkAtoms::y ||
        aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {
      nsSVGEffects::InvalidateRenderingObservers(this);
      nsSVGUtils::ScheduleReflowSVG(this);
      return NS_OK;
    }
    else if (aAttribute == nsGkAtoms::preserveAspectRatio) {
      
      
      
      
      InvalidateFrame();
      return NS_OK;
    }
  }
  if (aNameSpaceID == kNameSpaceID_XLink &&
      aAttribute == nsGkAtoms::href) {

    
    if (nsContentUtils::IsImageSrcSetDisabled()) {
      return NS_OK;
    }
    SVGImageElement *element = static_cast<SVGImageElement*>(mContent);

    if (element->mStringAttributes[SVGImageElement::HREF].IsExplicitlySet()) {
      element->LoadSVGImage(true, true);
    } else {
      element->CancelImageRequests(true);
    }
  }

  return nsSVGImageFrameBase::AttributeChanged(aNameSpaceID,
                                               aAttribute, aModType);
}

gfx::Matrix
nsSVGImageFrame::GetRasterImageTransform(int32_t aNativeWidth,
                                         int32_t aNativeHeight)
{
  float x, y, width, height;
  SVGImageElement *element = static_cast<SVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

  Matrix viewBoxTM =
    SVGContentUtils::GetViewBoxTransform(width, height,
                                         0, 0, aNativeWidth, aNativeHeight,
                                         element->mPreserveAspectRatio);

  return viewBoxTM * gfx::Matrix::Translation(x, y);
}

gfx::Matrix
nsSVGImageFrame::GetVectorImageTransform()
{
  float x, y, width, height;
  SVGImageElement *element = static_cast<SVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

  
  
  

  return gfx::Matrix::Translation(x, y);
}

bool
nsSVGImageFrame::TransformContextForPainting(gfxContext* aGfxContext,
                                             const gfxMatrix& aTransform)
{
  gfx::Matrix imageTransform;
  if (mImageContainer->GetType() == imgIContainer::TYPE_VECTOR) {
    imageTransform = GetVectorImageTransform() * ToMatrix(aTransform);
  } else {
    int32_t nativeWidth, nativeHeight;
    if (NS_FAILED(mImageContainer->GetWidth(&nativeWidth)) ||
        NS_FAILED(mImageContainer->GetHeight(&nativeHeight)) ||
        nativeWidth == 0 || nativeHeight == 0) {
      return false;
    }
    imageTransform =
      GetRasterImageTransform(nativeWidth, nativeHeight) * ToMatrix(aTransform);

    
    
    nscoord appUnitsPerDevPx = PresContext()->AppUnitsPerDevPixel();
    gfxFloat pageZoomFactor =
      nsPresContext::AppUnitsToFloatCSSPixels(appUnitsPerDevPx);
    imageTransform.Scale(pageZoomFactor, pageZoomFactor);
  }

  if (imageTransform.IsSingular()) {
    return false;
  }

  aGfxContext->Multiply(ThebesMatrix(imageTransform));
  return true;
}



nsresult
nsSVGImageFrame::PaintSVG(nsRenderingContext *aContext,
                          const gfxMatrix& aTransform,
                          const nsIntRect *aDirtyRect)
{
  nsresult rv = NS_OK;

  if (!StyleVisibility()->IsVisible())
    return NS_OK;

  float x, y, width, height;
  SVGImageElement *imgElem = static_cast<SVGImageElement*>(mContent);
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

    if (StyleDisplay()->IsScrollableOverflow()) {
      gfxRect clipRect = nsSVGUtils::GetClipRectForFrame(this, x, y,
                                                         width, height);
      nsSVGUtils::SetClipRect(ctx, aTransform, clipRect);
    }

    if (!TransformContextForPainting(ctx, aTransform)) {
      return NS_ERROR_FAILURE;
    }

    
    
    
    float opacity = 1.0f;
    if (nsSVGUtils::CanOptimizeOpacity(this)) {
      opacity = StyleDisplay()->mOpacity;
    }

    if (opacity != 1.0f || StyleDisplay()->mMixBlendMode != NS_STYLE_BLEND_NORMAL) {
      ctx->PushGroup(gfxContentType::COLOR_ALPHA);
    }

    nscoord appUnitsPerDevPx = PresContext()->AppUnitsPerDevPixel();
    nsRect dirtyRect; 
    if (aDirtyRect) {
      NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
                   (mState & NS_FRAME_IS_NONDISPLAY),
                   "Display lists handle dirty rect intersection test");
      dirtyRect = aDirtyRect->ToAppUnits(appUnitsPerDevPx);
      
      nsRect rootRect =
        nsSVGUtils::TransformFrameRectToOuterSVG(mRect, aTransform,
                                                 PresContext());
      dirtyRect.MoveBy(-rootRect.TopLeft());
    }

    
    
    
    
    uint32_t drawFlags = imgIContainer::FLAG_SYNC_DECODE;

    if (mImageContainer->GetType() == imgIContainer::TYPE_VECTOR) {
      
      
      SVGImageContext context(nsIntSize(width, height),
                              Some(imgElem->mPreserveAspectRatio.GetAnimValue()));

      nsRect destRect(0, 0,
                      appUnitsPerDevPx * width,
                      appUnitsPerDevPx * height);

      
      
      
      nsLayoutUtils::DrawSingleImage(
        aContext,
        PresContext(),
        mImageContainer,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        destRect,
        aDirtyRect ? dirtyRect : destRect,
        &context,
        drawFlags);
    } else { 
      nsLayoutUtils::DrawSingleUnscaledImage(
        aContext,
        PresContext(),
        mImageContainer,
        nsLayoutUtils::GetGraphicsFilterForFrame(this),
        nsPoint(0, 0),
        aDirtyRect ? &dirtyRect : nullptr,
        drawFlags);
    }

    if (opacity != 1.0f || StyleDisplay()->mMixBlendMode != NS_STYLE_BLEND_NORMAL) {
      ctx->PopGroupToSource();
      ctx->SetOperator(gfxContext::OPERATOR_OVER);
      ctx->Paint(opacity);
    }
    
  }

  return rv;
}

nsIFrame*
nsSVGImageFrame::GetFrameForPoint(const gfxPoint& aPoint)
{
  Rect rect;
  SVGImageElement *element = static_cast<SVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&rect.x, &rect.y,
                                   &rect.width, &rect.height, nullptr);

  if (!rect.Contains(ToPoint(aPoint))) {
    return nullptr;
  }

  
  
  
  
  
  
  
  
  if (StyleDisplay()->IsScrollableOverflow() && mImageContainer) {
    if (mImageContainer->GetType() == imgIContainer::TYPE_RASTER) {
      int32_t nativeWidth, nativeHeight;
      if (NS_FAILED(mImageContainer->GetWidth(&nativeWidth)) ||
          NS_FAILED(mImageContainer->GetHeight(&nativeHeight)) ||
          nativeWidth == 0 || nativeHeight == 0) {
        return nullptr;
      }
      Matrix viewBoxTM =
        SVGContentUtils::GetViewBoxTransform(rect.width, rect.height,
                                             0, 0, nativeWidth, nativeHeight,
                                             element->mPreserveAspectRatio);
      if (!nsSVGUtils::HitTestRect(viewBoxTM,
                                   0, 0, nativeWidth, nativeHeight,
                                   aPoint.x - rect.x, aPoint.y - rect.y)) {
        return nullptr;
      }
    }
  }

  return this;
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

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                    "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    return;
  }

  float x, y, width, height;
  SVGImageElement *element = static_cast<SVGImageElement*>(mContent);
  element->GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

  Rect extent(x, y, width, height);

  if (!extent.IsEmpty()) {
    mRect = nsLayoutUtils::RoundGfxRectToAppRect(extent, 
              PresContext()->AppUnitsPerCSSPixel());
  } else {
    mRect.SetEmpty();
  }

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);

    if (!mReflowCallbackPosted) {
      nsIPresShell* shell = PresContext()->PresShell();
      mReflowCallbackPosted = true;
      shell->PostReflowCallback(this);
    }
  }

  nsRect overflow = nsRect(nsPoint(0,0), mRect.Size());
  nsOverflowAreas overflowAreas(overflow, overflow);
  FinishAndStoreOverflow(overflowAreas, mRect.Size());

  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);

  
  
  if (!(GetParent()->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    InvalidateFrame();
  }
}

bool
nsSVGImageFrame::ReflowFinished()
{
  mReflowCallbackPosted = false;

  nsLayoutUtils::UpdateImageVisibilityForFrame(this);

  return false;
}

void
nsSVGImageFrame::ReflowCallbackCanceled()
{
  mReflowCallbackPosted = false;
}

uint16_t
nsSVGImageFrame::GetHitTestFlags()
{
  uint16_t flags = 0;

  switch(StyleVisibility()->mPointerEvents) {
    case NS_STYLE_POINTER_EVENTS_NONE:
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEPAINTED:
    case NS_STYLE_POINTER_EVENTS_AUTO:
      if (StyleVisibility()->IsVisible()) {
        
        flags |= SVG_HIT_TEST_FILL;
      }
      break;
    case NS_STYLE_POINTER_EVENTS_VISIBLEFILL:
    case NS_STYLE_POINTER_EVENTS_VISIBLESTROKE:
    case NS_STYLE_POINTER_EVENTS_VISIBLE:
      if (StyleVisibility()->IsVisible()) {
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




NS_IMPL_ISUPPORTS(nsSVGImageListener, imgINotificationObserver)

nsSVGImageListener::nsSVGImageListener(nsSVGImageFrame *aFrame) :  mFrame(aFrame)
{
}

NS_IMETHODIMP
nsSVGImageListener::Notify(imgIRequest *aRequest, int32_t aType, const nsIntRect* aData)
{
  if (!mFrame)
    return NS_ERROR_FAILURE;

  if (aType == imgINotificationObserver::LOAD_COMPLETE) {
    mFrame->InvalidateFrame();
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
    nsSVGUtils::ScheduleReflowSVG(mFrame);
  }

  if (aType == imgINotificationObserver::FRAME_UPDATE) {
    
    
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
    mFrame->InvalidateFrame();
  }

  if (aType == imgINotificationObserver::SIZE_AVAILABLE) {
    
    aRequest->GetImage(getter_AddRefs(mFrame->mImageContainer));
    mFrame->InvalidateFrame();
    nsSVGEffects::InvalidateRenderingObservers(mFrame);
    nsSVGUtils::ScheduleReflowSVG(mFrame);
  }

  return NS_OK;
}

