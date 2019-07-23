





































#include "nsSVGOuterSVGFrame.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsSVGSVGElement.h"
#include "nsSVGTextFrame.h"
#include "nsSVGRect.h"
#include "nsDisplayList.h"
#include "nsStubMutationObserver.h"
#include "gfxContext.h"

#if defined(DEBUG) && defined(SVG_DEBUG_PRINTING)
#include "nsIDeviceContext.h"
#include "nsTransform2D.h"
#endif

class nsSVGMutationObserver : public nsStubMutationObserver
{
public:
  
  NS_DECL_NSIMUTATIONOBSERVER_ATTRIBUTECHANGED

  
  NS_IMETHOD QueryInterface(const nsIID& aIID, void** aInstancePtr);
private:
  NS_IMETHOD_(nsrefcnt) AddRef() { return 1; }
  NS_IMETHOD_(nsrefcnt) Release() { return 1; }

  static void UpdateTextFragmentTrees(nsIFrame *aFrame);
};




NS_INTERFACE_MAP_BEGIN(nsSVGMutationObserver)
  NS_INTERFACE_MAP_ENTRY(nsIMutationObserver)
NS_INTERFACE_MAP_END

static nsSVGMutationObserver sSVGMutationObserver;




void
nsSVGMutationObserver::AttributeChanged(nsIDocument *aDocument,
                                        nsIContent *aContent,
                                        PRInt32 aNameSpaceID,
                                        nsIAtom *aAttribute,
                                        PRInt32 aModType)
{
  if (aNameSpaceID != kNameSpaceID_XML || aAttribute != nsGkAtoms::space) {
    return;
  }

  PRUint32 count = aDocument->GetNumberOfShells();
  for (PRUint32 i = 0; i < count; ++i) {
    nsIFrame *frame = aDocument->GetShellAt(i)->GetPrimaryFrameFor(aContent);
    if (!frame) {
      continue;
    }

    
    nsISVGTextContentMetrics* metrics;
    CallQueryInterface(frame, &metrics);
    if (metrics) {
      nsSVGTextContainerFrame *containerFrame =
        NS_STATIC_CAST(nsSVGTextContainerFrame *, frame);
      containerFrame->UpdateGraphic();
      continue;
    }
    
    UpdateTextFragmentTrees(frame);
  }
}




void
nsSVGMutationObserver::UpdateTextFragmentTrees(nsIFrame *aFrame)
{
  nsIFrame* kid = aFrame->GetFirstChild(nsnull);
  while (kid) {
    if (kid->GetType() == nsGkAtoms::svgTextFrame) {
      nsSVGTextFrame* textFrame = NS_STATIC_CAST(nsSVGTextFrame*, kid);
      textFrame->NotifyGlyphMetricsChange();
    } else {
      UpdateTextFragmentTrees(kid);
    }
    kid = kid->GetNextSibling();
  }
}




nsIFrame*
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsIContent* aContent, nsStyleContext* aContext)
{  
  nsCOMPtr<nsIDOMSVGSVGElement> svgElement = do_QueryInterface(aContent);
  if (!svgElement) {
#ifdef DEBUG
    printf("warning: trying to construct an SVGOuterSVGFrame for a content element that doesn't support the right interfaces\n");
#endif
    return nsnull;
  }

  return new (aPresShell) nsSVGOuterSVGFrame(aContext);
}

