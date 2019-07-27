





#include "nsSVGForeignObjectFrame.h"


#include "gfxContext.h"
#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"
#include "nsLayoutUtils.h"
#include "nsRegion.h"
#include "nsRenderingContext.h"
#include "nsSVGContainerFrame.h"
#include "nsSVGEffects.h"
#include "mozilla/dom/SVGForeignObjectElement.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsSVGUtils.h"
#include "mozilla/AutoRestore.h"

using namespace mozilla;
using namespace mozilla::dom;




nsContainerFrame*
NS_NewSVGForeignObjectFrame(nsIPresShell   *aPresShell,
                            nsStyleContext *aContext)
{
  return new (aPresShell) nsSVGForeignObjectFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGForeignObjectFrame)

nsSVGForeignObjectFrame::nsSVGForeignObjectFrame(nsStyleContext* aContext)
  : nsSVGForeignObjectFrameBase(aContext),
    mInReflow(false)
{
  AddStateBits(NS_FRAME_REFLOW_ROOT | NS_FRAME_MAY_BE_TRANSFORMED |
               NS_FRAME_SVG_LAYOUT);
}




NS_QUERYFRAME_HEAD(nsSVGForeignObjectFrame)
  NS_QUERYFRAME_ENTRY(nsISVGChildFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGForeignObjectFrameBase)

void
nsSVGForeignObjectFrame::Init(nsIContent*       aContent,
                              nsContainerFrame* aParent,
                              nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::foreignObject),
               "Content is not an SVG foreignObject!");

  nsSVGForeignObjectFrameBase::Init(aContent, aParent, aPrevInFlow);
  AddStateBits(aParent->GetStateBits() & NS_STATE_SVG_CLIPPATH_CHILD);
  AddStateBits(NS_FRAME_FONT_INFLATION_CONTAINER |
               NS_FRAME_FONT_INFLATION_FLOW_ROOT);
  if (!(mState & NS_FRAME_IS_NONDISPLAY)) {
    nsSVGUtils::GetOuterSVGFrame(this)->RegisterForeignObject(this);
  }
}

void nsSVGForeignObjectFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (!(mState & NS_FRAME_IS_NONDISPLAY)) {
      nsSVGUtils::GetOuterSVGFrame(this)->UnregisterForeignObject(this);
  }
  nsSVGForeignObjectFrameBase::DestroyFrom(aDestructRoot);
}

nsIAtom *
nsSVGForeignObjectFrame::GetType() const
{
  return nsGkAtoms::svgForeignObjectFrame;
}

nsresult
nsSVGForeignObjectFrame::AttributeChanged(int32_t  aNameSpaceID,
                                          nsIAtom *aAttribute,
                                          int32_t  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {
      nsSVGEffects::InvalidateRenderingObservers(this);
      nsSVGUtils::ScheduleReflowSVG(this);
      
      RequestReflow(nsIPresShell::eStyleChange);
    } else if (aAttribute == nsGkAtoms::x ||
               aAttribute == nsGkAtoms::y) {
      
      mCanvasTM = nullptr;
      nsSVGEffects::InvalidateRenderingObservers(this);
      nsSVGUtils::ScheduleReflowSVG(this);
    } else if (aAttribute == nsGkAtoms::transform) {
      
      
      
      
      mCanvasTM = nullptr;
    } else if (aAttribute == nsGkAtoms::viewBox ||
               aAttribute == nsGkAtoms::preserveAspectRatio) {
      nsSVGEffects::InvalidateRenderingObservers(this);
    }
  }

  return NS_OK;
}

void
nsSVGForeignObjectFrame::Reflow(nsPresContext*           aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                    "Should not have been called");

  
  
  
  
  
  NS_ASSERTION(!(GetStateBits() & NS_FRAME_IS_DIRTY),
               "Reflowing while a resize is pending is wasteful");

  

  NS_ASSERTION(!aReflowState.parentReflowState,
               "should only get reflow from being reflow root");
  NS_ASSERTION(aReflowState.ComputedWidth() == GetSize().width &&
               aReflowState.ComputedHeight() == GetSize().height,
               "reflow roots should be reflowed at existing size and "
               "svg.css should ensure we have no padding/border/margin");

  DoReflow();

  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize finalSize(wm, aReflowState.ComputedISize(),
                        aReflowState.ComputedBSize());
  aDesiredSize.SetSize(wm, finalSize);
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  aStatus = NS_FRAME_COMPLETE;
}

