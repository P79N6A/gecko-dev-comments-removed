




#include "nsRangeFrame.h"

#include "mozilla/EventStates.h"
#include "mozilla/TouchEvents.h"

#include "nsContentCreatorFunctions.h"
#include "nsContentList.h"
#include "nsContentUtils.h"
#include "nsCSSRendering.h"
#include "nsFormControlFrame.h"
#include "nsIContent.h"
#include "nsIDocument.h"
#include "nsNameSpaceManager.h"
#include "nsIPresShell.h"
#include "nsGkAtoms.h"
#include "mozilla/dom/HTMLInputElement.h"
#include "nsPresContext.h"
#include "nsNodeInfoManager.h"
#include "nsRenderingContext.h"
#include "mozilla/dom/Element.h"
#include "nsStyleSet.h"
#include "nsThemeConstants.h"

#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif

#define LONG_SIDE_TO_SHORT_SIDE_RATIO 10

using namespace mozilla;
using namespace mozilla::dom;

NS_IMPL_ISUPPORTS(nsRangeFrame::DummyTouchListener, nsIDOMEventListener)

nsIFrame*
NS_NewRangeFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsRangeFrame(aContext);
}

nsRangeFrame::nsRangeFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
{
}

nsRangeFrame::~nsRangeFrame()
{
#ifdef DEBUG
  if (mOuterFocusStyle) {
    mOuterFocusStyle->FrameRelease();
  }
#endif
}

NS_IMPL_FRAMEARENA_HELPERS(nsRangeFrame)

NS_QUERYFRAME_HEAD(nsRangeFrame)
  NS_QUERYFRAME_ENTRY(nsRangeFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

void
nsRangeFrame::Init(nsIContent*       aContent,
                   nsContainerFrame* aParent,
                   nsIFrame*         aPrevInFlow)
{
  
  
  
  
  
  
  
  if (!mDummyTouchListener) {
    mDummyTouchListener = new DummyTouchListener();
  }
  aContent->AddEventListener(NS_LITERAL_STRING("touchstart"), mDummyTouchListener, false);

  nsStyleSet *styleSet = PresContext()->StyleSet();

  mOuterFocusStyle =
    styleSet->ProbePseudoElementStyle(aContent->AsElement(),
                                      nsCSSPseudoElements::ePseudo_mozFocusOuter,
                                      StyleContext());

  return nsContainerFrame::Init(aContent, aParent, aPrevInFlow);
}

void
nsRangeFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  NS_ASSERTION(!GetPrevContinuation() && !GetNextContinuation(),
               "nsRangeFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first.");

  mContent->RemoveEventListener(NS_LITERAL_STRING("touchstart"), mDummyTouchListener, false);

  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsContentUtils::DestroyAnonymousContent(&mTrackDiv);
  nsContentUtils::DestroyAnonymousContent(&mProgressDiv);
  nsContentUtils::DestroyAnonymousContent(&mThumbDiv);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsRangeFrame::MakeAnonymousDiv(Element** aResult,
                               nsCSSPseudoElements::Type aPseudoType,
                               nsTArray<ContentInfo>& aElements)
{
  nsCOMPtr<nsIDocument> doc = mContent->GetComposedDoc();
  nsRefPtr<Element> resultElement = doc->CreateHTMLElement(nsGkAtoms::div);

  
  nsRefPtr<nsStyleContext> newStyleContext =
    PresContext()->StyleSet()->ResolvePseudoElementStyle(mContent->AsElement(),
                                                         aPseudoType,
                                                         StyleContext(),
                                                         resultElement);

  if (!aElements.AppendElement(ContentInfo(resultElement, newStyleContext))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  resultElement.forget(aResult);
  return NS_OK;
}

nsresult
nsRangeFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsresult rv;

  
  rv = MakeAnonymousDiv(getter_AddRefs(mTrackDiv),
                        nsCSSPseudoElements::ePseudo_mozRangeTrack,
                        aElements);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = MakeAnonymousDiv(getter_AddRefs(mProgressDiv),
                        nsCSSPseudoElements::ePseudo_mozRangeProgress,
                        aElements);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = MakeAnonymousDiv(getter_AddRefs(mThumbDiv),
                        nsCSSPseudoElements::ePseudo_mozRangeThumb,
                        aElements);
  return rv;
}

void
nsRangeFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                       uint32_t aFilter)
{
  if (mTrackDiv) {
    aElements.AppendElement(mTrackDiv);
  }

  if (mProgressDiv) {
    aElements.AppendElement(mProgressDiv);
  }

  if (mThumbDiv) {
    aElements.AppendElement(mThumbDiv);
  }
}

