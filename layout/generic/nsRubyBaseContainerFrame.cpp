







#include "nsRubyBaseContainerFrame.h"
#include "nsContentUtils.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsStyleStructInlines.h"
#include "WritingModes.h"
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
  nsIFrame* mBaseFrame;
  nsAutoTArray<nsIFrame*, RTC_ARRAY_SIZE> mTextFrames;
  RubyColumn() : mBaseFrame(nullptr) { }
};

class MOZ_STACK_CLASS RubyColumnEnumerator
{
public:
  RubyColumnEnumerator(nsRubyBaseContainerFrame* aRBCFrame,
                       const nsTArray<nsRubyTextContainerFrame*>& aRTCFrames);

  void Next();
  bool AtEnd() const;

  uint32_t GetLevelCount() const { return mFrames.Length(); }
  nsIFrame* GetFrame(uint32_t aIndex) const { return mFrames[aIndex]; }
  nsIFrame* GetBaseFrame() const { return GetFrame(0); }
  nsIFrame* GetTextFrame(uint32_t aIndex) const { return GetFrame(aIndex + 1); }
  void GetColumn(RubyColumn& aColumn) const;

private:
  nsAutoTArray<nsIFrame*, RTC_ARRAY_SIZE + 1> mFrames;
};

RubyColumnEnumerator::RubyColumnEnumerator(
  nsRubyBaseContainerFrame* aBaseContainer,
  const nsTArray<nsRubyTextContainerFrame*>& aTextContainers)
{
  const uint32_t rtcCount = aTextContainers.Length();
  mFrames.SetCapacity(rtcCount + 1);
  mFrames.AppendElement(aBaseContainer->GetFirstPrincipalChild());
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsRubyTextContainerFrame* container = aTextContainers[i];
    
    
    nsIFrame* rtFrame = !container->IsSpanContainer() ?
      aTextContainers[i]->GetFirstPrincipalChild() : nullptr;
    mFrames.AppendElement(rtFrame);
  }
}

void
RubyColumnEnumerator::Next()
{
  for (uint32_t i = 0, iend = mFrames.Length(); i < iend; i++) {
    if (mFrames[i]) {
      mFrames[i] = mFrames[i]->GetNextSibling();
    }
  }
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

void
RubyColumnEnumerator::GetColumn(RubyColumn& aColumn) const
{
  aColumn.mBaseFrame = mFrames[0];
  aColumn.mTextFrames.ClearAndRetainStorage();
  for (uint32_t i = 1, iend = mFrames.Length(); i < iend; i++) {
    aColumn.mTextFrames.AppendElement(mFrames[i]);
  }
}

static nscoord
CalculateColumnPrefISize(nsRenderingContext* aRenderingContext,
                         const RubyColumnEnumerator& aEnumerator)
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
  AutoTextContainerArray textContainers;
  GetTextContainers(textContainers);

  for (uint32_t i = 0, iend = textContainers.Length(); i < iend; i++) {
    if (textContainers[i]->IsSpanContainer()) {
      
      
      aData->currentLine += GetPrefISize(aRenderingContext);
      return;
    }
  }

  nscoord max = 0;
  RubyColumnEnumerator enumerator(this, textContainers);
  for (; !enumerator.AtEnd(); enumerator.Next()) {
    
    
    max = std::max(max, CalculateColumnPrefISize(aRenderingContext,
                                                 enumerator));
  }
  aData->currentLine += max;
}

 void
nsRubyBaseContainerFrame::AddInlinePrefISize(
    nsRenderingContext *aRenderingContext, nsIFrame::InlinePrefISizeData *aData)
{
  AutoTextContainerArray textContainers;
  GetTextContainers(textContainers);

  nscoord sum = 0;
  RubyColumnEnumerator enumerator(this, textContainers);
  for (; !enumerator.AtEnd(); enumerator.Next()) {
    sum += CalculateColumnPrefISize(aRenderingContext, enumerator);
  }
  for (uint32_t i = 0, iend = textContainers.Length(); i < iend; i++) {
    if (textContainers[i]->IsSpanContainer()) {
      nsIFrame* frame = textContainers[i]->GetFirstPrincipalChild();
      sum = std::max(sum, frame->GetPrefISize(aRenderingContext));
    }
  }
  aData->currentLine += sum;
}

 bool 
