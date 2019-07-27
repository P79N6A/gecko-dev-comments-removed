





#include "nsSVGOuterSVGFrame.h"


#include "nsDisplayList.h"
#include "nsIDocument.h"
#include "nsIDOMWindow.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIObjectLoadingContent.h"
#include "nsRenderingContext.h"
#include "nsSVGIntegrationUtils.h"
#include "nsSVGForeignObjectFrame.h"
#include "mozilla/dom/SVGSVGElement.h"
#include "mozilla/dom/SVGViewElement.h"
#include "nsSubDocumentFrame.h"

using namespace mozilla;
using namespace mozilla::dom;




void
nsSVGOuterSVGFrame::RegisterForeignObject(nsSVGForeignObjectFrame* aFrame)
{
  NS_ASSERTION(aFrame, "Who on earth is calling us?!");

  if (!mForeignObjectHash) {
    mForeignObjectHash = new nsTHashtable<nsPtrHashKey<nsSVGForeignObjectFrame> >();
  }

  NS_ASSERTION(!mForeignObjectHash->GetEntry(aFrame),
               "nsSVGForeignObjectFrame already registered!");

  mForeignObjectHash->PutEntry(aFrame);

  NS_ASSERTION(mForeignObjectHash->GetEntry(aFrame),
               "Failed to register nsSVGForeignObjectFrame!");
}

void
nsSVGOuterSVGFrame::UnregisterForeignObject(nsSVGForeignObjectFrame* aFrame)
{
  NS_ASSERTION(aFrame, "Who on earth is calling us?!");
  NS_ASSERTION(mForeignObjectHash && mForeignObjectHash->GetEntry(aFrame),
               "nsSVGForeignObjectFrame not in registry!");
  return mForeignObjectHash->RemoveEntry(aFrame);
}




nsContainerFrame*
NS_NewSVGOuterSVGFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{  
  return new (aPresShell) nsSVGOuterSVGFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGOuterSVGFrame)

nsSVGOuterSVGFrame::nsSVGOuterSVGFrame(nsStyleContext* aContext)
    : nsSVGOuterSVGFrameBase(aContext)
    , mFullZoom(aContext->PresContext()->GetFullZoom())
    , mViewportInitialized(false)
    , mIsRootContent(false)
{
  
  RemoveStateBits(NS_FRAME_SVG_LAYOUT);
}


static inline bool
DependsOnIntrinsicSize(const nsIFrame* aEmbeddingFrame)
{
  const nsStylePosition *pos = aEmbeddingFrame->StylePosition();
  const nsStyleCoord &width = pos->mWidth;
  const nsStyleCoord &height = pos->mHeight;

  
  
  
  return !width.ConvertsToLength() ||
         !height.ConvertsToLength();
}

void
nsSVGOuterSVGFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent,
                         nsIFrame*         aPrevInFlow)
{
  NS_ASSERTION(aContent->IsSVGElement(nsGkAtoms::svg),
               "Content is not an SVG 'svg' element!");

  AddStateBits(NS_STATE_IS_OUTER_SVG |
               NS_FRAME_FONT_INFLATION_CONTAINER |
               NS_FRAME_FONT_INFLATION_FLOW_ROOT);

  
  
  
  
  
  
  
  
  SVGSVGElement *svg = static_cast<SVGSVGElement*>(aContent);
  if (!svg->PassesConditionalProcessingTests()) {
    AddStateBits(NS_FRAME_IS_NONDISPLAY);
  }

  nsSVGOuterSVGFrameBase::Init(aContent, aParent, aPrevInFlow);

  nsIDocument* doc = mContent->GetCurrentDoc();
  if (doc) {
    
    if (doc->GetRootElement() == mContent) {
      mIsRootContent = true;

      nsIFrame* embeddingFrame;
      if (IsRootOfReplacedElementSubDoc(&embeddingFrame) && embeddingFrame) {
        if (MOZ_UNLIKELY(!embeddingFrame->HasAllStateBits(NS_FRAME_IS_DIRTY)) &&
            DependsOnIntrinsicSize(embeddingFrame)) {
          
          
          
          
          embeddingFrame->PresContext()->PresShell()->
            FrameNeedsReflow(embeddingFrame, nsIPresShell::eStyleChange, NS_FRAME_IS_DIRTY);
        }
      }
    }
  }
}




