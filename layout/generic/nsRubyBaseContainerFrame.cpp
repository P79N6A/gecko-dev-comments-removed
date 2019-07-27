







#include "nsRubyBaseContainerFrame.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/Maybe.h"
#include "mozilla/WritingModes.h"
#include "nsContentUtils.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleStructInlines.h"
#include "nsTextFrame.h"
#include "RubyUtils.h"

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






struct MOZ_STACK_CLASS mozilla::RubyColumn
{
  nsRubyBaseFrame* mBaseFrame;
  nsAutoTArray<nsRubyTextFrame*, RTC_ARRAY_SIZE> mTextFrames;
  bool mIsIntraLevelWhitespace;
  RubyColumn() : mBaseFrame(nullptr), mIsIntraLevelWhitespace(false) { }
};

class MOZ_STACK_CLASS RubyColumnEnumerator
{
public:
  RubyColumnEnumerator(nsRubyBaseContainerFrame* aRBCFrame,
                       const nsTArray<nsRubyTextContainerFrame*>& aRTCFrames);

  void Next();
  bool AtEnd() const;

  uint32_t GetLevelCount() const { return mFrames.Length(); }
  nsRubyContentFrame* GetFrameAtLevel(uint32_t aIndex) const;
  void GetColumn(RubyColumn& aColumn) const;

private:
  
  
  
  nsAutoTArray<nsRubyContentFrame*, RTC_ARRAY_SIZE + 1> mFrames;
  
  bool mAtIntraLevelWhitespace;
};

RubyColumnEnumerator::RubyColumnEnumerator(
  nsRubyBaseContainerFrame* aBaseContainer,
  const nsTArray<nsRubyTextContainerFrame*>& aTextContainers)
  : mAtIntraLevelWhitespace(false)
{
  const uint32_t rtcCount = aTextContainers.Length();
  mFrames.SetCapacity(rtcCount + 1);

  nsIFrame* rbFrame = aBaseContainer->GetFirstPrincipalChild();
  MOZ_ASSERT(!rbFrame || rbFrame->GetType() == nsGkAtoms::rubyBaseFrame);
  mFrames.AppendElement(static_cast<nsRubyContentFrame*>(rbFrame));
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextContainerFrame* container = aTextContainers[i];
    
    
    nsIFrame* rtFrame = !container->IsSpanContainer() ?
      container->GetFirstPrincipalChild() : nullptr;
    MOZ_ASSERT(!rtFrame || rtFrame->GetType() == nsGkAtoms::rubyTextFrame);
    mFrames.AppendElement(static_cast<nsRubyContentFrame*>(rtFrame));
  }

  
  
  
  
  
  
  
  
  
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    nsRubyContentFrame* frame = mFrames[i];
    if (frame && frame->IsIntraLevelWhitespace()) {
      mAtIntraLevelWhitespace = true;
      break;
    }
  }
}

void
RubyColumnEnumerator::Next()
{
  bool advancingToIntraLevelWhitespace = false;
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    nsRubyContentFrame* frame = mFrames[i];
    
    
    
    
    
    if (frame && (!mAtIntraLevelWhitespace ||
                  frame->IsIntraLevelWhitespace())) {
      nsIFrame* nextSibling = frame->GetNextSibling();
      MOZ_ASSERT(!nextSibling || nextSibling->GetType() == frame->GetType(),
                 "Frame type should be identical among a level");
      mFrames[i] = frame = static_cast<nsRubyContentFrame*>(nextSibling);
      if (!advancingToIntraLevelWhitespace &&
          frame && frame->IsIntraLevelWhitespace()) {
        advancingToIntraLevelWhitespace = true;
      }
    }
  }
  MOZ_ASSERT(!advancingToIntraLevelWhitespace || !mAtIntraLevelWhitespace,
             "Should never have adjacent intra-level whitespace columns");
  mAtIntraLevelWhitespace = advancingToIntraLevelWhitespace;
}

bool
RubyColumnEnumerator::AtEnd() const
{
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    if (mFrames[i]) {
      return false;
    }
  }
  return true;
}