class nsDisplayRangeFocusRing : public nsDisplayItem
{
public:
  nsDisplayRangeFocusRing(nsDisplayListBuilder* aBuilder, nsIFrame* aFrame)
    : nsDisplayItem(aBuilder, aFrame) {
    MOZ_COUNT_CTOR(nsDisplayRangeFocusRing);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayRangeFocusRing() {
    MOZ_COUNT_DTOR(nsDisplayRangeFocusRing);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) override;
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsRenderingContext* aCtx) override;
  NS_DISPLAY_DECL_NAME("RangeFocusRing", TYPE_OUTLINE)
};

nsRect
nsDisplayRangeFocusRing::GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap)
{
  *aSnap = false;
  nsRect rect(ToReferenceFrame(), Frame()->GetSize());

  
  
  nsStyleContext* styleContext =
    static_cast<nsRangeFrame*>(mFrame)->mOuterFocusStyle;
  MOZ_ASSERT(styleContext, "We only exist if mOuterFocusStyle is non-null");
  rect.Inflate(styleContext->StyleBorder()->GetComputedBorder());

  return rect;
}

void
nsDisplayRangeFocusRing::Paint(nsDisplayListBuilder* aBuilder,
                               nsRenderingContext* aCtx)
{
  bool unused;
  nsStyleContext* styleContext =
    static_cast<nsRangeFrame*>(mFrame)->mOuterFocusStyle;
  MOZ_ASSERT(styleContext, "We only exist if mOuterFocusStyle is non-null");
  nsCSSRendering::PaintBorder(mFrame->PresContext(), *aCtx, mFrame,
                              mVisibleRect, GetBounds(aBuilder, &unused),
                              styleContext);
}

void
nsRangeFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                               const nsRect&           aDirtyRect,
                               const nsDisplayListSet& aLists)
{
  if (IsThemed()) {
    DisplayBorderBackgroundOutline(aBuilder, aLists);
    
    
    
    
    
    
    
    nsIFrame* thumb = mThumbDiv->GetPrimaryFrame();
    if (thumb) {
      nsDisplayListSet set(aLists, aLists.Content());
      BuildDisplayListForChild(aBuilder, thumb, aDirtyRect, set, DISPLAY_CHILD_INLINE);
    }
  } else {
    BuildDisplayListForInline(aBuilder, aDirtyRect, aLists);
  }

  

  if (!aBuilder->IsForPainting() ||
      !IsVisibleForPainting(aBuilder)) {
    
    
    return;
  }

  EventStates eventStates = mContent->AsElement()->State();
  if (eventStates.HasState(NS_EVENT_STATE_DISABLED) ||
      !eventStates.HasState(NS_EVENT_STATE_FOCUSRING)) {
    return; 
  }

  if (!mOuterFocusStyle ||
      !mOuterFocusStyle->StyleBorder()->HasBorder()) {
    
    
    return;
  }

  const nsStyleDisplay *disp = StyleDisplay();
  if (IsThemed(disp) &&
      PresContext()->GetTheme()->ThemeDrawsFocusForWidget(disp->mAppearance)) {
    return; 
  }

  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplayRangeFocusRing(aBuilder, this));
}