NS_QUERYFRAME_HEAD(nsSVGOuterSVGFrame)
  NS_QUERYFRAME_ENTRY(nsISVGSVGFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsSVGOuterSVGFrameBase)



  



 nscoord
nsSVGOuterSVGFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_MIN_WIDTH(this, result);

  result = nscoord(0);

  return result;
}

 nscoord
nsSVGOuterSVGFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  nscoord result;
  DISPLAY_PREF_WIDTH(this, result);

  SVGSVGElement *svg = static_cast<SVGSVGElement*>(mContent);
  nsSVGLength2 &width = svg->mLengthAttributes[SVGSVGElement::ATTR_WIDTH];

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

 IntrinsicSize
nsSVGOuterSVGFrame::GetIntrinsicSize()
{
  
  

  IntrinsicSize intrinsicSize;

  SVGSVGElement *content = static_cast<SVGSVGElement*>(mContent);
  nsSVGLength2 &width  = content->mLengthAttributes[SVGSVGElement::ATTR_WIDTH];
  nsSVGLength2 &height = content->mLengthAttributes[SVGSVGElement::ATTR_HEIGHT];

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
  
  
  

  SVGSVGElement *content = static_cast<SVGSVGElement*>(mContent);
  nsSVGLength2 &width  = content->mLengthAttributes[SVGSVGElement::ATTR_WIDTH];
  nsSVGLength2 &height = content->mLengthAttributes[SVGSVGElement::ATTR_HEIGHT];

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

  SVGViewElement* viewElement = content->GetCurrentViewElement();
  const nsSVGViewBoxRect* viewbox = nullptr;

  
  if (viewElement && viewElement->mViewBox.HasRect()) {
    viewbox = &viewElement->mViewBox.GetAnimValue();
  } else if (content->mViewBox.HasRect()) {
    viewbox = &content->mViewBox.GetAnimValue();
  }

  if (viewbox) {
    float viewBoxWidth = viewbox->width;
    float viewBoxHeight = viewbox->height;

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


LogicalSize
nsSVGOuterSVGFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                WritingMode aWM,
                                const LogicalSize& aCBSize,
                                nscoord aAvailableISize,
                                const LogicalSize& aMargin,
                                const LogicalSize& aBorder,
                                const LogicalSize& aPadding,
                                ComputeSizeFlags aFlags)
{
  if (IsRootOfImage() || IsRootOfReplacedElementSubDoc()) {
    
    
    
    
    
    return aCBSize;
  }

  LogicalSize cbSize = aCBSize;
  IntrinsicSize intrinsicSize = GetIntrinsicSize();

  if (!mContent->GetParent()) {
    
    

    NS_ASSERTION(aCBSize.ISize(aWM) != NS_AUTOHEIGHT &&
                 aCBSize.BSize(aWM) != NS_AUTOHEIGHT,
                 "root should not have auto-width/height containing block");
    cbSize.ISize(aWM) *= PresContext()->GetFullZoom();
    cbSize.BSize(aWM) *= PresContext()->GetFullZoom();

    
    
    
    
    

    SVGSVGElement* content = static_cast<SVGSVGElement*>(mContent);

    nsSVGLength2 &width =
      content->mLengthAttributes[SVGSVGElement::ATTR_WIDTH];
    if (width.IsPercentage()) {
      MOZ_ASSERT(intrinsicSize.width.GetUnit() == eStyleUnit_None,
                 "GetIntrinsicSize should have reported no intrinsic width");
      float val = width.GetAnimValInSpecifiedUnits() / 100.0f;
      if (val < 0.0f) val = 0.0f;
      intrinsicSize.width.SetCoordValue(val * cbSize.Width(aWM));
    }

    nsSVGLength2 &height =
      content->mLengthAttributes[SVGSVGElement::ATTR_HEIGHT];
    NS_ASSERTION(aCBSize.BSize(aWM) != NS_AUTOHEIGHT,
                 "root should not have auto-height containing block");
    if (height.IsPercentage()) {
      MOZ_ASSERT(intrinsicSize.height.GetUnit() == eStyleUnit_None,
                 "GetIntrinsicSize should have reported no intrinsic height");
      float val = height.GetAnimValInSpecifiedUnits() / 100.0f;
      if (val < 0.0f) val = 0.0f;
      intrinsicSize.height.SetCoordValue(val * cbSize.Height(aWM));
    }
    MOZ_ASSERT(intrinsicSize.height.GetUnit() == eStyleUnit_Coord &&
               intrinsicSize.width.GetUnit() == eStyleUnit_Coord,
               "We should have just handled the only situation where"
               "we lack an intrinsic height or width.");
  }

  return nsLayoutUtils::ComputeSizeWithIntrinsicDimensions(aWM,
                            aRenderingContext, this,
                            intrinsicSize, GetIntrinsicRatio(),
                            cbSize,
                            aMargin,
                            aBorder,
                            aPadding);
}

void
nsSVGOuterSVGFrame::Reflow(nsPresContext*           aPresContext,
                           nsHTMLReflowMetrics&     aDesiredSize,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsSVGOuterSVGFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("enter nsSVGOuterSVGFrame::Reflow: availSize=%d,%d",
                  aReflowState.AvailableWidth(), aReflowState.AvailableHeight()));

  NS_PRECONDITION(mState & NS_FRAME_IN_REFLOW, "frame is not in reflow");

  aStatus = NS_FRAME_COMPLETE;

  aDesiredSize.Width()  = aReflowState.ComputedWidth() +
                          aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  aDesiredSize.Height() = aReflowState.ComputedHeight() +
                          aReflowState.ComputedPhysicalBorderPadding().TopBottom();

  NS_ASSERTION(!GetPrevInFlow(), "SVG can't currently be broken across pages.");

  SVGSVGElement *svgElem = static_cast<SVGSVGElement*>(mContent);

  nsSVGOuterSVGAnonChildFrame *anonKid =
    static_cast<nsSVGOuterSVGAnonChildFrame*>(GetFirstPrincipalChild());

  if (mState & NS_FRAME_FIRST_REFLOW) {
    
    svgElem->UpdateHasChildrenOnlyTransform();
  }

  
  

  svgFloatSize newViewportSize(
    nsPresContext::AppUnitsToFloatCSSPixels(aReflowState.ComputedWidth()),
    nsPresContext::AppUnitsToFloatCSSPixels(aReflowState.ComputedHeight()));

  svgFloatSize oldViewportSize = svgElem->GetViewportSize();

  uint32_t changeBits = 0;
  if (newViewportSize != oldViewportSize) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    if (svgElem->HasViewBoxOrSyntheticViewBox()) {
      nsIFrame* anonChild = GetFirstPrincipalChild();
      anonChild->AddStateBits(NS_FRAME_IS_DIRTY);
      for (nsIFrame* child = anonChild->GetFirstPrincipalChild(); child;
           child = child->GetNextSibling()) {
        child->AddStateBits(NS_FRAME_IS_DIRTY);
      }
    }
    changeBits |= COORD_CONTEXT_CHANGED;
    svgElem->SetViewportSize(newViewportSize);
  }
  if (mFullZoom != PresContext()->GetFullZoom()) {
    changeBits |= FULL_ZOOM_CHANGED;
    mFullZoom = PresContext()->GetFullZoom();
  }
  if (changeBits) {
    NotifyViewportOrTransformChanged(changeBits);
  }
  mViewportInitialized = true;

  
  
  
  mCallingReflowSVG = true;
  if (GetStateBits() & NS_FRAME_IS_NONDISPLAY) {
    ReflowSVGNonDisplayText(this);
  } else {
    
    
    anonKid->AddStateBits(mState & NS_FRAME_IS_DIRTY);
    anonKid->ReflowSVG();
    MOZ_ASSERT(!anonKid->GetNextSibling(),
               "We should have one anonymous child frame wrapping our real "
               "children");
  }
  mCallingReflowSVG = false;

  
  anonKid->SetPosition(GetContentRectRelativeToSelf().TopLeft());

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  if (!mIsRootContent) {
    aDesiredSize.mOverflowAreas.VisualOverflow().UnionRect(
      aDesiredSize.mOverflowAreas.VisualOverflow(),
      anonKid->GetVisualOverflowRect() + anonKid->GetPosition());
  }
  FinishAndStoreOverflow(&aDesiredSize);

  NS_FRAME_TRACE(NS_FRAME_TRACE_CALLS,
                  ("exit nsSVGOuterSVGFrame::Reflow: size=%d,%d",
                  aDesiredSize.Width(), aDesiredSize.Height()));
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

