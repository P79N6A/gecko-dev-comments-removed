






#include "nsFirstLetterFrame.h"
#include "nsPresContext.h"
#include "nsStyleContext.h"
#include "nsIContent.h"
#include "nsLineLayout.h"
#include "nsGkAtoms.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"
#include "nsFrameManager.h"
#include "RestyleManager.h"
#include "nsPlaceholderFrame.h"
#include "nsCSSFrameConstructor.h"

using namespace mozilla;
using namespace mozilla::layout;

nsFirstLetterFrame*
NS_NewFirstLetterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFirstLetterFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFirstLetterFrame)

NS_QUERYFRAME_HEAD(nsFirstLetterFrame)
  NS_QUERYFRAME_ENTRY(nsFirstLetterFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef DEBUG_FRAME_DUMP
nsresult
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

void
nsFirstLetterFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                     const nsRect&           aDirtyRect,
                                     const nsDisplayListSet& aLists)
{
  BuildDisplayListForInline(aBuilder, aDirtyRect, aLists);
}

void
nsFirstLetterFrame::Init(nsIContent*       aContent,
                         nsContainerFrame* aParent,
                         nsIFrame*         aPrevInFlow)
{
  nsRefPtr<nsStyleContext> newSC;
  if (aPrevInFlow) {
    
    
    
    nsStyleContext* parentStyleContext = mStyleContext->GetParent();
    if (parentStyleContext) {
      newSC = PresContext()->StyleSet()->
        ResolveStyleForNonElement(parentStyleContext);
      SetStyleContextWithoutNotification(newSC);
    }
  }

  nsContainerFrame::Init(aContent, aParent, aPrevInFlow);
}

nsresult
nsFirstLetterFrame::SetInitialChildList(ChildListID  aListID,
                                        nsFrameList& aChildList)
{
  RestyleManager* restyleManager = PresContext()->RestyleManager();

  for (nsFrameList::Enumerator e(aChildList); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->GetParent() == this, "Unexpected parent");
    restyleManager->ReparentStyleContext(e.get());
    nsLayoutUtils::MarkDescendantsDirty(e.get());
  }

  mFrames.SetFrames(aChildList);
  return NS_OK;
}

nsresult
nsFirstLetterFrame::GetChildFrameContainingOffset(int32_t inContentOffset,
                                                  bool inHint,
                                                  int32_t* outFrameContentOffset,
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
                                uint32_t aFlags)
{
  if (GetPrevInFlow()) {
    
    
    return nsSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }
  return nsContainerFrame::ComputeSize(aRenderingContext,
      aCBSize, aAvailableWidth, aMargin, aBorder, aPadding, aFlags);
}

void
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

  
  nsSize availSize(aReflowState.AvailableWidth(), aReflowState.AvailableHeight());
  const nsMargin& bp = aReflowState.ComputedPhysicalBorderPadding();
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
    nsLineLayout ll(aPresContext, nullptr, &aReflowState, nullptr);

    ll.BeginLineReflow(bp.left, bp.top, availSize.width, NS_UNCONSTRAINEDSIZE,
                       false, true,
                       ll.LineContainerFrame()->GetWritingMode(kid),
                       aReflowState.AvailableWidth());
    rs.mLineLayout = &ll;
    ll.SetInFirstLetter(true);
    ll.SetFirstLetterStyleOK(true);

    kid->WillReflow(aPresContext);
    kid->Reflow(aPresContext, aMetrics, rs, aReflowStatus);

    ll.EndLineReflow();
    ll.SetInFirstLetter(false);

    
    
    mBaseline = aMetrics.TopAscent();
  }
  else {
    
    nsLineLayout* ll = aReflowState.mLineLayout;
    bool          pushedFrame;

    ll->SetInFirstLetter(
      mStyleContext->GetPseudo() == nsCSSPseudoElements::firstLetter);
    ll->BeginSpan(this, &aReflowState, bp.left, availSize.width, &mBaseline);
    ll->ReflowFrame(kid, aReflowStatus, &aMetrics, pushedFrame);
    ll->EndSpan(this);
    ll->SetInFirstLetter(false);
  }

  
  kid->SetRect(nsRect(bp.left, bp.top, aMetrics.Width(), aMetrics.Height()));
  kid->FinishAndStoreOverflow(&aMetrics);
  kid->DidReflow(aPresContext, nullptr, nsDidReflowStatus::FINISHED);

  aMetrics.Width() += lr;
  aMetrics.Height() += tb;
  aMetrics.SetTopAscent(aMetrics.TopAscent() + bp.top);

  
  
  
  aMetrics.UnionOverflowAreasWithDesiredBounds();
  ConsiderChildOverflow(aMetrics.mOverflowAreas, kid);

  if (!NS_INLINE_IS_BREAK_BEFORE(aReflowStatus)) {
    
    
    if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
      if (aReflowState.mLineLayout) {
        aReflowState.mLineLayout->SetFirstLetterStyleOK(false);
      }
      nsIFrame* kidNextInFlow = kid->GetNextInFlow();
      if (kidNextInFlow) {
        
        static_cast<nsContainerFrame*>(kidNextInFlow->GetParent())
          ->DeleteNextInFlowChild(kidNextInFlow, true);
      }
    }
    else {
      
      
      if (!IsFloating()) {
        nsIFrame* nextInFlow;
        rv = CreateNextInFlow(kid, nextInFlow);
        if (NS_FAILED(rv)) {
          return;
        }
    
        
        const nsFrameList& overflow = mFrames.RemoveFramesAfter(kid);
        if (overflow.NotEmpty()) {
          SetOverflowFrames(overflow);
        }
      } else if (!kid->GetNextInFlow()) {
        
        
        
        nsIFrame* continuation;
        CreateContinuationForFloatingParent(aPresContext, kid,
                                            &continuation, true);
      }
    }
  }

  FinishAndStoreOverflow(&aMetrics);

  NS_FRAME_SET_TRUNCATION(aReflowStatus, aReflowState, aMetrics);
}

 bool
