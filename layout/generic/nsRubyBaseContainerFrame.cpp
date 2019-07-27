







#include "nsRubyBaseContainerFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyBaseContainerFrame)
  NS_QUERYFRAME_ENTRY(nsRubyBaseContainerFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyBaseContainerFrame)

nsContainerFrame*
NS_NewRubyBaseContainerFrame(nsIPresShell* aPresShell,
                             nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyBaseContainerFrame(aContext);
}







nsIAtom*
nsRubyBaseContainerFrame::GetType() const
{
  return nsGkAtoms::rubyBaseContainerFrame;
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyBaseContainerFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("RubyBaseContainer"), aResult);
}
#endif

 bool 
nsRubyBaseContainerFrame::IsFrameOfType(uint32_t aFlags) const 
{
  return nsContainerFrame::IsFrameOfType(aFlags & 
         ~(nsIFrame::eLineParticipant));
}

void nsRubyBaseContainerFrame::AppendTextContainer(nsIFrame* aFrame)
{
  nsRubyTextContainerFrame* rtcFrame = do_QueryFrame(aFrame);
  if (rtcFrame) {
    mTextContainers.AppendElement(rtcFrame);
  }
}

void nsRubyBaseContainerFrame::ClearTextContainers() {
  mTextContainers.Clear();
}

 bool
nsRubyBaseContainerFrame::CanContinueTextRun() const
{
  return true;
}

 void
nsRubyBaseContainerFrame::Reflow(nsPresContext* aPresContext,
                                 nsHTMLReflowMetrics& aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsRubyBaseContainerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);

  if (!aReflowState.mLineLayout) {
    NS_ASSERTION(
      aReflowState.mLineLayout,
      "No line layout provided to RubyBaseContainerFrame reflow method.");
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  aStatus = NS_FRAME_COMPLETE;
  nscoord isize = 0;
  int baseNum = 0;
  nscoord leftoverSpace = 0;
  nscoord spaceApart = 0;
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  WritingMode frameWM = aReflowState.GetWritingMode();
  LogicalMargin borderPadding =
    aReflowState.ComputedLogicalBorderPadding();
  nscoord baseStart = 0;

  LogicalSize availSize(lineWM, aReflowState.AvailableWidth(),
                        aReflowState.AvailableHeight());

  
  for (uint32_t i = 0; i < mTextContainers.Length(); i++) {
    nsRubyTextContainerFrame* rtcFrame = mTextContainers.ElementAt(i);
    nsHTMLReflowState rtcReflowState(aPresContext,
                                     *aReflowState.parentReflowState,
                                     rtcFrame, availSize);
    rtcReflowState.mLineLayout = aReflowState.mLineLayout;
    
    rtcFrame->BeginRTCLineLayout(aPresContext, rtcReflowState);
  }

  for (nsFrameList::Enumerator e(mFrames); !e.AtEnd(); e.Next()) {
    nsIFrame* rbFrame = e.get();
    if (rbFrame->GetType() != nsGkAtoms::rubyBaseFrame) {
      NS_ASSERTION(false, "Unrecognized child type for ruby base container");
      continue;
    }

    nsReflowStatus frameReflowStatus;
    nsHTMLReflowMetrics metrics(aReflowState, aDesiredSize.mFlags);

    
    
    
    
    nscoord prefWidth = rbFrame->GetPrefISize(aReflowState.rendContext);
    nscoord textWidth = 0;

    for (uint32_t i = 0; i < mTextContainers.Length(); i++) {
      nsRubyTextFrame* rtFrame = do_QueryFrame(mTextContainers.ElementAt(i)->
                          PrincipalChildList().FrameAt(baseNum));
      if (rtFrame) {
        int newWidth = rtFrame->GetPrefISize(aReflowState.rendContext);
        if (newWidth > textWidth) {
          textWidth = newWidth;
        }
      }
    }
    if (textWidth > prefWidth) {
      spaceApart = std::max((textWidth - prefWidth) / 2, spaceApart);
      leftoverSpace = spaceApart;
    } else {
      spaceApart = leftoverSpace;
      leftoverSpace = 0;
    }
    if (spaceApart > 0) {
      aReflowState.mLineLayout->AdvanceICoord(spaceApart);
    }
    baseStart = aReflowState.mLineLayout->GetCurrentICoord();

    bool pushedFrame;
    aReflowState.mLineLayout->ReflowFrame(rbFrame, frameReflowStatus,
                                          &metrics, pushedFrame);
    NS_ASSERTION(!pushedFrame, "Ruby line breaking is not yet implemented");

    isize += metrics.ISize(lineWM);
    rbFrame->SetSize(LogicalSize(lineWM, metrics.ISize(lineWM),
                                 metrics.BSize(lineWM)));
    FinishReflowChild(rbFrame, aPresContext, metrics, &aReflowState, 0, 0,
                      NS_FRAME_NO_MOVE_FRAME | NS_FRAME_NO_MOVE_VIEW);

    
    for (uint32_t i = 0; i < mTextContainers.Length(); i++) {
      nsRubyTextFrame* rtFrame = do_QueryFrame(mTextContainers.ElementAt(i)->
                          PrincipalChildList().FrameAt(baseNum));
      nsRubyTextContainerFrame* rtcFrame = mTextContainers.ElementAt(i);
      if (rtFrame) {
        nsHTMLReflowMetrics rtcMetrics(*aReflowState.parentReflowState,
                                       aDesiredSize.mFlags);
        nsHTMLReflowState rtcReflowState(aPresContext,
                                         *aReflowState.parentReflowState,
                                         rtcFrame, availSize);
        rtcReflowState.mLineLayout = rtcFrame->GetLineLayout();
        rtcFrame->ReflowRubyTextFrame(rtFrame, rbFrame, baseStart,
                                      aPresContext, rtcMetrics,
                                      rtcReflowState);
      }
    }
    baseNum++;
  }

  
  
  
  bool continueReflow = true;
  while (continueReflow) {
    continueReflow = false;
    for (uint32_t i = 0; i < mTextContainers.Length(); i++) {
      nsRubyTextFrame* rtFrame = do_QueryFrame(mTextContainers.ElementAt(i)->
                          PrincipalChildList().FrameAt(baseNum));
      nsRubyTextContainerFrame* rtcFrame = mTextContainers.ElementAt(i);
      if (rtFrame) {
        continueReflow = true;
        nsHTMLReflowMetrics rtcMetrics(*aReflowState.parentReflowState,
                                       aDesiredSize.mFlags);
        nsHTMLReflowState rtcReflowState(aPresContext,
                                         *aReflowState.parentReflowState,
                                         rtcFrame, availSize);
        rtcReflowState.mLineLayout = rtcFrame->GetLineLayout();
        rtcFrame->ReflowRubyTextFrame(rtFrame, nullptr, baseStart,
                                      aPresContext, rtcMetrics,
                                      rtcReflowState);
        
        
        baseStart += rtcMetrics.ISize(lineWM);
      }
    }
    baseNum++;
  }

  aDesiredSize.ISize(lineWM) = isize;
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
                                         borderPadding, lineWM, frameWM);
}
