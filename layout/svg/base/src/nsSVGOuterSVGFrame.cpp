






































#include "nsSVGOuterSVGFrame.h"


#include "DOMSVGTests.h"
#include "gfxMatrix.h"
#include "nsDisplayList.h"
#include "nsIDocument.h"
#include "nsIDOMSVGSVGElement.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObjectLoadingContent.h"
#include "nsRenderingContext.h"
#include "nsStubMutationObserver.h"
#include "nsSVGSVGElement.h"
#include "nsSVGTextFrame.h"

namespace dom = mozilla::dom;

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
nsSVGMutationObserver::AttributeChanged(nsIDocument* aDocument,
                                        dom::Element* aElement,
                                        PRInt32 aNameSpaceID,
                                        nsIAtom* aAttribute,
                                        PRInt32 aModType)
{
  if (aNameSpaceID != kNameSpaceID_XML || aAttribute != nsGkAtoms::space) {
    return;
  }

  nsIFrame* frame = aElement->GetPrimaryFrame();
  if (!frame) {
    return;
  }

  
  nsSVGTextContainerFrame* containerFrame = do_QueryFrame(frame);
  if (containerFrame) {
    containerFrame->NotifyGlyphMetricsChange();
    return;
  }
  
  UpdateTextFragmentTrees(frame);
}




void
nsSVGMutationObserver::UpdateTextFragmentTrees(nsIFrame *aFrame)
{
  nsIFrame* kid = aFrame->GetFirstPrincipalChild();
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
    , mFullZoom(0)
    , mViewportInitialized(false)
#ifdef XP_MACOSX
    , mEnableBitmapFallback(false)
#endif
    , mIsRootContent(false)
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

  
  
  
  
  
  
  
  
  nsSVGSVGElement *svg = static_cast<nsSVGSVGElement*>(aContent);
  if (!svg->PassesConditionalProcessingTests()) {
    AddStateBits(NS_STATE_SVG_NONDISPLAY_CHILD);
  }

  nsresult rv = nsSVGOuterSVGFrameBase::Init(aContent, aParent, aPrevInFlow);

  nsIDocument* doc = mContent->GetCurrentDoc();
  if (doc) {
    
    if (doc->GetRootElement() == mContent) {
      mIsRootContent = true;
    }
    
    
    doc->AddMutationObserverUnlessExists(&sSVGMutationObserver);
  }

  return rv;
}




NS_QUERYFRAME_HEAD(nsSVGOuterSVGFrame)
  NS_QUERYFRAME_ENTRY(nsISVGSVGFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGOuterSVGFrameBase)



  



 nscoord
nsSVGOuterSVGFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = nscoord(0);

  return result;
}

 nscoord
nsSVGOuterSVGFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
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

  if (!width.IsPercentage()) {
    nscoord val = nsPresContext::CSSPixelsToAppUnits(width.GetAnimValue(content));
    if (val < 0) val = 0;
    intrinsicSize.width.SetCoordValue(val);
  }

  if (!height.IsPercentage()) {
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
    nsSize ratio(NSToCoordRoundWithClamp(width.GetAnimValue(content)),
                 NSToCoordRoundWithClamp(height.GetAnimValue(content)));
    if (ratio.width < 0) {
      ratio.width = 0;
    }
    if (ratio.height < 0) {
      ratio.height = 0;
    }
    return ratio;
  }

  if (content->mViewBox.IsValid()) {
    const nsSVGViewBoxRect viewbox = content->mViewBox.GetAnimValue();
    float viewBoxWidth = viewbox.width;
    float viewBoxHeight = viewbox.height;

    if (viewBoxWidth < 0.0f) {
      viewBoxWidth = 0.0f;
    }
    if (viewBoxHeight < 0.0f) {
      viewBoxHeight = 0.0f;
    }
    return nsSize(NSToCoordRoundWithClamp(viewBoxWidth),
                  NSToCoordRoundWithClamp(viewBoxHeight));
  }

  return nsSVGOuterSVGFrameBase::GetIntrinsicRatio();
}

 nsSize
nsSVGOuterSVGFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                nsSize aCBSize, nscoord aAvailableWidth,
                                nsSize aMargin, nsSize aBorder, nsSize aPadding,
                                PRUint32 aFlags)
{
  nsSVGSVGElement* content = static_cast<nsSVGSVGElement*>(mContent);

  IntrinsicSize intrinsicSize = GetIntrinsicSize();

  if (!mContent->GetParent()) {
    if (IsRootOfImage() || IsRootOfReplacedElementSubDoc()) {
      
      
      
      return aCBSize;
    } else {
      
      
      
      
      nsSVGLength2 &width =
        content->mLengthAttributes[nsSVGSVGElement::WIDTH];
      if (width.IsPercentage()) {
        NS_ABORT_IF_FALSE(intrinsicSize.width.GetUnit() == eStyleUnit_None,
                          "GetIntrinsicSize should have reported no "
                          "intrinsic width");
        float val = width.GetAnimValInSpecifiedUnits() / 100.0f;
        if (val < 0.0f) val = 0.0f;
        intrinsicSize.width.SetCoordValue(val * aCBSize.width);
      }

      nsSVGLength2 &height =
        content->mLengthAttributes[nsSVGSVGElement::HEIGHT];
      NS_ASSERTION(aCBSize.height != NS_AUTOHEIGHT,
                   "root should not have auto-height containing block");
      if (height.IsPercentage()) {
        NS_ABORT_IF_FALSE(intrinsicSize.height.GetUnit() == eStyleUnit_None,
                          "GetIntrinsicSize should have reported no "
                          "intrinsic height");
        float val = height.GetAnimValInSpecifiedUnits() / 100.0f;
        if (val < 0.0f) val = 0.0f;
        intrinsicSize.height.SetCoordValue(val * aCBSize.height);
      }
      NS_ABORT_IF_FALSE(intrinsicSize.height.GetUnit() == eStyleUnit_Coord &&
                        intrinsicSize.width.GetUnit() == eStyleUnit_Coord,
                        "We should have just handled the only situation where"
                        "we lack an intrinsic height or width.");
    }
  }

  return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(
                            aRenderingContext, this,
                            intrinsicSize, GetIntrinsicRatio(), aCBSize,
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

  
  
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  FinishAndStoreOverflow(&aDesiredSize);

  
  

  svgFloatSize newViewportSize(
    nsPresContext::AppUnitsToFloatCSSPixels(aReflowState.ComputedWidth()),
    nsPresContext::AppUnitsToFloatCSSPixels(aReflowState.ComputedHeight()));

  nsSVGSVGElement *svgElem = static_cast<nsSVGSVGElement*>(mContent);

  if (newViewportSize != svgElem->GetViewportSize() ||
      mFullZoom != PresContext()->GetFullZoom()) {
    svgElem->SetViewportSize(newViewportSize);
    mViewportInitialized = true;
    mFullZoom = PresContext()->GetFullZoom();
    NotifyViewportChange();
  }

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsSVGOuterSVGFrame::Reflow: size=%d,%d",
                  aDesiredSize.width, aDesiredSize.height));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
  return NS_OK;
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::DidReflow(nsPresContext*   aPresContext,
                              const nsHTMLReflowState*  aReflowState,
                              nsDidReflowStatus aStatus)
{
  bool firstReflow = (GetStateBits() & NS_FRAME_FIRST_REFLOW) != 0;

  nsresult rv = nsSVGOuterSVGFrameBase::DidReflow(aPresContext,aReflowState,aStatus);

  if (firstReflow) {
    
    
    
    AddStateBits(NS_FRAME_FIRST_REFLOW);
  }

#ifdef DEBUG
  mCallingUpdateBounds = true;
#endif

  if (!(mState & NS_STATE_SVG_NONDISPLAY_CHILD)) {
    nsIFrame* kid = mFrames.FirstChild();
    while (kid) {
      nsISVGChildFrame* SVGFrame = do_QueryFrame(kid);
      if (SVGFrame && !(kid->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)) {
        SVGFrame->UpdateBounds(); 
      }
      kid = kid->GetNextSibling();
    }
  }

#ifdef DEBUG
  mCallingUpdateBounds = false;
#endif

  if (firstReflow) {
    
    RemoveStateBits(NS_FRAME_FIRST_REFLOW);
  }

  
  
  PresContext()->PresShell()->SynthesizeMouseMove(false);

  return rv;
}




