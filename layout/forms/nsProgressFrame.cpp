




































#include "nsProgressFrame.h"

#include "nsIDOMHTMLProgressElement.h"
#include "nsIContent.h"
#include "prtypes.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsINameSpaceManager.h"
#include "nsIDocument.h"
#include "nsIPresShell.h"
#include "nsNodeInfoManager.h"
#include "nsINodeInfo.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentUtils.h"
#include "nsFormControlFrame.h"
#include "nsFontMetrics.h"
#include "mozilla/dom/Element.h"


nsIFrame*
NS_NewProgressFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsProgressFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsProgressFrame)

nsProgressFrame::nsProgressFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
  , mBarDiv(nsnull)
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
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::div, nsnull,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = NS_NewHTMLElement(getter_AddRefs(mBarDiv), nodeInfo.forget(),
                                  mozilla::dom::NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCSSPseudoElements::Type pseudoType = nsCSSPseudoElements::ePseudo_mozProgressBar;
  nsRefPtr<nsStyleContext> newStyleContext = PresContext()->StyleSet()->
    ResolvePseudoElementStyle(mContent->AsElement(), pseudoType,
                              GetStyleContext());

  if (!aElements.AppendElement(ContentInfo(mBarDiv, newStyleContext))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}

void
nsProgressFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                          PRUint32 aFilter)
{
  aElements.MaybeAppendElement(mBarDiv);
}

NS_QUERYFRAME_HEAD(nsProgressFrame)
  NS_QUERYFRAME_ENTRY(nsProgressFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)


NS_IMETHODIMP
nsProgressFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  return BuildDisplayListForInline(aBuilder, aDirtyRect, aLists);
}

NS_IMETHODIMP nsProgressFrame::Reflow(nsPresContext*           aPresContext,
                                      nsHTMLReflowMetrics&     aDesiredSize,
                                      const nsHTMLReflowState& aReflowState,
                                      nsReflowStatus&          aStatus)
{
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

  aDesiredSize.width = aReflowState.ComputedWidth() +
                       aReflowState.mComputedBorderPadding.LeftRight();
  aDesiredSize.height = aReflowState.ComputedHeight() +
                        aReflowState.mComputedBorderPadding.TopBottom();
  aDesiredSize.height = NS_CSS_MINMAX(aDesiredSize.height,
                                      aReflowState.mComputedMinHeight,
                                      aReflowState.mComputedMaxHeight);

  aDesiredSize.SetOverflowAreasToDesiredBounds();
  ConsiderChildOverflow(aDesiredSize.mOverflowAreas, barFrame);
  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  return NS_OK;
}

void
nsProgressFrame::ReflowBarFrame(nsIFrame*                aBarFrame,
                                nsPresContext*           aPresContext,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  bool vertical = GetStyleDisplay()->mOrient == NS_STYLE_ORIENT_VERTICAL;
  nsHTMLReflowState reflowState(aPresContext, aReflowState, aBarFrame,
                                nsSize(aReflowState.ComputedWidth(),
                                       NS_UNCONSTRAINEDSIZE));
  nscoord size = vertical ? aReflowState.ComputedHeight()
                          : aReflowState.ComputedWidth();
  nscoord xoffset = aReflowState.mComputedBorderPadding.left;
  nscoord yoffset = aReflowState.mComputedBorderPadding.top;

  double position;
  nsCOMPtr<nsIDOMHTMLProgressElement> progressElement =
    do_QueryInterface(mContent);
  progressElement->GetPosition(&position);

  
  
  if (position >= 0.0) {
    size *= position;
  }

  if (!vertical && GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    xoffset += aReflowState.ComputedWidth() - size;
  }

  
  
  
  
  
  
  
  if (position != -1 || ShouldUseNativeStyle()) {
    if (vertical) {
      
      yoffset += aReflowState.ComputedHeight() - size;

      size -= reflowState.mComputedMargin.TopBottom() +
              reflowState.mComputedBorderPadding.TopBottom();
      size = NS_MAX(size, 0);
      reflowState.SetComputedHeight(size);
    } else {
      size -= reflowState.mComputedMargin.LeftRight() +
              reflowState.mComputedBorderPadding.LeftRight();
      size = NS_MAX(size, 0);
      reflowState.SetComputedWidth(size);
    }
  } else if (vertical) {
    
    
    
    yoffset += aReflowState.ComputedHeight() - reflowState.ComputedHeight();
  }

  xoffset += reflowState.mComputedMargin.left;
  yoffset += reflowState.mComputedMargin.top;

  nsHTMLReflowMetrics barDesiredSize;
  ReflowChild(aBarFrame, aPresContext, barDesiredSize, reflowState, xoffset,
              yoffset, 0, aStatus);
  FinishReflowChild(aBarFrame, aPresContext, &reflowState, barDesiredSize,
                    xoffset, yoffset, 0);
}