nsRubyContentFrame*
RubyColumnEnumerator::GetFrameAtLevel(uint32_t aIndex) const
{
  
  
  
  
  
  
  nsRubyContentFrame* frame = mFrames[aIndex];
  return !mAtIntraLevelWhitespace ||
         (frame && frame->IsIntraLevelWhitespace()) ? frame : nullptr;
}

void
RubyColumnEnumerator::GetColumn(RubyColumn& aColumn) const
{
  nsRubyContentFrame* rbFrame = GetFrameAtLevel(0);
  MOZ_ASSERT(!rbFrame || rbFrame->GetType() == nsGkAtoms::rubyBaseFrame);
  aColumn.mBaseFrame = static_cast<nsRubyBaseFrame*>(rbFrame);
  aColumn.mTextFrames.ClearAndRetainStorage();
  for (uint32_t i = 1, iend = mFrames.Length(); i < iend; i++) {
    nsRubyContentFrame* rtFrame = GetFrameAtLevel(i);
    MOZ_ASSERT(!rtFrame || rtFrame->GetType() == nsGkAtoms::rubyTextFrame);
    aColumn.mTextFrames.AppendElement(static_cast<nsRubyTextFrame*>(rtFrame));
  }
  aColumn.mIsIntraLevelWhitespace = mAtIntraLevelWhitespace;
}

static gfxBreakPriority
LineBreakBefore(nsIFrame* aFrame,
                nsRenderingContext* aRenderingContext,
                nsIFrame* aLineContainerFrame,
                const nsLineList::iterator* aLine)
{
  for (nsIFrame* child = aFrame; child;
       child = child->GetFirstPrincipalChild()) {
    if (!child->CanContinueTextRun()) {
      
      return gfxBreakPriority::eNormalBreak;
    }
    if (child->GetType() != nsGkAtoms::textFrame) {
      continue;
    }

    auto textFrame = static_cast<nsTextFrame*>(child);
    gfxSkipCharsIterator iter =
      textFrame->EnsureTextRun(nsTextFrame::eInflated,
                               aRenderingContext->ThebesContext(),
                               aLineContainerFrame, aLine);
    iter.SetOriginalOffset(textFrame->GetContentOffset());
    uint32_t pos = iter.GetSkippedOffset();
    gfxTextRun* textRun = textFrame->GetTextRun(nsTextFrame::eInflated);
    if (pos >= textRun->GetLength()) {
      
      return gfxBreakPriority::eNoBreak;
    }
    
    if (textRun->CanBreakLineBefore(pos)) {
      return gfxBreakPriority::eNormalBreak;
    }
    
    const nsStyleText* textStyle = textFrame->StyleText();
    if (textStyle->WordCanWrap(textFrame) && textRun->IsClusterStart(pos)) {
      return gfxBreakPriority::eWordWrapBreak;
    }
    
    return gfxBreakPriority::eNoBreak;
  }
  
  
  return gfxBreakPriority::eNoBreak;
}

static void
GetIsLineBreakAllowed(nsIFrame* aFrame, bool aIsLineBreakable,
                      bool* aAllowInitialLineBreak, bool* aAllowLineBreak)
{
  nsIFrame* parent = aFrame->GetParent();
  bool lineBreakSuppressed = parent->StyleContext()->ShouldSuppressLineBreak();
  
  
  bool allowLineBreak = !lineBreakSuppressed &&
                        aFrame->StyleText()->WhiteSpaceCanWrap(aFrame);
  bool allowInitialLineBreak = allowLineBreak;
  if (!aFrame->GetPrevInFlow()) {
    allowInitialLineBreak = !lineBreakSuppressed &&
                            parent->StyleText()->WhiteSpaceCanWrap(parent);
  }
  if (!aIsLineBreakable) {
    allowInitialLineBreak = false;
  }
  *aAllowInitialLineBreak = allowInitialLineBreak;
  *aAllowLineBreak = allowLineBreak;
}

