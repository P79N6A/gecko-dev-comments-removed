





































#include "nsSVGForeignObjectFrame.h"

#include "nsIDOMSVGForeignObjectElem.h"
#include "nsIDOMSVGMatrix.h"
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
               aAttribute == nsGkAtoms::y) {
      UpdateGraphic();
    } else if (aAttribute == nsGkAtoms::transform) {
      
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
               "reflow roots should be reflown at existing size and "
               "svg.css should ensure we have no padding/border/margin");

  DoReflow();

  
  aDesiredSize.width = aReflowState.ComputedWidth();
  aDesiredSize.height = aReflowState.ComputedHeight();
  aDesiredSize.mOverflowArea =
    nsRect(nsPoint(0, 0), nsSize(aDesiredSize.width, aDesiredSize.height));
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
GetTransformedRegion(float aX, float aY, float aWidth, float aHeight,
                     nsIDOMSVGMatrix* aMatrix, nsPresContext *aPresContext)
{
  float x[4], y[4];
  x[0] = aX;
  y[0] = aY;
  x[1] = aX + aWidth;
  y[1] = aY;
  x[2] = aX + aWidth;
  y[2] = aY + aHeight;
  x[3] = aX;
  y[3] = aY + aHeight;
 
  for (int i = 0; i < 4; i++) {
    nsSVGUtils::TransformPoint(aMatrix, &x[i], &y[i]);
  }

  float xmin, xmax, ymin, ymax;
  xmin = xmax = x[0];
  ymin = ymax = y[0];
  for (int i = 1; i < 4; i++) {
    if (x[i] < xmin)
      xmin = x[i];
    else if (x[i] > xmax)
      xmax = x[i];
    if (y[i] < ymin)
      ymin = y[i];
    else if (y[i] > ymax)
      ymax = y[i];
  }
 
  return nsSVGUtils::ToAppPixelRect(aPresContext, xmin, ymin, xmax, ymax);
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

  nsCOMPtr<nsIDOMSVGMatrix> tm = GetUnZoomedTMIncludingOffset();

  gfxMatrix matrix = nsSVGUtils::ConvertSVGMatrixToThebes(tm);

  nsIRenderingContext *ctx = aContext->GetRenderingContext(this);

  if (!ctx || matrix.IsSingular()) {
    NS_WARNING("Can't render foreignObject element!");
    return NS_ERROR_FAILURE;
  }

  
  if (aDirtyRect) {
    gfxRect extent = matrix.TransformBounds(
                       gfxRect(kid->GetRect().x, kid->GetRect().y, 
                               kid->GetRect().width, kid->GetRect().height));
    extent.RoundOut();
    nsIntRect rect;
    if (NS_SUCCEEDED(nsSVGUtils::GfxRectToIntRect(extent, &rect)) &&
        !aDirtyRect->Intersects(rect))
      return NS_OK;
  }

  gfxContext *gfx = aContext->GetGfxContext();

  gfx->Save();

  if (GetStyleDisplay()->IsScrollableOverflow()) {
    float x, y, width, height;
    static_cast<nsSVGElement*>(mContent)->
      GetAnimatedLengthValues(&x, &y, &width, &height, nsnull);

    nsCOMPtr<nsIDOMSVGMatrix> ctm = NS_NewSVGMatrix(GetCanvasTM());
    nsSVGUtils::SetClipRect(gfx, ctm, x, y, width, height);
  }

  gfx->Multiply(matrix);

  nsresult rv = nsLayoutUtils::PaintFrame(ctx, kid, nsRegion(kid->GetRect()),
                                          NS_RGBA(0,0,0,0));

  gfx->Restore();

  return rv;
}

