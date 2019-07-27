






#include "nsRubyFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "WritingModes.h"
#include "RubyUtils.h"
#include "nsRubyBaseContainerFrame.h"
#include "nsRubyTextContainerFrame.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyFrame)
  NS_QUERYFRAME_ENTRY(nsRubyFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

NS_IMPL_FRAMEARENA_HELPERS(nsRubyFrame)

nsContainerFrame*
NS_NewRubyFrame(nsIPresShell* aPresShell,
                nsStyleContext* aContext)
{
  return new (aPresShell) nsRubyFrame(aContext);
}






nsIAtom*
nsRubyFrame::GetType() const
{
  return nsGkAtoms::rubyFrame;
}

 bool
nsRubyFrame::IsFrameOfType(uint32_t aFlags) const
{
  return nsContainerFrame::IsFrameOfType(aFlags &
    ~(nsIFrame::eLineParticipant));
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Ruby"), aResult);
}
#endif




class MOZ_STACK_CLASS SegmentEnumerator
{
public:
  explicit SegmentEnumerator(nsRubyFrame* aRubyFrame);

  void Next();
  bool AtEnd() const { return !mBaseContainer; }

  nsRubyBaseContainerFrame* GetBaseContainer() const
  {
    return mBaseContainer;
  }

private:
  nsRubyBaseContainerFrame* mBaseContainer;
};

SegmentEnumerator::SegmentEnumerator(nsRubyFrame* aRubyFrame)
{
  nsIFrame* frame = aRubyFrame->GetFirstPrincipalChild();
  MOZ_ASSERT(!frame ||
             frame->GetType() == nsGkAtoms::rubyBaseContainerFrame);
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

void
SegmentEnumerator::Next()
{
  MOZ_ASSERT(mBaseContainer);
  nsIFrame* frame = mBaseContainer->GetNextSibling();
  while (frame && frame->GetType() != nsGkAtoms::rubyBaseContainerFrame) {
    frame = frame->GetNextSibling();
  }
  mBaseContainer = static_cast<nsRubyBaseContainerFrame*>(frame);
}

 void
nsRubyFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinISizeData *aData)
{
  nscoord max = 0;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    max = std::max(max, e.GetBaseContainer()->GetMinISize(aRenderingContext));
  }
  aData->currentLine += max;
}

 void
nsRubyFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefISizeData *aData)
{
  nscoord sum = 0;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    sum += e.GetBaseContainer()->GetPrefISize(aRenderingContext);
  }
  aData->currentLine += sum;
}

 LogicalSize
nsRubyFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                           WritingMode aWM,
                           const LogicalSize& aCBSize,
                           nscoord aAvailableISize,
                           const LogicalSize& aMargin,
                           const LogicalSize& aBorder,
                           const LogicalSize& aPadding,
                           ComputeSizeFlags aFlags)
{
  
  return LogicalSize(aWM, NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
}

 nscoord
nsRubyFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  return mBaseline;
}

 bool
nsRubyFrame::CanContinueTextRun() const
{
  return true;
}

 void
nsRubyFrame::Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsRubyFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  
  if (!aReflowState.mLineLayout) {
    NS_ASSERTION(aReflowState.mLineLayout,
                 "No line layout provided to RubyFrame reflow method.");
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  
  MoveOverflowToChildList();

  
  mBStartLeading = mBEndLeading = 0;

  
  WritingMode frameWM = aReflowState.GetWritingMode();
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalMargin borderPadding = aReflowState.ComputedLogicalBorderPadding();
  nscoord startEdge = borderPadding.IStart(frameWM);
  nscoord endEdge = aReflowState.AvailableISize() - borderPadding.IEnd(frameWM);
  NS_ASSERTION(aReflowState.AvailableISize() != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  aReflowState.mLineLayout->BeginSpan(this, &aReflowState,
                                      startEdge, endEdge, &mBaseline);

  aStatus = NS_FRAME_COMPLETE;
  for (SegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
    ReflowSegment(aPresContext, aReflowState, e.GetBaseContainer(), aStatus);

    if (NS_INLINE_IS_BREAK(aStatus)) {
      
      
      break;
    }
  }

  ContinuationTraversingState pullState(this);
  while (aStatus == NS_FRAME_COMPLETE) {
    nsRubyBaseContainerFrame* baseContainer = PullOneSegment(pullState);
    if (!baseContainer) {
      
      break;
    }
    ReflowSegment(aPresContext, aReflowState, baseContainer, aStatus);
  }
  
  MOZ_ASSERT(!NS_FRAME_OVERFLOW_IS_INCOMPLETE(aStatus));

  aDesiredSize.ISize(lineWM) = aReflowState.mLineLayout->EndSpan(this);
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize,
                                         borderPadding, lineWM, frameWM);
}

