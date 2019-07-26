









#include "nsSplittableFrame.h"
#include "nsContainerFrame.h"
#include "nsIFrameInlines.h"

NS_IMPL_FRAMEARENA_HELPERS(nsSplittableFrame)

void
nsSplittableFrame::Init(nsIContent*      aContent,
                        nsIFrame*        aParent,
                        nsIFrame*        aPrevInFlow)
{
  nsFrame::Init(aContent, aParent, aPrevInFlow);

  if (aPrevInFlow) {
    
    SetPrevInFlow(aPrevInFlow);
    aPrevInFlow->SetNextInFlow(this);
  }
}

void
nsSplittableFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  
  if (mPrevContinuation || mNextContinuation) {
    RemoveFromFlow(this);
  }

  
  nsFrame::DestroyFrom(aDestructRoot);
}

nsSplittableType
nsSplittableFrame::GetSplittableType() const
{
  return NS_FRAME_SPLITTABLE;
}

nsIFrame* nsSplittableFrame::GetPrevContinuation() const
{
  return mPrevContinuation;
}

void
nsSplittableFrame::SetPrevContinuation(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(), "setting a prev continuation with incorrect type!");
  NS_ASSERTION (!IsInPrevContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mPrevContinuation = aFrame;
  RemoveStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
}

nsIFrame* nsSplittableFrame::GetNextContinuation() const
{
  return mNextContinuation;
}

void
nsSplittableFrame::SetNextContinuation(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(),  "setting a next continuation with incorrect type!");
  NS_ASSERTION (!IsInNextContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mNextContinuation = aFrame;
  if (aFrame)
    aFrame->RemoveStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
}

nsIFrame*
nsSplittableFrame::FirstContinuation() const
{
  nsSplittableFrame* firstContinuation = const_cast<nsSplittableFrame*>(this);
  while (firstContinuation->mPrevContinuation)  {
    firstContinuation = static_cast<nsSplittableFrame*>(firstContinuation->mPrevContinuation);
  }
  MOZ_ASSERT(firstContinuation, "post-condition failed");
  return firstContinuation;
}

nsIFrame*
nsSplittableFrame::LastContinuation() const
{
  nsSplittableFrame* lastContinuation = const_cast<nsSplittableFrame*>(this);
  while (lastContinuation->mNextContinuation)  {
    lastContinuation = static_cast<nsSplittableFrame*>(lastContinuation->mNextContinuation);
  }
  MOZ_ASSERT(lastContinuation, "post-condition failed");
  return lastContinuation;
}

#ifdef DEBUG
bool nsSplittableFrame::IsInPrevContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  int32_t iterations = 0;
  while (aFrame1 && iterations < 10) {
    
    if (aFrame1 == aFrame2)
      return true;
    aFrame1 = aFrame1->GetPrevContinuation();
    ++iterations;
  }
  return false;
}

bool nsSplittableFrame::IsInNextContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  int32_t iterations = 0;
  while (aFrame1 && iterations < 10) {
    
    if (aFrame1 == aFrame2)
      return true;
    aFrame1 = aFrame1->GetNextContinuation();
    ++iterations;
  }
  return false;
}
#endif

