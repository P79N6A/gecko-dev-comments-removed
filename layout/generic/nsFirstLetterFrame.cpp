






































#include "nsCOMPtr.h"
#include "nsFirstLetterFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsLineLayout.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsFrameManager.h"
#include "nsPlaceholderFrame.h"
#include "nsCSSFrameConstructor.h"

nsIFrame*
NS_NewFirstLetterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFirstLetterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFirstLetterFrame)

NS_QUERYFRAME_HEAD(nsFirstLetterFrame)
  NS_QUERYFRAME_ENTRY(nsFirstLetterFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsFirstLetterFrameSuper)

#ifdef NS_DEBUG
NS_IMETHODIMP
nsFirstLetterFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Letter"), aResult);
}
#endif

nsIAtom*
nsFirstLetterFrame::GetType() const
{
  return nsGkAtoms::letterFrame;
}

PRIntn
nsFirstLetterFrame::GetSkipSides() const
{
  return 0;
}

NS_IMETHODIMP
nsFirstLetterFrame::Init(nsIContent*      aContent,
                         nsIFrame*        aParent,
                         nsIFrame*        aPrevInFlow)
{
  nsRefPtr<nsStyleContext> newSC;
  if (aPrevInFlow) {
    
    
    
    nsStyleContext* parentStyleContext = mStyleContext->GetParent();
    if (parentStyleContext) {
      newSC = mStyleContext->GetRuleNode()->GetPresContext()->StyleSet()->
        ResolveStyleForNonElement(parentStyleContext);
      if (newSC)
        SetStyleContextWithoutNotification(newSC);
    }
  }

  return nsFirstLetterFrameSuper::Init(aContent, aParent, aPrevInFlow);
}

NS_IMETHODIMP
nsFirstLetterFrame::SetInitialChildList(nsIAtom*     aListName,
                                        nsFrameList& aChildList)
{
  nsFrameManager *frameManager = PresContext()->FrameManager();

  for (nsFrameList::Enumerator e(aChildList); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->GetParent() == this, "Unexpected parent");
    frameManager->ReparentStyleContext(e.get());
  }

  mFrames.SetFrames(aChildList);
  return NS_OK;
}

NS_IMETHODIMP
nsFirstLetterFrame::GetChildFrameContainingOffset(PRInt32 inContentOffset,
                                                  PRBool inHint,
                                                  PRInt32* outFrameContentOffset,
                                                  nsIFrame **outChildFrame)
{
  nsIFrame *kid = mFrames.FirstChild();
  if (kid)
  {
    return kid->GetChildFrameContainingOffset(inContentOffset, inHint, outFrameContentOffset, outChildFrame);
  }
  else
    return nsFrame::GetChildFrameContainingOffset(inContentOffset, inHint, outFrameContentOffset, outChildFrame);
}



 void
nsFirstLetterFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                      nsIFrame::InlineMinWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::MIN_WIDTH);
}



 void
nsFirstLetterFrame::AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                       nsIFrame::InlinePrefWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::PREF_WIDTH);
}


 nscoord
nsFirstLetterFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::MinWidthFromInline(this, aRenderingContext);
}


 nscoord
nsFirstLetterFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::PrefWidthFromInline(this, aRenderingContext);
}

 nsSize
nsFirstLetterFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                nsSize aCBSize, nscoord aAvailableWidth,
                                nsSize aMargin, nsSize aBorder, nsSize aPadding,
                                PRBool aShrinkWrap)
{
  if (GetPrevInFlow()) {
    
    
    return nsSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }
  return nsFirstLetterFrameSuper::ComputeSize(aRenderingContext,
      aCBSize, aAvailableWidth, aMargin, aBorder, aPadding, aShrinkWrap);
}

