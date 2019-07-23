





































#include "nsSVGForeignObjectFrame.h"

#include "nsIDOMSVGForeignObjectElem.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsRegion.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsSVGUtils.h"
#include "nsIURI.h"
#include "nsSVGRect.h"
#include "nsSVGMatrix.h"
#include "nsINameSpaceManager.h"
#include "nsSVGForeignObjectElement.h"
#include "nsSVGContainerFrame.h"
#include "gfxContext.h"
#include "gfxMatrix.h"




nsIFrame*
NS_NewSVGForeignObjectFrame(nsIPresShell   *aPresShell,
                            nsStyleContext *aContext)
{
  return new (aPresShell) nsSVGForeignObjectFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGForeignObjectFrame)

nsSVGForeignObjectFrame::nsSVGForeignObjectFrame(nsStyleContext* aContext)
  : nsSVGForeignObjectFrameBase(aContext),
    mInReflow(PR_FALSE)
{
  AddStateBits(NS_FRAME_REFLOW_ROOT |
               NS_FRAME_MAY_BE_TRANSFORMED_OR_HAVE_RENDERING_OBSERVERS);
}




NS_QUERYFRAME_HEAD(nsSVGForeignObjectFrame)
  NS_QUERYFRAME_ENTRY(nsISVGChildFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGForeignObjectFrameBase)

NS_IMETHODIMP
nsSVGForeignObjectFrame::Init(nsIContent* aContent,
                              nsIFrame*   aParent,
                              nsIFrame*   aPrevInFlow)
{
#ifdef DEBUG
  nsCOMPtr<nsIDOMSVGForeignObjectElement> foreignObject = do_QueryInterface(aContent);
  NS_ASSERTION(foreignObject, "Content is not an SVG foreignObject!");
#endif

  nsresult rv = nsSVGForeignObjectFrameBase::Init(aContent, aParent, aPrevInFlow);
  AddStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM | 
               (aParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD));
  if (NS_SUCCEEDED(rv)) {
    nsSVGUtils::GetOuterSVGFrame(this)->RegisterForeignObject(this);
  }
  return rv;
}

void nsSVGForeignObjectFrame::Destroy()
{
  nsSVGUtils::GetOuterSVGFrame(this)->UnregisterForeignObject(this);
  nsSVGForeignObjectFrameBase::Destroy();
}

