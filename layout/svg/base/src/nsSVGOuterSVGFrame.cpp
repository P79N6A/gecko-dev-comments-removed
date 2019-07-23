





































#include "nsSVGOuterSVGFrame.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsSVGSVGElement.h"
#include "nsSVGTextFrame.h"
#include "nsSVGForeignObjectFrame.h"
#include "nsDisplayList.h"
#include "nsStubMutationObserver.h"
#include "gfxContext.h"
#include "nsPresShellIterator.h"
#include "nsIContentViewer.h"
#include "nsIDocShell.h"
#include "nsIDOMDocument.h"
#include "nsIDOMWindowInternal.h"
#include "nsPIDOMWindow.h"
#include "nsIObjectLoadingContent.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsSVGMatrix.h"

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
                                        PRInt32 aModType,
                                        PRUint32 aStateMask)
{
  if (aNameSpaceID != kNameSpaceID_XML || aAttribute != nsGkAtoms::space) {
    return;
  }

  nsPresShellIterator iter(aDocument);
  nsCOMPtr<nsIPresShell> shell;
  while ((shell = iter.GetNextShell())) {
    nsIFrame *frame = shell->GetPrimaryFrameFor(aContent);
    if (!frame) {
      continue;
    }

    
    nsSVGTextContainerFrame *containerFrame = do_QueryFrame(frame);
    if (containerFrame) {
      containerFrame->NotifyGlyphMetricsChange();
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
      nsSVGTextFrame* textFrame = static_cast<nsSVGTextFrame*>(kid);
      textFrame->NotifyGlyphMetricsChange();
    } else {
      UpdateTextFragmentTrees(kid);
    }
    kid = kid->GetNextSibling();
  }
}




nsIFrame*
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{  
  return new (aPresShell) nsSVGOuterSVGFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGOuterSVGFrame)

nsSVGOuterSVGFrame::nsSVGOuterSVGFrame(nsStyleContext* aContext)
    : nsSVGOuterSVGFrameBase(aContext)
    ,  mRedrawSuspendCount(0)
    , mFullZoom(0)
    , mViewportInitialized(PR_FALSE)
#ifdef XP_MACOSX
    , mEnableBitmapFallback(PR_FALSE)
#endif
    , mIsRootContent(PR_FALSE)
{
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::Init(nsIContent* aContent,
                         nsIFrame* aParent,
                         nsIFrame* aPrevInFlow)
{
#ifdef DEBUG
  nsCOMPtr<nsIDOMSVGSVGElement> svgElement = do_QueryInterface(aContent);
  NS_ASSERTION(svgElement, "Content is not an SVG 'svg' element!");
#endif

  AddStateBits(NS_STATE_IS_OUTER_SVG);

  nsresult rv = nsSVGOuterSVGFrameBase::Init(aContent, aParent, aPrevInFlow);

  nsIDocument* doc = mContent->GetCurrentDoc();
  if (doc) {
    
    if (doc->GetRootContent() == mContent) {
      mIsRootContent = PR_TRUE;
    }
    
    
    
    doc->AddMutationObserver(&sSVGMutationObserver);
  }

  SuspendRedraw();  

  return rv;
}




NS_QUERYFRAME_HEAD(nsSVGOuterSVGFrame)
  NS_QUERYFRAME_ENTRY(nsISVGSVGFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGOuterSVGFrameBase)



  



 nscoord
nsSVGOuterSVGFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = nscoord(0);

  return result;
}

 nscoord
nsSVGOuterSVGFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  nsSVGSVGElement *svg = static_cast<nsSVGSVGElement*>(mContent);
  nsSVGLength2 &width = svg->mLengthAttributes[nsSVGSVGElement::WIDTH];

  if (width.IsPercentage()) {
    
    
    
    result = nscoord(0);
  } else {
    result = nsPresContext::CSSPixelsToAppUnits(width.GetAnimValue(svg));
    if (result < 0) {
      result = nscoord(0);
    }
  }

  return result;
}

 nsIFrame::IntrinsicSize
