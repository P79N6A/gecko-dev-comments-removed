






#include "nsInlineFrame.h"
#include "nsLineLayout.h"
#include "nsBlockFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsGkAtoms.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsCSSAnonBoxes.h"
#include "nsAutoPtr.h"
#include "RestyleManager.h"
#include "nsDisplayList.h"
#include "mozilla/Likely.h"
#include "SVGTextFrame.h"

#ifdef DEBUG
#undef NOISY_PUSHING
#endif

using namespace mozilla;
using namespace mozilla::layout;






nsInlineFrame*
NS_NewInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsInlineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsInlineFrame)

NS_QUERYFRAME_HEAD(nsInlineFrame)
  NS_QUERYFRAME_ENTRY(nsInlineFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsContainerFrame)

#ifdef DEBUG_FRAME_DUMP
nsresult
nsInlineFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Inline"), aResult);
}
#endif

nsIAtom*
nsInlineFrame::GetType() const
{
  return nsGkAtoms::inlineFrame;
}

void
nsInlineFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    nsIFrame* svgTextFrame =
      nsLayoutUtils::GetClosestFrameOfType(GetParent(),
                                           nsGkAtoms::svgTextFrame);
    svgTextFrame->InvalidateFrame();
    return;
  }
  nsInlineFrameBase::InvalidateFrame(aDisplayItemKey);
}

void
nsInlineFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    nsIFrame* svgTextFrame =
      nsLayoutUtils::GetClosestFrameOfType(GetParent(),
                                           nsGkAtoms::svgTextFrame);
    svgTextFrame->InvalidateFrame();
    return;
  }
  nsInlineFrameBase::InvalidateFrameWithRect(aRect, aDisplayItemKey);
}

static inline bool
IsMarginZero(const nsStyleCoord &aCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Auto ||
         nsLayoutUtils::IsMarginZero(aCoord);
}

 bool
nsInlineFrame::IsSelfEmpty()
{
#if 0
  
  
  if (GetPresContext()->CompatibilityMode() == eCompatibility_FullStandards) {
    return false;
  }
#endif
  const nsStyleMargin* margin = StyleMargin();
  const nsStyleBorder* border = StyleBorder();
  const nsStylePadding* padding = StylePadding();
  
  
  
  WritingMode wm = GetWritingMode();
  bool haveStart, haveEnd;
  
  
  
  if (wm.IsVertical()) {
    haveStart =
      border->GetComputedBorderWidth(NS_SIDE_TOP) != 0 ||
      !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetTop()) ||
      !IsMarginZero(margin->mMargin.GetTop());
    haveEnd =
      border->GetComputedBorderWidth(NS_SIDE_BOTTOM) != 0 ||
      !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetBottom()) ||
      !IsMarginZero(margin->mMargin.GetBottom());
  } else {
    haveStart =
      border->GetComputedBorderWidth(NS_SIDE_LEFT) != 0 ||
      !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetLeft()) ||
      !IsMarginZero(margin->mMargin.GetLeft());
    haveEnd =
      border->GetComputedBorderWidth(NS_SIDE_RIGHT) != 0 ||
      !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetRight()) ||
      !IsMarginZero(margin->mMargin.GetRight());
  }
  if (haveStart || haveEnd) {
    
    
    if ((GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) &&
        StyleBorder()->mBoxDecorationBreak ==
          NS_STYLE_BOX_DECORATION_BREAK_SLICE) {
      
      
      if (!wm.IsBidiLTR()) {
        Swap(haveStart, haveEnd);
      }
      
      
      

      
      
      nsIFrame* firstCont = FirstContinuation();
      return
        (!haveStart || firstCont->FrameIsNonFirstInIBSplit()) &&
        (!haveEnd || firstCont->FrameIsNonLastInIBSplit());
    }
    return false;
  }
  return true;
}

bool
nsInlineFrame::IsEmpty()
{
  if (!IsSelfEmpty()) {
    return false;
  }

  for (nsIFrame *kid = mFrames.FirstChild(); kid; kid = kid->GetNextSibling()) {
    if (!kid->IsEmpty())
      return false;
  }

  return true;
}