static nscoord
CalculateColumnPrefISize(nsRenderingContext* aRenderingContext,
                         const RubyColumnEnumerator& aEnumerator)
{
  nscoord max = 0;
  uint32_t levelCount = aEnumerator.GetLevelCount();
  for (uint32_t i = 0; i < levelCount; i++) {
    nsIFrame* frame = aEnumerator.GetFrameAtLevel(i);
    if (frame) {
      nsIFrame::InlinePrefISizeData data;
      frame->AddInlinePrefISize(aRenderingContext, &data);
      MOZ_ASSERT(data.prevLines == 0, "Shouldn't have prev lines");
      max = std::max(max, data.currentLine);
    }
  }
  return max;
}




 void
nsRubyBaseContainerFrame::AddInlineMinISize(
  nsRenderingContext *aRenderingContext, nsIFrame::InlineMinISizeData *aData)
{
  AutoTextContainerArray textContainers;
  GetTextContainers(textContainers);

  for (uint32_t i = 0, iend = textContainers.Length(); i < iend; i++) {
    if (textContainers[i]->IsSpanContainer()) {
      
      
      nsIFrame::InlinePrefISizeData data;
      AddInlinePrefISize(aRenderingContext, &data);
      aData->currentLine += data.currentLine;
      if (data.currentLine > 0) {
        aData->atStartOfLine = false;
      }
      return;
    }
  }

  bool firstFrame = true;
  bool allowInitialLineBreak, allowLineBreak;
  GetIsLineBreakAllowed(this, !aData->atStartOfLine,
                        &allowInitialLineBreak, &allowLineBreak);
  for (nsIFrame* frame = this; frame; frame = frame->GetNextInFlow()) {
    RubyColumnEnumerator enumerator(
      static_cast<nsRubyBaseContainerFrame*>(frame), textContainers);
    for (; !enumerator.AtEnd(); enumerator.Next()) {
      if (firstFrame ? allowInitialLineBreak : allowLineBreak) {
        nsIFrame* baseFrame = enumerator.GetFrameAtLevel(0);
        if (baseFrame) {
          gfxBreakPriority breakPriority =
            LineBreakBefore(baseFrame, aRenderingContext, nullptr, nullptr);
          if (breakPriority != gfxBreakPriority::eNoBreak) {
            aData->OptionallyBreak(aRenderingContext);
          }
        }
      }
      firstFrame = false;
      nscoord isize = CalculateColumnPrefISize(aRenderingContext, enumerator);
      aData->currentLine += isize;
      if (isize > 0) {
        aData->atStartOfLine = false;
      }
    }
  }
}

 void
nsRubyBaseContainerFrame::AddInlinePrefISize(
  nsRenderingContext *aRenderingContext, nsIFrame::InlinePrefISizeData *aData)
{
  AutoTextContainerArray textContainers;
  GetTextContainers(textContainers);

  nscoord sum = 0;
  for (nsIFrame* frame = this; frame; frame = frame->GetNextInFlow()) {
    RubyColumnEnumerator enumerator(
      static_cast<nsRubyBaseContainerFrame*>(frame), textContainers);
    for (; !enumerator.AtEnd(); enumerator.Next()) {
      sum += CalculateColumnPrefISize(aRenderingContext, enumerator);
    }
  }
  for (uint32_t i = 0, iend = textContainers.Length(); i < iend; i++) {
    if (textContainers[i]->IsSpanContainer()) {
      nsIFrame* frame = textContainers[i]->GetFirstPrincipalChild();
      nsIFrame::InlinePrefISizeData data;
      frame->AddInlinePrefISize(aRenderingContext, &data);
      MOZ_ASSERT(data.prevLines == 0, "Shouldn't have prev lines");
      sum = std::max(sum, data.currentLine);
    }
  }
  aData->currentLine += sum;
}

 bool 
nsRubyBaseContainerFrame::IsFrameOfType(uint32_t aFlags) const 
{
  if (aFlags & eSupportsCSSTransforms) {
    return false;
  }
  return nsContainerFrame::IsFrameOfType(aFlags &
         ~(nsIFrame::eLineParticipant));
}

void
nsRubyBaseContainerFrame::GetTextContainers(TextContainerArray& aTextContainers)
{
  MOZ_ASSERT(aTextContainers.IsEmpty());
  for (RubyTextContainerIterator iter(this); !iter.AtEnd(); iter.Next()) {
    aTextContainers.AppendElement(iter.GetTextContainer());
  }
}

 bool
