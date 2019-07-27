







#include "nsRubyTextContainerFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"
#include "RubyReflowState.h"
#include "mozilla/UniquePtr.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyTextContainerFrame)
  NS_QUERYFRAME_ENTRY(nsRubyTextContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyTextContainerFrame)

nsContainerFrame*
NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                             nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyTextContainerFrame(aContext);
}







nsIAtom*
nsRubyTextContainerFrame::GetType() const
{
  return nsGkAtoms::rubyTextContainerFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyTextContainerFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RubyTextContainer"), aResult);
}
#endif

 bool
nsRubyTextContainerFrame::IsFrameOfType(uint32_t aFlags) const
{
  if (aFlags & eSupportsCSSTransforms) {
    return false;
  }
  return nsRubyTextContainerFrameSuper::IsFrameOfType(aFlags);
}

 void
nsRubyTextContainerFrame::SetInitialChildList(ChildListID aListID,
                                              nsFrameList& aChildList)
{
  nsRubyTextContainerFrameSuper::SetInitialChildList(aListID, aChildList);
  UpdateSpanFlag();
}

 void
nsRubyTextContainerFrame::AppendFrames(ChildListID aListID,
                                       nsFrameList& aFrameList)
{
  nsRubyTextContainerFrameSuper::AppendFrames(aListID, aFrameList);
  UpdateSpanFlag();
}

 void
nsRubyTextContainerFrame::InsertFrames(ChildListID aListID,
                                       nsIFrame* aPrevFrame,
                                       nsFrameList& aFrameList)
{
  nsRubyTextContainerFrameSuper::InsertFrames(aListID, aPrevFrame, aFrameList);
  UpdateSpanFlag();
}

 void
nsRubyTextContainerFrame::RemoveFrame(ChildListID aListID,
                                      nsIFrame* aOldFrame)
{
  nsRubyTextContainerFrameSuper::RemoveFrame(aListID, aOldFrame);
  UpdateSpanFlag();
}

void
nsRubyTextContainerFrame::UpdateSpanFlag()
{
  bool isSpan = false;
  
  if (!GetPrevContinuation() && !GetNextContinuation()) {
    nsIFrame* onlyChild = mFrames.OnlyChild();
    if (onlyChild && onlyChild->IsPseudoFrame(GetContent())) {
      
      
      isSpan = true;
    }
  }

  if (isSpan) {
    AddStateBits(NS_RUBY_TEXT_CONTAINER_IS_SPAN);
  } else {
    RemoveStateBits(NS_RUBY_TEXT_CONTAINER_IS_SPAN);
  }
}

 void
nsRubyTextContainerFrame::Reflow(nsPresContext* aPresContext,
                                 nsHTMLReflowMetrics& aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsRubyTextContainerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  MOZ_ASSERT(aReflowState.mRubyReflowState, "No ruby reflow state provided");

  
  
  

  
  
  
  
  aStatus = NS_FRAME_COMPLETE;
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  const RubyReflowState::TextContainerInfo& info =
    aReflowState.mRubyReflowState->GetCurrentTextContainerInfo(this);
  aDesiredSize.SetSize(lineWM, info.mLineSize);
}
