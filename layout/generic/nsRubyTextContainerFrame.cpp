







#include "nsRubyTextContainerFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"
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

  
  
  
  
  aStatus = NS_FRAME_COMPLETE;
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();

  nscoord minBCoord = nscoord_MAX;
  nscoord maxBCoord = nscoord_MIN;
  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nsIFrame* child = e.get();
    MOZ_ASSERT(child->GetType() == nsGkAtoms::rubyTextFrame);
    
    LogicalRect rect = child->GetLogicalRect(lineWM, 0);
    LogicalMargin margin = child->GetLogicalUsedMargin(lineWM);
    nscoord blockStart = rect.BStart(lineWM) - margin.BStart(lineWM);
    minBCoord = std::min(minBCoord, blockStart);
    nscoord blockEnd = rect.BEnd(lineWM) + margin.BEnd(lineWM);
    maxBCoord = std::max(maxBCoord, blockEnd);
  }

  MOZ_ASSERT(minBCoord <= maxBCoord || mFrames.IsEmpty());
  LogicalSize size(lineWM, mISize, 0);
  if (!mFrames.IsEmpty()) {
    size.BSize(lineWM) = maxBCoord - minBCoord;
    nscoord deltaBCoord = -minBCoord;
    if (lineWM.IsVerticalRL()) {
      deltaBCoord -= size.BSize(lineWM);
    }

    if (deltaBCoord != 0) {
      nscoord containerWidth = size.Width(lineWM);
      for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
        nsIFrame* child = e.get();
        LogicalPoint pos = child->GetLogicalPosition(lineWM, containerWidth);
        pos.B(lineWM) += deltaBCoord;
        
        
        child->SetPosition(lineWM, pos, containerWidth);
        nsContainerFrame::PlaceFrameView(child);
      }
    }
  }

  aDesiredSize.SetSize(lineWM, size);
}