void
nsSVGForeignObjectFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                          const nsRect&           aDirtyRect,
                                          const nsDisplayListSet& aLists)
{
  if (!static_cast<const nsSVGElement*>(mContent)->HasValidDimensions()) {
    return;
  }
  BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists);
}

bool
nsSVGForeignObjectFrame::IsSVGTransformed(Matrix *aOwnTransform,
                                          Matrix *aFromParentTransform) const
{
  bool foundTransform = false;

  
  nsIFrame *parent = GetParent();
  if (parent &&
      parent->IsFrameOfType(nsIFrame::eSVG | nsIFrame::eSVGContainer)) {
    foundTransform = static_cast<nsSVGContainerFrame*>(parent)->
                       HasChildrenOnlyTransform(aFromParentTransform);
  }

  nsSVGElement *content = static_cast<nsSVGElement*>(mContent);
  nsSVGAnimatedTransformList* transformList =
    content->GetAnimatedTransformList();
  if ((transformList && transformList->HasTransform()) ||
      content->GetAnimateMotionTransform()) {
    if (aOwnTransform) {
      *aOwnTransform = gfx::ToMatrix(content->PrependLocalTransformsTo(gfxMatrix(),
                                  nsSVGElement::eUserSpaceToParent));
    }
    foundTransform = true;
  }
  return foundTransform;
}

nsresult
nsSVGForeignObjectFrame::PaintSVG(nsRenderingContext *aContext,
                                  const gfxMatrix& aTransform,
                                  const nsIntRect* aDirtyRect)
{
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_FRAME_IS_NONDISPLAY),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  if (IsDisabled())
    return NS_OK;

  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return NS_OK;

  if (aTransform.IsSingular()) {
    NS_WARNING("Can't render foreignObject element!");
    return NS_ERROR_FAILURE;
  }

  nsRect kidDirtyRect = kid->GetVisualOverflowRect();

  
  if (aDirtyRect) {
    NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
                 (mState & NS_FRAME_IS_NONDISPLAY),
                 "Display lists handle dirty rect intersection test");
    
    gfxMatrix invmatrix = aTransform;
    DebugOnly<bool> ok = invmatrix.Invert();
    NS_ASSERTION(ok, "inverse of non-singular matrix should be non-singular");

    gfxRect transDirtyRect = gfxRect(aDirtyRect->x, aDirtyRect->y,
                                     aDirtyRect->width, aDirtyRect->height);
    transDirtyRect = invmatrix.TransformBounds(transDirtyRect);

    kidDirtyRect.IntersectRect(kidDirtyRect,
      nsLayoutUtils::RoundGfxRectToAppRect(transDirtyRect,
                       PresContext()->AppUnitsPerCSSPixel()));

    
    
    
    
    if (kidDirtyRect.IsEmpty())
      return NS_OK;
  }

  gfxContext *gfx = aContext->ThebesContext();

  gfx->Save();

  if (StyleDisplay()->IsScrollableOverflow()) {
    float x, y, width, height;
    static_cast<nsSVGElement*>(mContent)->
      GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, 0.0f, 0.0f, width, height);
    nsSVGUtils::SetClipRect(gfx, aTransform, clipRect);
  }

  
  
  
  float cssPxPerDevPx = PresContext()->
    AppUnitsToFloatCSSPixels(PresContext()->AppUnitsPerDevPixel());
  gfxMatrix canvasTMForChildren = aTransform;
  canvasTMForChildren.Scale(cssPxPerDevPx, cssPxPerDevPx);

  gfx->Multiply(canvasTMForChildren);

  uint32_t flags = nsLayoutUtils::PAINT_IN_TRANSFORM;
  if (SVGAutoRenderState::IsPaintingToWindow(aContext)) {
    flags |= nsLayoutUtils::PAINT_TO_WINDOW;
  }
  nsresult rv = nsLayoutUtils::PaintFrame(aContext, kid, nsRegion(kidDirtyRect),
                                          NS_RGBA(0,0,0,0), flags);

  gfx->Restore();

  return rv;
}