nsRubyBaseContainerFrame::CanContinueTextRun() const
{
  return true;
}

 LogicalSize
nsRubyBaseContainerFrame::ComputeSize(nsRenderingContext *aRenderingContext,
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
nsRubyBaseContainerFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  return mBaseline;
}

struct nsRubyBaseContainerFrame::ReflowState
{
  bool mAllowInitialLineBreak;
  bool mAllowLineBreak;
  const TextContainerArray& mTextContainers;
  const nsHTMLReflowState& mBaseReflowState;
  const nsTArray<UniquePtr<nsHTMLReflowState>>& mTextReflowStates;
};

 void
nsRubyBaseContainerFrame::Reflow(nsPresContext* aPresContext,
                                 nsHTMLReflowMetrics& aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus& aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsRubyBaseContainerFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aDesiredSize, aStatus);
  aStatus = NS_FRAME_COMPLETE;

  if (!aReflowState.mLineLayout) {
    NS_ASSERTION(
      aReflowState.mLineLayout,
      "No line layout provided to RubyBaseContainerFrame reflow method.");
    return;
  }

  AutoTextContainerArray textContainers;
  GetTextContainers(textContainers);

  MoveOverflowToChildList();
  
  const uint32_t rtcCount = textContainers.Length();
  for (uint32_t i = 0; i < rtcCount; i++) {
    textContainers[i]->MoveOverflowToChildList();
  }

  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalSize availSize(lineWM, aReflowState.AvailableISize(),
                        aReflowState.AvailableBSize());

  
  
  
  
  
  
  
  nsAutoTArray<UniquePtr<nsHTMLReflowState>, RTC_ARRAY_SIZE> reflowStates;
  nsAutoTArray<UniquePtr<nsLineLayout>, RTC_ARRAY_SIZE> lineLayouts;
  reflowStates.SetCapacity(rtcCount);
  lineLayouts.SetCapacity(rtcCount);

  
  bool hasSpan = false;
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextContainerFrame* textContainer = textContainers[i];
    if (textContainer->IsSpanContainer()) {
      hasSpan = true;
    }

    nsHTMLReflowState* reflowState = new nsHTMLReflowState(
      aPresContext, *aReflowState.parentReflowState, textContainer,
      availSize.ConvertTo(textContainer->GetWritingMode(), lineWM));
    reflowStates.AppendElement(reflowState);
    nsLineLayout* lineLayout = new nsLineLayout(aPresContext,
                                                reflowState->mFloatManager,
                                                reflowState, nullptr,
                                                aReflowState.mLineLayout);
    lineLayout->SetSuppressLineWrap(true);
    lineLayouts.AppendElement(lineLayout);

    
    
    
    lineLayout->Init(nullptr, reflowState->CalcLineHeight(), -1);
    reflowState->mLineLayout = lineLayout;

    
    
    
    
    lineLayout->BeginLineReflow(0, 0, reflowState->ComputedISize(),
                                NS_UNCONSTRAINEDSIZE,
                                false, false, lineWM, nsSize(0, 0));
    lineLayout->AttachRootFrameToBaseLineLayout();
  }

  aReflowState.mLineLayout->BeginSpan(this, &aReflowState,
                                      0, aReflowState.AvailableISize(),
                                      &mBaseline);

  bool allowInitialLineBreak, allowLineBreak;
  GetIsLineBreakAllowed(this, aReflowState.mLineLayout->LineIsBreakable(),
                        &allowInitialLineBreak, &allowLineBreak);

  nscoord isize = 0;
  
  ReflowState reflowState = {
    allowInitialLineBreak, allowLineBreak && !hasSpan,
    textContainers, aReflowState, reflowStates
  };
  isize = ReflowColumns(reflowState, aStatus);
  DebugOnly<nscoord> lineSpanSize = aReflowState.mLineLayout->EndSpan(this);
  aDesiredSize.ISize(lineWM) = isize;
  
  
  
  
  NS_WARN_IF_FALSE(NS_INLINE_IS_BREAK(aStatus) ||
                   isize == lineSpanSize || mFrames.IsEmpty(), "bad isize");

  
  
  MOZ_ASSERT(NS_INLINE_IS_BREAK_BEFORE(aStatus) ||
             NS_FRAME_IS_COMPLETE(aStatus) || !hasSpan);
  if (!NS_INLINE_IS_BREAK_BEFORE(aStatus) &&
      NS_FRAME_IS_COMPLETE(aStatus) && hasSpan) {
    
    ReflowState reflowState = {
      false, false, textContainers, aReflowState, reflowStates
    };
    nscoord spanISize = ReflowSpans(reflowState);
    isize = std::max(isize, spanISize);
    if (isize > aReflowState.AvailableISize() &&
        aReflowState.mLineLayout->HasOptionalBreakPosition()) {
      aStatus = NS_INLINE_LINE_BREAK_BEFORE();
    }
  }

  for (uint32_t i = 0; i < rtcCount; i++) {
    
    
    nsRubyTextContainerFrame* textContainer = textContainers[i];
    nsLineLayout* lineLayout = lineLayouts[i].get();

    RubyUtils::ClearReservedISize(textContainer);
    nscoord rtcISize = lineLayout->GetCurrentICoord();
    
    
    
    
    if (!textContainer->IsSpanContainer()) {
      rtcISize = isize;
    } else if (isize > rtcISize) {
      RubyUtils::SetReservedISize(textContainer, isize - rtcISize);
    }

    lineLayout->VerticalAlignLine();
    textContainer->SetISize(rtcISize);
    lineLayout->EndLineReflow();
  }

  
  
  WritingMode frameWM = aReflowState.GetWritingMode();
  LogicalMargin borderPadding(frameWM);
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize,
                                         borderPadding, lineWM, frameWM);
}






