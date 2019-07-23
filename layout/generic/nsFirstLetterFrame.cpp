






































#include "nsCOMPtr.h"
#include "nsHTMLContainerFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsLineLayout.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsFrameManager.h"

#define nsFirstLetterFrameSuper nsHTMLContainerFrame

class nsFirstLetterFrame : public nsFirstLetterFrameSuper {
public:
  nsFirstLetterFrame(nsStyleContext* aContext) : nsHTMLContainerFrame(aContext) {}

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    if (!GetStyleDisplay()->IsFloating())
      aFlags = aFlags & ~(nsIFrame::eLineParticipant);
    return nsFirstLetterFrameSuper::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer));
  }

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual void AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual PRBool CanContinueTextRun() const;

  NS_IMETHOD SetSelected(nsPresContext* aPresContext, nsIDOMRange *aRange,PRBool aSelected, nsSpread aSpread);


  NS_IMETHOD GetChildFrameContainingOffset(PRInt32 inContentOffset,
                                           PRBool inHint,
                                           PRInt32* outFrameContentOffset,
                                           nsIFrame **outChildFrame);

protected:
  virtual PRIntn GetSkipSides() const;

  void DrainOverflowFrames(nsPresContext* aPresContext);
};

nsIFrame*
NS_NewFirstLetterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFirstLetterFrame(aContext);
}

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
nsFirstLetterFrame::SetInitialChildList(nsIAtom*  aListName,
                                        nsIFrame* aChildList)
{
  mFrames.SetFrames(aChildList);
  nsFrameManager *frameManager = PresContext()->FrameManager();

  for (nsIFrame* frame = aChildList; frame; frame = frame->GetNextSibling()) {
    NS_ASSERTION(frame->GetParent() == this, "Unexpected parent");
    frameManager->ReParentStyleContext(frame);
  }
  return NS_OK;
}

NS_IMETHODIMP
nsFirstLetterFrame::SetSelected(nsPresContext* aPresContext, nsIDOMRange *aRange,PRBool aSelected, nsSpread aSpread)
{
  if (aSelected && ParentDisablesSelection())
    return NS_OK;
  nsIFrame *child = GetFirstChild(nsnull);
  while (child)
  {
    child->SetSelected(aPresContext, aRange, aSelected, aSpread);
    
    child = child->GetNextSibling();
  }
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
nsFirstLetterFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                      nsIFrame::InlineMinWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::MIN_WIDTH);
}



 void
nsFirstLetterFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                       nsIFrame::InlinePrefWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::PREF_WIDTH);
}


 nscoord
nsFirstLetterFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::MinWidthFromInline(this, aRenderingContext);
}


 nscoord
nsFirstLetterFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::PrefWidthFromInline(this, aRenderingContext);
}

 nsSize
nsFirstLetterFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
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
  if (NS_UNCONSTRAINEDSIZE != availSize.width) {
    availSize.width -= lr;
  }
  if (NS_UNCONSTRAINEDSIZE != availSize.height) {
    availSize.height -= tb;
  }

  
  if (!aReflowState.mLineLayout) {
    
    
    
    nsHTMLReflowState rs(aPresContext, aReflowState, kid, availSize);
    nsLineLayout ll(aPresContext, nsnull, &aReflowState, nsnull);
    ll.BeginLineReflow(0, 0, NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE,
                       PR_FALSE, PR_TRUE);
    rs.mLineLayout = &ll;
    ll.SetFirstLetterStyleOK(PR_TRUE);

    kid->WillReflow(aPresContext);
    kid->Reflow(aPresContext, aMetrics, rs, aReflowStatus);

    ll.EndLineReflow();
  }
  else {
    
    nsLineLayout* ll = aReflowState.mLineLayout;
    PRBool        pushedFrame;

    ll->BeginSpan(this, &aReflowState, bp.left, availSize.width);
    ll->ReflowFrame(kid, aReflowStatus, &aMetrics, pushedFrame);
    nsSize size;
    ll->EndSpan(this, size);
  }

  
  kid->SetRect(nsRect(bp.left, bp.top, aMetrics.width, aMetrics.height));
  kid->DidReflow(aPresContext, nsnull, NS_FRAME_REFLOW_FINISHED);
  aMetrics.width += lr;
  aMetrics.height += tb;
  aMetrics.ascent += bp.top;

  
  
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    if (aReflowState.mLineLayout) {
      aReflowState.mLineLayout->SetFirstLetterStyleOK(PR_FALSE);
    }
    nsIFrame* kidNextInFlow = kid->GetNextInFlow();
    if (kidNextInFlow) {
      
      static_cast<nsContainerFrame*>(kidNextInFlow->GetParent())
        ->DeleteNextInFlowChild(aPresContext, kidNextInFlow);
    }
  }
  else {
    
    
    nsIFrame* nextInFlow;
    rv = CreateNextInFlow(aPresContext, this, kid, nextInFlow);
    if (NS_FAILED(rv)) {
      return rv;
    }

    
    if (nextInFlow) {
      kid->SetNextSibling(nsnull);
      SetOverflowFrames(aPresContext, nextInFlow);
    }
    else {
      nsIFrame* nextSib = kid->GetNextSibling();
      if (nextSib) {
        kid->SetNextSibling(nsnull);
        SetOverflowFrames(aPresContext, nextSib);
      }
    }
  }

  NS_FRAME_SET_TRUNCATION(aReflowStatus, aReflowState, aMetrics);
  return rv;
}

 PRBool
nsFirstLetterFrame::CanContinueTextRun() const
{
  
  return PR_TRUE;
}

void
nsFirstLetterFrame::DrainOverflowFrames(nsPresContext* aPresContext)
{
  nsIFrame* overflowFrames;

  
  nsFirstLetterFrame* prevInFlow = (nsFirstLetterFrame*)GetPrevInFlow();
  if (nsnull != prevInFlow) {
    overflowFrames = prevInFlow->GetOverflowFrames(aPresContext, PR_TRUE);
    if (overflowFrames) {
      NS_ASSERTION(mFrames.IsEmpty(), "bad overflow list");

      
      
      nsIFrame* f = overflowFrames;
      while (f) {
        nsHTMLContainerFrame::ReparentFrameView(aPresContext, f, prevInFlow, this);
        f = f->GetNextSibling();
      }
      mFrames.InsertFrames(this, nsnull, overflowFrames);
    }
  }

  
  overflowFrames = GetOverflowFrames(aPresContext, PR_TRUE);
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    mFrames.AppendFrames(nsnull, overflowFrames);
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