nsIFrame::FrameSearchResult
nsInlineFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                   bool aRespectClusters)
{
  
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  int32_t startOffset = *aOffset;
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    *aOffset = 1 - startOffset;
  }
  return CONTINUE;
}

void
nsInlineFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  nsFrameList* overflowFrames = GetOverflowFrames();
  if (overflowFrames) {
    
    
    
    nsIFrame* lineContainer = nsLayoutUtils::FindNearestBlockAncestor(this);
    DrainSelfOverflowListInternal(eForDestroy, lineContainer);
  }
  nsContainerFrame::DestroyFrom(aDestructRoot);
}

nsresult
nsInlineFrame::StealFrame(nsIFrame* aChild,
                          bool      aForceNormal)
{
  if (aChild->HasAnyStateBits(NS_FRAME_IS_OVERFLOW_CONTAINER) &&
      !aForceNormal) {
    return nsContainerFrame::StealFrame(aChild, aForceNormal);
  }

  nsInlineFrame* parent = this;
  bool removed = false;
  do {
    removed = parent->mFrames.StartRemoveFrame(aChild);
    if (removed) {
      break;
    }

    
    
    nsFrameList* frameList = parent->GetOverflowFrames();
    if (frameList) {
      removed = frameList->ContinueRemoveFrame(aChild);
      if (frameList->IsEmpty()) {
        parent->DestroyOverflowList();
      }
      if (removed) {
        break;
      }
    }

    
    
    parent = static_cast<nsInlineFrame*>(parent->GetNextInFlow());
  } while (parent);

  MOZ_ASSERT(removed, "nsInlineFrame::StealFrame: can't find aChild");
  return removed ? NS_OK : NS_ERROR_UNEXPECTED;
}

void
nsInlineFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  BuildDisplayListForInline(aBuilder, aDirtyRect, aLists);

  
  
  
  
  if (!mFrames.FirstChild()) {
    DisplaySelectionOverlay(aBuilder, aLists.Content());
  }
}




 void
nsInlineFrame::AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 nsIFrame::InlineMinISizeData *aData)
{
  DoInlineIntrinsicISize(aRenderingContext, aData, nsLayoutUtils::MIN_ISIZE);
}

 void
nsInlineFrame::AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  nsIFrame::InlinePrefISizeData *aData)
{
  DoInlineIntrinsicISize(aRenderingContext, aData, nsLayoutUtils::PREF_ISIZE);
}


LogicalSize
nsInlineFrame::ComputeSize(nsRenderingContext *aRenderingContext,
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

nsRect
nsInlineFrame::ComputeTightBounds(gfxContext* aContext) const
{
  
  if (StyleContext()->HasTextDecorationLines()) {
    return GetVisualOverflowRect();
  }
  return ComputeSimpleTightBounds(aContext);
}

void
nsInlineFrame::ReparentFloatsForInlineChild(nsIFrame* aOurLineContainer,
                                            nsIFrame* aFrame,
                                            bool aReparentSiblings)
{
  
  
  NS_ASSERTION(aOurLineContainer->GetNextContinuation() ||
               aOurLineContainer->GetPrevContinuation(),
               "Don't call this when we have no continuation, it's a waste");
  if (!aFrame) {
    NS_ASSERTION(aReparentSiblings, "Why did we get called?");
    return;
  }

  nsIFrame* ancestor = aFrame;
  do {
    ancestor = ancestor->GetParent();
    if (!ancestor)
      return;
  } while (!ancestor->IsFloatContainingBlock());

  if (ancestor == aOurLineContainer)
    return;

  nsBlockFrame* ourBlock = nsLayoutUtils::GetAsBlock(aOurLineContainer);
  NS_ASSERTION(ourBlock, "Not a block, but broke vertically?");
  nsBlockFrame* frameBlock = nsLayoutUtils::GetAsBlock(ancestor);
  NS_ASSERTION(frameBlock, "ancestor not a block");

  while (true) {
    ourBlock->ReparentFloats(aFrame, frameBlock, false);

    if (!aReparentSiblings)
      return;
    nsIFrame* next = aFrame->GetNextSibling();
    if (!next)
      return;
    if (next->GetParent() == aFrame->GetParent()) {
      aFrame = next;
      continue;
    }
    
    
    
    
    ReparentFloatsForInlineChild(aOurLineContainer, next, aReparentSiblings);
    return;
  }
}

static void
ReparentChildListStyle(nsPresContext* aPresContext,
                       const nsFrameList::Slice& aFrames,
                       nsIFrame* aParentFrame)
{
  RestyleManager* restyleManager = aPresContext->RestyleManager();

  for (nsFrameList::Enumerator e(aFrames); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->GetParent() == aParentFrame, "Bogus parentage");
    restyleManager->ReparentStyleContext(e.get());
    nsLayoutUtils::MarkDescendantsDirty(e.get());
  }
}

