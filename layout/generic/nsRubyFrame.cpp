







#include "nsRubyFrame.h"

#include "RubyUtils.h"
#include "mozilla/Maybe.h"
#include "mozilla/WritingModes.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsRubyBaseContainerFrame.h"
#include "nsRubyTextContainerFrame.h"
#include "nsStyleContext.h"

using namespace mozilla;






NS_QUERYFRAME_HEAD(nsRubyFrame)
  NS_QUERYFRAME_ENTRY(nsRubyFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsRubyFrameSuper)

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
  if (aFlags & eBidiInlineContainer) {
    return false;
  }
  return nsRubyFrameSuper::IsFrameOfType(aFlags);
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsRubyFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Ruby"), aResult);
}
#endif

 void
nsRubyFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinISizeData *aData)
{
  for (nsIFrame* frame = this; frame; frame = frame->GetNextInFlow()) {
    for (RubySegmentEnumerator e(static_cast<nsRubyFrame*>(frame));
         !e.AtEnd(); e.Next()) {
      e.GetBaseContainer()->AddInlineMinISize(aRenderingContext, aData);
    }
  }
}

 void
nsRubyFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefISizeData *aData)
{
  for (nsIFrame* frame = this; frame; frame = frame->GetNextInFlow()) {
    for (RubySegmentEnumerator e(static_cast<nsRubyFrame*>(frame));
         !e.AtEnd(); e.Next()) {
      e.GetBaseContainer()->AddInlinePrefISize(aRenderingContext, aData);
    }
  }
}

 void
nsRubyFrame::Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus)
{
  MarkInReflow();
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
  nscoord startEdge = 0;
  const bool boxDecorationBreakClone =
    StyleBorder()->mBoxDecorationBreak == NS_STYLE_BOX_DECORATION_BREAK_CLONE;
  if (boxDecorationBreakClone || !GetPrevContinuation()) {
    startEdge = borderPadding.IStart(frameWM);
  }
  NS_ASSERTION(aReflowState.AvailableISize() != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  nscoord availableISize = aReflowState.AvailableISize();
  availableISize -= startEdge + borderPadding.IEnd(frameWM);
  aReflowState.mLineLayout->BeginSpan(this, &aReflowState,
                                      startEdge, availableISize, &mBaseline);

  aStatus = NS_FRAME_COMPLETE;
  for (RubySegmentEnumerator e(this); !e.AtEnd(); e.Next()) {
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
  if (boxDecorationBreakClone || !GetPrevContinuation()) {
    aDesiredSize.ISize(lineWM) += borderPadding.IStart(frameWM);
  }
  if (boxDecorationBreakClone || NS_FRAME_IS_COMPLETE(aStatus)) {
    aDesiredSize.ISize(lineWM) += borderPadding.IEnd(frameWM);
  }

  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize,
                                         borderPadding, lineWM, frameWM);
}

void
nsRubyFrame::ReflowSegment(nsPresContext* aPresContext,
                           const nsHTMLReflowState& aReflowState,
                           nsRubyBaseContainerFrame* aBaseContainer,
                           nsReflowStatus& aStatus)
{
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalSize availSize(lineWM, aReflowState.AvailableISize(),
                        aReflowState.AvailableBSize());
  WritingMode rubyWM = GetWritingMode();
  NS_ASSERTION(!rubyWM.IsOrthogonalTo(lineWM),
               "Ruby frame writing-mode shouldn't be orthogonal to its line");

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
  LogicalRect baseRect = aBaseContainer->GetLogicalRect(lineWM, 0);
  
  
  
  
  
  
  
  baseRect.BStart(lineWM) = 0;
  
  LogicalRect offsetRect = baseRect;
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextContainerFrame* textContainer = textContainers[i];
    WritingMode rtcWM = textContainer->GetWritingMode();
    nsReflowStatus textReflowStatus;
    nsHTMLReflowMetrics textMetrics(aReflowState);
    nsHTMLReflowState textReflowState(aPresContext, aReflowState, textContainer,
                                      availSize.ConvertTo(rtcWM, lineWM));
    
    
    
    
    textReflowState.mLineLayout = aReflowState.mLineLayout;
    textContainer->Reflow(aPresContext, textMetrics,
                          textReflowState, textReflowStatus);
    
    
    
    NS_ASSERTION(textReflowStatus == NS_FRAME_COMPLETE,
                 "Ruby text container must not break itself inside");
    
    
    LogicalSize size = textMetrics.Size(rubyWM).ConvertTo(lineWM, rubyWM);
    textContainer->SetSize(lineWM, size);

    nscoord reservedISize = RubyUtils::GetReservedISize(textContainer);
    segmentISize = std::max(segmentISize, size.ISize(lineWM) + reservedISize);

    uint8_t rubyPosition = textContainer->StyleText()->mRubyPosition;
    MOZ_ASSERT(rubyPosition == NS_STYLE_RUBY_POSITION_OVER ||
               rubyPosition == NS_STYLE_RUBY_POSITION_UNDER);
    Maybe<LogicalSide> side;
    if (rubyPosition == NS_STYLE_RUBY_POSITION_OVER) {
      side.emplace(lineWM.LogicalSideForLineRelativeDir(eLineRelativeDirOver));
    } else if (rubyPosition == NS_STYLE_RUBY_POSITION_UNDER) {
      side.emplace(lineWM.LogicalSideForLineRelativeDir(eLineRelativeDirUnder));
    } else {
      
      MOZ_ASSERT_UNREACHABLE("Unsupported ruby-position");
    }

    LogicalPoint position(lineWM);
    if (side.isSome()) {
      if (side.value() == eLogicalSideBStart) {
        offsetRect.BStart(lineWM) -= size.BSize(lineWM);
        offsetRect.BSize(lineWM) += size.BSize(lineWM);
        position = offsetRect.Origin(lineWM);
      } else if (side.value() == eLogicalSideBEnd) {
        position = offsetRect.Origin(lineWM) +
          LogicalPoint(lineWM, 0, offsetRect.BSize(lineWM));
        offsetRect.BSize(lineWM) += size.BSize(lineWM);
      } else {
        MOZ_ASSERT_UNREACHABLE("???");
      }
    }
    
    
    FinishReflowChild(textContainer, aPresContext, textMetrics,
                      &textReflowState, lineWM, position, 0, 0);
  }
  MOZ_ASSERT(baseRect.ISize(lineWM) == offsetRect.ISize(lineWM),
             "Annotations should only be placed on the block directions");

  nscoord deltaISize = segmentISize - baseMetrics.ISize(lineWM);
  if (deltaISize <= 0) {
    RubyUtils::ClearReservedISize(aBaseContainer);
  } else {
    RubyUtils::SetReservedISize(aBaseContainer, deltaISize);
    aReflowState.mLineLayout->AdvanceICoord(deltaISize);
  }

  
  nscoord startLeading = baseRect.BStart(lineWM) - offsetRect.BStart(lineWM);
  nscoord endLeading = offsetRect.BEnd(lineWM) - baseRect.BEnd(lineWM);
  
  NS_WARN_IF_FALSE(startLeading >= 0 && endLeading >= 0,
                   "Leadings should be non-negative (because adding "
                   "ruby annotation can only increase the size)");
  mBStartLeading = std::max(mBStartLeading, startLeading);
  mBEndLeading = std::max(mBEndLeading, endLeading);
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