nsSVGOuterSVGFrame::nsSVGOuterSVGFrame(nsStyleContext* aContext)
    : nsSVGOuterSVGFrameBase(aContext),
      mRedrawSuspendCount(0),
      mNeedsReflow(PR_FALSE),
      mViewportInitialized(PR_FALSE)
{
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::InitSVG()
{
  nsCOMPtr<nsISVGSVGElement> SVGElement = do_QueryInterface(mContent);
  NS_ASSERTION(SVGElement, "wrong content element");

  nsIDocument* doc = mContent->GetCurrentDoc();
  if (doc) {
    
    if (doc->GetRootContent() == mContent) {
      SVGElement->GetZoomAndPanEnum(getter_AddRefs(mZoomAndPan));
      SVGElement->GetCurrentTranslate(getter_AddRefs(mCurrentTranslate));
      SVGElement->GetCurrentScaleNumber(getter_AddRefs(mCurrentScale));
    }
    
    
    
    doc->AddMutationObserver(&sSVGMutationObserver);
  }

  SuspendRedraw();

  AddStateBits(NS_STATE_IS_OUTER_SVG);

  return NS_OK;
}




NS_INTERFACE_MAP_BEGIN(nsSVGOuterSVGFrame)
  NS_INTERFACE_MAP_ENTRY(nsISVGSVGFrame)
NS_INTERFACE_MAP_END_INHERITING(nsSVGOuterSVGFrameBase)



  



NS_IMETHODIMP
nsSVGOuterSVGFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  if (!aReflowState.ShouldReflowAllKids()) {
    
    
    
    
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }
  
  
  
  
  
  
  
  
  
  
  
  
  
  
#ifdef DEBUG
  
#endif
  
  nsCOMPtr<nsISVGSVGElement> SVGElement = do_QueryInterface(mContent);
  NS_ENSURE_TRUE(SVGElement, NS_ERROR_FAILURE);

  
  
  

  nsRect maxRect, preferredRect;
  CalculateAvailableSpace(&maxRect, &preferredRect, aPresContext, aReflowState);
  float preferredWidth = nsPresContext::AppUnitsToFloatCSSPixels(preferredRect.width);
  float preferredHeight = nsPresContext::AppUnitsToFloatCSSPixels(preferredRect.height);

  SuspendRedraw();

  nsCOMPtr<nsIDOMSVGRect> r;
  NS_NewSVGRect(getter_AddRefs(r), 0, 0, preferredWidth, preferredHeight);

  nsSVGSVGElement *svgElem = NS_STATIC_CAST(nsSVGSVGElement*, mContent);
  NS_ENSURE_TRUE(svgElem, NS_ERROR_FAILURE);
  svgElem->SetCoordCtxRect(r);

#ifdef DEBUG
  























#endif

  
  
  

  nsSVGSVGElement *svg = NS_STATIC_CAST(nsSVGSVGElement*, mContent);

  aDesiredSize.width =
    nsPresContext::CSSPixelsToAppUnits(svg->mViewportWidth);
  aDesiredSize.height =
    nsPresContext::CSSPixelsToAppUnits(svg->mViewportHeight);

  

  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  
  
  NotifyViewportChange();

  UnsuspendRedraw();
  
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::DidReflow(nsPresContext*   aPresContext,
                              const nsHTMLReflowState*  aReflowState,
                              nsDidReflowStatus aStatus)
{
  nsresult rv = nsSVGOuterSVGFrameBase::DidReflow(aPresContext,aReflowState,aStatus);

  if (!mViewportInitialized) {
    
    mViewportInitialized = PR_TRUE;

    
    nsIFrame* kid = mFrames.FirstChild();
    while (kid) {
      nsISVGChildFrame* SVGFrame = nsnull;
      CallQueryInterface(kid, &SVGFrame);
      if (SVGFrame) {
        SVGFrame->InitialUpdate(); 
      }
      kid = kid->GetNextSibling();
    }
    
    UnsuspendRedraw();
  }
  
  return rv;
}




NS_IMETHODIMP
nsSVGOuterSVGFrame::InsertFrames(nsIAtom*        aListName,
                                 nsIFrame*       aPrevFrame,
                                 nsIFrame*       aFrameList)
{
  SuspendRedraw();
  nsSVGOuterSVGFrameBase::InsertFrames(aListName, aPrevFrame, aFrameList);
  UnsuspendRedraw();
  
  return NS_OK;
}

class nsDisplaySVG : public nsDisplayItem {
public:
  nsDisplaySVG(nsSVGOuterSVGFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplaySVG);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVG() {
    MOZ_COUNT_DTOR(nsDisplaySVG);
  }
#endif

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("SVGEventReceiver")
};

nsIFrame*
nsDisplaySVG::HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt)
{
  return NS_STATIC_CAST(nsSVGOuterSVGFrame*, mFrame)->
    GetFrameForPoint(aPt - aBuilder->ToReferenceFrame(mFrame));
}