void
nsInlineFrame::Reflow(nsPresContext*          aPresContext,
                      nsHTMLReflowMetrics&     aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  MarkInReflow();
  DO_GLOBAL_REFLOW_COUNT("nsInlineFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  if (nullptr == aReflowState.mLineLayout) {
    NS_ERROR("must have non-null aReflowState.mLineLayout");
    return;
  }
  if (IsFrameTreeTooDeep(aReflowState, aMetrics, aStatus)) {
    return;
  }

  bool    lazilySetParentPointer = false;

  nsIFrame* lineContainer = aReflowState.mLineLayout->LineContainerFrame();

   
  nsInlineFrame* prevInFlow = (nsInlineFrame*)GetPrevInFlow();
  if (prevInFlow) {
    AutoFrameListPtr prevOverflowFrames(aPresContext,
                                        prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      
      nsContainerFrame::ReparentFrameViewList(*prevOverflowFrames, prevInFlow,
                                              this);

      
      
      
      
      
      if ((GetStateBits() & NS_FRAME_FIRST_REFLOW) && mFrames.IsEmpty() &&
          !GetNextInFlow()) {
        
        
        
        
        mFrames.SetFrames(*prevOverflowFrames);
        lazilySetParentPointer = true;
      } else {
        
        if (lineContainer && lineContainer->GetPrevContinuation()) {
          ReparentFloatsForInlineChild(lineContainer,
                                       prevOverflowFrames->FirstChild(),
                                       true);
        }
        
        
        const nsFrameList::Slice& newFrames =
          mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
        
        
        
        
        
        if (aReflowState.mLineLayout->GetInFirstLine()) {
          ReparentChildListStyle(aPresContext, newFrames, this);
        }
      }
    }
  }

  
#ifdef DEBUG
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsFrameList* overflowFrames = GetOverflowFrames();
    NS_ASSERTION(!overflowFrames || overflowFrames->IsEmpty(),
                 "overflow list is not empty for initial reflow");
  }
#endif
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    DrainFlags flags =
      lazilySetParentPointer ? eDontReparentFrames : DrainFlags(0);
    if (aReflowState.mLineLayout->GetInFirstLine()) {
      flags = DrainFlags(flags | eInFirstLine);
    }
    DrainSelfOverflowListInternal(flags, lineContainer);
  }

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nullptr;
  irs.mLineContainer = lineContainer;
  irs.mLineLayout = aReflowState.mLineLayout;
  irs.mNextInFlow = (nsInlineFrame*) GetNextInFlow();
  irs.mSetParentPointer = lazilySetParentPointer;

  if (mFrames.IsEmpty()) {
    
    
    bool complete;
    (void) PullOneFrame(aPresContext, irs, &complete);
  }

  ReflowFrames(aPresContext, aReflowState, irs, aMetrics, aStatus);

  ReflowAbsoluteFrames(aPresContext, aMetrics, aReflowState, aStatus);

  
  

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
}

nsresult 
nsInlineFrame::AttributeChanged(int32_t aNameSpaceID,
                                nsIAtom* aAttribute,
                                int32_t aModType)
{
  nsresult rv =
    nsInlineFrameBase::AttributeChanged(aNameSpaceID, aAttribute, aModType);

  if (NS_FAILED(rv)) {
    return rv;
  }

  if (IsSVGText()) {
    SVGTextFrame* f = static_cast<SVGTextFrame*>(
      nsLayoutUtils::GetClosestFrameOfType(this, nsGkAtoms::svgTextFrame));
    f->HandleAttributeChangeInDescendant(mContent->AsElement(),
                                         aNameSpaceID, aAttribute);
  }

  return NS_OK;
}