struct MOZ_STACK_CLASS nsRubyBaseContainerFrame::PullFrameState
{
  ContinuationTraversingState mBase;
  nsAutoTArray<ContinuationTraversingState, RTC_ARRAY_SIZE> mTexts;
  const TextContainerArray& mTextContainers;

  PullFrameState(nsRubyBaseContainerFrame* aBaseContainer,
                 const TextContainerArray& aTextContainers);
};

nscoord
nsRubyBaseContainerFrame::ReflowColumns(const ReflowState& aReflowState,
                                        nsReflowStatus& aStatus)
{
  nsLineLayout* lineLayout = aReflowState.mBaseReflowState.mLineLayout;
  const uint32_t rtcCount = aReflowState.mTextContainers.Length();
  nscoord icoord = lineLayout->GetCurrentICoord();
  MOZ_ASSERT(icoord == 0, "border/padding of rbc should have been suppressed");
  nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;
  aStatus = NS_FRAME_COMPLETE;

  uint32_t columnIndex = 0;
  RubyColumn column;
  column.mTextFrames.SetCapacity(rtcCount);
  RubyColumnEnumerator e(this, aReflowState.mTextContainers);
  for (; !e.AtEnd(); e.Next()) {
    e.GetColumn(column);
    icoord += ReflowOneColumn(aReflowState, columnIndex, column, reflowStatus);
    if (!NS_INLINE_IS_BREAK_BEFORE(reflowStatus)) {
      columnIndex++;
    }
    if (NS_INLINE_IS_BREAK(reflowStatus)) {
      break;
    }
    
    MOZ_ASSERT(reflowStatus == NS_FRAME_COMPLETE);
  }

  bool isComplete = false;
  PullFrameState pullFrameState(this, aReflowState.mTextContainers);
  while (!NS_INLINE_IS_BREAK(reflowStatus)) {
    
    MOZ_ASSERT(reflowStatus == NS_FRAME_COMPLETE);

    
    
    PullOneColumn(lineLayout, pullFrameState, column, isComplete);
    if (isComplete) {
      
      break;
    }
    icoord += ReflowOneColumn(aReflowState, columnIndex, column, reflowStatus);
    if (!NS_INLINE_IS_BREAK_BEFORE(reflowStatus)) {
      columnIndex++;
    }
  }

  if (!e.AtEnd() && NS_INLINE_IS_BREAK_AFTER(reflowStatus)) {
    
    
    e.Next();
    e.GetColumn(column);
    reflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
  }
  if (!e.AtEnd() || (GetNextInFlow() && !isComplete)) {
    NS_FRAME_SET_INCOMPLETE(aStatus);
  }

  if (NS_INLINE_IS_BREAK_BEFORE(reflowStatus)) {
    if (!columnIndex || !aReflowState.mAllowLineBreak) {
      
      
      aStatus = NS_INLINE_LINE_BREAK_BEFORE();
      return 0;
    }
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    MOZ_ASSERT(NS_FRAME_IS_COMPLETE(aStatus) || aReflowState.mAllowLineBreak);

    
    
    
    
    
    
    Maybe<RubyColumn> nextColumn;
    if (column.mIsIntraLevelWhitespace && !e.AtEnd()) {
      e.Next();
      nextColumn.emplace();
      e.GetColumn(nextColumn.ref());
    }
    nsIFrame* baseFrame = column.mBaseFrame;
    if (!baseFrame & nextColumn.isSome()) {
      baseFrame = nextColumn->mBaseFrame;
    }
    if (baseFrame) {
      PushChildren(baseFrame, baseFrame->GetPrevSibling());
    }
    for (uint32_t i = 0; i < rtcCount; i++) {
      nsRubyTextFrame* textFrame = column.mTextFrames[i];
      if (!textFrame && nextColumn.isSome()) {
        textFrame = nextColumn->mTextFrames[i];
      }
      if (textFrame) {
        aReflowState.mTextContainers[i]->PushChildren(
          textFrame, textFrame->GetPrevSibling());
      }
    }
  } else if (NS_INLINE_IS_BREAK_AFTER(reflowStatus)) {
    
    
    
    
    MOZ_ASSERT(e.AtEnd());
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
  }

  return icoord;
}

