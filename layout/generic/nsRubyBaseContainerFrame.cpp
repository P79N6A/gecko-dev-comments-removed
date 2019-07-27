







#include "nsRubyBaseContainerFrame.h"
#include "nsContentUtils.h"
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
  void GetFrames(nsIFrame*& aBaseFrame, nsTArray<nsIFrame*>& aTextFrames) const;

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

void
PairEnumerator::GetFrames(nsIFrame*& aBaseFrame,
                          nsTArray<nsIFrame*>& aTextFrames) const
{
  aBaseFrame = mFrames[0];
  aTextFrames.ClearAndRetainStorage();
  for (uint32_t i = 1, iend = mFrames.Length(); i < iend; i++) {
    aTextFrames.AppendElement(mFrames[i]);
  }
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

  nsTArray<nsRubyTextContainerFrame*>* containers = &mTextContainers;
  if (!GetPrevContinuation() && !GetNextContinuation()) {
    nsIFrame* onlyChild = rtcFrame->PrincipalChildList().OnlyChild();
    if (onlyChild && onlyChild->IsPseudoFrame(rtcFrame->GetContent())) {
      
      
      containers = &mSpanContainers;
    }
  }
  containers->AppendElement(rtcFrame);
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

  MoveOverflowToChildList();
  
  const uint32_t rtcCount = mTextContainers.Length();
  for (uint32_t i = 0; i < rtcCount; i++) {
    mTextContainers[i]->MoveOverflowToChildList();
  }

  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  LogicalSize availSize(lineWM, aReflowState.AvailableWidth(),
                        aReflowState.AvailableHeight());

  const uint32_t spanCount = mSpanContainers.Length();
  const uint32_t totalCount = rtcCount + spanCount;
  
  
  
  
  
  
  
  nsAutoTArray<UniquePtr<nsHTMLReflowState>, RTC_ARRAY_SIZE> reflowStates;
  nsAutoTArray<UniquePtr<nsLineLayout>, RTC_ARRAY_SIZE> lineLayouts;
  reflowStates.SetCapacity(totalCount);
  lineLayouts.SetCapacity(totalCount);

  nsAutoTArray<nsHTMLReflowState*, RTC_ARRAY_SIZE> rtcReflowStates;
  nsAutoTArray<nsHTMLReflowState*, RTC_ARRAY_SIZE> spanReflowStates;
  rtcReflowStates.SetCapacity(rtcCount);
  spanReflowStates.SetCapacity(spanCount);

  
  for (uint32_t i = 0; i < totalCount; i++) {
    nsIFrame* textContainer;
    nsTArray<nsHTMLReflowState*>* reflowStateArray;
    if (i < rtcCount) {
      textContainer = mTextContainers[i];
      reflowStateArray = &rtcReflowStates;
    } else {
      textContainer = mSpanContainers[i - rtcCount];
      reflowStateArray = &spanReflowStates;
    }
    nsHTMLReflowState* reflowState = new nsHTMLReflowState(
      aPresContext, *aReflowState.parentReflowState, textContainer, availSize);
    reflowStates.AppendElement(reflowState);
    reflowStateArray->AppendElement(reflowState);
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

  if (aReflowState.mLineLayout->LineIsBreakable() &&
      aReflowState.mLineLayout->NotifyOptionalBreakPosition(
        this, 0, startEdge <= aReflowState.AvailableISize(),
        gfxBreakPriority::eNormalBreak)) {
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  }

  nscoord isize = 0;
  if (aStatus == NS_FRAME_COMPLETE) {
    
    isize = ReflowPairs(aPresContext, aReflowState, rtcReflowStates, aStatus);
  }

  
  
  MOZ_ASSERT(NS_INLINE_IS_BREAK_BEFORE(aStatus) ||
             NS_FRAME_IS_COMPLETE(aStatus) || mSpanContainers.IsEmpty());
  if (!NS_INLINE_IS_BREAK_BEFORE(aStatus) &&
      NS_FRAME_IS_COMPLETE(aStatus) && !mSpanContainers.IsEmpty()) {
    
    nscoord spanISize = ReflowSpans(aPresContext, aReflowState,
                                    spanReflowStates, aStatus);
    
    
    MOZ_ASSERT(aStatus == NS_FRAME_COMPLETE);
    if (isize < spanISize) {
      nscoord delta = spanISize - isize;
      if (ShouldBreakBefore(aReflowState, delta)) {
        aStatus = NS_INLINE_LINE_BREAK_BEFORE();
      } else {
        aReflowState.mLineLayout->AdvanceICoord(delta);
        isize = spanISize;
      }
    }
  }

  DebugOnly<nscoord> lineSpanSize = aReflowState.mLineLayout->EndSpan(this);
  
  
  
  MOZ_ASSERT(NS_INLINE_IS_BREAK_BEFORE(aStatus) ||
             isize == lineSpanSize || mFrames.IsEmpty());
  for (uint32_t i = 0; i < totalCount; i++) {
    
    
    nsRubyTextContainerFrame* textContainer = i < rtcCount ?
      mTextContainers[i] : mSpanContainers[i - rtcCount];
    textContainer->SetISize(isize);
    lineLayouts[i]->EndLineReflow();
  }

  aDesiredSize.ISize(lineWM) = isize;
  nsLayoutUtils::SetBSizeFromFontMetrics(this, aDesiredSize, aReflowState,
                                         borderPadding, lineWM, frameWM);
}






struct MOZ_STACK_CLASS nsRubyBaseContainerFrame::PullFrameState
{
  ContinuationTraversingState mBase;
  nsAutoTArray<ContinuationTraversingState, RTC_ARRAY_SIZE> mTexts;

  explicit PullFrameState(nsRubyBaseContainerFrame* aFrame);
};

nscoord
nsRubyBaseContainerFrame::ReflowPairs(nsPresContext* aPresContext,
                                      const nsHTMLReflowState& aReflowState,
                                      nsTArray<nsHTMLReflowState*>& aReflowStates,
                                      nsReflowStatus& aStatus)
{
  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  const uint32_t rtcCount = mTextContainers.Length();
  nscoord istart = lineLayout->GetCurrentICoord();
  nscoord icoord = istart;
  nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;
  aStatus = NS_FRAME_COMPLETE;

  mPairCount = 0;
  nsIFrame* baseFrame = nullptr;
  nsAutoTArray<nsIFrame*, RTC_ARRAY_SIZE> textFrames;
  textFrames.SetCapacity(rtcCount);
  PairEnumerator e(this, mTextContainers);
  for (; !e.AtEnd(); e.Next()) {
    e.GetFrames(baseFrame, textFrames);
    icoord += ReflowOnePair(aPresContext, aReflowState, aReflowStates,
                            baseFrame, textFrames, reflowStatus);
    if (NS_INLINE_IS_BREAK(reflowStatus)) {
      break;
    }
    
    MOZ_ASSERT(reflowStatus == NS_FRAME_COMPLETE);
  }

  bool isComplete = false;
  PullFrameState pullFrameState(this);
  while (!NS_INLINE_IS_BREAK(reflowStatus)) {
    
    MOZ_ASSERT(reflowStatus == NS_FRAME_COMPLETE);

    
    
    PullOnePair(lineLayout, pullFrameState, baseFrame, textFrames, isComplete);
    if (isComplete) {
      
      break;
    }
    icoord += ReflowOnePair(aPresContext, aReflowState, aReflowStates,
                            baseFrame, textFrames, reflowStatus);
  }

  if (!e.AtEnd() && NS_INLINE_IS_BREAK_AFTER(reflowStatus)) {
    
    
    e.Next();
    e.GetFrames(baseFrame, textFrames);
    reflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
  }
  if (!e.AtEnd() || (GetNextInFlow() && !isComplete)) {
    NS_FRAME_SET_INCOMPLETE(aStatus);
  } else {
    
    
    
    if (lineLayout->NotifyOptionalBreakPosition(
          this, INT32_MAX, true, gfxBreakPriority::eNormalBreak)) {
      reflowStatus = NS_INLINE_LINE_BREAK_AFTER(reflowStatus);
    }
  }

  if (NS_INLINE_IS_BREAK_BEFORE(reflowStatus)) {
    if (!mPairCount || !mSpanContainers.IsEmpty()) {
      
      
      aStatus = NS_INLINE_LINE_BREAK_BEFORE();
      return 0;
    }
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    MOZ_ASSERT(NS_FRAME_IS_COMPLETE(aStatus) || mSpanContainers.IsEmpty());

    if (baseFrame) {
      PushChildren(baseFrame, baseFrame->GetPrevSibling());
    }
    for (uint32_t i = 0; i < rtcCount; i++) {
      nsIFrame* textFrame = textFrames[i];
      if (textFrame) {
        mTextContainers[i]->PushChildren(textFrame,
                                         textFrame->GetPrevSibling());
      }
    }
  } else if (NS_INLINE_IS_BREAK_AFTER(reflowStatus)) {
    
    
    
    
    MOZ_ASSERT(e.AtEnd());
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
  }

  return icoord - istart;
}

nscoord
nsRubyBaseContainerFrame::ReflowOnePair(nsPresContext* aPresContext,
                                        const nsHTMLReflowState& aReflowState,
                                        nsTArray<nsHTMLReflowState*>& aReflowStates,
                                        nsIFrame* aBaseFrame,
                                        const nsTArray<nsIFrame*>& aTextFrames,
                                        nsReflowStatus& aStatus)
{
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  const uint32_t rtcCount = mTextContainers.Length();
  MOZ_ASSERT(aTextFrames.Length() == rtcCount);
  MOZ_ASSERT(aReflowStates.Length() == rtcCount);
  nscoord istart = aReflowState.mLineLayout->GetCurrentICoord();
  nscoord pairISize = 0;

  nsAutoString baseText;
  if (aBaseFrame) {
    if (!nsContentUtils::GetNodeTextContent(aBaseFrame->GetContent(),
                                            true, baseText)) {
      NS_RUNTIMEABORT("OOM");
    }
  }

  
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsIFrame* textFrame = aTextFrames[i];
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
      nsHTMLReflowMetrics metrics(*aReflowStates[i]);

      bool pushedFrame;
      aReflowStates[i]->mLineLayout->ReflowFrame(textFrame, reflowStatus,
                                                 &metrics, pushedFrame);
      MOZ_ASSERT(!NS_INLINE_IS_BREAK(reflowStatus) && !pushedFrame,
                 "Any line break inside ruby box should has been suppressed");
      pairISize = std::max(pairISize, metrics.ISize(lineWM));
    }
  }
  if (ShouldBreakBefore(aReflowState, pairISize)) {
    
    
    
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
    return 0;
  }

  
  if (aBaseFrame) {
    MOZ_ASSERT(aBaseFrame->GetType() == nsGkAtoms::rubyBaseFrame);
    nsReflowStatus reflowStatus;
    nsHTMLReflowMetrics metrics(aReflowState);

    bool pushedFrame;
    aReflowState.mLineLayout->ReflowFrame(aBaseFrame, reflowStatus,
                                          &metrics, pushedFrame);
    MOZ_ASSERT(!NS_INLINE_IS_BREAK(reflowStatus) && !pushedFrame,
               "Any line break inside ruby box should has been suppressed");
    pairISize = std::max(pairISize, metrics.ISize(lineWM));
  }

  
  nscoord icoord = istart + pairISize;
  aReflowState.mLineLayout->AdvanceICoord(
    icoord - aReflowState.mLineLayout->GetCurrentICoord());
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsLineLayout* lineLayout = aReflowStates[i]->mLineLayout;
    lineLayout->AdvanceICoord(icoord - lineLayout->GetCurrentICoord());
    if (aBaseFrame && aTextFrames[i]) {
      lineLayout->AttachLastFrameToBaseLineLayout();
    }
  }

  mPairCount++;
  
  if (mSpanContainers.IsEmpty()) {
    if (aReflowState.mLineLayout->NotifyOptionalBreakPosition(
          this, mPairCount, icoord <= aReflowState.AvailableISize(),
          gfxBreakPriority::eNormalBreak)) {
      aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    }
  }

  return pairISize;
}