bool
nsInlineFrame::DrainSelfOverflowListInternal(DrainFlags aFlags,
                                             nsIFrame* aLineContainer)
{
  AutoFrameListPtr overflowFrames(PresContext(), StealOverflowFrames());
  if (overflowFrames) {
    
    
    
    if (!(aFlags & eDontReparentFrames)) {
      nsIFrame* firstChild = overflowFrames->FirstChild();
      if (aLineContainer && aLineContainer->GetPrevContinuation()) {
        ReparentFloatsForInlineChild(aLineContainer, firstChild, true);
      }
      const bool doReparentSC =
        (aFlags & eInFirstLine) && !(aFlags & eForDestroy);
      RestyleManager* restyleManager = PresContext()->RestyleManager();
      for (nsIFrame* f = firstChild; f; f = f->GetNextSibling()) {
        f->SetParent(this);
        if (doReparentSC) {
          restyleManager->ReparentStyleContext(f);
          nsLayoutUtils::MarkDescendantsDirty(f);
        }
      }
    }
    bool result = !overflowFrames->IsEmpty();
    mFrames.AppendFrames(nullptr, *overflowFrames);
    return result;
  }
  return false;
}

 bool
nsInlineFrame::DrainSelfOverflowList()
{
  nsIFrame* lineContainer = nsLayoutUtils::FindNearestBlockAncestor(this);
  
  
  DrainFlags flags = DrainFlags(0);
  for (nsIFrame* p = GetParent(); p != lineContainer; p = p->GetParent()) {
    if (p->GetType() == nsGkAtoms::lineFrame) {
      flags = DrainFlags(flags | eInFirstLine);
      break;
    }
  }
  return DrainSelfOverflowListInternal(flags, lineContainer);
}

 bool
nsInlineFrame::CanContinueTextRun() const
{
  
  return true;
}

 void
nsInlineFrame::PullOverflowsFromPrevInFlow()
{
  nsInlineFrame* prevInFlow = static_cast<nsInlineFrame*>(GetPrevInFlow());
  if (prevInFlow) {
    nsPresContext* presContext = PresContext();
    AutoFrameListPtr prevOverflowFrames(presContext,
                                        prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      nsContainerFrame::ReparentFrameViewList(*prevOverflowFrames, prevInFlow,
                                              this);
      mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
    }
  }
}