nsFirstLetterFrame::CanContinueTextRun() const
{
  
  return true;
}

nsresult
nsFirstLetterFrame::CreateContinuationForFloatingParent(nsPresContext* aPresContext,
                                                        nsIFrame* aChild,
                                                        nsIFrame** aContinuation,
                                                        bool aIsFluid)
{
  NS_ASSERTION(IsFloating(),
               "can only call this on floating first letter frames");
  NS_PRECONDITION(aContinuation, "bad args");

  *aContinuation = nullptr;
  nsresult rv = NS_OK;

  nsIPresShell* presShell = aPresContext->PresShell();
  nsPlaceholderFrame* placeholderFrame =
    presShell->FrameManager()->GetPlaceholderFrameFor(this);
  nsContainerFrame* parent = placeholderFrame->GetParent();

  nsIFrame* continuation = presShell->FrameConstructor()->
    CreateContinuingFrame(aPresContext, aChild, parent, aIsFluid);

  
  
  
  nsStyleContext* parentSC = this->StyleContext()->GetParent();
  if (parentSC) {
    nsRefPtr<nsStyleContext> newSC;
    newSC = presShell->StyleSet()->ResolveStyleForNonElement(parentSC);
    continuation->SetStyleContext(newSC);
    nsLayoutUtils::MarkDescendantsDirty(continuation);
  }

  
  
  
  
  nsFrameList temp(continuation, continuation);
  rv = parent->InsertFrames(kNoReflowPrincipalList, placeholderFrame, temp);

  *aContinuation = continuation;
  return rv;
}

void
nsFirstLetterFrame::DrainOverflowFrames(nsPresContext* aPresContext)
{
  
  nsFirstLetterFrame* prevInFlow = (nsFirstLetterFrame*)GetPrevInFlow();
  if (prevInFlow) {
    AutoFrameListPtr overflowFrames(aPresContext,
                                    prevInFlow->StealOverflowFrames());
    if (overflowFrames) {
      NS_ASSERTION(mFrames.IsEmpty(), "bad overflow list");

      
      
      nsContainerFrame::ReparentFrameViewList(*overflowFrames, prevInFlow,
                                              this);
      mFrames.InsertFrames(this, nullptr, *overflowFrames);
    }
  }

  
  AutoFrameListPtr overflowFrames(aPresContext, StealOverflowFrames());
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    mFrames.AppendFrames(nullptr, *overflowFrames);
  }

  
  
  
  nsIFrame* kid = mFrames.FirstChild();
  if (kid) {
    nsRefPtr<nsStyleContext> sc;
    nsIContent* kidContent = kid->GetContent();
    if (kidContent) {
      NS_ASSERTION(kidContent->IsNodeOfType(nsINode::eTEXT),
                   "should contain only text nodes");
      nsStyleContext* parentSC = prevInFlow ? mStyleContext->GetParent() :
                                              mStyleContext;
      sc = aPresContext->StyleSet()->ResolveStyleForNonElement(parentSC);
      kid->SetStyleContext(sc);
      nsLayoutUtils::MarkDescendantsDirty(kid);
    }
  }
}

nscoord
nsFirstLetterFrame::GetBaseline() const
{
  return mBaseline;
}

int
nsFirstLetterFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (GetPrevContinuation()) {
    
    
    
    
    
    return LOGICAL_SIDES_ALL;
  }
  return 0;  
}
