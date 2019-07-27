




#include "nsProgressFrame.h"

#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsNameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsNodeInfoManager.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsFormControlFrame.h"
#include "nsFontMetrics.h"
#include "mozilla/dom/Element.h"
#include "mozilla/dom/HTMLProgressElement.h"
#include "nsContentList.h"
#include "nsStyleSet.h"
#include "nsThemeConstants.h"
#include <algorithm>

using namespace mozilla;
using namespace mozilla::dom;

nsIFrame*
NS_NewProgressFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsProgressFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsProgressFrame)

nsProgressFrame::nsProgressFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
  , mBarDiv(nullptr)
{
}

nsProgressFrame::~nsProgressFrame()
{
}

void
nsProgressFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  NS_ASSERTION(!GetPrevContinuation(),
               "nsProgressFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first.");
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsContentUtils::DestroyAnonymousContent(&mBarDiv);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsProgressFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetComposedDoc();
  mBarDiv = doc->CreateHTMLElement(nsGkAtoms::div);

  
  nsCSSPseudoElements::Type pseudoType = nsCSSPseudoElements::ePseudo_mozProgressBar;
  nsRefPtr<nsStyleContext> newStyleContext = PresContext()->StyleSet()->
    ResolvePseudoElementStyle(mContent->AsElement(), pseudoType,
                              StyleContext(), mBarDiv->AsElement());

  if (!aElements.AppendElement(ContentInfo(mBarDiv, newStyleContext))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
nsProgressFrame::AppendAnonymousContentTo(nsTArray<nsIContent*>& aElements,
                                          uint32_t aFilter)
{
  if (mBarDiv) {
    aElements.AppendElement(mBarDiv);
  }
}

NS_QUERYFRAME_HEAD(nsProgressFrame)
  NS_QUERYFRAME_ENTRY(nsProgressFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)


void
nsProgressFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  BuildDisplayListForInline(aBuilder, aDirtyRect, aLists);
}

void
nsProgressFrame::Reflow(nsPresContext*           aPresContext,
                        nsHTMLReflowMetrics&     aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsProgressFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_ASSERTION(mBarDiv, "Progress bar div must exist!");
  NS_ASSERTION(!GetPrevContinuation(),
               "nsProgressFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first.");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, true);
  }

  nsIFrame* barFrame = mBarDiv->GetPrimaryFrame();
  NS_ASSERTION(barFrame, "The progress frame should have a child with a frame!");

  ReflowBarFrame(barFrame, aPresContext, aReflowState, aStatus);

  aDesiredSize.SetSize(aReflowState.GetWritingMode(),
                       aReflowState.ComputedSizeWithBorderPadding());
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  ConsiderChildOverflow(aDesiredSize.mOverflowAreas, barFrame);
  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

void
nsProgressFrame::ReflowBarFrame(nsIFrame*                aBarFrame,
                                nsPresContext*           aPresContext,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  bool vertical = ResolvedOrientationIsVertical();
  WritingMode wm = aBarFrame->GetWritingMode();
  LogicalSize availSize = aReflowState.ComputedSize(wm);
  availSize.BSize(wm) = NS_UNCONSTRAINEDSIZE;
  nsHTMLReflowState reflowState(aPresContext, aReflowState,
                                aBarFrame, availSize);
  nscoord size = vertical ? aReflowState.ComputedHeight()
                          : aReflowState.ComputedWidth();
  nscoord xoffset = aReflowState.ComputedPhysicalBorderPadding().left;
  nscoord yoffset = aReflowState.ComputedPhysicalBorderPadding().top;

  double position = static_cast<HTMLProgressElement*>(mContent)->Position();

  
  
  if (position >= 0.0) {
    size *= position;
  }

  if (!vertical && (wm.IsVertical() ? wm.IsVerticalRL() : !wm.IsBidiLTR())) {
    xoffset += aReflowState.ComputedWidth() - size;
  }

  
  
  
  
  
  
  
  if (position != -1 || ShouldUseNativeStyle()) {
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
  } else if (vertical) {
    
    
    
    yoffset += aReflowState.ComputedHeight() - reflowState.ComputedHeight();
  }

  xoffset += reflowState.ComputedPhysicalMargin().left;
  yoffset += reflowState.ComputedPhysicalMargin().top;

  nsHTMLReflowMetrics barDesiredSize(aReflowState);
  ReflowChild(aBarFrame, aPresContext, barDesiredSize, reflowState, xoffset,
              yoffset, 0, aStatus);
  FinishReflowChild(aBarFrame, aPresContext, barDesiredSize, &reflowState,
                    xoffset, yoffset, 0);
}

nsresult
nsProgressFrame::AttributeChanged(int32_t  aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  int32_t  aModType)
{
  NS_ASSERTION(mBarDiv, "Progress bar div must exist!");

  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::value || aAttribute == nsGkAtoms::max)) {
    nsIFrame* barFrame = mBarDiv->GetPrimaryFrame();
    NS_ASSERTION(barFrame, "The progress frame should have a child with a frame!");
    PresContext()->PresShell()->FrameNeedsReflow(barFrame, nsIPresShell::eResize,
                                                 NS_FRAME_IS_DIRTY);
    InvalidateFrame();
  }

  return nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

LogicalSize
nsProgressFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 WritingMode aWM,
                                 const LogicalSize& aCBSize,
                                 nscoord aAvailableISize,
                                 const LogicalSize& aMargin,
                                 const LogicalSize& aBorder,
                                 const LogicalSize& aPadding,
                                 bool aShrinkWrap)
{
  const WritingMode wm = GetWritingMode();
  LogicalSize autoSize(wm);
  autoSize.BSize(wm) = autoSize.ISize(wm) =
    NSToCoordRound(StyleFont()->mFont.size *
                   nsLayoutUtils::FontSizeInflationFor(this)); 

  if (ResolvedOrientationIsVertical()) {
    autoSize.Height(wm) *= 10; 
  } else {
    autoSize.Width(wm) *= 10; 
  }

  return autoSize.ConvertTo(aWM, wm);
}

nscoord
nsProgressFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  nsRefPtr<nsFontMetrics> fontMet;
  NS_ENSURE_SUCCESS(
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet)), 0);

  nscoord minISize = fontMet->Font().size; 

  if (ResolvedOrientationIsVertical() == GetWritingMode().IsVertical()) {
    
    minISize *= 10; 
  }

  return minISize;
}

nscoord
nsProgressFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  return GetMinISize(aRenderingContext);
}

bool
nsProgressFrame::ShouldUseNativeStyle() const
{
  
  
  
  
  return StyleDisplay()->mAppearance == NS_THEME_PROGRESSBAR &&
         mBarDiv->GetPrimaryFrame()->StyleDisplay()->mAppearance == NS_THEME_PROGRESSBAR_CHUNK &&
         !PresContext()->HasAuthorSpecifiedRules(const_cast<nsProgressFrame*>(this),
                                                 NS_AUTHOR_SPECIFIED_BORDER | NS_AUTHOR_SPECIFIED_BACKGROUND) &&
         !PresContext()->HasAuthorSpecifiedRules(mBarDiv->GetPrimaryFrame(),
                                                 NS_AUTHOR_SPECIFIED_BORDER | NS_AUTHOR_SPECIFIED_BACKGROUND);
}

Element*
nsProgressFrame::GetPseudoElement(nsCSSPseudoElements::Type aType)
{
  if (aType == nsCSSPseudoElements::ePseudo_mozProgressBar) {
    return mBarDiv;
  }

  return nsContainerFrame::GetPseudoElement(aType);
}