void
nsDisplaySVG::Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect)
{
  NS_STATIC_CAST(nsSVGOuterSVGFrame*, mFrame)->
    Paint(*aCtx, aDirtyRect, aBuilder->ToReferenceFrame(mFrame));
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::AttributeChanged(PRInt32         aNameSpaceID,
                                     nsIAtom*        aAttribute,
                                     PRInt32         aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      !(GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      (aAttribute == nsGkAtoms::width || aAttribute == nsGkAtoms::height)) {
    PresContext()->PresShell()->
      FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
  }

  return NS_OK;
}

nsIFrame*
nsSVGOuterSVGFrame::GetFrameForPoint(const nsPoint& aPoint)
{
  
  
  

  float x = PresContext()->AppUnitsToDevPixels(aPoint.x);
  float y = PresContext()->AppUnitsToDevPixels(aPoint.y);

  nsRect thisRect(nsPoint(0,0), GetSize());
  if (!thisRect.Contains(aPoint)) {
    return nsnull;
  }

  nsIFrame* hit;
  nsSVGUtils::HitTestChildren(this, x, y, &hit);

  return hit;
}




NS_IMETHODIMP
nsSVGOuterSVGFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  
  
  return aLists.Content()->AppendNewToTop(new (aBuilder) nsDisplaySVG(this));
}

void
nsSVGOuterSVGFrame::Paint(nsIRenderingContext& aRenderingContext,
                          const nsRect& aDirtyRect, nsPoint aPt)
{
#if defined(DEBUG) && defined(SVG_DEBUG_PRINTING)
  {
    nsCOMPtr<nsIDeviceContext>  dx;
    aRenderingContext.GetDeviceContext(*getter_AddRefs(dx));
    float zoom,tzoom,scale;
    dx->GetZoom(zoom);
    dx->GetTextZoom(tzoom);
    dx->GetCanonicalPixelScale(scale);
    printf("nsSVGOuterSVGFrame(%p)::Paint()[ z=%f tz=%f ps=%f\n",this,zoom,tzoom,scale);
    printf("dirtyrect= %d, %d, %d, %d\n", aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
    nsTransform2D* xform;
    aRenderingContext.GetCurrentTransform(xform);
    printf("translation=(%f,%f)\n", xform->GetXTranslation(), xform->GetYTranslation());
    float sx=1.0f,sy=1.0f;
    xform->TransformNoXLate(&sx,&sy);
    printf("scale=(%f,%f)\n", sx, sy);
    float twipsPerScPx = aPresContext->ScaledPixelsToTwips();
    float twipsPerPx = aPresContext->PixelsToTwips();
    printf("tw/sc(px)=%f tw/px=%f\n", twipsPerScPx, twipsPerPx);
    int fontsc;
    GetPresContext()->GetFontScaler(&fontsc);
    printf("font scale=%d\n",fontsc);
    printf("]\n");
  }
#endif
  
  
  aRenderingContext.PushState();
  
  nsRect clipRect;
  clipRect.IntersectRect(aDirtyRect, nsRect(aPt, GetSize()));
  aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
  aRenderingContext.Translate(aPt.x, aPt.y);
  nsRect dirtyRect = clipRect - aPt;

#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime start = PR_Now();
#endif

  dirtyRect.ScaleRoundOut(1.0f / PresContext()->AppUnitsPerDevPixel());

  nsSVGRenderState ctx(&aRenderingContext);

  
  
#ifdef XP_MACOSX
  ctx.GetGfxContext()->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
#endif

  
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsSVGUtils::PaintChildWithEffects(&ctx, &dirtyRect, kid);
  }


#ifdef XP_MACOSX
  ctx.GetGfxContext()->PopGroupToSource();
  ctx.GetGfxContext()->Paint();
#endif

#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime end = PR_Now();
  printf("SVG Paint Timing: %f ms\n", (end-start)/1000.0);
#endif
  
  aRenderingContext.PopState();
}

nsIAtom *
nsSVGOuterSVGFrame::GetType() const
{
  return nsGkAtoms::svgOuterSVGFrame;
}




nsresult
nsSVGOuterSVGFrame::InvalidateRect(nsRect aRect)
{
  aRect.ScaleRoundOut(PresContext()->AppUnitsPerDevPixel());
  Invalidate(aRect);

  return NS_OK;
}

PRBool
nsSVGOuterSVGFrame::IsRedrawSuspended()
{
  return (mRedrawSuspendCount>0) || !mViewportInitialized;
}