nscoord
nsRubyBaseContainerFrame::ReflowOneColumn(const ReflowState& aReflowState,
                                          uint32_t aColumnIndex,
                                          const RubyColumn& aColumn,
                                          nsReflowStatus& aStatus)
{
  const nsHTMLReflowState& baseReflowState = aReflowState.mBaseReflowState;
  const auto& textReflowStates = aReflowState.mTextReflowStates;
  nscoord istart = baseReflowState.mLineLayout->GetCurrentICoord();

  if (aColumn.mBaseFrame) {
    bool allowBreakBefore = aColumnIndex ?
      aReflowState.mAllowLineBreak : aReflowState.mAllowInitialLineBreak;
    if (allowBreakBefore) {
      gfxBreakPriority breakPriority = LineBreakBefore(
        aColumn.mBaseFrame, baseReflowState.rendContext,
        baseReflowState.mLineLayout->LineContainerFrame(),
        baseReflowState.mLineLayout->GetLine());
      if (breakPriority != gfxBreakPriority::eNoBreak) {
        gfxBreakPriority lastBreakPriority =
          baseReflowState.mLineLayout->LastOptionalBreakPriority();
        if (breakPriority >= lastBreakPriority) {
          
          
          if (istart > baseReflowState.AvailableISize() ||
              baseReflowState.mLineLayout->NotifyOptionalBreakPosition(
                aColumn.mBaseFrame, 0, true, breakPriority)) {
            aStatus = NS_INLINE_LINE_BREAK_BEFORE();
            return 0;
          }
        }
      }
    }
  }

  const uint32_t rtcCount = aReflowState.mTextContainers.Length();
  MOZ_ASSERT(aColumn.mTextFrames.Length() == rtcCount);
  MOZ_ASSERT(textReflowStates.Length() == rtcCount);
  nscoord columnISize = 0;

  nsAutoString baseText;
  if (aColumn.mBaseFrame) {
    if (!nsContentUtils::GetNodeTextContent(aColumn.mBaseFrame->GetContent(),
                                            true, baseText)) {
      NS_RUNTIMEABORT("OOM");
    }
  }

  
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextFrame* textFrame = aColumn.mTextFrames[i];
    if (textFrame) {
      nsAutoString annotationText;
      if (!nsContentUtils::GetNodeTextContent(textFrame->GetContent(),
                                              true, annotationText)) {
        NS_RUNTIMEABORT("OOM");
      }
      
      
      
      
      
      if (annotationText.Equals(baseText)) {
        textFrame->AddStateBits(NS_RUBY_TEXT_FRAME_AUTOHIDE);
      } else {
        textFrame->RemoveStateBits(NS_RUBY_TEXT_FRAME_AUTOHIDE);
      }
      RubyUtils::ClearReservedISize(textFrame);

      bool pushedFrame;
      nsReflowStatus reflowStatus;
      nsLineLayout* lineLayout = textReflowStates[i]->mLineLayout;
      nscoord textIStart = lineLayout->GetCurrentICoord();
      lineLayout->ReflowFrame(textFrame, reflowStatus, nullptr, pushedFrame);
      if (MOZ_UNLIKELY(NS_INLINE_IS_BREAK(reflowStatus) || pushedFrame)) {
        MOZ_ASSERT_UNREACHABLE(
            "Any line break inside ruby box should have been suppressed");
        
        
        textFrame->DrainSelfOverflowList();
      }
      nscoord textISize = lineLayout->GetCurrentICoord() - textIStart;
      columnISize = std::max(columnISize, textISize);
    }
  }

  
  if (aColumn.mBaseFrame) {
    RubyUtils::ClearReservedISize(aColumn.mBaseFrame);

    bool pushedFrame;
    nsReflowStatus reflowStatus;
    nsLineLayout* lineLayout = baseReflowState.mLineLayout;
    nscoord baseIStart = lineLayout->GetCurrentICoord();
    lineLayout->ReflowFrame(aColumn.mBaseFrame, reflowStatus,
                            nullptr, pushedFrame);
    if (MOZ_UNLIKELY(NS_INLINE_IS_BREAK(reflowStatus) || pushedFrame)) {
      MOZ_ASSERT_UNREACHABLE(
        "Any line break inside ruby box should have been suppressed");
      
      
      aColumn.mBaseFrame->DrainSelfOverflowList();
    }
    nscoord baseISize = lineLayout->GetCurrentICoord() - baseIStart;
    columnISize = std::max(columnISize, baseISize);
  }

  
  nscoord icoord = istart + columnISize;
  nscoord deltaISize = icoord - baseReflowState.mLineLayout->GetCurrentICoord();
  if (deltaISize > 0) {
    baseReflowState.mLineLayout->AdvanceICoord(deltaISize);
    if (aColumn.mBaseFrame) {
      RubyUtils::SetReservedISize(aColumn.mBaseFrame, deltaISize);
    }
  }
  for (uint32_t i = 0; i < rtcCount; i++) {
    if (aReflowState.mTextContainers[i]->IsSpanContainer()) {
      continue;
    }
    nsLineLayout* lineLayout = textReflowStates[i]->mLineLayout;
    nsRubyTextFrame* textFrame = aColumn.mTextFrames[i];
    nscoord deltaISize = icoord - lineLayout->GetCurrentICoord();
    if (deltaISize > 0) {
      lineLayout->AdvanceICoord(deltaISize);
      if (textFrame && !textFrame->IsAutoHidden()) {
        RubyUtils::SetReservedISize(textFrame, deltaISize);
      }
    }
    if (aColumn.mBaseFrame && textFrame) {
      lineLayout->AttachLastFrameToBaseLineLayout();
    }
  }

  return columnISize;
}