void
nsRangeFrame::Reflow(nsPresContext*           aPresContext,
                     nsHTMLReflowMetrics&     aDesiredSize,
                     const nsHTMLReflowState& aReflowState,
                     nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsRangeFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_ASSERTION(mTrackDiv, "::-moz-range-track div must exist!");
  NS_ASSERTION(mProgressDiv, "::-moz-range-progress div must exist!");
  NS_ASSERTION(mThumbDiv, "::-moz-range-thumb div must exist!");
  NS_ASSERTION(!GetPrevContinuation() && !GetNextContinuation(),
               "nsRangeFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first.");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, true);
  }

  WritingMode wm = aReflowState.GetWritingMode();
  nscoord computedBSize = aReflowState.ComputedBSize();
  if (computedBSize == NS_AUTOHEIGHT) {
    computedBSize = 0;
  }
  LogicalSize
    finalSize(wm,
              aReflowState.ComputedISize() +
              aReflowState.ComputedLogicalBorderPadding().IStartEnd(wm),
              computedBSize +
              aReflowState.ComputedLogicalBorderPadding().BStartEnd(wm));
  aDesiredSize.SetSize(wm, finalSize);

  ReflowAnonymousContent(aPresContext, aDesiredSize, aReflowState);

  aDesiredSize.SetOverflowAreasToDesiredBounds();

  nsIFrame* trackFrame = mTrackDiv->GetPrimaryFrame();
  if (trackFrame) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, trackFrame);
  }

  nsIFrame* rangeProgressFrame = mProgressDiv->GetPrimaryFrame();
  if (rangeProgressFrame) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, rangeProgressFrame);
  }

  nsIFrame* thumbFrame = mThumbDiv->GetPrimaryFrame();
  if (thumbFrame) {
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, thumbFrame);
  }

  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

void
nsRangeFrame::ReflowAnonymousContent(nsPresContext*           aPresContext,
                                     nsHTMLReflowMetrics&     aDesiredSize,
                                     const nsHTMLReflowState& aReflowState)
{
  
  
  nscoord rangeFrameContentBoxWidth = aReflowState.ComputedWidth();
  nscoord rangeFrameContentBoxHeight = aReflowState.ComputedHeight();
  if (rangeFrameContentBoxHeight == NS_AUTOHEIGHT) {
    rangeFrameContentBoxHeight = 0;
  }

  nsIFrame* trackFrame = mTrackDiv->GetPrimaryFrame();

  if (trackFrame) { 

    
    
    
    
    
    

    WritingMode wm = trackFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState trackReflowState(aPresContext, aReflowState,
                                       trackFrame, availSize);

    
    
    
    nscoord trackX = rangeFrameContentBoxWidth / 2;
    nscoord trackY = rangeFrameContentBoxHeight / 2;

    
    trackX -= trackReflowState.ComputedPhysicalBorderPadding().left +
                trackReflowState.ComputedWidth() / 2;
    trackY -= trackReflowState.ComputedPhysicalBorderPadding().top +
                trackReflowState.ComputedHeight() / 2;

    
    trackX += aReflowState.ComputedPhysicalBorderPadding().left;
    trackY += aReflowState.ComputedPhysicalBorderPadding().top;

    nsReflowStatus frameStatus;
    nsHTMLReflowMetrics trackDesiredSize(aReflowState);
    ReflowChild(trackFrame, aPresContext, trackDesiredSize,
                trackReflowState, trackX, trackY, 0, frameStatus);
    MOZ_ASSERT(NS_FRAME_IS_FULLY_COMPLETE(frameStatus),
               "We gave our child unconstrained height, so it should be complete");
    FinishReflowChild(trackFrame, aPresContext, trackDesiredSize,
                      &trackReflowState, trackX, trackY, 0);
  }

  nsIFrame* thumbFrame = mThumbDiv->GetPrimaryFrame();

  if (thumbFrame) { 
    WritingMode wm = thumbFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState thumbReflowState(aPresContext, aReflowState,
                                       thumbFrame, availSize);

    
    

    nsReflowStatus frameStatus;
    nsHTMLReflowMetrics thumbDesiredSize(aReflowState);
    ReflowChild(thumbFrame, aPresContext, thumbDesiredSize,
                thumbReflowState, 0, 0, 0, frameStatus);
    MOZ_ASSERT(NS_FRAME_IS_FULLY_COMPLETE(frameStatus),
               "We gave our child unconstrained height, so it should be complete");
    FinishReflowChild(thumbFrame, aPresContext, thumbDesiredSize,
                      &thumbReflowState, 0, 0, 0);
    DoUpdateThumbPosition(thumbFrame, nsSize(aDesiredSize.Width(),
                                             aDesiredSize.Height()));
  }

  nsIFrame* rangeProgressFrame = mProgressDiv->GetPrimaryFrame();

  if (rangeProgressFrame) { 
    WritingMode wm = rangeProgressFrame->GetWritingMode();
    LogicalSize availSize = aReflowState.ComputedSize(wm);
    availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
    nsHTMLReflowState progressReflowState(aPresContext, aReflowState,
                                          rangeProgressFrame, availSize);

    
    
    

    nsReflowStatus frameStatus;
    nsHTMLReflowMetrics progressDesiredSize(aReflowState);
    ReflowChild(rangeProgressFrame, aPresContext,
                progressDesiredSize, progressReflowState, 0, 0,
                0, frameStatus);
    MOZ_ASSERT(NS_FRAME_IS_FULLY_COMPLETE(frameStatus),
               "We gave our child unconstrained height, so it should be complete");
    FinishReflowChild(rangeProgressFrame, aPresContext,
                      progressDesiredSize, &progressReflowState, 0, 0, 0);
    DoUpdateRangeProgressFrame(rangeProgressFrame, nsSize(aDesiredSize.Width(),
                                                          aDesiredSize.Height()));
  }
}

