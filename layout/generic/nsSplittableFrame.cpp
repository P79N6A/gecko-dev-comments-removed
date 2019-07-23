









































#include "nsSplittableFrame.h"
#include "nsIContent.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"

NS_IMPL_FRAMEARENA_HELPERS(nsSplittableFrame)

NS_IMETHODIMP
nsSplittableFrame::Init(nsIContent*      aContent,
                        nsIFrame*        aParent,
                        nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsFrame::Init(aContent, aParent, aPrevInFlow);

  if (aPrevInFlow) {
    
    SetPrevInFlow(aPrevInFlow);
    aPrevInFlow->SetNextInFlow(this);
  }

  return rv;
}

void
nsSplittableFrame::Destroy()
{
  
  if (mPrevContinuation || mNextContinuation) {
    RemoveFromFlow(this);
  }

  
  nsFrame::Destroy();
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
PRBool nsSplittableFrame::IsInPrevContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  PRInt32 iterations = 0;
  while (aFrame1 && iterations < 10) {
    
    if (aFrame1 == aFrame2)
      return PR_TRUE;
    aFrame1 = aFrame1->GetPrevContinuation();
    ++iterations;
  }
  return PR_FALSE;
}

PRBool nsSplittableFrame::IsInNextContinuationChain(nsIFrame* aFrame1, nsIFrame* aFrame2)
{
  PRInt32 iterations = 0;
  while (aFrame1 && iterations < 10) {
    
    if (aFrame1 == aFrame2)
      return PR_TRUE;
    aFrame1 = aFrame1->GetNextContinuation();
    ++iterations;
  }
  return PR_FALSE;
}
#endif

nsIFrame* nsSplittableFrame::GetPrevInFlow() const
{
  return (GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? mPrevContinuation : nsnull;
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
    mNextContinuation : nsnull;
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

  aFrame->SetPrevInFlow(nsnull);
  aFrame->SetNextInFlow(nsnull);
}

#ifdef DEBUG
void
nsSplittableFrame::DumpBaseRegressionData(nsPresContext* aPresContext, FILE* out, PRInt32 aIndent)
{
  nsFrame::DumpBaseRegressionData(aPresContext, out, aIndent);
  if (nsnull != mNextContinuation) {
    IndentBy(out, aIndent);
    fprintf(out, "<next-continuation va=\"%ld\"/>\n", PRUptrdiff(mNextContinuation));
  }
  if (nsnull != mPrevContinuation) {
    IndentBy(out, aIndent);
    fprintf(out, "<prev-continuation va=\"%ld\"/>\n", PRUptrdiff(mPrevContinuation));
  }

}
#endif