nsIFrame* nsSplittableFrame::GetPrevInFlow() const
{
  return (GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? mPrevContinuation : nullptr;
}

void
nsSplittableFrame::SetPrevInFlow(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(), "setting a prev in flow with incorrect type!");
  NS_ASSERTION (!IsInPrevContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mPrevContinuation = aFrame;
  AddStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
}

nsIFrame* nsSplittableFrame::GetNextInFlow() const
{
  return mNextContinuation && (mNextContinuation->GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? 
    mNextContinuation : nullptr;
}

void
nsSplittableFrame::SetNextInFlow(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(),  "setting a next in flow with incorrect type!");
  NS_ASSERTION (!IsInNextContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mNextContinuation = aFrame;
  if (aFrame)
    aFrame->AddStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
}

nsIFrame*
nsSplittableFrame::FirstInFlow() const
{
  nsSplittableFrame* firstInFlow = const_cast<nsSplittableFrame*>(this);
  while (nsIFrame* prev = firstInFlow->GetPrevInFlow())  {
    firstInFlow = static_cast<nsSplittableFrame*>(prev);
  }
  MOZ_ASSERT(firstInFlow, "post-condition failed");
  return firstInFlow;
}

nsIFrame*
nsSplittableFrame::LastInFlow() const
{
  nsSplittableFrame* lastInFlow = const_cast<nsSplittableFrame*>(this);
  while (nsIFrame* next = lastInFlow->GetNextInFlow())  {
    lastInFlow = static_cast<nsSplittableFrame*>(next);
  }
  MOZ_ASSERT(lastInFlow, "post-condition failed");
  return lastInFlow;
}


void
nsSplittableFrame::RemoveFromFlow(nsIFrame* aFrame)
{
  nsIFrame* prevContinuation = aFrame->GetPrevContinuation();
  nsIFrame* nextContinuation = aFrame->GetNextContinuation();

  
  
  if (aFrame->GetPrevInFlow() && aFrame->GetNextInFlow()) {
    if (prevContinuation) {
      prevContinuation->SetNextInFlow(nextContinuation);
    }
    if (nextContinuation) {
      nextContinuation->SetPrevInFlow(prevContinuation);
    }
  } else {
    if (prevContinuation) {
      prevContinuation->SetNextContinuation(nextContinuation);
    }
    if (nextContinuation) {
      nextContinuation->SetPrevContinuation(prevContinuation);
    }
  }

  aFrame->SetPrevInFlow(nullptr);
  aFrame->SetNextInFlow(nullptr);
}

nscoord
nsSplittableFrame::GetConsumedHeight() const
{
  nscoord height = 0;
  for (nsIFrame* prev = GetPrevInFlow(); prev; prev = prev->GetPrevInFlow()) {
    height += prev->GetContentRectRelativeToSelf().height;
  }
  return height;
}

nscoord
nsSplittableFrame::GetEffectiveComputedHeight(const nsHTMLReflowState& aReflowState,
                                              nscoord aConsumedHeight) const
{
  nscoord height = aReflowState.ComputedHeight();
  if (height == NS_INTRINSICSIZE) {
    return NS_INTRINSICSIZE;
  }

  if (aConsumedHeight == NS_INTRINSICSIZE) {
    aConsumedHeight = GetConsumedHeight();
  }

  height -= aConsumedHeight;

  
  return std::max(0, height);
}

int
nsSplittableFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (IS_TRUE_OVERFLOW_CONTAINER(this)) {
    return LOGICAL_SIDES_B_BOTH;
  }

  if (MOZ_UNLIKELY(StyleBorder()->mBoxDecorationBreak ==
                     NS_STYLE_BOX_DECORATION_BREAK_CLONE)) {
    return 0;
  }

  int skip = 0;
  if (GetPrevInFlow()) {
    skip |= LOGICAL_SIDE_B_START;
  }

  if (aReflowState) {
    
    
    
    

    if (NS_UNCONSTRAINEDSIZE != aReflowState->AvailableHeight()) {
      nscoord effectiveCH = this->GetEffectiveComputedHeight(*aReflowState);
      if (effectiveCH != NS_INTRINSICSIZE &&
          effectiveCH > aReflowState->AvailableHeight()) {
        
        
        skip |= LOGICAL_SIDE_B_END;
      }
    }
  } else {
    nsIFrame* nif = GetNextInFlow();
    if (nif && !IS_TRUE_OVERFLOW_CONTAINER(nif)) {
      skip |= LOGICAL_SIDE_B_END;
    }
  }

 return skip;
}

#ifdef DEBUG
void
nsSplittableFrame::DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, int32_t aIndent)
{
  nsFrame::DumpBaseRegressionData(aPresContext, out, aIndent);
  if (nullptr != mNextContinuation) {
    IndentBy(out, aIndent);
    fprintf(out, "<next-continuation va=\"%p\"/>\n", (void*)mNextContinuation);
  }
  if (nullptr != mPrevContinuation) {
    IndentBy(out, aIndent);
    fprintf(out, "<prev-continuation va=\"%p\"/>\n", (void*)mPrevContinuation);
  }

}
#endif