nsSVGOuterSVGFrame::GetIntrinsicSize()
{
  
  

  IntrinsicSize intrinsicSize;

  nsSVGSVGElement *content = static_cast<nsSVGSVGElement*>(mContent);
  nsSVGLength2 &width  = content->mLengthAttributes[nsSVGSVGElement::WIDTH];
  nsSVGLength2 &height = content->mLengthAttributes[nsSVGSVGElement::HEIGHT];

  if (width.IsPercentage()) {
    float val = width.GetAnimValInSpecifiedUnits() / 100.0f;
    if (val < 0.0f) val = 0.0f;
    intrinsicSize.width.SetPercentValue(val);
  } else {
    nscoord val = nsPresContext::CSSPixelsToAppUnits(width.GetAnimValue(content));
    if (val < 0) val = 0;
    intrinsicSize.width.SetCoordValue(val);
  }

  if (height.IsPercentage()) {
    float val = height.GetAnimValInSpecifiedUnits() / 100.0f;
    if (val < 0.0f) val = 0.0f;
    intrinsicSize.height.SetPercentValue(val);
  } else {
    nscoord val = nsPresContext::CSSPixelsToAppUnits(height.GetAnimValue(content));
    if (val < 0) val = 0;
    intrinsicSize.height.SetCoordValue(val);
  }

  return intrinsicSize;
}

 nsSize
nsSVGOuterSVGFrame::GetIntrinsicRatio()
{
  
  
  

  nsSVGSVGElement *content = static_cast<nsSVGSVGElement*>(mContent);
  nsSVGLength2 &width  = content->mLengthAttributes[nsSVGSVGElement::WIDTH];
  nsSVGLength2 &height = content->mLengthAttributes[nsSVGSVGElement::HEIGHT];

  if (!width.IsPercentage() && !height.IsPercentage()) {
    nsSize ratio(width.GetAnimValue(content), height.GetAnimValue(content));
    if (ratio.width < 0) {
      ratio.width = 0;
    }
    if (ratio.height < 0) {
      ratio.height = 0;
    }
    return ratio;
  }

  if (content->HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
    
    
    const nsSVGViewBoxRect viewbox = content->mViewBox.GetAnimValue();
    float viewBoxWidth = viewbox.width;
    float viewBoxHeight = viewbox.height;

    if (viewBoxWidth < 0.0f) {
      viewBoxWidth = 0.0f;
    }
    if (viewBoxHeight < 0.0f) {
      viewBoxHeight = 0.0f;
    }
    return nsSize(viewBoxWidth, viewBoxHeight);
  }

  return nsSVGOuterSVGFrameBase::GetIntrinsicRatio();
}

 nsSize
nsSVGOuterSVGFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
                                nsSize aCBSize, nscoord aAvailableWidth,
                                nsSize aMargin, nsSize aBorder, nsSize aPadding,
                                PRBool aShrinkWrap)
{
  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox) &&
      EmbeddedByReference()) {
    
    
    return aCBSize;
  }

  return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(
                            aRenderingContext, this,
                            GetIntrinsicSize(), GetIntrinsicRatio(), aCBSize,
                            aMargin, aBorder, aPadding);
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::Reflow(nsPresContext*           aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsSVGOuterSVGFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("enter nsSVGOuterSVGFrame::Reflow: availSize=%d,%d",
                  aReflowState.availableWidth, aReflowState.availableHeight));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  aStatus = NS_FRAME_COMPLETE;

  aDesiredSize.width  = aReflowState.ComputedWidth() +
                          aReflowState.mComputedBorderPadding.LeftRight();
  aDesiredSize.height = aReflowState.ComputedHeight() +
                          aReflowState.mComputedBorderPadding.TopBottom();

  NS_ASSERTION(!GetPrevInFlow(), "SVG can't currently be broken across pages.");

  
  
  aDesiredSize.mOverflowArea.SetRect(0, 0, aDesiredSize.width, aDesiredSize.height);
  FinishAndStoreOverflow(&aDesiredSize);

  
  

  svgFloatSize newViewportSize(
    nsPresContext::AppUnitsToFloatCSSPixels(aReflowState.ComputedWidth()),
    nsPresContext::AppUnitsToFloatCSSPixels(aReflowState.ComputedHeight()));

  nsSVGSVGElement *svgElem = static_cast<nsSVGSVGElement*>(mContent);

  if (newViewportSize != svgElem->GetViewportSize() ||
      mFullZoom != PresContext()->GetFullZoom()) {
    svgElem->SetViewportSize(newViewportSize);
    mViewportInitialized = PR_TRUE;
    mFullZoom = PresContext()->GetFullZoom();
    NotifyViewportChange();
  }

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsSVGOuterSVGFrame::Reflow: size=%d,%d",
                  aDesiredSize.width, aDesiredSize.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