#ifdef ACCESSIBILITY
a11y::AccType
nsRangeFrame::AccessibleType()
{
  return a11y::eHTMLRangeType;
}
#endif

double
nsRangeFrame::GetValueAsFractionOfRange()
{
  MOZ_ASSERT(mContent->IsHTMLElement(nsGkAtoms::input), "bad cast");
  dom::HTMLInputElement* input = static_cast<dom::HTMLInputElement*>(mContent);

  MOZ_ASSERT(input->GetType() == NS_FORM_INPUT_RANGE);

  Decimal value = input->GetValueAsDecimal();
  Decimal minimum = input->GetMinimum();
  Decimal maximum = input->GetMaximum();

  MOZ_ASSERT(value.isFinite() && minimum.isFinite() && maximum.isFinite(),
             "type=range should have a default maximum/minimum");
  
  if (maximum <= minimum) {
    MOZ_ASSERT(value == minimum, "Unsanitized value");
    return 0.0;
  }
  
  MOZ_ASSERT(value >= minimum && value <= maximum, "Unsanitized value");
  
  return ((value - minimum) / (maximum - minimum)).toDouble();
}

Decimal
nsRangeFrame::GetValueAtEventPoint(WidgetGUIEvent* aEvent)
{
  MOZ_ASSERT(aEvent->mClass == eMouseEventClass ||
             aEvent->mClass == eTouchEventClass,
             "Unexpected event type - aEvent->refPoint may be meaningless");

  MOZ_ASSERT(mContent->IsHTMLElement(nsGkAtoms::input), "bad cast");
  dom::HTMLInputElement* input = static_cast<dom::HTMLInputElement*>(mContent);

  MOZ_ASSERT(input->GetType() == NS_FORM_INPUT_RANGE);

  Decimal minimum = input->GetMinimum();
  Decimal maximum = input->GetMaximum();
  MOZ_ASSERT(minimum.isFinite() && maximum.isFinite(),
             "type=range should have a default maximum/minimum");
  if (maximum <= minimum) {
    return minimum;
  }
  Decimal range = maximum - minimum;

  LayoutDeviceIntPoint absPoint;
  if (aEvent->mClass == eTouchEventClass) {
    MOZ_ASSERT(aEvent->AsTouchEvent()->touches.Length() == 1,
               "Unexpected number of touches");
    absPoint = aEvent->AsTouchEvent()->touches[0]->mRefPoint;
  } else {
    absPoint = aEvent->refPoint;
  }
  nsPoint point =
    nsLayoutUtils::GetEventCoordinatesRelativeTo(aEvent, absPoint, this);

  if (point == nsPoint(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE)) {
    
    return static_cast<dom::HTMLInputElement*>(mContent)->GetValueAsDecimal();
  }

  nsRect rangeContentRect = GetContentRectRelativeToSelf();
  nsSize thumbSize;

  if (IsThemed()) {
    
    nsPresContext *presContext = PresContext();
    bool notUsedCanOverride;
    LayoutDeviceIntSize size;
    presContext->GetTheme()->
      GetMinimumWidgetSize(presContext, this, NS_THEME_RANGE_THUMB, &size,
                           &notUsedCanOverride);
    thumbSize.width = presContext->DevPixelsToAppUnits(size.width);
    thumbSize.height = presContext->DevPixelsToAppUnits(size.height);
    MOZ_ASSERT(thumbSize.width > 0 && thumbSize.height > 0);
  } else {
    nsIFrame* thumbFrame = mThumbDiv->GetPrimaryFrame();
    if (thumbFrame) { 
      thumbSize = thumbFrame->GetSize();
    }
  }

  Decimal fraction;
  if (IsHorizontal()) {
    nscoord traversableDistance = rangeContentRect.width - thumbSize.width;
    if (traversableDistance <= 0) {
      return minimum;
    }
    nscoord posAtStart = rangeContentRect.x + thumbSize.width/2;
    nscoord posAtEnd = posAtStart + traversableDistance;
    nscoord posOfPoint = mozilla::clamped(point.x, posAtStart, posAtEnd);
    fraction = Decimal(posOfPoint - posAtStart) / Decimal(traversableDistance);
    if (IsRightToLeft()) {
      fraction = Decimal(1) - fraction;
    }
  } else {
    nscoord traversableDistance = rangeContentRect.height - thumbSize.height;
    if (traversableDistance <= 0) {
      return minimum;
    }
    nscoord posAtStart = rangeContentRect.y + thumbSize.height/2;
    nscoord posAtEnd = posAtStart + traversableDistance;
    nscoord posOfPoint = mozilla::clamped(point.y, posAtStart, posAtEnd);
    
    
    fraction = Decimal(1) - Decimal(posOfPoint - posAtStart) / Decimal(traversableDistance);
  }

  MOZ_ASSERT(fraction >= Decimal(0) && fraction <= Decimal(1));
  return minimum + fraction * range;
}