NS_IMETHODIMP
nsProgressFrame::AttributeChanged(PRInt32  aNameSpaceID,
                                  nsIAtom* aAttribute,
                                  PRInt32  aModType)
{
  NS_ASSERTION(mBarDiv, "Progress bar div must exist!");

  if (aNameSpaceID == kNameSpaceID_None &&
      (aAttribute == nsGkAtoms::value || aAttribute == nsGkAtoms::max)) {
    nsIFrame* barFrame = mBarDiv->GetPrimaryFrame();
    NS_ASSERTION(barFrame, "The progress frame should have a child with a frame!");
    PresContext()->PresShell()->FrameNeedsReflow(barFrame, nsIPresShell::eResize,
                                                 NS_FRAME_IS_DIRTY);
    Invalidate(GetVisualOverflowRectRelativeToSelf());
  }

  return nsContainerFrame::AttributeChanged(aNameSpaceID, aAttribute, aModType);
}

nsSize
nsProgressFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, bool aShrinkWrap)
{
  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  nsRefPtr<nsFontMetrics> fontMet;
  NS_ENSURE_SUCCESS(nsLayoutUtils::GetFontMetricsForFrame(this,
                                                          getter_AddRefs(fontMet),
                                                          inflation),
                    nsSize(0, 0));

  nsSize autoSize;
  autoSize.height = autoSize.width = fontMet->Font().size; 

  if (GetStyleDisplay()->mOrient == NS_STYLE_ORIENT_VERTICAL) {
    autoSize.height *= 10; 
  } else {
    autoSize.width *= 10; 
  }

  return autoSize;
}

nscoord
nsProgressFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  nsRefPtr<nsFontMetrics> fontMet;
  NS_ENSURE_SUCCESS(
      nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fontMet)), 0);

  nscoord minWidth = fontMet->Font().size; 

  if (GetStyleDisplay()->mOrient == NS_STYLE_ORIENT_HORIZONTAL) {
    minWidth *= 10; 
  }

  return minWidth;
}

nscoord
nsProgressFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  return GetMinWidth(aRenderingContext);
}

bool
nsProgressFrame::ShouldUseNativeStyle() const
{
  
  
  
  
  return GetStyleDisplay()->mAppearance == NS_THEME_PROGRESSBAR &&
         mBarDiv->GetPrimaryFrame()->GetStyleDisplay()->mAppearance == NS_THEME_PROGRESSBAR_CHUNK &&
         !PresContext()->HasAuthorSpecifiedRules(const_cast<nsProgressFrame*>(this),
                                                 NS_AUTHOR_SPECIFIED_BORDER | NS_AUTHOR_SPECIFIED_BACKGROUND) &&
         !PresContext()->HasAuthorSpecifiedRules(mBarDiv->GetPrimaryFrame(),
                                                 NS_AUTHOR_SPECIFIED_BORDER | NS_AUTHOR_SPECIFIED_BACKGROUND);
}

