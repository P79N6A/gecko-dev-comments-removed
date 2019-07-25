




































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


nsIFrame*
NS_NewProgressFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsProgressFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsProgressFrame)

nsProgressFrame::nsProgressFrame(nsStyleContext* aContext)
  : nsHTMLContainerFrame(aContext)
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
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), PR_FALSE);
  nsContentUtils::DestroyAnonymousContent(&mBarDiv);
  nsHTMLContainerFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsProgressFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(nsGkAtoms::div, nsnull,
                                                 kNameSpaceID_XHTML);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);

  
  nsresult rv = NS_NewHTMLElement(getter_AddRefs(mBarDiv), nodeInfo.forget(),
                                  mozilla::dom::NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aElements.AppendElement(mBarDiv)) {
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
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsHTMLContainerFrame)


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
    nsFormControlFrame::RegUnRegAccessKey(this, PR_TRUE);
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
  nsHTMLReflowState reflowState(aPresContext, aReflowState, aBarFrame,
                                nsSize(aReflowState.ComputedWidth(),
                                       NS_UNCONSTRAINEDSIZE));
  nscoord width = aReflowState.ComputedWidth();
  nscoord xoffset = aReflowState.mComputedBorderPadding.left;
  nscoord yoffset = aReflowState.mComputedBorderPadding.top;

  double position;
  nsCOMPtr<nsIDOMHTMLProgressElement> progressElement =
    do_QueryInterface(mContent);
  progressElement->GetPosition(&position);

  
  
  if (position >= 0.0) {
    width *= position;
  }

  if (GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
    xoffset += aReflowState.ComputedWidth() - width;
  }

  
  width -= reflowState.mComputedMargin.LeftRight() +
           reflowState.mComputedBorderPadding.LeftRight();
  width = NS_MAX(width, 0);
  reflowState.SetComputedWidth(width);

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
  }

  return nsHTMLContainerFrame::AttributeChanged(aNameSpaceID, aAttribute,
                                                aModType);
}

nsSize
nsProgressFrame::ComputeAutoSize(nsRenderingContext *aRenderingContext,
                                 nsSize aCBSize, nscoord aAvailableWidth,
                                 nsSize aMargin, nsSize aBorder,
                                 nsSize aPadding, PRBool aShrinkWrap)
{
  nsRefPtr<nsFontMetrics> fontMet;
  NS_ENSURE_SUCCESS(nsLayoutUtils::GetFontMetricsForFrame(this,
                                                          getter_AddRefs(fontMet)),
                    nsSize(0, 0));

  nsSize autoSize;
  autoSize.height = autoSize.width = fontMet->Font().size; 
  autoSize.width *= 10; 

  return autoSize;
}

