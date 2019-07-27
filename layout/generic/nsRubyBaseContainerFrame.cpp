







#include "nsRubyBaseContainerFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"

using namespace mozilla;

#define RTC_ARRAY_SIZE 1






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

class MOZ_STACK_CLASS PairEnumerator
{
public:
  PairEnumerator(nsRubyBaseContainerFrame* aRBCFrame,
                 const nsTArray<nsRubyTextContainerFrame*>& aRTCFrames);

  void Next();
  bool AtEnd() const;

  uint32_t GetLevelCount() const { return mFrames.Length(); }
  nsIFrame* GetFrame(uint32_t aIndex) const { return mFrames[aIndex]; }
  nsIFrame* GetBaseFrame() const { return GetFrame(0); }
  nsIFrame* GetTextFrame(uint32_t aIndex) const { return GetFrame(aIndex + 1); }

private:
  nsAutoTArray<nsIFrame*, RTC_ARRAY_SIZE + 1> mFrames;
};

PairEnumerator::PairEnumerator(
    nsRubyBaseContainerFrame* aBaseContainer,
    const nsTArray<nsRubyTextContainerFrame*>& aTextContainers)
{
  const uint32_t rtcCount = aTextContainers.Length();
  mFrames.SetCapacity(rtcCount + 1);
  mFrames.AppendElement(aBaseContainer->GetFirstPrincipalChild());
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsIFrame* rtFrame = aTextContainers[i]->GetFirstPrincipalChild();
    mFrames.AppendElement(rtFrame);
  }
}

void
PairEnumerator::Next()
{
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    if (mFrames[i]) {
      mFrames[i] = mFrames[i]->GetNextSibling();
    }
  }
}

bool
PairEnumerator::AtEnd() const
{
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    if (mFrames[i]) {
      return false;
    }
  }
  return true;
}

nscoord
nsRubyBaseContainerFrame::CalculateMaxSpanISize(
    nsRenderingContext* aRenderingContext)
{
  nscoord max = 0;
  uint32_t spanCount = mSpanContainers.Length();
  for (uint32_t i = 0; i < spanCount; i++) {
    nsIFrame* frame = mSpanContainers[i]->GetFirstPrincipalChild();
    nscoord isize = frame->GetPrefISize(aRenderingContext);
    max = std::max(max, isize);
  }
  return max;
}

static nscoord
CalculatePairPrefISize(nsRenderingContext* aRenderingContext,
                       const PairEnumerator& aEnumerator)
{
  nscoord max = 0;
  uint32_t levelCount = aEnumerator.GetLevelCount();
  for (uint32_t i = 0; i < levelCount; i++) {
    nsIFrame* frame = aEnumerator.GetFrame(i);
    if (frame) {
      max = std::max(max, frame->GetPrefISize(aRenderingContext));
    }
  }
  return max;
}

 void
nsRubyBaseContainerFrame::AddInlineMinISize(
    nsRenderingContext *aRenderingContext, nsIFrame::InlineMinISizeData *aData)
{
  if (!mSpanContainers.IsEmpty()) {
    
    
    aData->currentLine += GetPrefISize(aRenderingContext);
    return;
  }

  nscoord max = 0;
  PairEnumerator enumerator(this, mTextContainers);
  for (; !enumerator.AtEnd(); enumerator.Next()) {
    
    
    max = std::max(max, CalculatePairPrefISize(aRenderingContext, enumerator));
  }
  aData->currentLine += max;
}

 void
nsRubyBaseContainerFrame::AddInlinePrefISize(
    nsRenderingContext *aRenderingContext, nsIFrame::InlinePrefISizeData *aData)
{
  nscoord sum = 0;
  PairEnumerator enumerator(this, mTextContainers);
  for (; !enumerator.AtEnd(); enumerator.Next()) {
    sum += CalculatePairPrefISize(aRenderingContext, enumerator);
  }
  sum = std::max(sum, CalculateMaxSpanISize(aRenderingContext));
  aData->currentLine += sum;
}

 bool 
nsRubyBaseContainerFrame::IsFrameOfType(uint32_t aFlags) const 
{
  return nsContainerFrame::IsFrameOfType(aFlags & 
         ~(nsIFrame::eLineParticipant));
}

void nsRubyBaseContainerFrame::AppendTextContainer(nsIFrame* aFrame)
{
  nsRubyTextContainerFrame* rtcFrame = do_QueryFrame(aFrame);
  MOZ_ASSERT(rtcFrame, "Must provide a ruby text container.");

  nsIFrame* onlyChild = rtcFrame->PrincipalChildList().OnlyChild();
  if (onlyChild && onlyChild->IsPseudoFrame(rtcFrame->GetContent())) {
    
    
    mSpanContainers.AppendElement(rtcFrame);
  } else {
    mTextContainers.AppendElement(rtcFrame);
  }
}

void nsRubyBaseContainerFrame::ClearTextContainers() {
  mSpanContainers.Clear();
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
