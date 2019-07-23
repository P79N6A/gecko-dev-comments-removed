





































#include "nsSVGForeignObjectFrame.h"

#include "nsISVGValue.h"
#include "nsIDOMSVGGElement.h"
#include "nsIDOMSVGForeignObjectElem.h"
#include "nsIDOMSVGMatrix.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMSVGPoint.h"
#include "nsSpaceManager.h"
#include "nsSVGOuterSVGFrame.h"
#include "nsISVGValueUtils.h"
#include "nsRegion.h"
#include "nsGkAtoms.h"
#include "nsLayoutUtils.h"
#include "nsSVGUtils.h"
#include "nsIURI.h"
#include "nsSVGPoint.h"
#include "nsSVGRect.h"
#include "nsSVGMatrix.h"
#include "nsINameSpaceManager.h"
#include "nsSVGForeignObjectElement.h"
#include "nsSVGContainerFrame.h"
#include "gfxContext.h"
#include "gfxMatrix.h"




nsIFrame*
NS_NewSVGForeignObjectFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{
  nsCOMPtr<nsIDOMSVGForeignObjectElement> foreignObject = do_QueryInterface(aContent);
  if (!foreignObject) {
#ifdef DEBUG
    printf("warning: trying to construct an SVGForeignObjectFrame for a content element that doesn't support the right interfaces\n");
#endif
    return nsnull;
  }

  return new (aPresShell) nsSVGForeignObjectFrame(aContext);
}

nsSVGForeignObjectFrame::nsSVGForeignObjectFrame(nsStyleContext* aContext)
  : nsSVGForeignObjectFrameBase(aContext),
    mPropagateTransform(PR_TRUE), mInReflow(PR_FALSE)
{
  AddStateBits(NS_FRAME_REFLOW_ROOT);
}




NS_INTERFACE_MAP_BEGIN(nsSVGForeignObjectFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGChildFrame)
NS_INTERFACE_MAP_END_INHERITING(nsSVGForeignObjectFrameBase)




void nsSVGForeignObjectFrame::Destroy()
{
  nsSVGUtils::StyleEffects(this);
  nsSVGForeignObjectFrameBase::Destroy();
}

nsIAtom *
nsSVGForeignObjectFrame::GetType() const
{
  return nsGkAtoms::svgForeignObjectFrame;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                          nsIAtom*        aAttribute,
                                          PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::width ||
        aAttribute == nsGkAtoms::height) {
      PostChildDirty();
      UpdateGraphic();
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
nsSVGForeignObjectFrame::DidSetStyleContext()
{
  nsSVGUtils::StyleEffects(this);
  return NS_OK;
}

 void
nsSVGForeignObjectFrame::MarkIntrinsicWidthsDirty()
{
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW)
    
    return;

  
  
  
  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid)
    return;
  
  
  
  
  kid->AddStateBits(NS_FRAME_IS_DIRTY);
  
  
  
  PresContext()->PresShell()->FrameNeedsReflow(kid,
                                                  nsIPresShell::eResize);
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
               aReflowState.mComputedHeight == GetSize().height,
               "reflow roots should be reflown at existing size and "
               "svg.css should ensure we have no padding/border/margin");

  DoReflow();

  aDesiredSize.width = aReflowState.ComputedWidth();
  aDesiredSize.height = aReflowState.mComputedHeight;
  aDesiredSize.mOverflowArea =
    nsRect(nsPoint(0, 0), nsSize(aDesiredSize.width, aDesiredSize.height));
  aStatus = NS_FRAME_COMPLETE;

  return NS_OK;
}










static void
TransformRect(float* aX, float *aY, float* aWidth, float *aHeight,
              nsIDOMSVGMatrix* aMatrix)
{
  float x[4], y[4];
  x[0] = *aX;
  y[0] = *aY;
  x[1] = x[0] + *aWidth;
  y[1] = y[0];
  x[2] = x[0] + *aWidth;
  y[2] = y[0] + *aHeight;
  x[3] = x[0];
  y[3] = y[0] + *aHeight;
 
  int i;
  for (i = 0; i < 4; i++) {
    nsSVGUtils::TransformPoint(aMatrix, &x[i], &y[i]);
  }

  float xmin, xmax, ymin, ymax;
  xmin = xmax = x[0];
  ymin = ymax = y[0];
  for (i=1; i<4; i++) {
    if (x[i] < xmin)
      xmin = x[i];
    if (y[i] < ymin)
      ymin = y[i];
    if (x[i] > xmax)
      xmax = x[i];
    if (y[i] > ymax)
      ymax = y[i];
  }
 
  *aX = xmin;
  *aY = ymin;
  *aWidth = xmax - xmin;
  *aHeight = ymax - ymin;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::PaintSVG(nsSVGRenderState *aContext,
                                  nsRect *aDirtyRect)
{
  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid)
    return NS_OK;

  nsCOMPtr<nsIDOMSVGMatrix> tm = GetTMIncludingOffset();

  gfxMatrix matrix = nsSVGUtils::ConvertSVGMatrixToThebes(tm);

  nsIRenderingContext *ctx = aContext->GetRenderingContext();

  if (!ctx || matrix.IsSingular()) {
    NS_WARNING("Can't render foreignObject element!");
    return NS_ERROR_FAILURE;
  }

  gfxContext *gfx = aContext->GetGfxContext();

  gfx->Save();
  gfx->Multiply(matrix);

  nsresult rv = nsLayoutUtils::PaintFrame(ctx, kid, nsRegion(kid->GetRect()),
                                          NS_RGBA(0,0,0,0));

  gfx->Restore();

  return rv;
}