static PLDHashOperator
ReflowForeignObject(nsVoidPtrHashKey *aEntry, void* aUserArg)
{
  static_cast<nsSVGForeignObjectFrame*>
    (const_cast<void*>(aEntry->GetKey()))->MaybeReflowFromOuterSVGFrame();
  return PL_DHASH_NEXT;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::DidReflow(nsPresContext*   aPresContext,
                              const nsHTMLReflowState*  aReflowState,
                              nsDidReflowStatus aStatus)
{
  PRBool firstReflow = (GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  nsresult rv = nsSVGOuterSVGFrameBase::DidReflow(aPresContext,aReflowState,aStatus);

  if (firstReflow) {
    
    nsIFrame* kid = mFrames.FirstChild();
    while (kid) {
      nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
      if (SVGFrame) {
        SVGFrame->InitialUpdate(); 
      }
      kid = kid->GetNextSibling();
    }
    
    UnsuspendRedraw(); 
  } else {
    
    
    if (mForeignObjectHash.IsInitialized()) {
#ifdef DEBUG
      PRUint32 count =
#endif
        mForeignObjectHash.EnumerateEntries(ReflowForeignObject, nsnull);
      NS_ASSERTION(count == mForeignObjectHash.Count(),
                   "We didn't reflow all our nsSVGForeignObjectFrames!");
    }
  }
  
  return rv;
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

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState);
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
                     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("SVGEventReceiver")
};

nsIFrame*
nsDisplaySVG::HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                      HitTestState* aState)
{
  return static_cast<nsSVGOuterSVGFrame*>(mFrame)->
    GetFrameForPoint(aPt - aBuilder->ToReferenceFrame(mFrame));
}

void
nsDisplaySVG::Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
                    const nsRect& aDirtyRect)
{
  static_cast<nsSVGOuterSVGFrame*>(mFrame)->
    Paint(*aCtx, aDirtyRect, aBuilder->ToReferenceFrame(mFrame));
}


static inline PRBool
DependsOnIntrinsicSize(const nsIFrame* aEmbeddingFrame)
{
  const nsStylePosition *pos = aEmbeddingFrame->GetStylePosition();
  nsStyleUnit widthUnit  = pos->mWidth.GetUnit();
  nsStyleUnit heightUnit = pos->mHeight.GetUnit();

  
  
  
  return (widthUnit != eStyleUnit_Coord) || (heightUnit != eStyleUnit_Coord);
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     PRInt32  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      !(GetStateBits() & NS_FRAME_FIRST_REFLOW) &&
      (aAttribute == nsGkAtoms::width || aAttribute == nsGkAtoms::height)) {
    nsIFrame* embeddingFrame;
    EmbeddedByReference(&embeddingFrame);
    if (embeddingFrame) {
      if (DependsOnIntrinsicSize(embeddingFrame)) {
        
        
        embeddingFrame->PresContext()->PresShell()->
          FrameNeedsReflow(embeddingFrame, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
      }
      
    } else {
      
      
      PresContext()->PresShell()->
        FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
    }
  }

  return NS_OK;
}

nsIFrame*
nsSVGOuterSVGFrame::GetFrameForPoint(const nsPoint& aPoint)
{
  nsRect thisRect(nsPoint(0,0), GetSize());
  if (!thisRect.Contains(aPoint)) {
    return nsnull;
  }

  return nsSVGUtils::HitTestChildren(this, aPoint);
}