nsIAtom *
nsSVGForeignObjectFrame::GetType() const
{
  return nsGkAtoms::svgForeignObjectFrame;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                          nsIAtom *aAttribute,
                                          PRInt32  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {
      UpdateGraphic(); 
      
      RequestReflow(nsIPresShell::eStyleChange);
    } else if (aAttribute == nsGkAtoms::x ||
               aAttribute == nsGkAtoms::y ||
               aAttribute == nsGkAtoms::transform) {
      
      mCanvasTM = nsnull;
      UpdateGraphic();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::Reflow(nsPresContext*           aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  
  

  NS_ASSERTION(!aReflowState.parentReflowState,
               "should only get reflow from being reflow root");
  NS_ASSERTION(aReflowState.ComputedWidth() == GetSize().width &&
               aReflowState.ComputedHeight() == GetSize().height,
               "reflow roots should be reflowed at existing size and "
               "svg.css should ensure we have no padding/border/margin");

  DoReflow();

  aDesiredSize.width = aReflowState.ComputedWidth();
  aDesiredSize.height = aReflowState.ComputedHeight();
  aDesiredSize.mOverflowArea =
    nsRect(0, 0, aReflowState.ComputedWidth(), aReflowState.ComputedHeight());
  aStatus = NS_FRAME_COMPLETE;

  return NS_OK;
}

void
nsSVGForeignObjectFrame::InvalidateInternal(const nsRect& aDamageRect,
                                            nscoord aX, nscoord aY,
                                            nsIFrame* aForChild,
                                            PRUint32 aFlags)
{
  

  if (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return;

  nsRegion* region = (aFlags & INVALIDATE_CROSS_DOC)
    ? &mSubDocDirtyRegion : &mSameDocDirtyRegion;
  region->Or(*region, aDamageRect + nsPoint(aX, aY));
  FlushDirtyRegion();
}







static nsRect
ToCanvasBounds(const gfxRect &aUserspaceRect,
               const gfxMatrix &aToCanvas,
               const nsPresContext *presContext)
{
  return nsLayoutUtils::RoundGfxRectToAppRect(
                          aToCanvas.TransformBounds(aUserspaceRect),
                          presContext->AppUnitsPerDevPixel());
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::PaintSVG(nsSVGRenderState *aContext,
                                  const nsIntRect *aDirtyRect)
{
  if (IsDisabled())
    return NS_OK;

  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid)
    return NS_OK;

  gfxMatrix matrix = GetCanvasTMForChildren();

  nsIRenderingContext *ctx = aContext->GetRenderingContext(this);

  if (!ctx || matrix.IsSingular()) {
    NS_WARNING("Can't render foreignObject element!");
    return NS_ERROR_FAILURE;
  }

  
  if (aDirtyRect) {
    PRInt32 appUnitsPerDevPx = PresContext()->AppUnitsPerDevPixel();
    if (!mRect.ToOutsidePixels(appUnitsPerDevPx).Intersects(*aDirtyRect))
      return NS_OK;
  }

  gfxContext *gfx = aContext->GetGfxContext();

  gfx->Save();

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float x, y, width, height;
    static_cast<nsSVGElement*>(mContent)->
      GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

    gfxRect clipRect =
      nsSVGUtils::GetClipRectForFrame(this, 0.0f, 0.0f, width, height);
    nsSVGUtils::SetClipRect(gfx, GetCanvasTM(), clipRect);
  }

  gfx->Multiply(matrix);

  nsresult rv = nsLayoutUtils::PaintFrame(ctx, kid, nsRegion(kid->GetRect()),
                                          NS_RGBA(0,0,0,0),
                                          nsLayoutUtils::PAINT_IN_TRANSFORM);

  gfx->Restore();

  return rv;
}

gfxMatrix
nsSVGForeignObjectFrame::GetTransformMatrix(nsIFrame **aOutAncestor)
{
  NS_PRECONDITION(aOutAncestor, "We need an ancestor to write to!");

  
  *aOutAncestor = nsSVGUtils::GetOuterSVGFrame(this);
  NS_ASSERTION(*aOutAncestor, "How did we end up without an outer frame?");

  
  return GetCanvasTMForChildren();
}
 
NS_IMETHODIMP_(nsIFrame*)
nsSVGForeignObjectFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  if (IsDisabled() || (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD))
    return nsnull;

  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid)
    return nsnull;

  float x, y, width, height;
  static_cast<nsSVGElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

  gfxMatrix tm = GetCanvasTM().Invert();
  if (tm.IsSingular())
    return nsnull;
  
  

  gfxPoint pt = gfxPoint(aPoint.x, aPoint.y) / PresContext()->AppUnitsPerDevPixel();
  pt = tm.Transform(pt);

  if (!gfxRect(0.0f, 0.0f, width, height).Contains(pt))
    return nsnull;

  

  pt = pt * nsPresContext::AppUnitsPerCSSPixel();
  nsPoint point = nsPoint(NSToIntRound(pt.x), NSToIntRound(pt.y));

  return nsLayoutUtils::GetFrameForPoint(kid, point);
}