void
nsInlineFrame::ReflowFrames(nsPresContext* aPresContext,
                            const nsHTMLReflowState& aReflowState,
                            InlineReflowState& irs,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus)
{
  aStatus = NS_FRAME_COMPLETE;

  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  bool inFirstLine = aReflowState.mLineLayout->GetInFirstLine();
  RestyleManager* restyleManager = aPresContext->RestyleManager();
  WritingMode frameWM = aReflowState.GetWritingMode();
  WritingMode lineWM = aReflowState.mLineLayout->mRootSpan->mWritingMode;
  LogicalMargin framePadding = aReflowState.ComputedLogicalBorderPadding();
  nscoord startEdge = 0;
  const bool boxDecorationBreakClone =
    MOZ_UNLIKELY(StyleBorder()->mBoxDecorationBreak ==
                   NS_STYLE_BOX_DECORATION_BREAK_CLONE);
  
  
  
  
  if ((!GetPrevContinuation() && !FrameIsNonFirstInIBSplit()) ||
      boxDecorationBreakClone) {
    startEdge = framePadding.IStart(frameWM);
  }
  nscoord availableISize = aReflowState.AvailableISize();
  NS_ASSERTION(availableISize != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  
  availableISize -= startEdge;
  availableISize -= framePadding.IEnd(frameWM);
  lineLayout->BeginSpan(this, &aReflowState, startEdge,
                        startEdge + availableISize, &mBaseline);

  
  nsIFrame* frame = mFrames.FirstChild();
  bool done = false;
  while (frame) {
    
    if (irs.mSetParentPointer) {
      bool havePrevBlock =
        irs.mLineContainer && irs.mLineContainer->GetPrevContinuation();
      nsIFrame* child = frame;
      do {
        
        
        if (havePrevBlock) {
          
          
          
          
          
          
          
          
          ReparentFloatsForInlineChild(irs.mLineContainer, child, false);
        }
        child->SetParent(this);
        if (inFirstLine) {
          restyleManager->ReparentStyleContext(child);
          nsLayoutUtils::MarkDescendantsDirty(child);
        }
        
        
        
        
        nsIFrame* nextSibling = child->GetNextSibling();
        child = child->GetNextInFlow();
        if (MOZ_UNLIKELY(child)) {
          while (child != nextSibling && nextSibling) {
            nextSibling = nextSibling->GetNextSibling();
          }
          if (!nextSibling) {
            child = nullptr;
          }
        }
        MOZ_ASSERT(!child || mFrames.ContainsFrame(child));
      } while (child);

      
      
      nsIFrame* realFrame = nsPlaceholderFrame::GetRealFrameFor(frame);
      if (realFrame->GetType() == nsGkAtoms::letterFrame) {
        nsIFrame* child = realFrame->GetFirstPrincipalChild();
        if (child) {
          NS_ASSERTION(child->GetType() == nsGkAtoms::textFrame,
                       "unexpected frame type");
          nsIFrame* nextInFlow = child->GetNextInFlow();
          for ( ; nextInFlow; nextInFlow = nextInFlow->GetNextInFlow()) {
            NS_ASSERTION(nextInFlow->GetType() == nsGkAtoms::textFrame,
                         "unexpected frame type");
            if (mFrames.ContainsFrame(nextInFlow)) {
              nextInFlow->SetParent(this);
              if (inFirstLine) {
                restyleManager->ReparentStyleContext(nextInFlow);
                nsLayoutUtils::MarkDescendantsDirty(nextInFlow);
              }
            }
            else {
#ifdef DEBUG              
              
              
              for ( ; nextInFlow; nextInFlow = nextInFlow->GetNextInFlow()) {
                NS_ASSERTION(!mFrames.ContainsFrame(nextInFlow),
                             "unexpected letter frame flow");
              }
#endif
              break;
            }
          }
        }
      }
    }
    MOZ_ASSERT(frame->GetParent() == this);

    if (!done) {
      bool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
      ReflowInlineFrame(aPresContext, aReflowState, irs, frame, aStatus);
      done = NS_INLINE_IS_BREAK(aStatus) || 
             (!reflowingFirstLetter && NS_FRAME_IS_NOT_COMPLETE(aStatus));
      if (done) {
        if (!irs.mSetParentPointer) {
          break;
        }
        
        nsFrameList* pushedFrames = GetOverflowFrames();
        if (pushedFrames && pushedFrames->FirstChild() == frame) {
          
          break;
        }
      } else {
        irs.mPrevFrame = frame;
      }
    }
    frame = frame->GetNextSibling();
  }

  
  if (!done && GetNextInFlow()) {
    while (true) {
      bool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
      bool isComplete;
      if (!frame) { 
                    
        frame = PullOneFrame(aPresContext, irs, &isComplete);
      }
#ifdef NOISY_PUSHING
      printf("%p pulled up %p\n", this, frame);
#endif
      if (nullptr == frame) {
        if (!isComplete) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
        break;
      }
      ReflowInlineFrame(aPresContext, aReflowState, irs, frame, aStatus);
      if (NS_INLINE_IS_BREAK(aStatus) || 
          (!reflowingFirstLetter && NS_FRAME_IS_NOT_COMPLETE(aStatus))) {
        break;
      }
      irs.mPrevFrame = frame;
      frame = frame->GetNextSibling();
    }
  }

  NS_ASSERTION(!NS_FRAME_IS_COMPLETE(aStatus) || !GetOverflowFrames(),
               "We can't be complete AND have overflow frames!");

  
  
  
  
  
  
  
  aMetrics.ISize(lineWM) = lineLayout->EndSpan(this);

  

  
  
  
  
  

  
  
  
  
  if ((!GetPrevContinuation() && !FrameIsNonFirstInIBSplit()) ||
      boxDecorationBreakClone) {
    aMetrics.ISize(lineWM) += framePadding.IStart(frameWM);
  }

  







  if ((NS_FRAME_IS_COMPLETE(aStatus) &&
       !LastInFlow()->GetNextContinuation() &&
       !FrameIsNonLastInIBSplit()) ||
      boxDecorationBreakClone) {
    aMetrics.ISize(lineWM) += framePadding.IEnd(frameWM);
  }

  nsLayoutUtils::SetBSizeFromFontMetrics(this, aMetrics,
                                         framePadding, lineWM, frameWM);

  
  
  aMetrics.mOverflowAreas.Clear();

#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": metrics=%d,%d ascent=%d\n",
         aMetrics.Width(), aMetrics.Height(), aMetrics.TopAscent());
#endif
}

