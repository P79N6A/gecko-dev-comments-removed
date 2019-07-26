







#include "nsGridContainerFrame.h"

#include "nsPresContext.h"
#include "nsStyleContext.h"






NS_QUERYFRAME_HEAD(nsGridContainerFrame)
  NS_QUERYFRAME_ENTRY(nsGridContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsGridContainerFrame)

nsIFrame*
NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                         nsStyleContext* aContext)
{
  return new (aPresShell) nsGridContainerFrame(aContext);
}







void
nsGridContainerFrame::Reflow(nsPresContext*           aPresContext,
                             nsHTMLReflowMetrics&     aDesiredSize,
                             const nsHTMLReflowState& aReflowState,
                             nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsGridContainerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if (IsFrameTreeTooDeep(aReflowState, aDesiredSize, aStatus)) {
    return;
  }

  nsMargin bp = aReflowState.ComputedPhysicalBorderPadding();
  ApplySkipSides(bp);
  nscoord contentHeight = GetEffectiveComputedHeight(aReflowState);
  if (contentHeight == NS_AUTOHEIGHT) {
    contentHeight = 0;
  }
  aDesiredSize.Width() = aReflowState.ComputedWidth() + bp.LeftRight();
  aDesiredSize.Height() = contentHeight + bp.TopBottom();
  aDesiredSize.SetOverflowAreasToDesiredBounds();
  aStatus = NS_FRAME_COMPLETE;
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aDesiredSize);
}

nsIAtom*
nsGridContainerFrame::GetType() const
{
  return nsGkAtoms::gridContainerFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsGridContainerFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("GridContainer"), aResult);
}
#endif