class nsDisplaySVG : public nsDisplayItem {
public:
  nsDisplaySVG(nsDisplayListBuilder* aBuilder,
               nsSVGOuterSVGFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplaySVG);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplaySVG() {
    MOZ_COUNT_DTOR(nsDisplaySVG);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames);
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("SVGEventReceiver", TYPE_SVG_EVENT_RECEIVER)
};

void
nsDisplaySVG::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                      HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsSVGOuterSVGFrame *outerSVGFrame = static_cast<nsSVGOuterSVGFrame*>(mFrame);
  nsRect rectAtOrigin = aRect - ToReferenceFrame();
  nsRect thisRect(nsPoint(0,0), outerSVGFrame->GetSize());
  if (!thisRect.Intersects(rectAtOrigin))
    return;

  nsPoint rectCenter(rectAtOrigin.x + rectAtOrigin.width / 2,
                     rectAtOrigin.y + rectAtOrigin.height / 2);

  nsIFrame* frame = nsSVGUtils::HitTestChildren(
    outerSVGFrame, rectCenter + outerSVGFrame->GetPosition() -
                   outerSVGFrame->GetContentRect().TopLeft());
  if (frame) {
    aOutFrames->AppendElement(frame);
  }
}

void
nsDisplaySVG::Paint(nsDisplayListBuilder* aBuilder,
                    nsRenderingContext* aContext)
{
  nsSVGOuterSVGFrame *frame = static_cast<nsSVGOuterSVGFrame*>(mFrame);

  if (frame->GetStateBits() & NS_STATE_SVG_NONDISPLAY_CHILD)
    return;

#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime start = PR_Now();
#endif

  aContext->PushState();

#ifdef XP_MACOSX
  if (frame->BitmapFallbackEnabled()) {
    
    
    aContext->ThebesContext()->PushGroup(gfxASurface::CONTENT_COLOR_ALPHA);
  }
#endif

  frame->Paint(aBuilder, aContext, mVisibleRect, ToReferenceFrame());

#ifdef XP_MACOSX
  if (frame->BitmapFallbackEnabled()) {
    
    aContext->ThebesContext()->PopGroupToSource();
    aContext->ThebesContext()->Paint();
  }

  if (aContext->ThebesContext()->HasError() && !frame->BitmapFallbackEnabled()) {
    frame->SetBitmapFallbackEnabled(true);
    
    
    
    
    
    
    
    nsIFrame* ancestor = frame;
    PRUint32 flags = 0;
    while (true) {
      nsIFrame* next = nsLayoutUtils::GetCrossDocParentFrame(ancestor);
      if (!next)
        break;
      if (ancestor->GetParent() != next) {
        
        
        
        
        flags |= nsIFrame::INVALIDATE_CROSS_DOC;
      }
      ancestor = next;
    }
    ancestor->InvalidateWithFlags(nsRect(nsPoint(0, 0), ancestor->GetSize()), flags);
  }
#endif

  aContext->PopState();

#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime end = PR_Now();
  printf("SVG Paint Timing: %f ms\n", (end-start)/1000.0);
#endif
}


static inline bool
DependsOnIntrinsicSize(const nsIFrame* aEmbeddingFrame)
{
  const nsStylePosition *pos = aEmbeddingFrame->GetStylePosition();
  const nsStyleCoord &width = pos->mWidth;
  const nsStyleCoord &height = pos->mHeight;

  
  
  
  return !width.ConvertsToLength() ||
         !height.ConvertsToLength();
}