NS_IMETHODIMP
nsSVGOuterSVGFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  return aLists.Content()->AppendNewToTop(new (aBuilder) nsDisplaySVG(this));
}

void
nsSVGOuterSVGFrame::Paint(nsIRenderingContext& aRenderingContext,
                          const nsRect& aDirtyRect, nsPoint aPt)
{
  
  aRenderingContext.PushState();

  nsMargin bp = GetUsedBorderAndPadding();
  ApplySkipSides(bp);

  nsRect viewportRect = GetContentRect();
  nsPoint viewportOffset = aPt + nsPoint(bp.left, bp.top);
  viewportRect.MoveTo(viewportOffset);

  nsRect clipRect;
  clipRect.IntersectRect(aDirtyRect, viewportRect);
  aRenderingContext.SetClipRect(clipRect, nsClipCombine_kIntersect);
  aRenderingContext.Translate(viewportRect.x, viewportRect.y);
  nsRect dirtyRect = clipRect - viewportOffset;

#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime start = PR_Now();
#endif

  nsIntRect dirtyPxRect = dirtyRect.ToOutsidePixels(PresContext()->AppUnitsPerDevPixel());

  nsSVGRenderState ctx(&aRenderingContext);

#ifdef XP_MACOSX
  if (mEnableBitmapFallback) {
    
    
    ctx.GetGfxContext()->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  }
#endif

  nsSVGUtils::PaintFrameWithEffects(&ctx, &dirtyPxRect, this);

#ifdef XP_MACOSX
  if (mEnableBitmapFallback) {
    
    ctx.GetGfxContext()->PopGroupToSource();
    ctx.GetGfxContext()->Paint();
  }
  
  if (ctx.GetGfxContext()->HasError() && !mEnableBitmapFallback) {
    mEnableBitmapFallback = PR_TRUE;
    
    
    
    
    
    
    
    nsIFrame* frame = this;
    PRUint32 flags = 0;
    while (PR_TRUE) {
      nsIFrame* next = nsLayoutUtils::GetCrossDocParentFrame(frame);
      if (!next)
        break;
      if (frame->GetParent() != next) {
        
        
        
        
        flags |= INVALIDATE_CROSS_DOC;
      }
      frame = next;
    }
    frame->InvalidateWithFlags(nsRect(nsPoint(0, 0), frame->GetSize()), flags);
  }
#endif

#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime end = PR_Now();
  printf("SVG Paint Timing: %f ms\n", (end-start)/1000.0);
#endif
  
  aRenderingContext.PopState();
}

nsSplittableType
nsSVGOuterSVGFrame::GetSplittableType() const
{
  return NS_FRAME_NOT_SPLITTABLE;
}

nsIAtom *
nsSVGOuterSVGFrame::GetType() const
{
  return nsGkAtoms::svgOuterSVGFrame;
}




void
nsSVGOuterSVGFrame::InvalidateCoveredRegion(nsIFrame *aFrame)
{
  nsISVGChildFrame *svgFrame = do_QueryFrame(aFrame);
  if (!svgFrame)
    return;

  nsRect rect = nsSVGUtils::FindFilterInvalidation(aFrame, svgFrame->GetCoveredRegion());
  Invalidate(rect);
}