nsIFrame*
nsSVGForeignObjectFrame::GetFrameForPoint(const gfxPoint& aPoint)
{
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_FRAME_IS_NONDISPLAY),
               "If display lists are enabled, only hit-testing of a "
               "clipPath's contents should take this code path");

  if (IsDisabled() || (GetStateBits() & NS_FRAME_IS_NONDISPLAY))
    return nullptr;

  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return nullptr;

  float x, y, width, height;
  static_cast<nsSVGElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

  if (!gfxRect(x, y, width, height).Contains(aPoint) ||
      !nsSVGUtils::HitTestClip(this, aPoint)) {
    return nullptr;
  }

  
  

  gfxPoint pt = (aPoint + gfxPoint(x, y)) * nsPresContext::AppUnitsPerCSSPixel();
  nsPoint point = nsPoint(NSToIntRound(pt.x), NSToIntRound(pt.y));

  return nsLayoutUtils::GetFrameForPoint(kid, point);
}

nsRect
nsSVGForeignObjectFrame::GetCoveredRegion()
{
  float x, y, w, h;
  static_cast<SVGForeignObjectElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &w, &h, nullptr);
  if (w < 0.0f) w = 0.0f;
  if (h < 0.0f) h = 0.0f;
  
  return nsSVGUtils::ToCanvasBounds(gfxRect(0.0, 0.0, w, h),
                                    GetCanvasTM(FOR_OUTERSVG_TM),
                                    PresContext());
}

void
nsSVGForeignObjectFrame::ReflowSVG()
{
  NS_ASSERTION(nsSVGUtils::OuterSVGIsCallingReflowSVG(this),
               "This call is probably a wasteful mistake");

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_FRAME_IS_NONDISPLAY),
                    "ReflowSVG mechanism not designed for this");

  if (!nsSVGUtils::NeedsReflowSVG(this)) {
    return;
  }

  
  

  float x, y, w, h;
  static_cast<SVGForeignObjectElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &w, &h, nullptr);

  
  if (w < 0.0f) w = 0.0f;
  if (h < 0.0f) h = 0.0f;

  mRect = nsLayoutUtils::RoundGfxRectToAppRect(
                           gfxRect(x, y, w, h),
                           PresContext()->AppUnitsPerCSSPixel());

  
  
  nsIFrame* kid = GetFirstPrincipalChild();
  kid->AddStateBits(NS_FRAME_IS_DIRTY);

  
  nsPresContext::InterruptPreventer noInterrupts(PresContext());

  DoReflow();

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsSVGEffects::UpdateEffects(this);
  }

  
  
  if (StyleSVGReset()->HasFilters()) {
    InvalidateFrame();
  }

  
  
  nsRect overflow = nsRect(nsPoint(0,0), mRect.Size());
  nsOverflowAreas overflowAreas(overflow, overflow);
  FinishAndStoreOverflow(overflowAreas, mRect.Size());

  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);
}

void
nsSVGForeignObjectFrame::NotifySVGChanged(uint32_t aFlags)
{
  NS_ABORT_IF_FALSE(aFlags & (TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED),
                    "Invalidation logic may need adjusting");

  bool needNewBounds = false; 
  bool needReflow = false;
  bool needNewCanvasTM = false;

  if (aFlags & COORD_CONTEXT_CHANGED) {
    SVGForeignObjectElement *fO =
      static_cast<SVGForeignObjectElement*>(mContent);
    
    
    if (fO->mLengthAttributes[SVGForeignObjectElement::ATTR_X].IsPercentage() ||
        fO->mLengthAttributes[SVGForeignObjectElement::ATTR_Y].IsPercentage()) {
      needNewBounds = true;
      needNewCanvasTM = true;
    }
    
    
    if (fO->mLengthAttributes[SVGForeignObjectElement::ATTR_WIDTH].IsPercentage() ||
        fO->mLengthAttributes[SVGForeignObjectElement::ATTR_HEIGHT].IsPercentage()) {
      needNewBounds = true;
      needReflow = true;
    }
  }

  if (aFlags & TRANSFORM_CHANGED) {
    if (mCanvasTM && mCanvasTM->IsSingular()) {
      needNewBounds = true; 
    }
    needNewCanvasTM = true;
    
    
    
    
    
    
    
  }

  if (needNewBounds) {
    
    
    
    
    
    nsSVGUtils::ScheduleReflowSVG(this);
  }

  
  
  
  
  
  
  
  if (needReflow && !PresContext()->PresShell()->IsReflowLocked()) {
    RequestReflow(nsIPresShell::eResize);
  }

  if (needNewCanvasTM) {
    
    
    mCanvasTM = nullptr;
  }
}