void
nsSVGOuterSVGFrame::DidReflow(nsPresContext*   aPresContext,
                              const nsHTMLReflowState*  aReflowState,
                              nsDidReflowStatus aStatus)
{
  nsSVGOuterSVGFrameBase::DidReflow(aPresContext,aReflowState,aStatus);

  
  
  PresContext()->PresShell()->SynthesizeMouseMove(false, false);
}

 bool
nsSVGOuterSVGFrame::UpdateOverflow()
{
  

  

  nsRect rect(nsPoint(0, 0), GetSize());
  nsOverflowAreas overflowAreas(rect, rect);

  if (!mIsRootContent) {
    nsIFrame *anonKid = GetFirstPrincipalChild();
    overflowAreas.VisualOverflow().UnionRect(
      overflowAreas.VisualOverflow(),
      anonKid->GetVisualOverflowRect() + anonKid->GetPosition());
  }

  return FinishAndStoreOverflow(overflowAreas, GetSize());
}








class nsDisplayOuterSVG : public nsDisplayItem {
public:
  nsDisplayOuterSVG(nsDisplayListBuilder* aBuilder,
                    nsSVGOuterSVGFrame* aFrame) :
    nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayOuterSVG);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayOuterSVG() {
    MOZ_COUNT_DTOR(nsDisplayOuterSVG);
  }