NS_IMETHODIMP
nsSVGOuterSVGFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     PRInt32  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      !(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    if (aAttribute == nsGkAtoms::viewBox ||
        aAttribute == nsGkAtoms::preserveAspectRatio ||
        aAttribute == nsGkAtoms::transform) {

      
      mCanvasTM = nsnull;

      nsSVGUtils::NotifyChildrenOfSVGChange(
          this, aAttribute == nsGkAtoms::viewBox ?
                  TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED : TRANSFORM_CHANGED);

    } else if (aAttribute == nsGkAtoms::width ||
               aAttribute == nsGkAtoms::height) {

        nsIFrame* embeddingFrame;
        if (IsRootOfReplacedElementSubDoc(&embeddingFrame) && embeddingFrame) {
          if (DependsOnIntrinsicSize(embeddingFrame)) {
            
            
            embeddingFrame->PresContext()->PresShell()->
              FrameNeedsReflow(embeddingFrame, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
          }
          
        } else {
          
          
          PresContext()->PresShell()->
            FrameNeedsReflow(this, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
        }
    }
  }

  return NS_OK;
}




NS_IMETHODIMP
nsSVGOuterSVGFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  nsresult rv = DisplayBorderBackgroundOutline(aBuilder, aLists);
  NS_ENSURE_SUCCESS(rv, rv);

  return aLists.Content()->AppendNewToTop(
      new (aBuilder) nsDisplaySVG(aBuilder, this));
}

void
nsSVGOuterSVGFrame::Paint(const nsDisplayListBuilder* aBuilder,
                          nsRenderingContext* aContext,
                          const nsRect& aDirtyRect, nsPoint aPt)
{
  nsRect viewportRect = GetContentRect();
  nsPoint viewportOffset = aPt + viewportRect.TopLeft() - GetPosition();
  viewportRect.MoveTo(viewportOffset);

  nsRect clipRect;
  clipRect.IntersectRect(aDirtyRect, viewportRect);
  aContext->IntersectClip(clipRect);
  aContext->Translate(viewportRect.TopLeft());
  nsRect dirtyRect = clipRect - viewportOffset;

  nsIntRect dirtyPxRect = dirtyRect.ToOutsidePixels(PresContext()->AppUnitsPerDevPixel());

  
  
  SVGAutoRenderState state(aContext, SVGAutoRenderState::GetRenderMode(aContext));

  if (aBuilder->IsPaintingToWindow()) {
    state.SetPaintingToWindow(true);
  }

  nsSVGUtils::PaintFrameWithEffects(aContext, &dirtyPxRect, this);
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
nsSVGOuterSVGFrame::NotifyViewportChange()
{
  
  if (!mViewportInitialized) {
    return;
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

  
  nsSVGUtils::NotifyChildrenOfSVGChange(this, flags);
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
    mCanvasTM = new gfxMatrix(TM);
  }
  return *mCanvasTM;
}




bool
nsSVGOuterSVGFrame::IsRootOfReplacedElementSubDoc(nsIFrame **aEmbeddingFrame)
{
  if (!mContent->GetParent()) {
    
    nsCOMPtr<nsISupports> container = PresContext()->GetContainer();
    nsCOMPtr<nsIDOMWindow> window = do_GetInterface(container);
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
        return true;
      }
    }
  }
  if (aEmbeddingFrame) {
    *aEmbeddingFrame = nsnull;
  }
  return false;
}

bool
nsSVGOuterSVGFrame::IsRootOfImage()
{
  if (!mContent->GetParent()) {
    
    nsIDocument* doc = mContent->GetCurrentDoc();
    if (doc && doc->IsBeingUsedAsImage()) {
      
      return true;
    }
  }

  return false;
}

bool
nsSVGOuterSVGFrame::VerticalScrollbarNotNeeded() const
{
  nsSVGLength2 &height = static_cast<nsSVGSVGElement*>(mContent)->
                           mLengthAttributes[nsSVGSVGElement::HEIGHT];
  return height.IsPercentage() && height.GetBaseValInSpecifiedUnits() <= 100;
}
