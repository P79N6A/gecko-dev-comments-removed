







#include "nsRubyTextFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"
#include "nsLineLayout.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyTextFrame)
  NS_QUERYFRAME_ENTRY(nsRubyTextFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsRubyTextFrameSuper)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyTextFrame)

nsContainerFrame*
NS_NewRubyTextFrame(nsIPresShell* aPresShell,
                    nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyTextFrame(aContext);
}







nsIAtom*
nsRubyTextFrame::GetType() const
{
  return nsGkAtoms::rubyTextFrame;
}

 bool
nsRubyTextFrame::CanContinueTextRun() const
{
  return false;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyTextFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RubyText"), aResult);
}
#endif



 void
nsRubyTextFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                  const nsRect&           aDirtyRect,
                                  const nsDisplayListSet& aLists)
{
  if (GetStateBits() & NS_RUBY_TEXT_FRAME_AUTOHIDE) {
    return;
  }

  nsRubyTextFrameSuper::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}

 void
nsRubyTextFrame::Reflow(nsPresContext* aPresContext,
                        nsHTMLReflowMetrics& aDesiredSize,
                        const nsHTMLReflowState& aReflowState,
                        nsReflowStatus& aStatus)
{
  
  
  
  
  
  
  nsRubyTextFrameSuper::Reflow(aPresContext, aDesiredSize,
                               aReflowState, aStatus);

  if (GetStateBits() & NS_RUBY_TEXT_FRAME_AUTOHIDE) {
    
    
    WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
    aDesiredSize.ISize(lineWM) = 0;
    aDesiredSize.SetOverflowAreasToDesiredBounds();
  }
}