#endif

  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState,
                       nsTArray<nsIFrame*> *aOutFrames) override;
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) override;

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion* aInvalidRegion) override;

  NS_DISPLAY_DECL_NAME("SVGOuterSVG", TYPE_SVG_OUTER_SVG)
};

void
nsDisplayOuterSVG::HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                           HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames)
{
  nsSVGOuterSVGFrame *outerSVGFrame = static_cast<nsSVGOuterSVGFrame*>(mFrame);

  nsPoint refFrameToContentBox =
    ToReferenceFrame() + outerSVGFrame->GetContentRectRelativeToSelf().TopLeft();

  nsPoint pointRelativeToContentBox =
    nsPoint(aRect.x + aRect.width / 2, aRect.y + aRect.height / 2) -
      refFrameToContentBox;

  gfxPoint svgViewportRelativePoint =
    gfxPoint(pointRelativeToContentBox.x, pointRelativeToContentBox.y) /
      outerSVGFrame->PresContext()->AppUnitsPerCSSPixel();

  nsSVGOuterSVGAnonChildFrame *anonKid =
    static_cast<nsSVGOuterSVGAnonChildFrame*>(
      outerSVGFrame->GetFirstPrincipalChild());

  nsIFrame* frame =
    nsSVGUtils::HitTestChildren(anonKid, svgViewportRelativePoint);
  if (frame) {
    aOutFrames->AppendElement(frame);
  }
}