SVGBBox
nsSVGForeignObjectFrame::GetBBoxContribution(const Matrix &aToBBoxUserspace,
                                             uint32_t aFlags)
{
  SVGForeignObjectElement *content =
    static_cast<SVGForeignObjectElement*>(mContent);

  float x, y, w, h;
  content->GetAnimatedLengthValues(&x, &y, &w, &h, nullptr);

  if (w < 0.0f) w = 0.0f;
  if (h < 0.0f) h = 0.0f;

  if (aToBBoxUserspace.IsSingular()) {
    
    return SVGBBox();
  }
  return aToBBoxUserspace.TransformBounds(gfx::Rect(0.0, 0.0, w, h));
}



gfxMatrix
nsSVGForeignObjectFrame::GetCanvasTM(uint32_t aFor, nsIFrame* aTransformRoot)
{
  if (!(GetStateBits() & NS_FRAME_IS_NONDISPLAY) && !aTransformRoot) {
    if (aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }
  if (!mCanvasTM) {
    NS_ASSERTION(GetParent(), "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(GetParent());
    SVGForeignObjectElement *content =
      static_cast<SVGForeignObjectElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(
        this == aTransformRoot ? gfxMatrix() :
                                 parent->GetCanvasTM(aFor, aTransformRoot));

    mCanvasTM = new gfxMatrix(tm);
  }
  return *mCanvasTM;
}




void nsSVGForeignObjectFrame::RequestReflow(nsIPresShell::IntrinsicDirty aType)
{
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW)
    
    return;

  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return;

  PresContext()->PresShell()->FrameNeedsReflow(kid, aType, NS_FRAME_IS_DIRTY);
}

void
nsSVGForeignObjectFrame::DoReflow()
{
  
  if (IsDisabled() &&
      !(GetStateBits() & NS_FRAME_FIRST_REFLOW))
    return;

  nsPresContext *presContext = PresContext();
  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return;

  
  nsRefPtr<nsRenderingContext> renderingContext =
    presContext->PresShell()->CreateReferenceRenderingContext();

  mInReflow = true;

  WritingMode wm = kid->GetWritingMode();
  nsHTMLReflowState reflowState(presContext, kid,
                                renderingContext,
                                LogicalSize(wm, GetLogicalSize(wm).ISize(wm),
                                            NS_UNCONSTRAINEDSIZE));
  nsHTMLReflowMetrics desiredSize(reflowState);
  nsReflowStatus status;

  
  
  NS_ASSERTION(reflowState.ComputedPhysicalBorderPadding() == nsMargin(0, 0, 0, 0) &&
               reflowState.ComputedPhysicalMargin() == nsMargin(0, 0, 0, 0),
               "style system should ensure that :-moz-svg-foreign-content "
               "does not get styled");
  NS_ASSERTION(reflowState.ComputedWidth() == mRect.width,
               "reflow state made child wrong size");
  reflowState.SetComputedHeight(mRect.height);

  ReflowChild(kid, presContext, desiredSize, reflowState, 0, 0,
              NS_FRAME_NO_MOVE_FRAME, status);
  NS_ASSERTION(mRect.width == desiredSize.Width() &&
               mRect.height == desiredSize.Height(), "unexpected size");
  FinishReflowChild(kid, presContext, desiredSize, &reflowState, 0, 0,
                    NS_FRAME_NO_MOVE_FRAME);
  
  mInReflow = false;
}

nsRect
nsSVGForeignObjectFrame::GetInvalidRegion()
{
  MOZ_ASSERT(!NS_SVGDisplayListPaintingEnabled(),
             "Only called by nsDisplayOuterSVG code");

  nsIFrame* kid = GetFirstPrincipalChild();
  if (kid->HasInvalidFrameInSubtree()) {
    gfxRect r(mRect.x, mRect.y, mRect.width, mRect.height);
    r.Scale(1.0 / nsPresContext::AppUnitsPerCSSPixel());
    nsRect rect = nsSVGUtils::ToCanvasBounds(r, GetCanvasTM(FOR_OUTERSVG_TM), PresContext());
    rect = nsSVGUtils::GetPostFilterVisualOverflowRect(this, rect);
    return rect;
  }
  return nsRect();
}