void
nsInlineFrame::ReflowInlineFrame(nsPresContext* aPresContext,
                                 const nsHTMLReflowState& aReflowState,
                                 InlineReflowState& irs,
                                 nsIFrame* aFrame,
                                 nsReflowStatus& aStatus)
{
  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  bool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
  bool pushedFrame;
  lineLayout->ReflowFrame(aFrame, aStatus, nullptr, pushedFrame);
  
  if (NS_INLINE_IS_BREAK_BEFORE(aStatus)) {
    if (aFrame != mFrames.FirstChild()) {
      
      
      
      aStatus = NS_FRAME_NOT_COMPLETE |
        NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |
        (aStatus & NS_INLINE_BREAK_TYPE_MASK);
      PushFrames(aPresContext, aFrame, irs.mPrevFrame, irs);
    }
    else {
      
      
    }
    return;
  }

  
  if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
    CreateNextInFlow(aFrame);
  }

  if (NS_INLINE_IS_BREAK_AFTER(aStatus)) {
    nsIFrame* nextFrame = aFrame->GetNextSibling();
    if (nextFrame) {
      NS_FRAME_SET_INCOMPLETE(aStatus);
      PushFrames(aPresContext, nextFrame, aFrame, irs);
    }
    else {
      
      
      nsInlineFrame* nextInFlow = static_cast<nsInlineFrame*>(GetNextInFlow());
      while (nextInFlow) {
        if (nextInFlow->mFrames.NotEmpty()) {
          NS_FRAME_SET_INCOMPLETE(aStatus);
          break;
        }
        nextInFlow = static_cast<nsInlineFrame*>(nextInFlow->GetNextInFlow());
      }
    }
    return;
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus) && !reflowingFirstLetter) {
    nsIFrame* nextFrame = aFrame->GetNextSibling();
    if (nextFrame) {
      PushFrames(aPresContext, nextFrame, aFrame, irs);
    }
  }
}

nsIFrame*
nsInlineFrame::PullOneFrame(nsPresContext* aPresContext,
                            InlineReflowState& irs,
                            bool* aIsComplete)
{
  bool isComplete = true;

  nsIFrame* frame = nullptr;
  nsInlineFrame* nextInFlow = irs.mNextInFlow;
  while (nextInFlow) {
    frame = nextInFlow->mFrames.FirstChild();
    if (!frame) {
      
      nsFrameList* overflowFrames = nextInFlow->GetOverflowFrames();
      if (overflowFrames) {
        frame = overflowFrames->RemoveFirstChild();
        if (overflowFrames->IsEmpty()) {
          
          nextInFlow->DestroyOverflowList();
        } else {
          
          
          
        }
        
        
        nextInFlow->mFrames.SetFrames(frame);
      }
    }

    if (frame) {
      
      
      
      if (irs.mLineContainer && irs.mLineContainer->GetNextContinuation()) {
        
        
        
        ReparentFloatsForInlineChild(irs.mLineContainer, frame, false);
      }
      nextInFlow->mFrames.RemoveFirstChild();
      

      mFrames.InsertFrame(this, irs.mPrevFrame, frame);
      isComplete = false;
      if (irs.mLineLayout) {
        irs.mLineLayout->SetDirtyNextLine();
      }
      nsContainerFrame::ReparentFrameView(frame, nextInFlow, this);
      break;
    }
    nextInFlow = static_cast<nsInlineFrame*>(nextInFlow->GetNextInFlow());
    irs.mNextInFlow = nextInFlow;
  }

  *aIsComplete = isComplete;
  return frame;
}

