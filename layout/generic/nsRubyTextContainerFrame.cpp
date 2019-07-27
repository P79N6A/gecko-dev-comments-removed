







#include "nsRubyTextContainerFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"
#include "mozilla/UniquePtr.h"






NS_QUERYFRAME_HEAD(nsRubyTextContainerFrame)
  NS_QUERYFRAME_ENTRY(nsRubyTextContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsBlockFrame)

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

void
nsRubyTextContainerFrame::BeginRTCLineLayout(nsPresContext* aPresContext,
                                             const nsHTMLReflowState& aReflowState)
{
  
  nscoord consumedBSize = GetConsumedBSize();

  ClearLineCursor();

  mISize = 0;

  nsBlockReflowState state(aReflowState, aPresContext, this, true, true,
                           false, consumedBSize);

  NS_ASSERTION(!mLines.empty(),
    "There should be at least one line in the ruby text container");
  line_iterator firstLine = begin_lines();
  mLineLayout = mozilla::MakeUnique<nsLineLayout>(
                           state.mPresContext,
                           state.mReflowState.mFloatManager,
                           &state.mReflowState, &firstLine);
  mLineLayout->Init(&state, state.mMinLineHeight, state.mLineNumber);

  mozilla::WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  mozilla::LogicalRect lineRect(state.mContentArea);
  nscoord iStart = lineRect.IStart(lineWM);
  nscoord availISize = lineRect.ISize(lineWM);
  nscoord availBSize = NS_UNCONSTRAINEDSIZE;

  mLineLayout->BeginLineReflow(iStart, state.mBCoord,
                              availISize, availBSize,
                              false,
                              false,
                              lineWM, state.mContainerWidth);
}

void
nsRubyTextContainerFrame::ReflowRubyTextFrame(
                            nsRubyTextFrame* rtFrame,
                            nsIFrame* rbFrame,
                            nscoord baseStart,
                            nsPresContext* aPresContext,
                            nsHTMLReflowMetrics& aDesiredSize,
                            const nsHTMLReflowState& aReflowState)
{
  nsReflowStatus frameReflowStatus;
  nsHTMLReflowMetrics metrics(aReflowState, aDesiredSize.mFlags);
  mozilla::WritingMode lineWM = mLineLayout->GetWritingMode();
  mozilla::LogicalSize availSize(lineWM, aReflowState.AvailableWidth(),
                   aReflowState.AvailableHeight());
  nsHTMLReflowState childReflowState(aPresContext, aReflowState, rtFrame, availSize);

  
  
  int baseWidth;
  if (rbFrame) {
    baseWidth = rbFrame->ISize();

    
    
    if (!rtFrame->GetNextSibling()) {
      rbFrame = rbFrame->GetNextSibling();
      while (rbFrame) {
        baseWidth += rbFrame->ISize();
        rbFrame = rbFrame->GetNextSibling();
      }
    }
  } else {
    baseWidth = 0;
  }
  
  int baseCenter = baseStart + baseWidth / 2;
  
  
  nscoord ICoord = baseCenter - rtFrame->GetPrefISize(aReflowState.rendContext) / 2;
  if (ICoord > mLineLayout->GetCurrentICoord()) {
    mLineLayout->AdvanceICoord(ICoord - mLineLayout->GetCurrentICoord());
  } 

  bool pushedFrame;
  mLineLayout->ReflowFrame(rtFrame, frameReflowStatus,
                           &metrics, pushedFrame);

  NS_ASSERTION(!pushedFrame, "Ruby line breaking is not yet implemented");

  mISize += metrics.ISize(lineWM);
  rtFrame->SetSize(nsSize(metrics.ISize(lineWM), metrics.BSize(lineWM)));
  FinishReflowChild(rtFrame, aPresContext, metrics, &childReflowState, 0, 0,
                    NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW);
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
  mozilla::WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  mozilla::WritingMode frameWM = aReflowState.GetWritingMode();
  mozilla::LogicalMargin borderPadding =
    aReflowState.ComputedLogicalBorderPadding();

  aDesiredSize.ISize(lineWM) = mISize;
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
                                         borderPadding, lineWM, frameWM);

  nscoord bsize = aDesiredSize.BSize(lineWM);
  if (!mLines.empty()) {
    
    
    mLines.begin()->SetLogicalAscent(aDesiredSize.BlockStartAscent());
    mLines.begin()->SetBounds(aReflowState.GetWritingMode(), 0, 0, mISize,
                              bsize, mISize);
  }

  if (mLineLayout) {
    mLineLayout->EndLineReflow();
    mLineLayout = nullptr;
  }
}