#ifdef DEBUG
static void
SanityCheckRubyPosition(int8_t aRubyPosition)
{
  uint8_t horizontalPosition = aRubyPosition &
    (NS_STYLE_RUBY_POSITION_LEFT | NS_STYLE_RUBY_POSITION_RIGHT);
  MOZ_ASSERT(horizontalPosition == NS_STYLE_RUBY_POSITION_LEFT ||
             horizontalPosition == NS_STYLE_RUBY_POSITION_RIGHT);
  uint8_t verticalPosition = aRubyPosition &
    (NS_STYLE_RUBY_POSITION_OVER | NS_STYLE_RUBY_POSITION_UNDER |
     NS_STYLE_RUBY_POSITION_INTER_CHARACTER);
  MOZ_ASSERT(verticalPosition == NS_STYLE_RUBY_POSITION_OVER ||
             verticalPosition == NS_STYLE_RUBY_POSITION_UNDER ||
             verticalPosition == NS_STYLE_RUBY_POSITION_INTER_CHARACTER);
}
#endif

void
nsRubyFrame::ReflowSegment(nsPresContext* aPresContext,
                           const nsHTMLReflowState& aReflowState,
                           nsRubyBaseContainerFrame* aBaseContainer,
                           nsReflowStatus& aStatus)
{
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalSize availSize(lineWM, aReflowState.AvailableISize(),
                        aReflowState.AvailableBSize());

  nsAutoTArray<nsRubyTextContainerFrame*, RTC_ARRAY_SIZE> textContainers;
  for (RubyTextContainerIterator iter(aBaseContainer); !iter.AtEnd(); iter.Next()) {
    textContainers.AppendElement(iter.GetTextContainer());
  }
  const uint32_t rtcCount = textContainers.Length();

  nsHTMLReflowMetrics baseMetrics(aReflowState);
  bool pushedFrame;
  aReflowState.mLineLayout->ReflowFrame(aBaseContainer, aStatus,
                                        &baseMetrics, pushedFrame);

  if (NS_INLINE_IS_BREAK_BEFORE(aStatus)) {
    if (aBaseContainer != mFrames.FirstChild()) {
      
      
      aStatus = NS_INLINE_LINE_BREAK_AFTER(NS_FRAME_NOT_COMPLETE);
      PushChildren(aBaseContainer, aBaseContainer->GetPrevSibling());
      aReflowState.mLineLayout->SetDirtyNextLine();
    }
    
    
    return;
  }
  if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
    
    
    
    
    MOZ_ASSERT(NS_INLINE_IS_BREAK_AFTER(aStatus));
    
    
    nsIFrame* lastChild;
    if (rtcCount > 0) {
      lastChild = textContainers.LastElement();
    } else {
      lastChild = aBaseContainer;
    }

    
    nsIFrame* newBaseContainer = CreateNextInFlow(aBaseContainer);
    
    
    if (newBaseContainer) {
      
      mFrames.RemoveFrame(newBaseContainer);
      mFrames.InsertFrame(nullptr, lastChild, newBaseContainer);

      
      nsIFrame* newLastChild = newBaseContainer;
      for (uint32_t i = 0; i < rtcCount; i++) {
        nsIFrame* newTextContainer = CreateNextInFlow(textContainers[i]);
        MOZ_ASSERT(newTextContainer, "Next-in-flow of rtc should not exist "
                   "if the corresponding rbc does not");
        mFrames.RemoveFrame(newTextContainer);
        mFrames.InsertFrame(nullptr, newLastChild, newTextContainer);
        newLastChild = newTextContainer;
      }
    }
    if (lastChild != mFrames.LastChild()) {
      
      
      
      PushChildren(lastChild->GetNextSibling(), lastChild);
      aReflowState.mLineLayout->SetDirtyNextLine();
    }
  } else {
    
    
    
    
    for (uint32_t i = 0; i < rtcCount; i++) {
      nsIFrame* nextRTC = textContainers[i]->GetNextInFlow();
      if (nextRTC) {
        nextRTC->GetParent()->DeleteNextInFlowChild(nextRTC, true);
      }
    }
  }

  nscoord segmentISize = baseMetrics.ISize(lineWM);
  nsRect baseRect = aBaseContainer->GetRect();
  
  
  
  
  
  
  
  (lineWM.IsVertical() ? baseRect.x : baseRect.y) = 0;
  
  nsRect offsetRect = baseRect;
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextContainerFrame* textContainer = textContainers[i];
    nsReflowStatus textReflowStatus;
    nsHTMLReflowMetrics textMetrics(aReflowState);
    nsHTMLReflowState textReflowState(aPresContext, aReflowState,
                                      textContainer, availSize);
    
    
    
    
    textReflowState.mLineLayout = aReflowState.mLineLayout;
    textContainer->Reflow(aPresContext, textMetrics,
                          textReflowState, textReflowStatus);
    
    
    
    NS_ASSERTION(textReflowStatus == NS_FRAME_COMPLETE,
                 "Ruby text container must not break itself inside");
    nscoord isize = textMetrics.ISize(lineWM);
    nscoord bsize = textMetrics.BSize(lineWM);
    textContainer->SetSize(LogicalSize(lineWM, isize, bsize));

    nscoord reservedISize = RubyUtils::GetReservedISize(textContainer);
    segmentISize = std::max(segmentISize, isize + reservedISize);

    nscoord x, y;
    uint8_t rubyPosition = textContainer->StyleText()->mRubyPosition;