void
nsInlineFrame::PushFrames(nsPresContext* aPresContext,
                          nsIFrame* aFromChild,
                          nsIFrame* aPrevSibling,
                          InlineReflowState& aState)
{
  NS_PRECONDITION(aFromChild, "null pointer");
  NS_PRECONDITION(aPrevSibling, "pushing first child");
  NS_PRECONDITION(aPrevSibling->GetNextSibling() == aFromChild, "bad prev sibling");

#ifdef NOISY_PUSHING
  printf("%p pushing aFromChild %p, disconnecting from prev sib %p\n", 
         this, aFromChild, aPrevSibling);
#endif

  
  
  SetOverflowFrames(mFrames.RemoveFramesAfter(aPrevSibling));
  if (aState.mLineLayout) {
    aState.mLineLayout->SetDirtyNextLine();
  }
}




nsIFrame::LogicalSides
nsInlineFrame::GetLogicalSkipSides(const nsHTMLReflowState* aReflowState) const
{
  if (MOZ_UNLIKELY(StyleBorder()->mBoxDecorationBreak ==
                     NS_STYLE_BOX_DECORATION_BREAK_CLONE)) {
    return LogicalSides();
  }

  LogicalSides skip;
  if (!IsFirst()) {
    nsInlineFrame* prev = (nsInlineFrame*) GetPrevContinuation();
    if ((GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET) ||
        (prev && (prev->mRect.height || prev->mRect.width))) {
      
      
      skip |= eLogicalSideBitsIStart;
    }
    else {
      
      
    }
  }
  if (!IsLast()) {
    nsInlineFrame* next = (nsInlineFrame*) GetNextContinuation();
    if ((GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET) ||
        (next && (next->mRect.height || next->mRect.width))) {
      
      
      skip |= eLogicalSideBitsIEnd;
    }
    else {
      
      
    }
  }

  if (GetStateBits() & NS_FRAME_PART_OF_IBSPLIT) {
    
    
    
    
    
    if (skip != LogicalSides(eLogicalSideBitsIBoth)) {
      
      
      nsIFrame* firstContinuation = FirstContinuation();
      if (firstContinuation->FrameIsNonLastInIBSplit()) {
        skip |= eLogicalSideBitsIEnd;
      }
      if (firstContinuation->FrameIsNonFirstInIBSplit()) {
        skip |= eLogicalSideBitsIStart;
      }
    }
  }

  return skip;
}

nscoord
nsInlineFrame::GetLogicalBaseline(mozilla::WritingMode aWritingMode) const
{
  return mBaseline;
}

#ifdef ACCESSIBILITY
a11y::AccType
nsInlineFrame::AccessibleType()
{
  
  
  if (mContent->IsHTMLElement(nsGkAtoms::input))  
    return a11y::eHTMLButtonType;
  if (mContent->IsHTMLElement(nsGkAtoms::img))  
    return a11y::eHyperTextType;

  return a11y::eNoType;
}
#endif





nsFirstLineFrame*
NS_NewFirstLineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFirstLineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFirstLineFrame)

void
nsFirstLineFrame::Init(nsIContent*       aContent,
                       nsContainerFrame* aParent,
                       nsIFrame*         aPrevInFlow)
{
  nsInlineFrame::Init(aContent, aParent, aPrevInFlow);
  if (!aPrevInFlow) {
    MOZ_ASSERT(StyleContext()->GetPseudo() == nsCSSPseudoElements::firstLine);
    return;
  }

  
  
  if (aPrevInFlow->StyleContext()->GetPseudo() == nsCSSPseudoElements::firstLine) {
    MOZ_ASSERT(FirstInFlow() == aPrevInFlow);
    
    
    
    
    nsStyleContext* parentContext = aParent->StyleContext();
    nsRefPtr<nsStyleContext> newSC = PresContext()->StyleSet()->
      ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozLineFrame, parentContext);
    SetStyleContext(newSC);
  } else {
    MOZ_ASSERT(FirstInFlow() != aPrevInFlow);
    MOZ_ASSERT(aPrevInFlow->StyleContext()->GetPseudo() ==
                 nsCSSAnonBoxes::mozLineFrame);
  }
}