void
nsRangeFrame::UpdateForValueChange()
{
  if (NS_SUBTREE_DIRTY(this)) {
    return; 
  }
  nsIFrame* rangeProgressFrame = mProgressDiv->GetPrimaryFrame();
  nsIFrame* thumbFrame = mThumbDiv->GetPrimaryFrame();
  if (!rangeProgressFrame && !thumbFrame) {
    return; 
  }
  if (rangeProgressFrame) {
    DoUpdateRangeProgressFrame(rangeProgressFrame, GetSize());
  }
  if (thumbFrame) {
    DoUpdateThumbPosition(thumbFrame, GetSize());
  }
  if (IsThemed()) {
    
    
    InvalidateFrame();
  }

#ifdef ACCESSIBILITY
  nsAccessibilityService* accService = nsIPresShell::AccService();
  if (accService) {
    accService->RangeValueChanged(PresContext()->PresShell(), mContent);
  }
#endif

  SchedulePaint();
}

void
nsRangeFrame::DoUpdateThumbPosition(nsIFrame* aThumbFrame,
                                    const nsSize& aRangeSize)
{
  MOZ_ASSERT(aThumbFrame);

  
  
  
  
  
  
  

  nsMargin borderAndPadding = GetUsedBorderAndPadding();
  nsPoint newPosition(borderAndPadding.left, borderAndPadding.top);

  nsSize rangeContentBoxSize(aRangeSize);
  rangeContentBoxSize.width -= borderAndPadding.LeftRight();
  rangeContentBoxSize.height -= borderAndPadding.TopBottom();

  nsSize thumbSize = aThumbFrame->GetSize();
  double fraction = GetValueAsFractionOfRange();
  MOZ_ASSERT(fraction >= 0.0 && fraction <= 1.0);

  if (IsHorizontal()) {
    if (thumbSize.width < rangeContentBoxSize.width) {
      nscoord traversableDistance =
        rangeContentBoxSize.width - thumbSize.width;
      if (IsRightToLeft()) {
        newPosition.x += NSToCoordRound((1.0 - fraction) * traversableDistance);
      } else {
        newPosition.x += NSToCoordRound(fraction * traversableDistance);
      }
      newPosition.y += (rangeContentBoxSize.height - thumbSize.height)/2;
    }
  } else {
    if (thumbSize.height < rangeContentBoxSize.height) {
      nscoord traversableDistance =
        rangeContentBoxSize.height - thumbSize.height;
      newPosition.x += (rangeContentBoxSize.width - thumbSize.width)/2;
      newPosition.y += NSToCoordRound((1.0 - fraction) * traversableDistance);
    }
  }
  aThumbFrame->SetPosition(newPosition);
}