nsresult
nsSVGForeignObjectFrame::TransformPointFromOuterPx(const nsPoint &aIn,
                                                   nsPoint* aOut)
{
  if (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMSVGMatrix> tm = GetUnZoomedTMIncludingOffset();

  nsCOMPtr<nsIDOMSVGMatrix> inverse;
  nsresult rv = tm->Inverse(getter_AddRefs(inverse));
  if (NS_FAILED(rv))
    return rv;

  float x = PresContext()->AppUnitsToDevPixels(aIn.x);
  float y = PresContext()->AppUnitsToDevPixels(aIn.y);
  nsSVGUtils::TransformPoint(inverse, &x, &y);
  *aOut = nsPoint(PresContext()->DevPixelsToAppUnits(NSToIntRound(x)),
                  PresContext()->DevPixelsToAppUnits(NSToIntRound(y)));
  return NS_OK;
}

gfxMatrix
nsSVGForeignObjectFrame::GetTransformMatrix(nsIFrame **aOutAncestor)
{
  NS_PRECONDITION(aOutAncestor, "We need an ancestor to write to!");

  
  *aOutAncestor = nsSVGUtils::GetOuterSVGFrame(this);
  NS_ASSERTION(*aOutAncestor, "How did we end up without an outer frame?");

  
  nsCOMPtr<nsIDOMSVGMatrix> tm = GetUnZoomedTMIncludingOffset();

  return nsSVGUtils::ConvertSVGMatrixToThebes(tm);
}
 
NS_IMETHODIMP_(nsIFrame*)
nsSVGForeignObjectFrame::GetFrameForPoint(const nsPoint &aPoint)
{
  if (IsDisabled())
    return NS_OK;

  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid) {
    return nsnull;
  }
  nsPoint pt;
  if (NS_FAILED(TransformPointFromOuterPx(aPoint, &pt)))
    return nsnull;
  return nsLayoutUtils::GetFrameForPoint(kid, pt);
}

nsPoint
nsSVGForeignObjectFrame::TransformPointFromOuter(nsPoint aPt)
{
  nsPoint pt(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  TransformPointFromOuterPx(aPt, &pt);
  return pt;
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

  nsCOMPtr<nsIDOMSVGMatrix> ctm = NS_NewSVGMatrix(GetCanvasTM());
  if (!ctm)
    return NS_ERROR_FAILURE;

  float x, y, w, h;
  static_cast<nsSVGForeignObjectElement*>(mContent)->
    GetAnimatedLengthValues(&x, &y, &w, &h, nsnull);

  
  if (w < 0.0f) w = 0.0f;
  if (h < 0.0f) h = 0.0f;

  mRect = GetTransformedRegion(x, y, w, h, ctm, PresContext());

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
  return aToBBoxUserspace.TransformBounds(gfxRect(x, y, w, h));
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




already_AddRefed<nsIDOMSVGMatrix>
nsSVGForeignObjectFrame::GetUnZoomedTMIncludingOffset()
{
  nsCOMPtr<nsIDOMSVGMatrix> ctm = NS_NewSVGMatrix(GetCanvasTM());
  if (!ctm)
    return nsnull;

  nsSVGForeignObjectElement *fO =
    static_cast<nsSVGForeignObjectElement*>(mContent);
  float x, y;
  fO->GetAnimatedLengthValues(&x, &y, nsnull);
  nsCOMPtr<nsIDOMSVGMatrix> localTM;
  ctm->Translate(x, y, getter_AddRefs(localTM));

  float cssPxPerDevPx =
    PresContext()->AppUnitsToFloatCSSPixels(
                                PresContext()->AppUnitsPerDevPixel());
  nsCOMPtr<nsIDOMSVGMatrix> cssPxToDevPxMatrix;
  NS_NewSVGMatrix(getter_AddRefs(cssPxToDevPxMatrix),
                  cssPxPerDevPx, 0.0f,
                  0.0f, cssPxPerDevPx);

  
  nsIDOMSVGMatrix* matrix;
  localTM->Multiply(cssPxToDevPxMatrix, &matrix);
  return matrix;
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

  nsPresContext* presContext = PresContext();
  nsCOMPtr<nsIDOMSVGMatrix> ctm = NS_NewSVGMatrix(GetCanvasTM());

  nsSVGForeignObjectElement *fO =
    static_cast<nsSVGForeignObjectElement*>(mContent);
  float x, y;
  fO->GetAnimatedLengthValues(&x, &y, nsnull);
  nsCOMPtr<nsIDOMSVGMatrix> localTM;
  ctm->Translate(x, y, getter_AddRefs(localTM));

  nsIntRect r = aRect.ToOutsidePixels(presContext->AppUnitsPerDevPixel());
  nsRect rect = GetTransformedRegion(r.x, r.y, r.width, r.height,
                                     localTM, presContext);

  
  
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