void
nsDisplayOuterSVG::Paint(nsDisplayListBuilder* aBuilder,
                         nsRenderingContext* aContext)
{
#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime start = PR_Now();
#endif

  
  SVGAutoRenderState state(aContext->GetDrawTarget());

  if (aBuilder->IsPaintingToWindow()) {
    state.SetPaintingToWindow(true);
  }

  nsRect viewportRect =
    mFrame->GetContentRectRelativeToSelf() + ToReferenceFrame();

  nsRect clipRect = mVisibleRect.Intersect(viewportRect);

  uint32_t appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();

  nsIntRect contentAreaDirtyRect =
    (clipRect - viewportRect.TopLeft()).
      ToOutsidePixels(appUnitsPerDevPixel);

  gfxPoint devPixelOffset =
    nsLayoutUtils::PointToGfxPoint(viewportRect.TopLeft(), appUnitsPerDevPixel);

  aContext->ThebesContext()->Save();
  
  
  gfxMatrix tm = nsSVGIntegrationUtils::GetCSSPxToDevPxMatrix(mFrame) *
                   gfxMatrix::Translation(devPixelOffset);
  nsSVGUtils::PaintFrameWithEffects(mFrame, *aContext->ThebesContext(), tm, &contentAreaDirtyRect);
  aContext->ThebesContext()->Restore();

#if defined(DEBUG) && defined(SVG_DEBUG_PAINT_TIMING)
  PRTime end = PR_Now();
  printf("SVG Paint Timing: %f ms\n", (end-start)/1000.0);
#endif
}

static PLDHashOperator CheckForeignObjectInvalidatedArea(nsPtrHashKey<nsSVGForeignObjectFrame>* aEntry, void* aData)
{
  nsRegion* region = static_cast<nsRegion*>(aData);
  region->Or(*region, aEntry->GetKey()->GetInvalidRegion());
  return PL_DHASH_NEXT;
}

nsRegion
nsSVGOuterSVGFrame::FindInvalidatedForeignObjectFrameChildren(nsIFrame* aFrame)
{
  nsRegion result;
  if (mForeignObjectHash && mForeignObjectHash->Count()) {
    mForeignObjectHash->EnumerateEntries(CheckForeignObjectInvalidatedArea, &result);
  }
  return result;
}

void
nsDisplayOuterSVG::ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                             const nsDisplayItemGeometry* aGeometry,
                                             nsRegion* aInvalidRegion)
{
  nsSVGOuterSVGFrame *frame = static_cast<nsSVGOuterSVGFrame*>(mFrame);
  frame->InvalidateSVG(frame->FindInvalidatedForeignObjectFrameChildren(frame));

  nsRegion result = frame->GetInvalidRegion();
  result.MoveBy(ToReferenceFrame());
  frame->ClearInvalidRegion();

  nsDisplayItem::ComputeInvalidationRegion(aBuilder, aGeometry, aInvalidRegion);
  aInvalidRegion->Or(*aInvalidRegion, result);
}

