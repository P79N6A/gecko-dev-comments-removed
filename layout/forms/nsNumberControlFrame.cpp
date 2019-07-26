




#include "nsNumberControlFrame.h"

#include "nsIPresShell.h"
#include "nsFontMetrics.h"
#include "nsFormControlFrame.h"
#include "nsGkAtoms.h"
#include "nsINodeInfo.h"
#include "nsINameSpaceManager.h"
#include "nsContentUtils.h"
#include "nsContentCreatorFunctions.h"
#include "nsContentList.h"
#include "nsStyleSet.h"

using namespace mozilla;

nsIFrame*
NS_NewNumberControlFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsNumberControlFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsNumberControlFrame)

NS_QUERYFRAME_HEAD(nsNumberControlFrame)
  NS_QUERYFRAME_ENTRY(nsIAnonymousContentCreator)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

nsNumberControlFrame::nsNumberControlFrame(nsStyleContext* aContext)
  : nsContainerFrame(aContext)
{
}

void
nsNumberControlFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  NS_ASSERTION(!GetPrevContinuation() && !GetNextContinuation(),
               "nsNumberControlFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first");
  nsFormControlFrame::RegUnRegAccessKey(static_cast<nsIFrame*>(this), false);
  nsContentUtils::DestroyAnonymousContent(&mOuterWrapper);
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
nsNumberControlFrame::Reflow(nsPresContext* aPresContext,
                             nsHTMLReflowMetrics& aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsNumberControlFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  NS_ASSERTION(mOuterWrapper, "Outer wrapper div must exist!");

  NS_ASSERTION(!GetPrevContinuation() && !GetNextContinuation(),
               "nsNumberControlFrame should not have continuations; if it does we "
               "need to call RegUnregAccessKey only for the first");

  NS_ASSERTION(!mFrames.FirstChild() ||
               !mFrames.FirstChild()->GetNextSibling(),
               "We expect at most one direct child frame");

  if (mState & NS_FRAME_FIRST_REFLOW) {
    nsFormControlFrame::RegUnRegAccessKey(this, true);
  }

  nsHTMLReflowMetrics wrappersDesiredSize;
  nsIFrame* outerWrapperFrame = mOuterWrapper->GetPrimaryFrame();
  if (outerWrapperFrame) { 
    NS_ASSERTION(outerWrapperFrame == mFrames.FirstChild(), "huh?");
    nsresult rv =
      ReflowAnonymousContent(aPresContext, wrappersDesiredSize,
                             aReflowState, outerWrapperFrame);
    NS_ENSURE_SUCCESS(rv, rv);
    ConsiderChildOverflow(aDesiredSize.mOverflowAreas, outerWrapperFrame);
  }

  nscoord computedHeight = aReflowState.ComputedHeight();
  if (computedHeight == NS_AUTOHEIGHT) {
    computedHeight =
      outerWrapperFrame ? outerWrapperFrame->GetSize().height : 0;
  }
  aDesiredSize.width = aReflowState.ComputedWidth() +
                         aReflowState.mComputedBorderPadding.LeftRight();
  aDesiredSize.height = computedHeight +
                          aReflowState.mComputedBorderPadding.TopBottom();

  if (outerWrapperFrame) {
    aDesiredSize.ascent = wrappersDesiredSize.ascent +
                            outerWrapperFrame->GetPosition().y;
  }

  aDesiredSize.SetOverflowAreasToDesiredBounds();

  FinishAndStoreOverflow(&aDesiredSize);

  aStatus = NS_FRAME_COMPLETE;

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);

  return NS_OK;
}