void
nsRangeFrame::DoUpdateRangeProgressFrame(nsIFrame* aRangeProgressFrame,
                                         const nsSize& aRangeSize)
{
  MOZ_ASSERT(aRangeProgressFrame);

  
  
  
  
  
  
  
  

  nsMargin borderAndPadding = GetUsedBorderAndPadding();
  nsSize progSize = aRangeProgressFrame->GetSize();
  nsRect progRect(borderAndPadding.left, borderAndPadding.top,
                  progSize.width, progSize.height);

  nsSize rangeContentBoxSize(aRangeSize);
  rangeContentBoxSize.width -= borderAndPadding.LeftRight();
  rangeContentBoxSize.height -= borderAndPadding.TopBottom();

  double fraction = GetValueAsFractionOfRange();
  MOZ_ASSERT(fraction >= 0.0 && fraction <= 1.0);

  if (IsHorizontal()) {
    nscoord progLength = NSToCoordRound(fraction * rangeContentBoxSize.width);
    if (IsRightToLeft()) {
      progRect.x += rangeContentBoxSize.width - progLength;
    }
    progRect.y += (rangeContentBoxSize.height - progSize.height)/2;
    progRect.width = progLength;
  } else {
    nscoord progLength = NSToCoordRound(fraction * rangeContentBoxSize.height);
    progRect.x += (rangeContentBoxSize.width - progSize.width)/2;
    progRect.y += rangeContentBoxSize.height - progLength;
    progRect.height = progLength;
  }
  aRangeProgressFrame->SetRect(progRect);
}

nsresult
nsRangeFrame::AttributeChanged(int32_t  aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t  aModType)
{
  NS_ASSERTION(mTrackDiv, "The track div must exist!");
  NS_ASSERTION(mThumbDiv, "The thumb div must exist!");

  if (aNameSpaceID == kNameSpaceID_None) {
    if (aAttribute == nsGkAtoms::value ||
        aAttribute == nsGkAtoms::min ||
        aAttribute == nsGkAtoms::max ||
        aAttribute == nsGkAtoms::step) {
      
      
      
      
      
      
      
      
      
      
      MOZ_ASSERT(mContent->IsHTMLElement(nsGkAtoms::input), "bad cast");
      bool typeIsRange = static_cast<dom::HTMLInputElement*>(mContent)->GetType() ==
                           NS_FORM_INPUT_RANGE;
      
      
      if (typeIsRange) {
        UpdateForValueChange();
      }
    } else if (aAttribute == nsGkAtoms::orient) {
      PresContext()->PresShell()->FrameNeedsReflow(this, nsIPresShell::eResize,
                                                   NS_FRAME_IS_DIRTY);
    }
  }

  return nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

LogicalSize
nsRangeFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                              WritingMode aWM,
                              const LogicalSize& aCBSize,
                              nscoord aAvailableISize,
                              const LogicalSize& aMargin,
                              const LogicalSize& aBorder,
                              const LogicalSize& aPadding,
                              bool aShrinkWrap)
{
  nscoord oneEm = NSToCoordRound(StyleFont()->mFont.size *
                                 nsLayoutUtils::FontSizeInflationFor(this)); 

  bool isInlineOriented = IsInlineOriented();

  const WritingMode wm = GetWritingMode();
  LogicalSize autoSize(wm);

  
  
  
  
  

  if (isInlineOriented) {
    autoSize.ISize(wm) = LONG_SIDE_TO_SHORT_SIDE_RATIO * oneEm;
    autoSize.BSize(wm) = IsThemed() ? 0 : oneEm;
  } else {
    autoSize.ISize(wm) = IsThemed() ? 0 : oneEm;
    autoSize.BSize(wm) = LONG_SIDE_TO_SHORT_SIDE_RATIO * oneEm;
  }

  return autoSize.ConvertTo(aWM, wm);
}

nscoord
nsRangeFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  
  
  
  return nscoord(0);
}