nsRubyBaseContainerFrame::IsFrameOfType(uint32_t aFlags) const 
{
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
  bool mAllowLineBreak;
  const TextContainerArray& mTextContainers;
  const nsHTMLReflowState& mBaseReflowState;
  const nsTArray<UniquePtr<nsHTMLReflowState>>& mTextReflowStates;
};


static bool
ShouldBreakBefore(const nsHTMLReflowState& aReflowState, nscoord aExtraISize)
{
  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  int32_t offset;
  gfxBreakPriority priority;
  nscoord icoord = lineLayout->GetCurrentICoord();
  return icoord + aExtraISize > aReflowState.AvailableISize() &&
         lineLayout->GetLastOptionalBreakPosition(&offset, &priority);
}

 void
nsRubyBaseContainerFrame::Reflow(nsPresContext* aPresContext,
                                 nsHTMLReflowMetrics& aDesiredSize,
                                 const nsHTMLReflowState& aReflowState,
                                 nsReflowStatus& aStatus)
{
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
  LogicalSize availSize(lineWM, aReflowState.AvailableWidth(),
                        aReflowState.AvailableHeight());

  
  
  
  
  
  
  
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
      aPresContext, *aReflowState.parentReflowState, textContainer, availSize);
    reflowStates.AppendElement(reflowState);
    nsLineLayout* lineLayout = new nsLineLayout(aPresContext,
                                                reflowState->mFloatManager,
                                                reflowState, nullptr,
                                                aReflowState.mLineLayout);
    lineLayouts.AppendElement(lineLayout);

    
    
    
    lineLayout->Init(nullptr, reflowState->CalcLineHeight(), -1);
    reflowState->mLineLayout = lineLayout;

    LogicalMargin borderPadding = reflowState->ComputedLogicalBorderPadding();
    nscoord containerWidth =
      reflowState->ComputedWidth() + borderPadding.LeftRight(lineWM);

    lineLayout->BeginLineReflow(borderPadding.IStart(lineWM),
                                borderPadding.BStart(lineWM),
                                reflowState->ComputedISize(),
                                NS_UNCONSTRAINEDSIZE,
                                false, false, lineWM, containerWidth);
    lineLayout->AttachRootFrameToBaseLineLayout();
  }

  WritingMode frameWM = aReflowState.GetWritingMode();
  LogicalMargin borderPadding = aReflowState.ComputedLogicalBorderPadding();
  nscoord startEdge = borderPadding.IStart(frameWM);
  nscoord endEdge = aReflowState.AvailableISize() - borderPadding.IEnd(frameWM);
  aReflowState.mLineLayout->BeginSpan(this, &aReflowState,
                                      startEdge, endEdge, &mBaseline);

  nsIFrame* parent = GetParent();
  bool inNestedRuby = parent->StyleContext()->IsInlineDescendantOfRuby();
  
  
  bool allowLineBreak = !inNestedRuby && StyleText()->WhiteSpaceCanWrap(this);
  bool allowInitialLineBreak = allowLineBreak;
  if (!GetPrevInFlow()) {
    allowInitialLineBreak = !inNestedRuby &&
      parent->StyleText()->WhiteSpaceCanWrap(parent);
  }
  if (allowInitialLineBreak && aReflowState.mLineLayout->LineIsBreakable() &&
      aReflowState.mLineLayout->NotifyOptionalBreakPosition(
        this, 0, startEdge <= aReflowState.AvailableISize(),
        gfxBreakPriority::eNormalBreak)) {
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  }

  nscoord isize = 0;
  if (aStatus == NS_FRAME_COMPLETE) {
    
    ReflowState reflowState = {
      allowLineBreak && !hasSpan, textContainers, aReflowState, reflowStates
    };
    isize = ReflowColumns(reflowState, aStatus);
  }

  
  
  MOZ_ASSERT(NS_INLINE_IS_BREAK_BEFORE(aStatus) ||
             NS_FRAME_IS_COMPLETE(aStatus) || !hasSpan);
  if (!NS_INLINE_IS_BREAK_BEFORE(aStatus) &&
      NS_FRAME_IS_COMPLETE(aStatus) && hasSpan) {
    
    ReflowState reflowState = {
      false, textContainers, aReflowState, reflowStates
    };
    nscoord spanISize = ReflowSpans(reflowState);
    nscoord deltaISize = spanISize - isize;
    if (deltaISize <= 0) {
      RubyUtils::ClearReservedISize(this);
    } else if (allowLineBreak && ShouldBreakBefore(aReflowState, deltaISize)) {
      aStatus = NS_INLINE_LINE_BREAK_BEFORE();
    } else {
      RubyUtils::SetReservedISize(this, deltaISize);
      aReflowState.mLineLayout->AdvanceICoord(deltaISize);
      isize = spanISize;
    }
    
    
    
    if (!NS_INLINE_IS_BREAK(aStatus) && allowLineBreak &&
        aReflowState.mLineLayout->NotifyOptionalBreakPosition(
          this, INT32_MAX, startEdge + isize <= aReflowState.AvailableISize(),
          gfxBreakPriority::eNormalBreak)) {
      aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    }
  }

  DebugOnly<nscoord> lineSpanSize = aReflowState.mLineLayout->EndSpan(this);
  
  
  
  MOZ_ASSERT(NS_INLINE_IS_BREAK_BEFORE(aStatus) ||
             isize == lineSpanSize || mFrames.IsEmpty());
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
    LogicalSize lineSize(lineWM, isize, lineLayout->GetFinalLineBSize());
    textContainer->SetLineSize(lineSize);
    lineLayout->EndLineReflow();
  }

  aDesiredSize.ISize(lineWM) = isize;
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
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
  nscoord istart = lineLayout->GetCurrentICoord();
  nscoord icoord = istart;
  nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;
  aStatus = NS_FRAME_COMPLETE;

  mColumnCount = 0;
  RubyColumn column;
  column.mTextFrames.SetCapacity(rtcCount);
  RubyColumnEnumerator e(this, aReflowState.mTextContainers);
  for (; !e.AtEnd(); e.Next()) {
    e.GetColumn(column);
    icoord += ReflowOneColumn(aReflowState, column, reflowStatus);
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
    icoord += ReflowOneColumn(aReflowState, column, reflowStatus);
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
    if (!mColumnCount || !aReflowState.mAllowLineBreak) {
      
      
      aStatus = NS_INLINE_LINE_BREAK_BEFORE();
      return 0;
    }
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    MOZ_ASSERT(NS_FRAME_IS_COMPLETE(aStatus) || aReflowState.mAllowLineBreak);

    if (column.mBaseFrame) {
      PushChildren(column.mBaseFrame, column.mBaseFrame->GetPrevSibling());
    }
    for (uint32_t i = 0; i < rtcCount; i++) {
      nsIFrame* textFrame = column.mTextFrames[i];
      if (textFrame) {
        aReflowState.mTextContainers[i]->PushChildren(
          textFrame, textFrame->GetPrevSibling());
      }
    }
  } else if (NS_INLINE_IS_BREAK_AFTER(reflowStatus)) {
    
    
    
    
    MOZ_ASSERT(e.AtEnd());
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
  }

  return icoord - istart;
}