PRBool
nsSVGOuterSVGFrame::UpdateAndInvalidateCoveredRegion(nsIFrame *aFrame)
{
  nsISVGChildFrame *svgFrame = do_QueryFrame(aFrame);
  if (!svgFrame)
    return PR_FALSE;

  nsRect oldRegion = svgFrame->GetCoveredRegion();
  Invalidate(nsSVGUtils::FindFilterInvalidation(aFrame, oldRegion));
  svgFrame->UpdateCoveredRegion();
  nsRect newRegion = svgFrame->GetCoveredRegion();
  if (oldRegion == newRegion)
    return PR_FALSE;

  Invalidate(nsSVGUtils::FindFilterInvalidation(aFrame, newRegion));
  return PR_TRUE;
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
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
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

  NS_ASSERTION(mRedrawSuspendCount >=0, "unbalanced suspend count!");

  if (--mRedrawSuspendCount > 0)
    return NS_OK;

  for (nsIFrame* kid = mFrames.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
    if (SVGFrame) {
      SVGFrame->NotifyRedrawUnsuspended();
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::NotifyViewportChange()
{
  
  if (!mViewportInitialized) {
    return NS_OK;
  }

  PRUint32 flags = COORD_CONTEXT_CHANGED;

  
#if 1
  {
#else

  if (mContent->HasAttr(kNameSpaceID_None, nsGkAtoms::viewBox)) {
#endif

    mCanvasTM = nsnull;

    flags |= TRANSFORM_CHANGED;
  }

  
  SuspendRedraw();
  nsSVGUtils::NotifyChildrenOfSVGChange(this, flags);
  UnsuspendRedraw();
  return NS_OK;
}




gfxMatrix
nsSVGOuterSVGFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    nsSVGSVGElement *content = static_cast<nsSVGSVGElement*>(mContent);

    float devPxPerCSSPx =
      1.0f / PresContext()->AppUnitsToFloatCSSPixels(
                                PresContext()->AppUnitsPerDevPixel());

    gfxMatrix viewBoxTM = content->GetViewBoxTransform();

    gfxMatrix zoomPanTM;
    if (mIsRootContent) {
      const nsSVGTranslatePoint& translate = content->GetCurrentTranslate();
      zoomPanTM.Translate(gfxPoint(translate.GetX(), translate.GetY()));
      zoomPanTM.Scale(content->GetCurrentScale(), content->GetCurrentScale());
    }

    gfxMatrix TM = viewBoxTM * zoomPanTM * gfxMatrix().Scale(devPxPerCSSPx, devPxPerCSSPx);
    mCanvasTM = NS_NewSVGMatrix(TM);
  }
  return nsSVGUtils::ConvertSVGMatrixToThebes(mCanvasTM);
}




void
nsSVGOuterSVGFrame::RegisterForeignObject(nsSVGForeignObjectFrame* aFrame)
{
  NS_ASSERTION(aFrame, "Who on earth is calling us?!");

  if (!mForeignObjectHash.IsInitialized()) {
    if (!mForeignObjectHash.Init()) {
      NS_ERROR("Failed to initialize foreignObject hash.");
      return;
    }
  }

  NS_ASSERTION(!mForeignObjectHash.GetEntry(aFrame),
               "nsSVGForeignObjectFrame already registered!");

  mForeignObjectHash.PutEntry(aFrame);

  NS_ASSERTION(mForeignObjectHash.GetEntry(aFrame),
               "Failed to register nsSVGForeignObjectFrame!");
}

void
nsSVGOuterSVGFrame::UnregisterForeignObject(nsSVGForeignObjectFrame* aFrame)
{
  NS_ASSERTION(aFrame, "Who on earth is calling us?!");
  NS_ASSERTION(mForeignObjectHash.GetEntry(aFrame),
               "nsSVGForeignObjectFrame not in registry!");
  return mForeignObjectHash.RemoveEntry(aFrame);
}

PRBool
nsSVGOuterSVGFrame::EmbeddedByReference(nsIFrame **aEmbeddingFrame)
{
  if (mContent->GetParent() == nsnull) {
    
    nsCOMPtr<nsISupports> container = PresContext()->GetContainer();
    nsCOMPtr<nsIDOMWindowInternal> window = do_GetInterface(container);
    if (window) {
      nsCOMPtr<nsIDOMElement> frameElement;
      window->GetFrameElement(getter_AddRefs(frameElement));
      nsCOMPtr<nsIObjectLoadingContent> olc = do_QueryInterface(frameElement);
      if (olc) {
        
        if (aEmbeddingFrame) {
          nsCOMPtr<nsIContent> element = do_QueryInterface(frameElement);
          *aEmbeddingFrame =
            static_cast<nsGenericElement*>(element.get())->GetPrimaryFrame();
          NS_ASSERTION(*aEmbeddingFrame, "Yikes, no embedding frame!");
        }
        return PR_TRUE;
      }
    }
  }
  if (aEmbeddingFrame) {
    *aEmbeddingFrame = nsnull;
  }
  return PR_FALSE;
}
