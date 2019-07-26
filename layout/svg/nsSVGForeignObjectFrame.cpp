





#include "nsSVGForeignObjectFrame.h"


#include "gfxContext.h"
#include "gfxMatrix.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
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




nsIFrame*
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

NS_IMETHODIMP
nsSVGForeignObjectFrame::Init(nsIContent* aContent,
                              nsIFrame*   aParent,
                              nsIFrame*   aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVG(nsGkAtoms::foreignObject),
               "Content is not an SVG foreignObject!");

  nsresult rv = nsSVGForeignObjectFrameBase::Init(aContent, aParent, aPrevInFlow);
  AddStateBits(aParent->GetStateBits() &
               (NS_STATE_SVG_NONDISPLAY_CHILD | NS_STATE_SVG_CLIPPATH_CHILD));
  AddStateBits(NS_FRAME_FONT_INFLATION_CONTAINER |
               NS_FRAME_FONT_INFLATION_FLOW_ROOT);
  if (NS_SUCCEEDED(rv) &&
      !(mState & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    nsSVGUtils::GetOuterSVGFrame(this)->RegisterForeignObject(this);
  }
  return rv;
}

void nsSVGForeignObjectFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (!(mState & NS_STATE_SVG_NONDISPLAY_CHILD)) {
      nsSVGUtils::GetOuterSVGFrame(this)->UnregisterForeignObject(this);
  }
  nsSVGForeignObjectFrameBase::DestroyFrom(aDestructRoot);
}

nsIAtom *
nsSVGForeignObjectFrame::GetType() const
{
  return nsGkAtoms::svgForeignObjectFrame;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::AttributeChanged(int32_t  aNameSpaceID,
                                          nsIAtom *aAttribute,
                                          int32_t  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {
      nsSVGUtils::InvalidateBounds(this, false);
      nsSVGUtils::ScheduleReflowSVG(this);
      
      RequestReflow(nsIPresShell::eStyleChange);
    } else if (aAttribute == nsGkAtoms::x ||
               aAttribute == nsGkAtoms::y ||
               aAttribute == nsGkAtoms::transform) {
      
      mCanvasTM = nullptr;
      nsSVGUtils::InvalidateBounds(this, false);
      nsSVGUtils::ScheduleReflowSVG(this);
    } else if (aAttribute == nsGkAtoms::viewBox ||
               aAttribute == nsGkAtoms::preserveAspectRatio) {
      nsSVGUtils::InvalidateBounds(this);
    }
  }

  return NS_OK;
}

 void
nsSVGForeignObjectFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsSVGForeignObjectFrameBase::DidSetStyleContext(aOldStyleContext);

  
  
  
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    
    
    nsSVGUtils::InvalidateBounds(this, false);
    nsSVGUtils::ScheduleReflowSVG(this);
  }
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::Reflow(nsPresContext*           aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
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

  aDesiredSize.width = aReflowState.ComputedWidth();
  aDesiredSize.height = aReflowState.ComputedHeight();
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  aStatus = NS_FRAME_COMPLETE;

  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                          const nsRect&           aDirtyRect,
                                          const nsDisplayListSet& aLists)
{
  if (!static_cast<const nsSVGElement*>(mContent)->HasValidDimensions()) {
    return NS_OK;
  }
  return BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, aLists);
}

