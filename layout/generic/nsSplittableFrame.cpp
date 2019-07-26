









#include "nsSplittableFrame.h"
#include "nsContainerFrame.h"

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

NS_METHOD nsSplittableFrame::SetPrevContinuation(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(), "setting a prev continuation with incorrect type!");
  NS_ASSERTION (!IsInPrevContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mPrevContinuation = aFrame;
  RemoveStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
  return NS_OK;
}

nsIFrame* nsSplittableFrame::GetNextContinuation() const
{
  return mNextContinuation;
}

NS_METHOD nsSplittableFrame::SetNextContinuation(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(),  "setting a next continuation with incorrect type!");
  NS_ASSERTION (!IsInNextContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mNextContinuation = aFrame;
  if (aFrame)
    aFrame->RemoveStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
  return NS_OK;
}

nsIFrame* nsSplittableFrame::GetFirstContinuation() const
{
  nsSplittableFrame* firstContinuation = const_cast<nsSplittableFrame*>(this);
  while (firstContinuation->mPrevContinuation)  {
    firstContinuation = static_cast<nsSplittableFrame*>(firstContinuation->mPrevContinuation);
  }
  NS_POSTCONDITION(firstContinuation, "illegal state in continuation chain.");
  return firstContinuation;
}

nsIFrame* nsSplittableFrame::GetLastContinuation() const
{
  nsSplittableFrame* lastContinuation = const_cast<nsSplittableFrame*>(this);
  while (lastContinuation->mNextContinuation)  {
    lastContinuation = static_cast<nsSplittableFrame*>(lastContinuation->mNextContinuation);
  }
  NS_POSTCONDITION(lastContinuation, "illegal state in continuation chain.");
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

NS_METHOD nsSplittableFrame::SetPrevInFlow(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(), "setting a prev in flow with incorrect type!");
  NS_ASSERTION (!IsInPrevContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mPrevContinuation = aFrame;
  AddStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
  return NS_OK;
}

nsIFrame* nsSplittableFrame::GetNextInFlow() const
{
  return mNextContinuation && (mNextContinuation->GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? 
    mNextContinuation : nullptr;
}

NS_METHOD nsSplittableFrame::SetNextInFlow(nsIFrame* aFrame)
{
  NS_ASSERTION (!aFrame || GetType() == aFrame->GetType(),  "setting a next in flow with incorrect type!");
  NS_ASSERTION (!IsInNextContinuationChain(aFrame, this), "creating a loop in continuation chain!");
  mNextContinuation = aFrame;
  if (aFrame)
    aFrame->AddStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
  return NS_OK;
}

nsIFrame* nsSplittableFrame::GetFirstInFlow() const
{
  nsSplittableFrame* firstInFlow = const_cast<nsSplittableFrame*>(this);
  while (nsIFrame *prev = firstInFlow->GetPrevInFlow())  {
    firstInFlow = static_cast<nsSplittableFrame*>(prev);
  }
  NS_POSTCONDITION(firstInFlow, "illegal state in flow chain.");
  return firstInFlow;
}

nsIFrame* nsSplittableFrame::GetLastInFlow() const
{
  nsSplittableFrame* lastInFlow = const_cast<nsSplittableFrame*>(this);
  while (nsIFrame* next = lastInFlow->GetNextInFlow())  {
    lastInFlow = static_cast<nsSplittableFrame*>(next);
  }
  NS_POSTCONDITION(lastInFlow, "illegal state in flow chain.");
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
    height += prev->GetRect().height;
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

  if (aConsumedHeight != 0 && aConsumedHeight != NS_INTRINSICSIZE) {
    
    
    height += aReflowState.mComputedBorderPadding.top;
  }

  
  height = std::max(0, height);

  return height;
}

int
nsSplittableFrame::GetSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (IS_TRUE_OVERFLOW_CONTAINER(this)) {
    return (1 << NS_SIDE_TOP) | (1 << NS_SIDE_BOTTOM);
  }

  int skip = 0;

  if (GetPrevInFlow()) {
    skip |= 1 << NS_SIDE_TOP;
  }

  if (aReflowState) {
    
    
    
    

    if (NS_UNCONSTRAINEDSIZE != aReflowState->availableHeight) {
      nscoord effectiveCH = this->GetEffectiveComputedHeight(*aReflowState);
      if (effectiveCH > aReflowState->availableHeight) {
        
        
        skip |= 1 << NS_SIDE_BOTTOM;
      }
    }
  } else {
    nsIFrame* nif = GetNextInFlow();
    if (nif && !IS_TRUE_OVERFLOW_CONTAINER(nif)) {
      skip |= 1 << NS_SIDE_BOTTOM;
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