NS_IMETHODIMP_(nsRect)
nsSVGForeignObjectFrame::GetCoveredRegion()
{
  return mRect;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::UpdateCoveredRegion()
{
  if (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return NS_ERROR_FAILURE;

  float x, y, w, h;
  static_cast<nsSVGForeignObjectElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &w, &h, nsnull);

  
  if (w < 0.0f) w = 0.0f;
  if (h < 0.0f) h = 0.0f;

  
  mRect = ToCanvasBounds(gfxRect(0.0, 0.0, w, h), GetCanvasTM(), PresContext());
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::InitialUpdate()
{
  NS_ASSERTION(GetStateBits() & NS_FRAME_FIRST_REFLOW,
               "Yikes! We've been called already! Hopefully we weren't called "
               "before our nsSVGOuterSVGFrame's initial Reflow()!!!");

  UpdateCoveredRegion();

  
  nsPresContext::InterruptPreventer noInterrupts(PresContext());
  DoReflow();

  NS_ASSERTION(!(mState & NS_FRAME_IN_REFLOW),
               "We don't actually participate in reflow");
  
  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}

void
nsSVGForeignObjectFrame::NotifySVGChanged(PRUint32 aFlags)
{
  PRBool reflow = PR_FALSE;

  if (aFlags & TRANSFORM_CHANGED) {
    
    
    
    
    
    
    
    mCanvasTM = nsnull;
    if (!(aFlags & SUPPRESS_INVALIDATION)) {
      UpdateGraphic();
    }

  } else if (aFlags & COORD_CONTEXT_CHANGED) {
    
    
    nsSVGForeignObjectElement *fO =
      static_cast<nsSVGForeignObjectElement*>(mContent);
    if (fO->mLengthAttributes[nsSVGForeignObjectElement::WIDTH].IsPercentage() ||
        fO->mLengthAttributes[nsSVGForeignObjectElement::HEIGHT].IsPercentage()) {
      reflow = PR_TRUE;
    }
  }

  if (reflow) {
    
    
    
    
    
    
    
    PRBool reflowing;
    PresContext()->PresShell()->IsReflowLocked(&reflowing);
    if (!reflowing) {
      UpdateGraphic(); 
      RequestReflow(nsIPresShell::eResize);
    }
  }
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyRedrawSuspended()
{
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyRedrawUnsuspended()
{
  if (!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    if (GetStateBits() & NS_STATE_SVG_DIRTY) {
      UpdateGraphic(); 
    } else {
      FlushDirtyRegion(); 
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::SetMatrixPropagation(PRBool aPropagate)
{
  if (aPropagate) {
    AddStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM);
  } else {
    RemoveStateBits(NS_STATE_SVG_PROPAGATE_TRANSFORM);
  }
  return NS_OK;
}

PRBool
nsSVGForeignObjectFrame::GetMatrixPropagation()
{
  return (GetStateBits() & NS_STATE_SVG_PROPAGATE_TRANSFORM) != 0;
}

gfxRect
nsSVGForeignObjectFrame::GetBBoxContribution(const gfxMatrix &aToBBoxUserspace)
{
  NS_ASSERTION(!(GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD),
               "Should not be calling this on a non-display child");

  nsSVGForeignObjectElement *content =
    static_cast<nsSVGForeignObjectElement*>(mContent);

  float x, y, w, h;
  content->GetAnimatedLengthValues(&x, &y, &w, &h, nsnull);

  if (w < 0.0f) w = 0.0f;
  if (h < 0.0f) h = 0.0f;

  if (aToBBoxUserspace.IsSingular()) {
    
    return gfxRect(0.0, 0.0, 0.0, 0.0);
  }
  return aToBBoxUserspace.TransformBounds(gfxRect(0.0, 0.0, w, h));
}



gfxMatrix
nsSVGForeignObjectFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    NS_ASSERTION(mParent, "null parent");

    nsSVGContainerFrame *parent = static_cast<nsSVGContainerFrame*>(mParent);
    nsSVGForeignObjectElement *content =
      static_cast<nsSVGForeignObjectElement*>(mContent);

    gfxMatrix tm = content->PrependLocalTransformTo(parent->GetCanvasTM());

    mCanvasTM = NS_NewSVGMatrix(tm);
  }
  return nsSVGUtils::ConvertSVGMatrixToThebes(mCanvasTM);
}




gfxMatrix
nsSVGForeignObjectFrame::GetCanvasTMForChildren()
{
  float cssPxPerDevPx = PresContext()->
    AppUnitsToFloatCSSPixels(PresContext()->AppUnitsPerDevPixel());

  return GetCanvasTM().Scale(cssPxPerDevPx, cssPxPerDevPx);
}

void nsSVGForeignObjectFrame::RequestReflow(nsIPresShell::IntrinsicDirty aType)
{
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW)
    
    return;

  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid)
    return;

  PresContext()->PresShell()->FrameNeedsReflow(kid, aType, NS_FRAME_IS_DIRTY);
}

void nsSVGForeignObjectFrame::UpdateGraphic()
{
  nsSVGUtils::UpdateGraphic(this);

  
  
  mSameDocDirtyRegion.SetEmpty();
  mSubDocDirtyRegion.SetEmpty();
}

void
nsSVGForeignObjectFrame::MaybeReflowFromOuterSVGFrame()
{
  nsIFrame* kid = GetFirstChild(nsnull);

  
  
  
  

  if (kid->GetStateBits() & NS_FRAME_IS_DIRTY) {
    return;
  }
  kid->AddStateBits(NS_FRAME_IS_DIRTY); 
  if (kid->GetStateBits() & NS_FRAME_HAS_DIRTY_CHILDREN) {
    return;
  }

  
  nsPresContext::InterruptPreventer noInterrupts(PresContext());
  DoReflow();
}

void
nsSVGForeignObjectFrame::DoReflow()
{
#ifdef DEBUG
  printf("**nsSVGForeignObjectFrame::DoReflow()\n");
#endif

  NS_ASSERTION(!(nsSVGUtils::GetOuterSVGFrame(this)->
                             GetStateBits() & NS_FRAME_FIRST_REFLOW),
               "Calling InitialUpdate too early - must not call DoReflow!!!");

  if (IsDisabled())
    return;

  if (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return;

  nsPresContext *presContext = PresContext();
  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid)
    return;

  
  nsSize availableSpace(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  nsCOMPtr<nsIRenderingContext> renderingContext;
  nsIPresShell* presShell = presContext->PresShell();
  NS_ASSERTION(presShell, "null presShell");
  presShell->CreateRenderingContext(this,getter_AddRefs(renderingContext));
  if (!renderingContext)
    return;

  nsSVGForeignObjectElement *fO = static_cast<nsSVGForeignObjectElement*>
                                             (mContent);

  float width =
    fO->mLengthAttributes[nsSVGForeignObjectElement::WIDTH].GetAnimValue(fO);
  float height =
    fO->mLengthAttributes[nsSVGForeignObjectElement::HEIGHT].GetAnimValue(fO);

  nsSize size(nsPresContext::CSSPixelsToAppUnits(width),
              nsPresContext::CSSPixelsToAppUnits(height));

  mInReflow = PR_TRUE;

  nsHTMLReflowState reflowState(presContext, kid,
                                renderingContext,
                                nsSize(size.width, NS_UNCONSTRAINEDSIZE));
  nsHTMLReflowMetrics desiredSize;
  nsReflowStatus status;

  
  
  NS_ASSERTION(reflowState.mComputedBorderPadding == nsMargin(0, 0, 0, 0) &&
               reflowState.mComputedMargin == nsMargin(0, 0, 0, 0),
               "style system should ensure that :-moz-svg-foreign content "
               "does not get styled");
  NS_ASSERTION(reflowState.ComputedWidth() == size.width,
               "reflow state made child wrong size");
  reflowState.SetComputedHeight(size.height);
  
  ReflowChild(kid, presContext, desiredSize, reflowState, 0, 0,
              NS_FRAME_NO_MOVE_FRAME, status);
  NS_ASSERTION(size.width == desiredSize.width &&
               size.height == desiredSize.height, "unexpected size");
  FinishReflowChild(kid, presContext, &reflowState, desiredSize, 0, 0,
                    NS_FRAME_NO_MOVE_FRAME);
  
  mInReflow = PR_FALSE;
  FlushDirtyRegion();
}

void
nsSVGForeignObjectFrame::InvalidateDirtyRect(nsSVGOuterSVGFrame* aOuter,
    const nsRect& aRect, PRUint32 aFlags)
{
  if (aRect.IsEmpty())
    return;

  
  

  gfxRect r(aRect.x, aRect.y, aRect.width, aRect.height);
  r.Scale(1.0 / nsPresContext::AppUnitsPerCSSPixel());

  nsRect rect = ToCanvasBounds(r, GetCanvasTM(), PresContext());

  
  rect.IntersectRect(rect, mRect);
  if (rect.IsEmpty())
    return;

  
  
  rect.UnionRect(rect, mRect);

  rect = nsSVGUtils::FindFilterInvalidation(this, rect);
  aOuter->InvalidateWithFlags(rect, aFlags);
}

void
nsSVGForeignObjectFrame::FlushDirtyRegion()
{
  if ((mSameDocDirtyRegion.IsEmpty() && mSubDocDirtyRegion.IsEmpty()) ||
      mInReflow)
    return;

  nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(this);
  if (!outerSVGFrame) {
    NS_ERROR("null outerSVGFrame");
    return;
  }

  if (outerSVGFrame->IsRedrawSuspended())
    return;

  InvalidateDirtyRect(outerSVGFrame, mSameDocDirtyRegion.GetBounds(), 0);
  InvalidateDirtyRect(outerSVGFrame, mSubDocDirtyRegion.GetBounds(), INVALIDATE_CROSS_DOC);

  mSameDocDirtyRegion.SetEmpty();
  mSubDocDirtyRegion.SetEmpty();
}