#ifdef DEBUG_FRAME_DUMP
nsresult
nsFirstLineFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Line"), aResult);
}
#endif

nsIAtom*
nsFirstLineFrame::GetType() const
{
  return nsGkAtoms::lineFrame;
}

nsIFrame*
nsFirstLineFrame::PullOneFrame(nsPresContext* aPresContext, InlineReflowState& irs,
                               bool* aIsComplete)
{
  nsIFrame* frame = nsInlineFrame::PullOneFrame(aPresContext, irs, aIsComplete);
  if (frame && !GetPrevInFlow()) {
    
    
    NS_ASSERTION(frame->GetParent() == this, "Incorrect parent?");
    aPresContext->RestyleManager()->ReparentStyleContext(frame);
    nsLayoutUtils::MarkDescendantsDirty(frame);
  }
  return frame;
}

void
nsFirstLineFrame::Reflow(nsPresContext* aPresContext,
                         nsHTMLReflowMetrics& aMetrics,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus& aStatus)
{
  MarkInReflow();
  if (nullptr == aReflowState.mLineLayout) {
    return;  
  }

  nsIFrame* lineContainer = aReflowState.mLineLayout->LineContainerFrame();

  
  nsFirstLineFrame* prevInFlow = (nsFirstLineFrame*)GetPrevInFlow();
  if (prevInFlow) {
    AutoFrameListPtr prevOverflowFrames(aPresContext,
                                        prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      if (lineContainer && lineContainer->GetPrevContinuation()) {
        ReparentFloatsForInlineChild(lineContainer,
                                     prevOverflowFrames->FirstChild(),
                                     true);
      }
      const nsFrameList::Slice& newFrames =
        mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
      ReparentChildListStyle(aPresContext, newFrames, this);
    }
  }

  
  DrainSelfOverflowList();

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nullptr;
  irs.mLineContainer = lineContainer;
  irs.mLineLayout = aReflowState.mLineLayout;
  irs.mNextInFlow = (nsInlineFrame*) GetNextInFlow();

  bool wasEmpty = mFrames.IsEmpty();
  if (wasEmpty) {
    
    
    bool complete;
    PullOneFrame(aPresContext, irs, &complete);
  }

  if (nullptr == GetPrevInFlow()) {
    
    
    
    
    
    
    irs.mPrevFrame = mFrames.LastChild();
    for (;;) {
      bool complete;
      nsIFrame* frame = PullOneFrame(aPresContext, irs, &complete);
      if (!frame) {
        break;
      }
      irs.mPrevFrame = frame;
    }
    irs.mPrevFrame = nullptr;
  }

  NS_ASSERTION(!aReflowState.mLineLayout->GetInFirstLine(),
               "Nested first-line frames? BOGUS");
  aReflowState.mLineLayout->SetInFirstLine(true);
  ReflowFrames(aPresContext, aReflowState, irs, aMetrics, aStatus);
  aReflowState.mLineLayout->SetInFirstLine(false);

  ReflowAbsoluteFrames(aPresContext, aMetrics, aReflowState, aStatus);

  
}

 void
nsFirstLineFrame::PullOverflowsFromPrevInFlow()
{
  nsFirstLineFrame* prevInFlow = static_cast<nsFirstLineFrame*>(GetPrevInFlow());
  if (prevInFlow) {
    nsPresContext* presContext = PresContext();
    AutoFrameListPtr prevOverflowFrames(presContext,
                                        prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      const nsFrameList::Slice& newFrames =
        mFrames.InsertFrames(this, nullptr, *prevOverflowFrames);
      ReparentChildListStyle(presContext, newFrames, this);
    }
  }
}

 bool
nsFirstLineFrame::DrainSelfOverflowList()
{
  AutoFrameListPtr overflowFrames(PresContext(), StealOverflowFrames());
  if (overflowFrames) {
    bool result = !overflowFrames->IsEmpty();
    const nsFrameList::Slice& newFrames =
      mFrames.AppendFrames(nullptr, *overflowFrames);
    ReparentChildListStyle(PresContext(), newFrames, this);
    return result;
  }
  return false;
}