NS_IMETHODIMP
nsFirstLetterFrame::Reflow(nsPresContext*          aPresContext,
                           nsHTMLReflowMetrics&     aMetrics,
                           const nsHTMLReflowState& aReflowState,
                           nsReflowStatus&          aReflowStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsFirstLetterFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aReflowStatus);
  nsresult rv = NS_OK;

  
  DrainOverflowFrames(aPresContext);

  nsIFrame* kid = mFrames.FirstChild();

  
  nsSize availSize(aReflowState.availableWidth, aReflowState.availableHeight);
  const nsMargin& bp = aReflowState.mComputedBorderPadding;
  nscoord lr = bp.left + bp.right;
  nscoord tb = bp.top + bp.bottom;
  NS_ASSERTION(availSize.width != NS_UNCONSTRAINEDSIZE,
               "should no longer use unconstrained widths");
  availSize.width -= lr;
  if (NS_UNCONSTRAINEDSIZE != availSize.height) {
    availSize.height -= tb;
  }

  
  if (!aReflowState.mLineLayout) {
    
    
    
    nsHTMLReflowState rs(aPresContext, aReflowState, kid, availSize);
    nsLineLayout ll(aPresContext, nsnull, &aReflowState, nsnull);
    ll.BeginLineReflow(bp.left, bp.top, availSize.width, NS_UNCONSTRAINEDSIZE,
                       PR_FALSE, PR_TRUE);
    rs.mLineLayout = &ll;
    ll.SetInFirstLetter(PR_TRUE);
    ll.SetFirstLetterStyleOK(PR_TRUE);

    kid->WillReflow(aPresContext);
    kid->Reflow(aPresContext, aMetrics, rs, aReflowStatus);

    ll.EndLineReflow();
    ll.SetInFirstLetter(PR_FALSE);
  }
  else {
    
    nsLineLayout* ll = aReflowState.mLineLayout;
    PRBool        pushedFrame;

    ll->SetInFirstLetter(
      mStyleContext->GetPseudo() == nsCSSPseudoElements::firstLetter);
    ll->BeginSpan(this, &aReflowState, bp.left, availSize.width);
    ll->ReflowFrame(kid, aReflowStatus, &aMetrics, pushedFrame);
    ll->EndSpan(this);
    ll->SetInFirstLetter(PR_FALSE);
  }

  
  kid->SetRect(nsRect(bp.left, bp.top, aMetrics.width, aMetrics.height));
  kid->FinishAndStoreOverflow(&aMetrics);
  kid->DidReflow(aPresContext, nsnull, NS_FRAME_REFLOW_FINISHED);

  aMetrics.width += lr;
  aMetrics.height += tb;
  aMetrics.ascent += bp.top;
  mBaseline = aMetrics.ascent;

  
  
  
  aMetrics.UnionOverflowAreasWithDesiredBounds();
  ConsiderChildOverflow(aMetrics.mOverflowAreas, kid);

  
  
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    if (aReflowState.mLineLayout) {
      aReflowState.mLineLayout->SetFirstLetterStyleOK(PR_FALSE);
    }
    nsIFrame* kidNextInFlow = kid->GetNextInFlow();
    if (kidNextInFlow) {
      
      static_cast<nsContainerFrame*>(kidNextInFlow->GetParent())
        ->DeleteNextInFlowChild(aPresContext, kidNextInFlow, PR_TRUE);
    }
  }
  else {
    
    
    if (!GetStyleDisplay()->IsFloating()) {
      nsIFrame* nextInFlow;
      rv = CreateNextInFlow(aPresContext, kid, nextInFlow);
      if (NS_FAILED(rv)) {
        return rv;
      }

      
      const nsFrameList& overflow = mFrames.RemoveFramesAfter(kid);
      if (overflow.NotEmpty()) {
        SetOverflowFrames(aPresContext, overflow);
      }
    } else if (!kid->GetNextInFlow()) {
      
      
      
      nsIFrame* continuation;
      rv = CreateContinuationForFloatingParent(aPresContext, kid,
                                               &continuation, PR_TRUE);
    }
  }

  FinishAndStoreOverflow(&aMetrics);

  NS_FRAME_SET_TRUNCATION(aReflowStatus, aReflowState, aMetrics);
  return rv;
}

 PRBool
nsFirstLetterFrame::CanContinueTextRun() const
{
  
  return PR_TRUE;
}

nsresult
nsFirstLetterFrame::CreateContinuationForFloatingParent(nsPresContext* aPresContext,
                                                        nsIFrame* aChild,
                                                        nsIFrame** aContinuation,
                                                        PRBool aIsFluid)
{
  NS_ASSERTION(GetStyleDisplay()->IsFloating(),
               "can only call this on floating first letter frames");
  NS_PRECONDITION(aContinuation, "bad args");

  *aContinuation = nsnull;
  nsresult rv = NS_OK;

  nsIPresShell* presShell = aPresContext->PresShell();
  nsPlaceholderFrame* placeholderFrame =
    presShell->FrameManager()->GetPlaceholderFrameFor(this);
  nsIFrame* parent = placeholderFrame->GetParent();

  nsIFrame* continuation;
  rv = presShell->FrameConstructor()->
    CreateContinuingFrame(aPresContext, aChild, parent, &continuation, aIsFluid);
  if (NS_FAILED(rv) || !continuation) {
    return rv;
  }

  
  
  
  nsStyleContext* parentSC = this->GetStyleContext()->GetParent();
  if (parentSC) {
    nsRefPtr<nsStyleContext> newSC;
    newSC = presShell->StyleSet()->ResolveStyleForNonElement(parentSC);
    if (newSC) {
      continuation->SetStyleContext(newSC);
    }
  }

  
  
  
  
  nsFrameList temp(continuation, continuation);
  rv = parent->InsertFrames(nsGkAtoms::nextBidi, placeholderFrame, temp);

  *aContinuation = continuation;
  return rv;
}

void
nsFirstLetterFrame::DrainOverflowFrames(nsPresContext* aPresContext)
{
  nsAutoPtr<nsFrameList> overflowFrames;

  
  nsFirstLetterFrame* prevInFlow = (nsFirstLetterFrame*)GetPrevInFlow();
  if (nsnull != prevInFlow) {
    overflowFrames = prevInFlow->StealOverflowFrames();
    if (overflowFrames) {
      NS_ASSERTION(mFrames.IsEmpty(), "bad overflow list");

      
      
      nsContainerFrame::ReparentFrameViewList(aPresContext, *overflowFrames,
                                              prevInFlow, this);
      mFrames.InsertFrames(this, nsnull, *overflowFrames);
    }
  }

  
  overflowFrames = StealOverflowFrames();
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    mFrames.AppendFrames(nsnull, *overflowFrames);
  }

  
  
  
  nsIFrame* kid = mFrames.FirstChild();
  if (kid) {
    nsRefPtr<nsStyleContext> sc;
    nsIContent* kidContent = kid->GetContent();
    if (kidContent) {
      NS_ASSERTION(kidContent->IsNodeOfType(nsINode::eTEXT),
                   "should contain only text nodes");
      sc = aPresContext->StyleSet()->ResolveStyleForNonElement(mStyleContext);
      if (sc) {
        kid->SetStyleContext(sc);
      }
    }
  }
}

nscoord
nsFirstLetterFrame::GetBaseline() const
{
  return mBaseline;
}