nsRubyBaseContainerFrame::PullFrameState::PullFrameState(
    nsRubyBaseContainerFrame* aBaseContainer,
    const TextContainerArray& aTextContainers)
  : mBase(aBaseContainer)
  , mTextContainers(aTextContainers)
{
  const uint32_t rtcCount = aTextContainers.Length();
  for (uint32_t i = 0; i < rtcCount; i++) {
    mTexts.AppendElement(aTextContainers[i]);
  }
}

void
nsRubyBaseContainerFrame::PullOneColumn(nsLineLayout* aLineLayout,
                                        PullFrameState& aPullFrameState,
                                        RubyColumn& aColumn,
                                        bool& aIsComplete)
{
  const TextContainerArray& textContainers = aPullFrameState.mTextContainers;
  const uint32_t rtcCount = textContainers.Length();

  nsIFrame* nextBase = GetNextInFlowChild(aPullFrameState.mBase);
  MOZ_ASSERT(!nextBase || nextBase->GetType() == nsGkAtoms::rubyBaseFrame);
  aColumn.mBaseFrame = static_cast<nsRubyBaseFrame*>(nextBase);
  aIsComplete = !aColumn.mBaseFrame;
  bool pullingIntraLevelWhitespace =
    aColumn.mBaseFrame && aColumn.mBaseFrame->IsIntraLevelWhitespace();

  aColumn.mTextFrames.ClearAndRetainStorage();
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsIFrame* nextText =
      textContainers[i]->GetNextInFlowChild(aPullFrameState.mTexts[i]);
    MOZ_ASSERT(!nextText || nextText->GetType() == nsGkAtoms::rubyTextFrame);
    nsRubyTextFrame* textFrame = static_cast<nsRubyTextFrame*>(nextText);
    aColumn.mTextFrames.AppendElement(textFrame);
    
    
    aIsComplete = aIsComplete && !nextText;
    if (nextText && !pullingIntraLevelWhitespace) {
      pullingIntraLevelWhitespace = textFrame->IsIntraLevelWhitespace();
    }
  }

  aColumn.mIsIntraLevelWhitespace = pullingIntraLevelWhitespace;
  if (pullingIntraLevelWhitespace) {
    
    
    
    if (aColumn.mBaseFrame && !aColumn.mBaseFrame->IsIntraLevelWhitespace()) {
      aColumn.mBaseFrame = nullptr;
    }
    for (uint32_t i = 0; i < rtcCount; i++) {
      nsRubyTextFrame*& textFrame = aColumn.mTextFrames[i];
      if (textFrame && !textFrame->IsIntraLevelWhitespace()) {
        textFrame = nullptr;
      }
    }
  }

  
  if (aColumn.mBaseFrame) {
    DebugOnly<nsIFrame*> pulled = PullNextInFlowChild(aPullFrameState.mBase);
    MOZ_ASSERT(pulled == aColumn.mBaseFrame, "pulled a wrong frame?");
  }
  for (uint32_t i = 0; i < rtcCount; i++) {
    if (aColumn.mTextFrames[i]) {
      DebugOnly<nsIFrame*> pulled =
        textContainers[i]->PullNextInFlowChild(aPullFrameState.mTexts[i]);
      MOZ_ASSERT(pulled == aColumn.mTextFrames[i], "pulled a wrong frame?");
    }
  }

  if (!aIsComplete) {
    
    aLineLayout->SetDirtyNextLine();
  }
}

nscoord
nsRubyBaseContainerFrame::ReflowSpans(const ReflowState& aReflowState)
{
  nscoord spanISize = 0;
  for (uint32_t i = 0, iend = aReflowState.mTextContainers.Length();
       i < iend; i++) {
    nsRubyTextContainerFrame* container = aReflowState.mTextContainers[i];
    if (!container->IsSpanContainer()) {
      continue;
    }

    nsIFrame* rtFrame = container->GetFirstPrincipalChild();
    nsReflowStatus reflowStatus;
    bool pushedFrame;
    nsLineLayout* lineLayout = aReflowState.mTextReflowStates[i]->mLineLayout;
    MOZ_ASSERT(lineLayout->GetCurrentICoord() == 0,
               "border/padding of rtc should have been suppressed");
    lineLayout->ReflowFrame(rtFrame, reflowStatus, nullptr, pushedFrame);
    MOZ_ASSERT(!NS_INLINE_IS_BREAK(reflowStatus) && !pushedFrame,
               "Any line break inside ruby box should has been suppressed");
    spanISize = std::max(spanISize, lineLayout->GetCurrentICoord());
  }
  return spanISize;
}