nsresult
nsNumberControlFrame::
  ReflowAnonymousContent(nsPresContext* aPresContext,
                         nsHTMLReflowMetrics& aWrappersDesiredSize,
                         const nsHTMLReflowState& aParentReflowState,
                         nsIFrame* aOuterWrapperFrame)
{
  MOZ_ASSERT(aOuterWrapperFrame);

  
  
  nscoord inputFrameContentBoxWidth = aParentReflowState.ComputedWidth();

  nsHTMLReflowState wrapperReflowState(aPresContext, aParentReflowState,
                                       aOuterWrapperFrame,
                                       nsSize(inputFrameContentBoxWidth,
                                              NS_UNCONSTRAINEDSIZE));

  nscoord xoffset = aParentReflowState.mComputedBorderPadding.left +
                      wrapperReflowState.mComputedMargin.left;
  nscoord yoffset = aParentReflowState.mComputedBorderPadding.top +
                      wrapperReflowState.mComputedMargin.top;

  nsReflowStatus childStatus;
  nsresult rv = ReflowChild(aOuterWrapperFrame, aPresContext,
                            aWrappersDesiredSize, wrapperReflowState,
                            xoffset, yoffset, 0, childStatus);
  NS_ENSURE_SUCCESS(rv, rv);
  MOZ_ASSERT(NS_FRAME_IS_FULLY_COMPLETE(childStatus),
             "We gave our child unconstrained height, so it should be complete");
  return FinishReflowChild(aOuterWrapperFrame, aPresContext,
                           &wrapperReflowState, aWrappersDesiredSize,
                           xoffset, yoffset, 0);
}

nsresult
nsNumberControlFrame::MakeAnonymousElement(nsIContent** aResult,
                                           nsTArray<ContentInfo>& aElements,
                                           nsIAtom* aTagName,
                                           nsCSSPseudoElements::Type aPseudoType,
                                           nsStyleContext* aParentContext)
{
  
  nsCOMPtr<nsIDocument> doc = mContent->GetDocument();

  nsCOMPtr<nsINodeInfo> nodeInfo;
  nodeInfo = doc->NodeInfoManager()->GetNodeInfo(aTagName, nullptr,
                                                 kNameSpaceID_XHTML,
                                                 nsIDOMNode::ELEMENT_NODE);
  NS_ENSURE_TRUE(nodeInfo, NS_ERROR_OUT_OF_MEMORY);
  nsresult rv = NS_NewHTMLElement(aResult, nodeInfo.forget(),
                                  dom::NOT_FROM_PARSER);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  
  
  NS_ASSERTION(aPseudoType != nsCSSPseudoElements::ePseudo_NotPseudoElement,
               "Expecting anonymous children to all be pseudo-elements");
  
  nsRefPtr<nsStyleContext> newStyleContext =
    PresContext()->StyleSet()->ResolvePseudoElementStyle(mContent->AsElement(),
                                                         aPseudoType,
                                                         aParentContext);

  if (!aElements.AppendElement(ContentInfo(*aResult, newStyleContext))) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  return NS_OK;
}

nsresult
nsNumberControlFrame::CreateAnonymousContent(nsTArray<ContentInfo>& aElements)
{
  nsresult rv;

  
  
  
  
  
  
  
  
  
  
  
  


  
  rv = MakeAnonymousElement(getter_AddRefs(mOuterWrapper),
                            aElements,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberWrapper,
                            mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  ContentInfo& outerWrapperCI = aElements.LastElement();

  
  rv = MakeAnonymousElement(getter_AddRefs(mTextField),
                            outerWrapperCI.mChildren,
                            nsGkAtoms::input,
                            nsCSSPseudoElements::ePseudo_mozNumberText,
                            outerWrapperCI.mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  mTextField->SetAttr(kNameSpaceID_None, nsGkAtoms::type,
                      NS_LITERAL_STRING("text"), PR_FALSE);

  
  rv = MakeAnonymousElement(getter_AddRefs(mSpinBox),
                            outerWrapperCI.mChildren,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberSpinBox,
                            outerWrapperCI.mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  ContentInfo& spinBoxCI = outerWrapperCI.mChildren.LastElement();

  
  rv = MakeAnonymousElement(getter_AddRefs(mSpinUp),
                            spinBoxCI.mChildren,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberSpinUp,
                            spinBoxCI.mStyleContext);
  NS_ENSURE_SUCCESS(rv, rv);

  
  rv = MakeAnonymousElement(getter_AddRefs(mSpinDown),
                            spinBoxCI.mChildren,
                            nsGkAtoms::div,
                            nsCSSPseudoElements::ePseudo_mozNumberSpinDown,
                            spinBoxCI.mStyleContext);
  return rv;
}

nsIAtom*
nsNumberControlFrame::GetType() const
{
  return nsGkAtoms::numberControlFrame;
}

void
nsNumberControlFrame::AppendAnonymousContentTo(nsBaseContentList& aElements,
                                               uint32_t aFilter)
{
  
  aElements.MaybeAppendElement(mOuterWrapper);
}
