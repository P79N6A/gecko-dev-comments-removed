




#include "nsMeterFrame.h"

#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsNodeInfoManager.h"
#include "nsINodeInfo.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsFormControlFrame.h"
#include "nsFontMetrics.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLMeterElement.h"
#include "nsContentList.h"
#include "nsStyleSet.h"
#include "nsThemeConstants.h"
#include <algorithm>

using mozilla::dom::Element;
using mozilla::dom::HTMLMeterElement;

nsIFrame*
NS_NewMeterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsMeterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsMeterFrame)

nsMeterFrame::nsMeterFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
  , mBarDiv(nullptr)
{
}

nsMeterFrame::~nsMeterFrame()
{
}

void
nsMeterFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  NS_ASSERTION(!GetPrevContinuation(),
               "nsMeterFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first.");
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsContentUtils::DestroyAnonymousContent(&mBarDiv);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsMeterFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  
  mBarDiv = doc->CreateHTMLElement(nsGkAtoms::div);

  
  nsCSSPseudoElements::Type pseudoType = nsCSSPseudoElements::ePseudo_mozMeterBar;
  nsRefPtr<nsStyleContext> newStyleContext = PresContext()->StyleSet()->
    ResolvePseudoElementStyle(mContent->AsElement(), pseudoType,
                              StyleContext(), mBarDiv->AsElement());

  if (!aElements.AppendElement(ContentInfo(mBarDiv, newStyleContext))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
nsMeterFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                       uint32_t aFilter)
{
  aElements.MaybeAppendElement(mBarDiv);
}

NS_QUERYFRAME_HEAD(nsMeterFrame)
  NS_QUERYFRAME_ENTRY(nsMeterFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)


nsresult nsMeterFrame::Reflow(nsPresContext*           aPresContext,
                                   nsHTMLReflowMetrics&     aDesiredSize,
                                   const nsHTMLReflowState& aReflowState,
                                   nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsMeterFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_ASSERTION(mBarDiv, "Meter bar div must exist!");
  NS_ASSERTION(!GetPrevContinuation(),
               "nsMeterFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first.");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, true);
  }

  nsIFrame* barFrame = mBarDiv->GetPrimaryFrame();
  NS_ASSERTION(barFrame, "The meter frame should have a child with a frame!");

  ReflowBarFrame(barFrame, aPresContext, aReflowState, aStatus);

  aDesiredSize.Width() = aReflowState.ComputedWidth() +
                       aReflowState.ComputedPhysicalBorderPadding().LeftRight();
  aDesiredSize.Height() = aReflowState.ComputedHeight() +
                        aReflowState.ComputedPhysicalBorderPadding().TopBottom();

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  ConsiderChildOverflow(aDesiredSize.mOverflowAreas, barFrame);
  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  return NS_OK;
}

void
nsMeterFrame::ReflowBarFrame(nsIFrame*                aBarFrame,
                             nsPresContext*           aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  bool vertical = StyleDisplay()->mOrient == NS_STYLE_ORIENT_VERTICAL;
  nsHTMLReflowState reflowState(aPresContext, aReflowState, aBarFrame,
                                nsSize(aReflowState.ComputedWidth(),
                                       NS_UNCONSTRAINEDSIZE));
  nscoord size = vertical ? aReflowState.ComputedHeight()
                          : aReflowState.ComputedWidth();
  nscoord xoffset = aReflowState.ComputedPhysicalBorderPadding().left;
  nscoord yoffset = aReflowState.ComputedPhysicalBorderPadding().top;

  
  HTMLMeterElement* meterElement = static_cast<HTMLMeterElement*>(mContent);

  double max = meterElement->Max();
  double min = meterElement->Min();
  double value = meterElement->Value();

  double position = max - min;
  position = position != 0 ? (value - min) / position : 1;

  size = NSToCoordRound(size * position);

  if (!vertical && StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    xoffset += aReflowState.ComputedWidth() - size;
  }

  
  if (vertical) {
    
    yoffset += aReflowState.ComputedHeight() - size;

    size -= reflowState.ComputedPhysicalMargin().TopBottom() +
            reflowState.ComputedPhysicalBorderPadding().TopBottom();
    size = std::max(size, 0);
    reflowState.SetComputedHeight(size);
  } else {
    size -= reflowState.ComputedPhysicalMargin().LeftRight() +
            reflowState.ComputedPhysicalBorderPadding().LeftRight();
    size = std::max(size, 0);
    reflowState.SetComputedWidth(size);
  }

  xoffset += reflowState.ComputedPhysicalMargin().left;
  yoffset += reflowState.ComputedPhysicalMargin().top;

  nsHTMLReflowMetrics barDesiredSize(reflowState);
  ReflowChild(aBarFrame, aPresContext, barDesiredSize, reflowState, xoffset,
              yoffset, 0, aStatus);
  FinishReflowChild(aBarFrame, aPresContext, barDesiredSize, &reflowState,
                    xoffset, yoffset, 0);
}