nscoord
nsRangeFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  bool isInline = IsInlineOriented();

  if (!isInline && IsThemed()) {
    
    
    
    
    return 0;
  }

  nscoord prefISize = NSToCoordRound(StyleFont()->mFont.size *
                                     nsLayoutUtils::FontSizeInflationFor(this)); 

  if (isInline) {
    prefISize *= LONG_SIDE_TO_SHORT_SIDE_RATIO;
  }

  return prefISize;
}

bool
nsRangeFrame::IsHorizontal() const
{
  dom::HTMLInputElement* element =
    static_cast<dom::HTMLInputElement*>(mContent);
  return element->AttrValueIs(kNameSpaceID_None, nsGkAtoms::orient,
                              nsGkAtoms::horizontal, eCaseMatters) ||
         (!element->AttrValueIs(kNameSpaceID_None, nsGkAtoms::orient,
                               nsGkAtoms::vertical, eCaseMatters) &&
          GetWritingMode().IsVertical() ==
            element->AttrValueIs(kNameSpaceID_None, nsGkAtoms::orient,
                                 nsGkAtoms::block, eCaseMatters));
}

double
nsRangeFrame::GetMin() const
{
  return static_cast<dom::HTMLInputElement*>(mContent)->GetMinimum().toDouble();
}

double
nsRangeFrame::GetMax() const
{
  return static_cast<dom::HTMLInputElement*>(mContent)->GetMaximum().toDouble();
}

double
nsRangeFrame::GetValue() const
{
  return static_cast<dom::HTMLInputElement*>(mContent)->GetValueAsDecimal().toDouble();
}

nsIAtom*
nsRangeFrame::GetType() const
{
  return nsGkAtoms::rangeFrame;
}

#define STYLES_DISABLING_NATIVE_THEMING \
  NS_AUTHOR_SPECIFIED_BACKGROUND | \
  NS_AUTHOR_SPECIFIED_PADDING | \
  NS_AUTHOR_SPECIFIED_BORDER

bool
nsRangeFrame::ShouldUseNativeStyle() const
{
  return (StyleDisplay()->mAppearance == NS_THEME_RANGE) &&
         !PresContext()->HasAuthorSpecifiedRules(const_cast<nsRangeFrame*>(this),
                                                 (NS_AUTHOR_SPECIFIED_BORDER |
                                                  NS_AUTHOR_SPECIFIED_BACKGROUND)) &&
         !PresContext()->HasAuthorSpecifiedRules(mTrackDiv->GetPrimaryFrame(),
                                                 STYLES_DISABLING_NATIVE_THEMING) &&
         !PresContext()->HasAuthorSpecifiedRules(mProgressDiv->GetPrimaryFrame(),
                                                 STYLES_DISABLING_NATIVE_THEMING) &&
         !PresContext()->HasAuthorSpecifiedRules(mThumbDiv->GetPrimaryFrame(),
                                                 STYLES_DISABLING_NATIVE_THEMING);
}

Element*
nsRangeFrame::GetPseudoElement(nsCSSPseudoElements::Type aType)
{
  if (aType == nsCSSPseudoElements::ePseudo_mozRangeTrack) {
    return mTrackDiv;
  }

  if (aType == nsCSSPseudoElements::ePseudo_mozRangeThumb) {
    return mThumbDiv;
  }

  if (aType == nsCSSPseudoElements::ePseudo_mozRangeProgress) {
    return mProgressDiv;
  }

  return nsContainerFrame::GetPseudoElement(aType);
}

nsStyleContext*
nsRangeFrame::GetAdditionalStyleContext(int32_t aIndex) const
{
  
  
  
  return aIndex == 0 ? mOuterFocusStyle : nullptr;
}

void
nsRangeFrame::SetAdditionalStyleContext(int32_t aIndex,
                                        nsStyleContext* aStyleContext)
{
  MOZ_ASSERT(aIndex == 0,
             "GetAdditionalStyleContext is handling other indexes?");

#ifdef DEBUG
  if (mOuterFocusStyle) {
    mOuterFocusStyle->FrameRelease();
  }
#endif

  
  mOuterFocusStyle = aStyleContext;

#ifdef DEBUG
  if (mOuterFocusStyle) {
    mOuterFocusStyle->FrameAddRef();
  }
#endif
}