nscoord
nsRubyBaseContainerFrame::ReflowOneColumn(const ReflowState& aReflowState,
                                          const RubyColumn& aColumn,
                                          nsReflowStatus& aStatus)
{
  const nsHTMLReflowState& baseReflowState = aReflowState.mBaseReflowState;
  const auto& textReflowStates = aReflowState.mTextReflowStates;

  WritingMode lineWM = baseReflowState.mLineLayout->GetWritingMode();
  const uint32_t rtcCount = aReflowState.mTextContainers.Length();
  MOZ_ASSERT(aColumn.mTextFrames.Length() == rtcCount);
  MOZ_ASSERT(textReflowStates.Length() == rtcCount);
  nscoord istart = baseReflowState.mLineLayout->GetCurrentICoord();
  nscoord columnISize = 0;

  nsAutoString baseText;
  if (aColumn.mBaseFrame) {
    if (!nsContentUtils::GetNodeTextContent(aColumn.mBaseFrame->GetContent(),
                                            true, baseText)) {
      NS_RUNTIMEABORT("OOM");
    }
  }

  
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsIFrame* textFrame = aColumn.mTextFrames[i];
    if (textFrame) {
      MOZ_ASSERT(textFrame->GetType() == nsGkAtoms::rubyTextFrame);
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

      nsReflowStatus reflowStatus;
      nsHTMLReflowMetrics metrics(*textReflowStates[i]);
      RubyUtils::ClearReservedISize(textFrame);

      bool pushedFrame;
      textReflowStates[i]->mLineLayout->ReflowFrame(textFrame, reflowStatus,
                                                    &metrics, pushedFrame);
      MOZ_ASSERT(!NS_INLINE_IS_BREAK(reflowStatus) && !pushedFrame,
                 "Any line break inside ruby box should has been suppressed");
      columnISize = std::max(columnISize, metrics.ISize(lineWM));
    }
  }
  if (aReflowState.mAllowLineBreak &&
      ShouldBreakBefore(baseReflowState, columnISize)) {
    
    
    
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
    return 0;
  }

  
  if (aColumn.mBaseFrame) {
    MOZ_ASSERT(aColumn.mBaseFrame->GetType() == nsGkAtoms::rubyBaseFrame);
    nsReflowStatus reflowStatus;
    nsHTMLReflowMetrics metrics(baseReflowState);
    RubyUtils::ClearReservedISize(aColumn.mBaseFrame);

    bool pushedFrame;
    baseReflowState.mLineLayout->ReflowFrame(aColumn.mBaseFrame, reflowStatus,
                                             &metrics, pushedFrame);
    MOZ_ASSERT(!NS_INLINE_IS_BREAK(reflowStatus) && !pushedFrame,
               "Any line break inside ruby box should has been suppressed");
    columnISize = std::max(columnISize, metrics.ISize(lineWM));
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
    nsIFrame* textFrame = aColumn.mTextFrames[i];
    nscoord deltaISize = icoord - lineLayout->GetCurrentICoord();
    if (deltaISize > 0) {
      lineLayout->AdvanceICoord(deltaISize);
      if (textFrame) {
        RubyUtils::SetReservedISize(textFrame, deltaISize);
      }
    }
    if (aColumn.mBaseFrame && textFrame) {
      lineLayout->AttachLastFrameToBaseLineLayout();
    }
  }

  mColumnCount++;
  if (aReflowState.mAllowLineBreak &&
      baseReflowState.mLineLayout->NotifyOptionalBreakPosition(
        this, mColumnCount, icoord <= baseReflowState.AvailableISize(),
        gfxBreakPriority::eNormalBreak)) {
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
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

  aColumn.mBaseFrame = PullNextInFlowChild(aPullFrameState.mBase);
  aIsComplete = !aColumn.mBaseFrame;

  aColumn.mTextFrames.ClearAndRetainStorage();
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsIFrame* nextText =
      textContainers[i]->PullNextInFlowChild(aPullFrameState.mTexts[i]);
    aColumn.mTextFrames.AppendElement(nextText);
    
    
    aIsComplete = aIsComplete && !nextText;
  }

  if (!aIsComplete) {
    
    aLineLayout->SetDirtyNextLine();
  }
}

nscoord
nsRubyBaseContainerFrame::ReflowSpans(const ReflowState& aReflowState)
{
  WritingMode lineWM =
    aReflowState.mBaseReflowState.mLineLayout->GetWritingMode();
  nscoord spanISize = 0;

  for (uint32_t i = 0, iend = aReflowState.mTextContainers.Length();
       i < iend; i++) {
    nsRubyTextContainerFrame* container = aReflowState.mTextContainers[i];
    if (!container->IsSpanContainer()) {
      continue;
    }

    nsIFrame* rtFrame = container->GetFirstPrincipalChild();
    nsReflowStatus reflowStatus;
    nsHTMLReflowMetrics metrics(*aReflowState.mTextReflowStates[i]);
    bool pushedFrame;
    aReflowState.mTextReflowStates[i]->mLineLayout->
      ReflowFrame(rtFrame, reflowStatus, &metrics, pushedFrame);
    MOZ_ASSERT(!NS_INLINE_IS_BREAK(reflowStatus) && !pushedFrame,
               "Any line break inside ruby box should has been suppressed");
    spanISize = std::max(spanISize, metrics.ISize(lineWM));
  }

  return spanISize;
}