nsRubyBaseContainerFrame::PullFrameState::PullFrameState(
    nsRubyBaseContainerFrame* aFrame)
  : mBase(aFrame)
{
  const uint32_t rtcCount = aFrame->mTextContainers.Length();
  for (uint32_t i = 0; i < rtcCount; i++) {
    mTexts.AppendElement(aFrame->mTextContainers[i]);
  }
}

void
nsRubyBaseContainerFrame::PullOnePair(nsLineLayout* aLineLayout,
                                      PullFrameState& aPullFrameState,
                                      nsIFrame*& aBaseFrame,
                                      nsTArray<nsIFrame*>& aTextFrames,
                                      bool& aIsComplete)
{
  const uint32_t rtcCount = mTextContainers.Length();

  aBaseFrame = PullNextInFlowChild(aPullFrameState.mBase);
  aIsComplete = !aBaseFrame;

  aTextFrames.ClearAndRetainStorage();
  for (uint32_t i = 0; i < rtcCount; i++) {
    nsIFrame* nextText =
      mTextContainers[i]->PullNextInFlowChild(aPullFrameState.mTexts[i]);
    aTextFrames.AppendElement(nextText);
    
    
    aIsComplete = aIsComplete && !nextText;
  }

  if (!aIsComplete) {
    
    aLineLayout->SetDirtyNextLine();
  }
}

nscoord
nsRubyBaseContainerFrame::ReflowSpans(nsPresContext* aPresContext,
                                      const nsHTMLReflowState& aReflowState,
                                      nsTArray<nsHTMLReflowState*>& aReflowStates,
                                      nsReflowStatus& aStatus)
{
  WritingMode lineWM = aReflowState.mLineLayout->GetWritingMode();
  const uint32_t spanCount = mSpanContainers.Length();
  nscoord spanISize = 0;

  for (uint32_t i = 0; i < spanCount; i++) {
    nsRubyTextContainerFrame* container = mSpanContainers[i];
    nsIFrame* rtFrame = container->GetFirstPrincipalChild();
    nsReflowStatus reflowStatus;
    nsHTMLReflowMetrics metrics(*aReflowStates[i]);
    bool pushedFrame;
    aReflowStates[i]->mLineLayout->ReflowFrame(rtFrame, reflowStatus,
                                               &metrics, pushedFrame);
    MOZ_ASSERT(!NS_INLINE_IS_BREAK(reflowStatus) && !pushedFrame,
               "Any line break inside ruby box should has been suppressed");
    spanISize = std::max(spanISize, metrics.ISize(lineWM));
  }

  return spanISize;
}