#ifdef DEBUG
    SanityCheckRubyPosition(rubyPosition);
#endif
    if (lineWM.IsVertical()) {
      
      if (rubyPosition & NS_STYLE_RUBY_POSITION_LEFT) {
        x = offsetRect.X() - bsize;
        offsetRect.SetLeftEdge(x);
      } else {
        x = offsetRect.XMost();
        offsetRect.SetRightEdge(x + bsize);
      }
      y = offsetRect.Y();
    } else {
      
      x = offsetRect.X();
      if (rubyPosition & NS_STYLE_RUBY_POSITION_OVER) {
        y = offsetRect.Y() - bsize;
        offsetRect.SetTopEdge(y);
      } else if (rubyPosition & NS_STYLE_RUBY_POSITION_UNDER) {
        y = offsetRect.YMost();
        offsetRect.SetBottomEdge(y + bsize);
      } else {
        
        MOZ_ASSERT_UNREACHABLE("Unsupported ruby-position");
        y = offsetRect.Y();
      }
    }
    FinishReflowChild(textContainer, aPresContext, textMetrics,
                      &textReflowState, x, y, 0);
  }

  nscoord deltaISize = segmentISize - baseMetrics.ISize(lineWM);
  if (deltaISize <= 0) {
    RubyUtils::ClearReservedISize(aBaseContainer);
  } else {
    RubyUtils::SetReservedISize(aBaseContainer, deltaISize);
    aReflowState.mLineLayout->AdvanceICoord(deltaISize);
  }

  
  LogicalMargin leadings(lineWM, offsetRect - baseRect);
  NS_ASSERTION(leadings.BStart(lineWM) >= 0 && leadings.BEnd(lineWM) >= 0,
               "Leadings should be non-negative (because adding "
               "ruby annotation can only increase the size)");
  mBStartLeading = std::max(mBStartLeading, leadings.BStart(lineWM));
  mBEndLeading = std::max(mBEndLeading, leadings.BEnd(lineWM));
}

nsRubyBaseContainerFrame*
nsRubyFrame::PullOneSegment(ContinuationTraversingState& aState)
{
  
  nsIFrame* baseFrame = PullNextInFlowChild(aState);
  if (!baseFrame) {
    return nullptr;
  }
  MOZ_ASSERT(baseFrame->GetType() == nsGkAtoms::rubyBaseContainerFrame);

  
  nsIFrame* nextFrame;
  while ((nextFrame = GetNextInFlowChild(aState)) != nullptr &&
         nextFrame->GetType() == nsGkAtoms::rubyTextContainerFrame) {
    PullNextInFlowChild(aState);
  }

  return static_cast<nsRubyBaseContainerFrame*>(baseFrame);
}
