






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

void
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
nsFirstLetterFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                      nsIFrame::InlineMinISizeData *aData)
{
  DoInlineIntrinsicISize(aRenderingContext, aData, nsLayoutUtils::MIN_ISIZE);
}



 void
nsFirstLetterFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                       nsIFrame::InlinePrefISizeData *aData)
{
  DoInlineIntrinsicISize(aRenderingContext, aData, nsLayoutUtils::PREF_ISIZE);
}


 nscoord
nsFirstLetterFrame::GetMinISize(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::MinISizeFromInline(this, aRenderingContext);
}


 nscoord
nsFirstLetterFrame::GetPrefISize(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::PrefISizeFromInline(this, aRenderingContext);
}


LogicalSize
nsFirstLetterFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                                WritingMode aWM,
                                const LogicalSize& aCBSize,
                                nscoord aAvailableISize,
                                const LogicalSize& aMargin,
                                const LogicalSize& aBorder,
                                const LogicalSize& aPadding,
                                uint32_t aFlags)
{
  if (GetPrevInFlow()) {
    
    
    return LogicalSize(aWM, NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
  }
  return nsContainerFrame::ComputeSize(aRenderingContext, aWM,
      aCBSize, aAvailableISize, aMargin, aBorder, aPadding, aFlags);
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

  
  WritingMode wm = aReflowState.GetWritingMode();
  LogicalSize availSize = aReflowState.AvailableSize();
  const LogicalMargin& bp = aReflowState.ComputedLogicalBorderPadding();
  NS_ASSERTION(availSize.ISize(wm) != NS_UNCONSTRAINEDSIZE,
               "should no longer use unconstrained inline size");
  availSize.ISize(wm) -= bp.IStartEnd(wm);
  if (NS_UNCONSTRAINEDSIZE != availSize.BSize(wm)) {
    availSize.BSize(wm) -= bp.BStartEnd(wm);
  }

  
  if (!aReflowState.mLineLayout) {
    
    
    
    WritingMode kidWritingMode = GetWritingMode(kid);
    LogicalSize kidAvailSize = availSize.ConvertTo(kidWritingMode, wm);
    nsHTMLReflowState rs(aPresContext, aReflowState, kid, kidAvailSize);
    nsLineLayout ll(aPresContext, nullptr, &aReflowState, nullptr);

    ll.BeginLineReflow(bp.IStart(wm), bp.BStart(wm),
                       availSize.ISize(wm), NS_UNCONSTRAINEDSIZE,
                       false, true, kidWritingMode,
                       aReflowState.AvailableWidth());
    rs.mLineLayout = &ll;
    ll.SetInFirstLetter(true);
    ll.SetFirstLetterStyleOK(true);

    kid->WillReflow(aPresContext);
    kid->Reflow(aPresContext, aMetrics, rs, aReflowStatus);

    ll.EndLineReflow();
    ll.SetInFirstLetter(false);

    
    
    mBaseline = aMetrics.BlockStartAscent();
  }
  else {
    
    nsLineLayout* ll = aReflowState.mLineLayout;
    bool          pushedFrame;

    ll->SetInFirstLetter(
      mStyleContext->GetPseudo() == nsCSSPseudoElements::firstLetter);
    ll->BeginSpan(this, &aReflowState, bp.IStart(wm),
                  availSize.ISize(wm), &mBaseline);
    ll->ReflowFrame(kid, aReflowStatus, &aMetrics, pushedFrame);
    ll->EndSpan(this);
    ll->SetInFirstLetter(false);
  }

  
  WritingMode lineWM = aMetrics.GetWritingMode();
  LogicalSize convertedSize = aMetrics.Size(lineWM).ConvertTo(wm, lineWM);
  kid->SetRect(nsRect(bp.IStart(wm), bp.BStart(wm),
                      convertedSize.ISize(wm), convertedSize.BSize(wm)));
  kid->FinishAndStoreOverflow(&aMetrics);
  kid->DidReflow(aPresContext, nullptr, nsDidReflowStatus::FINISHED);

  convertedSize.ISize(wm) += bp.IStartEnd(wm);
  convertedSize.BSize(wm) += bp.BStartEnd(wm);
  aMetrics.SetSize(wm, convertedSize);
  aMetrics.SetBlockStartAscent(aMetrics.BlockStartAscent() +
                               bp.BStart(wm));

  
  
  
  aMetrics.UnionOverflowAreasWithDesiredBounds();
  ConsiderChildOverflow(aMetrics.mOverflowAreas, kid);

  if (!NS_INLINE_IS_BREAK_BEFORE(aReflowStatus)) {
    
    
    if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
      if (aReflowState.mLineLayout) {
        aReflowState.mLineLayout->SetFirstLetterStyleOK(false);
      }
      nsIFrame* kidNextInFlow = kid->GetNextInFlow();
      if (kidNextInFlow) {
        
        kidNextInFlow->GetParent()->DeleteNextInFlowChild(kidNextInFlow, true);
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
  parent->InsertFrames(kNoReflowPrincipalList, placeholderFrame, temp);

  *aContinuation = continuation;
  return NS_OK;
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
nsFirstLetterFrame::GetLogicalBaseline(WritingMode aWritingMode) const
{
  return mBaseline;
}

nsIFrame::LogicalSides
nsFirstLetterFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (GetPrevContinuation()) {
    
    
    
    
    
    return LogicalSides(eLogicalSideBitsAll);
  }
  return LogicalSides();  
}