bool
nsSVGForeignObjectFrame::IsSVGTransformed(gfxMatrix *aOwnTransform,
                                         gfxMatrix *aFromParentTransform) const
{
  bool foundTransform = false;

  
  nsIFrame *parent = GetParent();
  if (parent &&
      parent->IsFrameOfType(nsIFrame::eSVG | nsIFrame::eSVGContainer)) {
    foundTransform = static_cast<nsSVGContainerFrame*>(parent)->
                       HasChildrenOnlyTransform(aFromParentTransform);
  }

  nsSVGElement *content = static_cast<nsSVGElement*>(mContent);
  if (content->GetAnimatedTransformList()) {
    if (aOwnTransform) {
      *aOwnTransform = content->PrependLocalTransformsTo(gfxMatrix(),
                                  nsSVGElement::eUserSpaceToParent);
    }
    foundTransform = true;
  }
  return foundTransform;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::PaintSVG(nsRenderingContext *aContext,
                                  const nsIntRect *aDirtyRect)
{
  NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only painting of non-display "
               "SVG should take this code path");

  if (IsDisabled())
    return NS_OK;

  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return NS_OK;

  gfxMatrix canvasTM = GetCanvasTM(FOR_PAINTING);

  if (canvasTM.IsSingular()) {
    NS_WARNING("Can't render foreignObject element!");
    return NS_ERROR_FAILURE;
  }

  nsRect kidDirtyRect = kid->GetVisualOverflowRect();

  
  if (aDirtyRect) {
    NS_ASSERTION(!NS_SVGDisplayListPaintingEnabled() ||
                 (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
                 "Display lists handle dirty rect intersection test");
    
    gfxMatrix invmatrix = canvasTM;
    invmatrix.Invert();
    NS_ASSERTION(!invmatrix.IsSingular(),
                 "inverse of non-singular matrix should be non-singular");

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

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float x, y, width, height;
    static_cast<nsSVGElement*>(mContent)->
      GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, 0.0f, 0.0f, width, height);
    nsSVGUtils::SetClipRect(gfx, canvasTM, clipRect);
  }

  
  
  
  float cssPxPerDevPx = PresContext()->
    AppUnitsToFloatCSSPixels(PresContext()->AppUnitsPerDevPixel());
  gfxMatrix canvasTMForChildren = canvasTM;
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

NS_IMETHODIMP_(nsIFrame*)
nsSVGForeignObjectFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  NS_ASSERTION(!NS_SVGDisplayListHitTestingEnabled() ||
               (mState & NS_STATE_SVG_NONDISPLAY_CHILD),
               "If display lists are enabled, only hit-testing of a "
               "clipPath's contents should take this code path");

  if (IsDisabled() || (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD))
    return nullptr;

  nsIFrame* kid = GetFirstPrincipalChild();
  if (!kid)
    return nullptr;

  float x, y, width, height;
  static_cast<nsSVGElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &width, &height, nullptr);

  gfxMatrix tm = GetCanvasTM(FOR_HIT_TESTING).Invert();
  if (tm.IsSingular())
    return nullptr;
  
  

  gfxPoint pt = gfxPoint(aPoint.x, aPoint.y) / PresContext()->AppUnitsPerDevPixel();
  pt = tm.Transform(pt);

  if (!gfxRect(0.0f, 0.0f, width, height).Contains(pt))
    return nullptr;

  

  pt = pt * nsPresContext::AppUnitsPerCSSPixel();
  nsPoint point = nsPoint(NSToIntRound(pt.x), NSToIntRound(pt.y));

  nsIFrame *frame = nsLayoutUtils::GetFrameForPoint(kid, point);
  if (frame && nsSVGUtils::HitTestClip(this, aPoint))
    return frame;

  return nullptr;
}

NS_IMETHODIMP_(nsRect)
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

  NS_ABORT_IF_FALSE(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
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
nsSVGForeignObjectFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace,
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
  return aToBBoxUserspace.TransformBounds(gfxRect(0.0, 0.0, w, h));
}



gfxMatrix
nsSVGForeignObjectFrame::GetCanvasTM(uint32_t aFor)
{
  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    if ((aFor == FOR_PAINTING && NS_SVGDisplayListPaintingEnabled()) ||
        (aFor == FOR_HIT_TESTING && NS_SVGDisplayListHitTestingEnabled())) {
      return nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(this);
    }
  }
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    SVGForeignObjectElement *content =
      static_cast<SVGForeignObjectElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformsTo(parent->GetCanvasTM(aFor));

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

  
  nsIPresShell* presShell = presContext->PresShell();
  NS_ASSERTION(presShell, "null presShell");
  nsRefPtr<nsRenderingContext> renderingContext =
    presShell->GetReferenceRenderingContext();
  if (!renderingContext)
    return;

  mInReflow = true;

  nsHTMLReflowState reflowState(presContext, kid,
                                renderingContext,
                                nsSize(mRect.width, NS_UNCONSTRAINEDSIZE));
  nsHTMLReflowMetrics desiredSize;
  nsReflowStatus status;

  
  
  NS_ASSERTION(reflowState.mComputedBorderPadding == nsMargin(0, 0, 0, 0) &&
               reflowState.mComputedMargin == nsMargin(0, 0, 0, 0),
               "style system should ensure that :-moz-svg-foreign-content "
               "does not get styled");
  NS_ASSERTION(reflowState.ComputedWidth() == mRect.width,
               "reflow state made child wrong size");
  reflowState.SetComputedHeight(mRect.height);

  ReflowChild(kid, presContext, desiredSize, reflowState, 0, 0,
              NS_FRAME_NO_MOVE_FRAME, status);
  NS_ASSERTION(mRect.width == desiredSize.width &&
               mRect.height == desiredSize.height, "unexpected size");
  FinishReflowChild(kid, presContext, &reflowState, desiredSize, 0, 0,
                    NS_FRAME_NO_MOVE_FRAME);
  
  mInReflow = false;
}

nsRect
nsSVGForeignObjectFrame::GetInvalidRegion()
{
  nsIFrame* kid = GetFirstPrincipalChild();
  if (kid->HasInvalidFrameInSubtree()) {
    gfxRect r(mRect.x, mRect.y, mRect.width, mRect.height);
    r.Scale(1.0 / nsPresContext::AppUnitsPerCSSPixel());
    nsRect rect = nsSVGUtils::ToCanvasBounds(r, GetCanvasTM(FOR_PAINTING), PresContext());
    rect = nsSVGUtils::GetPostFilterVisualOverflowRect(this, rect);
    return rect;
  }
  return nsRect();
}