nsresult
nsSVGForeignObjectFrame::TransformPointFromOuterPx(float aX, float aY, nsPoint* aOut)
{
  if (mParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return NS_ERROR_FAILURE;

  nsCOMPtr<nsIDOMSVGMatrix> tm = GetTMIncludingOffset();
  nsCOMPtr<nsIDOMSVGMatrix> inverse;
  nsresult rv = tm->Inverse(getter_AddRefs(inverse));
  if (NS_FAILED(rv))
    return rv;
   
  nsSVGUtils::TransformPoint(inverse, &aX, &aY);
  *aOut = nsPoint(nsPresContext::CSSPixelsToAppUnits(aX),
                  nsPresContext::CSSPixelsToAppUnits(aY));
  return NS_OK;
}
 
NS_IMETHODIMP
nsSVGForeignObjectFrame::GetFrameForPointSVG(float x, float y, nsIFrame** hit)
{
  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid) {
    *hit = nsnull;
    return NS_OK;
  }
  nsPoint pt;
  nsresult rv = TransformPointFromOuterPx(x, y, &pt);
  if (NS_FAILED(rv))
    return rv;
  *hit = nsLayoutUtils::GetFrameForPoint(kid, pt);
  return NS_OK;
}

nsPoint
nsSVGForeignObjectFrame::TransformPointFromOuter(nsPoint aPt)
{
  nsPoint pt(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  TransformPointFromOuterPx(nsPresContext::AppUnitsToFloatCSSPixels(aPt.x),
                            nsPresContext::AppUnitsToFloatCSSPixels(aPt.y),
                            &pt);
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
  if (mParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return NS_ERROR_FAILURE;

  float x, y, w, h;
  GetBBoxInternal(&x, &y, &w, &h);

  mRect = nsSVGUtils::ToBoundingPixelRect(x, y, x + w, y + h);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::InitialUpdate()
{
  UpdateCoveredRegion();
  DoReflow();

  NS_ASSERTION(!(mState & NS_FRAME_IN_REFLOW),
               "We don't actually participate in reflow");
  
  
  mState &= ~(NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_DIRTY |
              NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyCanvasTMChanged(PRBool suppressInvalidation)
{
  mCanvasTM = nsnull;
  UpdateGraphic();
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyRedrawSuspended()
{
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::NotifyRedrawUnsuspended()
{
  if (!(mParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
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
  mPropagateTransform = aPropagate;
  return NS_OK;
}

NS_IMETHODIMP
nsSVGForeignObjectFrame::SetOverrideCTM(nsIDOMSVGMatrix *aCTM)
{
  mOverrideCTM = aCTM;
  return NS_OK;
}

void
nsSVGForeignObjectFrame::GetBBoxInternal(float* aX, float *aY, float* aWidth,
                                         float *aHeight)
{
  nsCOMPtr<nsIDOMSVGMatrix> ctm = GetCanvasTM();
  if (!ctm)
    return;
  
  nsSVGForeignObjectElement *fO = NS_STATIC_CAST(nsSVGForeignObjectElement*,
                                                 mContent);
  fO->GetAnimatedLengthValues(aX, aY, aWidth, aHeight, nsnull);
  
  TransformRect(aX, aY, aWidth, aHeight, ctm);
}
  
NS_IMETHODIMP
nsSVGForeignObjectFrame::GetBBox(nsIDOMSVGRect **_retval)
{
  if (mParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD) {
    *_retval = nsnull;
    return NS_ERROR_FAILURE;
  }

  float x, y, w, h;
  GetBBoxInternal(&x, &y, &w, &h);
  return NS_NewSVGRect(_retval, x, y, w, h);
}



already_AddRefed<nsIDOMSVGMatrix>
nsSVGForeignObjectFrame::GetTMIncludingOffset()
{
  nsCOMPtr<nsIDOMSVGMatrix> ctm = GetCanvasTM();
  if (!ctm)
    return nsnull;

  nsSVGForeignObjectElement *fO =
    NS_STATIC_CAST(nsSVGForeignObjectElement*, mContent);
  float x, y;
  fO->GetAnimatedLengthValues(&x, &y, nsnull);
  nsIDOMSVGMatrix* matrix;
  ctm->Translate(x, y, &matrix);
  return matrix;
}

already_AddRefed<nsIDOMSVGMatrix>
nsSVGForeignObjectFrame::GetCanvasTM()
{
  if (!mPropagateTransform) {
    nsIDOMSVGMatrix *retval;
    if (mOverrideCTM) {
      retval = mOverrideCTM;
      NS_ADDREF(retval);
    } else {
      NS_NewSVGMatrix(&retval);
    }
    return retval;
  }

  if (!mCanvasTM) {
    
    NS_ASSERTION(mParent, "null parent");
    nsSVGContainerFrame *containerFrame = NS_STATIC_CAST(nsSVGContainerFrame*,
                                                         mParent);
    nsCOMPtr<nsIDOMSVGMatrix> parentTM = containerFrame->GetCanvasTM();
    NS_ASSERTION(parentTM, "null TM");

    
    nsSVGGraphicElement *element =
      NS_STATIC_CAST(nsSVGGraphicElement*, mContent);
    nsCOMPtr<nsIDOMSVGMatrix> localTM = element->GetLocalTransformMatrix();
    
    if (localTM)
      parentTM->Multiply(localTM, getter_AddRefs(mCanvasTM));
    else
      mCanvasTM = parentTM;
  }

  nsIDOMSVGMatrix* retval = mCanvasTM.get();
  NS_IF_ADDREF(retval);
  return retval;
}




void nsSVGForeignObjectFrame::PostChildDirty()
{
  nsIFrame* kid = GetFirstChild(nsnull);
  if (!kid)
    return;
  PresContext()->PresShell()->
    FrameNeedsReflow(kid, nsIPresShell::eStyleChange);
}

void nsSVGForeignObjectFrame::UpdateGraphic()
{
  if (GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return;

  nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(this);
  if (!outerSVGFrame) {
    NS_ERROR("null outerSVGFrame");
    return;
  }
  
  if (outerSVGFrame->IsRedrawSuspended()) {
    AddStateBits(NS_STATE_SVG_DIRTY);
  } else {
    RemoveStateBits(NS_STATE_SVG_DIRTY);

    outerSVGFrame->InvalidateRect(mRect);

    UpdateCoveredRegion();

    nsRect filterRect;
    filterRect = nsSVGUtils::FindFilterInvalidation(this);
    if (!filterRect.IsEmpty()) {
      outerSVGFrame->InvalidateRect(filterRect);
    } else {
      outerSVGFrame->InvalidateRect(mRect);
    }
  }

  
  mDirtyRegion.SetEmpty();
}

void
nsSVGForeignObjectFrame::DoReflow()
{
#ifdef DEBUG
  printf("**nsSVGForeignObjectFrame::DoReflow()\n");
#endif

  if (mParent->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
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

  nsSVGForeignObjectElement *fO = NS_STATIC_CAST(nsSVGForeignObjectElement*,
                                                 mContent);

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
  reflowState.mComputedHeight = size.height;
  
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
nsSVGForeignObjectFrame::FlushDirtyRegion() {
  if (mDirtyRegion.IsEmpty() || mInReflow)
    return;

  nsSVGOuterSVGFrame *outerSVGFrame = nsSVGUtils::GetOuterSVGFrame(this);
  if (!outerSVGFrame) {
    NS_ERROR("null outerSVGFrame");
    return;
  }

  if (outerSVGFrame->IsRedrawSuspended())
    return;

  nsRect rect = nsSVGUtils::FindFilterInvalidation(this);
  if (!rect.IsEmpty()) {
    outerSVGFrame->InvalidateRect(rect);
    mDirtyRegion.SetEmpty();
    return;
  }
  
  nsCOMPtr<nsIDOMSVGMatrix> tm = GetTMIncludingOffset();
  nsRect r = mDirtyRegion.GetBounds();
  r.ScaleRoundOut(1.0f / nsPresContext::AppUnitsPerCSSPixel());
  float x = r.x, y = r.y, w = r.width, h = r.height;
  TransformRect(&x, &y, &w, &h, tm);
  r = nsSVGUtils::ToBoundingPixelRect(x, y, x+w, y+h);
  outerSVGFrame->InvalidateRect(r);

  mDirtyRegion.SetEmpty();
}

void
nsSVGForeignObjectFrame::InvalidateInternal(const nsRect& aDamageRect,
                                            nscoord aX, nscoord aY, nsIFrame* aForChild,
                                            PRBool aImmediate)
{
  mDirtyRegion.Or(mDirtyRegion, aDamageRect + nsPoint(aX, aY));
  FlushDirtyRegion();
}