nsresult
nsMeterFrame::AttributeChanged(int32_t  aNameSpaceID,
                               nsIAtom* aAttribute,
                               int32_t  aModType)
{
  NS_ASSERTION(mBarDiv, "Meter bar div must exist!");

  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::value ||
       aAttribute == nsGkAtoms::max   ||
       aAttribute == nsGkAtoms::min )) {
    nsIFrame* barFrame = mBarDiv->GetPrimaryFrame();
    NS_ASSERTION(barFrame, "The meter frame should have a child with a frame!");
    PresContext()->PresShell()->FrameNeedsReflow(barFrame,
                                                 nsIPresShell::eResize,
                                                 NS_FRAME_IS_DIRTY);
    InvalidateFrame();
  }

  return nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                            aModType);
}

nsSize
nsMeterFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                              nsSize aCBSize, nscoord aAvailableWidth,
                              nsSize aMargin, nsSize aBorder,
                              nsSize aPadding, bool aShrinkWrap)
{
  nsRefPtr<nsFontMetrics> fontMet;
  NS_ENSURE_SUCCESS(nsLayoutUtils::GetFontMetricsForFrame(this,
                                                          getter_AddRefs(fontMet)),
                    nsSize(0, 0));

  nsSize autoSize;
  autoSize.height = autoSize.width = fontMet->Font().size; 

  if (StyleDisplay()->mOrient == NS_STYLE_ORIENT_VERTICAL) {
    autoSize.height *= 5; 
  } else {
    autoSize.width *= 5; 
  }

  return autoSize;
}

nscoord
nsMeterFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nsRefPtr<nsFontMetrics> fontMet;
  NS_ENSURE_SUCCESS(
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet)), 0);

  nscoord minWidth = fontMet->Font().size; 

  if (StyleDisplay()->mOrient == NS_STYLE_ORIENT_AUTO ||
      StyleDisplay()->mOrient == NS_STYLE_ORIENT_HORIZONTAL) {
    
    minWidth *= 5; 
  }

  return minWidth;
}

nscoord
nsMeterFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  return GetMinWidth(aRenderingContext);
}

bool
nsMeterFrame::ShouldUseNativeStyle() const
{
  
  
  
  
  return StyleDisplay()->mAppearance == NS_THEME_METERBAR &&
         mBarDiv->GetPrimaryFrame()->StyleDisplay()->mAppearance == NS_THEME_METERBAR_CHUNK &&
         !PresContext()->HasAuthorSpecifiedRules(const_cast<nsMeterFrame*>(this),
                                                 NS_AUTHOR_SPECIFIED_BORDER | NS_AUTHOR_SPECIFIED_BACKGROUND) &&
         !PresContext()->HasAuthorSpecifiedRules(mBarDiv->GetPrimaryFrame(),
                                                 NS_AUTHOR_SPECIFIED_BORDER | NS_AUTHOR_SPECIFIED_BACKGROUND);
}

Element*
nsMeterFrame::GetPseudoElement(nsCSSPseudoElements::Type aType)
{
  if (aType == nsCSSPseudoElements::ePseudo_mozMeterBar) {
    return mBarDiv;
  }

  return nsContainerFrame::GetPseudoElement(aType);
}