nsresult
nsSVGOuterSVGFrame::AttributeChanged(int32_t  aNameSpaceID,
                                     nsIAtom* aAttribute,
                                     int32_t  aModType)
{
  if (aNameSpaceID == kNameSpaceID_None &&
      !(GetStateBits() & (NS_FRAME_FIRST_REFLOW | NS_FRAME_IS_NONDISPLAY))) {
    if (aAttribute == nsGkAtoms::viewBox ||
        aAttribute == nsGkAtoms::preserveAspectRatio ||
        aAttribute == nsGkAtoms::transform) {

      
      mCanvasTM = nullptr;

      nsSVGUtils::NotifyChildrenOfSVGChange(GetFirstPrincipalChild(),
                aAttribute == nsGkAtoms::viewBox ?
                  TRANSFORM_CHANGED | COORD_CONTEXT_CHANGED : TRANSFORM_CHANGED);

      if (aAttribute != nsGkAtoms::transform) {
        static_cast<SVGSVGElement*>(mContent)->ChildrenOnlyTransformChanged();
      }

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




void
nsSVGOuterSVGFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  if (GetStateBits() & NS_FRAME_IS_NONDISPLAY) {
    return;
  }

  DisplayBorderBackgroundOutline(aBuilder, aLists);

  
  
  DisplayListClipState::AutoSaveRestore autoSR(aBuilder);
  if (mIsRootContent ||
      StyleDisplay()->IsScrollableOverflow()) {
    autoSR.ClipContainingBlockDescendantsToContentBox(aBuilder, this);
  }

  if ((aBuilder->IsForEventDelivery() &&
       NS_SVGDisplayListHitTestingEnabled()) ||
      (!aBuilder->IsForEventDelivery() &&
       NS_SVGDisplayListPaintingEnabled())) {
    nsDisplayList *contentList = aLists.Content();
    nsDisplayListSet set(contentList, contentList, contentList,
                         contentList, contentList, contentList);
    BuildDisplayListForNonBlockChildren(aBuilder, aDirtyRect, set);
  } else {
    aLists.Content()->AppendNewToTop(
      new (aBuilder) nsDisplayOuterSVG(aBuilder, this));
  }
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
nsSVGOuterSVGFrame::NotifyViewportOrTransformChanged(uint32_t aFlags)
{
  MOZ_ASSERT(aFlags &&
             !(aFlags & ~(COORD_CONTEXT_CHANGED | TRANSFORM_CHANGED |
                          FULL_ZOOM_CHANGED)),
             "Unexpected aFlags value");

  
  if (!mViewportInitialized) {
    return;
  }

  SVGSVGElement *content = static_cast<SVGSVGElement*>(mContent);

  if (aFlags & COORD_CONTEXT_CHANGED) {
    if (content->HasViewBoxRect()) {
      
      
      
      aFlags = TRANSFORM_CHANGED;
    }
    else if (content->ShouldSynthesizeViewBox()) {
      
      
      
      aFlags |= TRANSFORM_CHANGED;
    }
    else if (mCanvasTM && mCanvasTM->IsSingular()) {
      
      
      
      
      aFlags |= TRANSFORM_CHANGED;
    }
  }

  bool haveNonFulLZoomTransformChange = (aFlags & TRANSFORM_CHANGED);

  if (aFlags & FULL_ZOOM_CHANGED) {
    
    aFlags = (aFlags & ~FULL_ZOOM_CHANGED) | TRANSFORM_CHANGED;
  }

  if (aFlags & TRANSFORM_CHANGED) {
    
    mCanvasTM = nullptr;

    if (haveNonFulLZoomTransformChange &&
        !(mState & NS_FRAME_IS_NONDISPLAY)) {
      uint32_t flags = (mState & NS_FRAME_IN_REFLOW) ?
                         SVGSVGElement::eDuringReflow : 0;
      content->ChildrenOnlyTransformChanged(flags);
    }
  }

  nsSVGUtils::NotifyChildrenOfSVGChange(GetFirstPrincipalChild(), aFlags);
}




nsresult
nsSVGOuterSVGFrame::PaintSVG(gfxContext& aContext,
                             const gfxMatrix& aTransform,
                             const nsIntRect* aDirtyRect)
{
  NS_ASSERTION(GetFirstPrincipalChild()->GetType() ==
                 nsGkAtoms::svgOuterSVGAnonChildFrame &&
               !GetFirstPrincipalChild()->GetNextSibling(),
               "We should have a single, anonymous, child");
  nsSVGOuterSVGAnonChildFrame *anonKid =
    static_cast<nsSVGOuterSVGAnonChildFrame*>(GetFirstPrincipalChild());
  return anonKid->PaintSVG(aContext, aTransform, aDirtyRect);
}

SVGBBox
nsSVGOuterSVGFrame::GetBBoxContribution(const gfx::Matrix &aToBBoxUserspace,
                                        uint32_t aFlags)
{
  NS_ASSERTION(GetFirstPrincipalChild()->GetType() ==
                 nsGkAtoms::svgOuterSVGAnonChildFrame &&
               !GetFirstPrincipalChild()->GetNextSibling(),
               "We should have a single, anonymous, child");
  
  
  nsSVGOuterSVGAnonChildFrame *anonKid =
    static_cast<nsSVGOuterSVGAnonChildFrame*>(GetFirstPrincipalChild());
  return anonKid->GetBBoxContribution(aToBBoxUserspace, aFlags);
}




gfxMatrix
nsSVGOuterSVGFrame::GetCanvasTM()
{
  if (!mCanvasTM) {
    SVGSVGElement *content = static_cast<SVGSVGElement*>(mContent);

    float devPxPerCSSPx =
      1.0f / PresContext()->AppUnitsToFloatCSSPixels(
                                PresContext()->AppUnitsPerDevPixel());

    gfxMatrix tm = content->PrependLocalTransformsTo(
                     gfxMatrix::Scaling(devPxPerCSSPx, devPxPerCSSPx));
    mCanvasTM = new gfxMatrix(tm);
  }
  return *mCanvasTM;
}




bool
nsSVGOuterSVGFrame::IsRootOfReplacedElementSubDoc(nsIFrame **aEmbeddingFrame)
{
  if (!mContent->GetParent()) {
    
    nsCOMPtr<nsIDocShell> docShell = PresContext()->GetDocShell();
    nsCOMPtr<nsIDOMWindow> window;
    if (docShell) {
      window = docShell->GetWindow();
    }

    if (window) {
      nsCOMPtr<nsIDOMElement> frameElement;
      window->GetFrameElement(getter_AddRefs(frameElement));
      nsCOMPtr<nsIObjectLoadingContent> olc = do_QueryInterface(frameElement);
      if (olc) {
        
        if (aEmbeddingFrame) {
          nsCOMPtr<nsIContent> element = do_QueryInterface(frameElement);
          *aEmbeddingFrame = element->GetPrimaryFrame();
          NS_ASSERTION(*aEmbeddingFrame, "Yikes, no embedding frame!");
        }
        return true;
      }
    }
  }
  if (aEmbeddingFrame) {
    *aEmbeddingFrame = nullptr;
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
  nsSVGLength2 &height = static_cast<SVGSVGElement*>(mContent)->
                           mLengthAttributes[SVGSVGElement::ATTR_HEIGHT];
  return height.IsPercentage() && height.GetBaseValInSpecifiedUnits() <= 100;
}





nsContainerFrame*
NS_NewSVGOuterSVGAnonChildFrame(nsIPresShell* aPresShell,
                                nsStyleContext* aContext)
{
  return new (aPresShell) nsSVGOuterSVGAnonChildFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsSVGOuterSVGAnonChildFrame)

#ifdef DEBUG
void
nsSVGOuterSVGAnonChildFrame::Init(nsIContent*       aContent,
                                  nsContainerFrame* aParent,
                                  nsIFrame*         aPrevInFlow)
{
  MOZ_ASSERT(aParent->GetType() == nsGkAtoms::svgOuterSVGFrame,
             "Unexpected parent");
  nsSVGOuterSVGAnonChildFrameBase::Init(aContent, aParent, aPrevInFlow);
}
#endif

nsIAtom *
nsSVGOuterSVGAnonChildFrame::GetType() const
{
  return nsGkAtoms::svgOuterSVGAnonChildFrame;
}

bool
nsSVGOuterSVGAnonChildFrame::HasChildrenOnlyTransform(gfx::Matrix *aTransform) const
{
  
  

  SVGSVGElement *content = static_cast<SVGSVGElement*>(mContent);

  bool hasTransform = content->HasChildrenOnlyTransform();

  if (hasTransform && aTransform) {
    
    gfxMatrix identity;
    *aTransform = gfx::ToMatrix(
      content->PrependLocalTransformsTo(identity,
                                        nsSVGElement::eChildToUserSpace));
  }

  return hasTransform;
}