NS_IMETHODIMP
nsSVGOuterSVGFrame::SuspendRedraw()
{
#ifdef DEBUG
  
#endif
  if (++mRedrawSuspendCount != 1)
    return NS_OK;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawSuspended();
    }
  }
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::UnsuspendRedraw()
{
#ifdef DEBUG

#endif
  if (--mRedrawSuspendCount > 0)
    return NS_OK;
  
  NS_ASSERTION(mRedrawSuspendCount >=0, "unbalanced suspend count!");
  
  
  
  if (mNeedsReflow)
    InitiateReflow();
  
  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame=nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawUnsuspended();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::NotifyViewportChange()
{
  
  if (!mViewportInitialized) return NS_OK;

  
  mCanvasTM = nsnull;
  
  
  SuspendRedraw();
  nsIFrame* kid = mFrames.FirstChild();
  while (kid) {
    nsISVGChildFrame* SVGFrame = nsnull;
    CallQueryInterface(kid, &SVGFrame);
    if (SVGFrame)
      SVGFrame->NotifyCanvasTMChanged(PR_FALSE); 
    kid = kid->GetNextSibling();
  }
  UnsuspendRedraw();
  return NS_OK;
}




already_AddRefed<nsIDOMSVGMatrix>
nsSVGOuterSVGFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    nsSVGSVGElement *svgElement = NS_STATIC_CAST(nsSVGSVGElement*, mContent);
    svgElement->GetViewboxToViewportTransform(getter_AddRefs(mCanvasTM));

    if (mZoomAndPan) {
      
      
      PRUint16 val;
      mZoomAndPan->GetIntegerValue(val);
      if (val == nsIDOMSVGZoomAndPan::SVG_ZOOMANDPAN_MAGNIFY) {
        nsCOMPtr<nsIDOMSVGMatrix> zoomPanMatrix;
        nsCOMPtr<nsIDOMSVGMatrix> temp;
        float scale, x, y;
        mCurrentScale->GetValue(&scale);
        mCurrentTranslate->GetX(&x);
        mCurrentTranslate->GetY(&y);
        svgElement->CreateSVGMatrix(getter_AddRefs(zoomPanMatrix));
        zoomPanMatrix->Translate(x, y, getter_AddRefs(temp));
        temp->Scale(scale, getter_AddRefs(zoomPanMatrix));
        zoomPanMatrix->Multiply(mCanvasTM, getter_AddRefs(temp));
        temp.swap(mCanvasTM);
      }
    }
  }
  nsIDOMSVGMatrix* retval = mCanvasTM.get();
  NS_IF_ADDREF(retval);
  return retval;
}




void nsSVGOuterSVGFrame::InitiateReflow()
{
  mNeedsReflow = PR_FALSE;
  
  nsIPresShell* presShell = PresContext()->PresShell();
  presShell->FrameNeedsReflow(this, nsIPresShell::eStyleChange,
                              NS_FRAME_IS_DIRTY);
  
  
  
  presShell->FlushPendingNotifications(Flush_OnlyReflow);  
}


void
nsSVGOuterSVGFrame::CalculateAvailableSpace(nsRect *maxRect,
                                            nsRect *preferredRect,
                                            nsPresContext* aPresContext,
                                            const nsHTMLReflowState& aReflowState)
{
  *preferredRect = aPresContext->GetVisibleArea();
  
  
  if (aReflowState.availableWidth != NS_INTRINSICSIZE)
    maxRect->width = aReflowState.availableWidth;
  else if (aReflowState.parentReflowState &&
           aReflowState.parentReflowState->ComputedWidth() != NS_INTRINSICSIZE)
    maxRect->width = aReflowState.parentReflowState->ComputedWidth();
  else
    maxRect->width = NS_MAXSIZE;
  
  if (aReflowState.availableHeight != NS_INTRINSICSIZE)
    maxRect->height = aReflowState.availableHeight;    
  else if (aReflowState.parentReflowState &&
           aReflowState.parentReflowState->mComputedHeight != NS_INTRINSICSIZE)
    maxRect->height = aReflowState.parentReflowState->mComputedHeight;
  else
    maxRect->height = NS_MAXSIZE;

  if (preferredRect->width > maxRect->width)
    preferredRect->width = maxRect->width;
  if (preferredRect->height > maxRect->height)
    preferredRect->height = maxRect->height;
}  
