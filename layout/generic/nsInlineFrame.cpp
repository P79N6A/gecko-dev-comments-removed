






































#include "nsCOMPtr.h"
#include "nsInlineFrame.h"
#include "nsBlockFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsGkAtoms.h"
#include "nsHTMLParts.h"
#include "nsStyleContext.h"
#include "nsIPresShell.h"
#include "nsPresContext.h"
#include "nsIRenderingContext.h"
#include "nsIFontMetrics.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsCSSAnonBoxes.h"
#include "nsAutoPtr.h"
#include "nsFrameManager.h"
#ifdef ACCESSIBILITY
#include "nsIServiceManager.h"
#include "nsAccessibilityService.h"
#endif
#include "nsDisplayList.h"

#ifdef DEBUG
#undef NOISY_PUSHING
#endif






nsIFrame*
NS_NewInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsInlineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsInlineFrame)

NS_QUERYFRAME_HEAD(nsInlineFrame)
  NS_QUERYFRAME_ENTRY(nsInlineFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsInlineFrameSuper)

#ifdef DEBUG
NS_IMETHODIMP
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

static inline PRBool
IsMarginZero(const nsStyleCoord &aCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Auto ||
         nsLayoutUtils::IsMarginZero(aCoord);
}

 PRBool
nsInlineFrame::IsSelfEmpty()
{
#if 0
  
  
  if (GetPresContext()->CompatibilityMode() == eCompatibility_FullStandards) {
    return PR_FALSE;
  }
#endif
  const nsStyleMargin* margin = GetStyleMargin();
  const nsStyleBorder* border = GetStyleBorder();
  const nsStylePadding* padding = GetStylePadding();
  
  
  
  PRBool haveRight =
    border->GetActualBorderWidth(NS_SIDE_RIGHT) != 0 ||
    !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetRight()) ||
    !IsMarginZero(margin->mMargin.GetRight());
  PRBool haveLeft =
    border->GetActualBorderWidth(NS_SIDE_LEFT) != 0 ||
    !nsLayoutUtils::IsPaddingZero(padding->mPadding.GetLeft()) ||
    !IsMarginZero(margin->mMargin.GetLeft());
  if (haveLeft || haveRight) {
    if (GetStateBits() & NS_FRAME_IS_SPECIAL) {
      PRBool haveStart, haveEnd;
      if (NS_STYLE_DIRECTION_LTR == GetStyleVisibility()->mDirection) {
        haveStart = haveLeft;
        haveEnd = haveRight;
      } else {
        haveStart = haveRight;
        haveEnd = haveLeft;
      }
      
      
      

      
      
      nsIFrame* firstCont = GetFirstContinuation();
      return
        (!haveStart || nsLayoutUtils::FrameIsNonFirstInIBSplit(firstCont)) &&
        (!haveEnd || nsLayoutUtils::FrameIsNonLastInIBSplit(firstCont));
    }
    return PR_FALSE;
  }
  return PR_TRUE;
}

PRBool
nsInlineFrame::IsEmpty()
{
  if (!IsSelfEmpty()) {
    return PR_FALSE;
  }

  for (nsIFrame *kid = mFrames.FirstChild(); kid; kid = kid->GetNextSibling()) {
    if (!kid->IsEmpty())
      return PR_FALSE;
  }

  return PR_TRUE;
}

PRBool
nsInlineFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset,
                                   PRBool aRespectClusters)
{
  
  NS_ASSERTION (aOffset && *aOffset <= 1, "aOffset out of range");
  PRInt32 startOffset = *aOffset;
  if (startOffset < 0)
    startOffset = 1;
  if (aForward == (startOffset == 0)) {
    
    
    *aOffset = 1 - startOffset;
  }
  return PR_FALSE;
}

NS_IMETHODIMP
nsInlineFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists)
{
  nsresult rv = nsHTMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
  NS_ENSURE_SUCCESS(rv, rv);
  
  
  
  
  
  if (!mFrames.FirstChild()) {
    rv = DisplaySelectionOverlay(aBuilder, aLists.Content());
  }
  return rv;
}




 void
nsInlineFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 nsIFrame::InlineMinWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::MIN_WIDTH);
}

 void
nsInlineFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  nsIFrame::InlinePrefWidthData *aData)
{
  DoInlineIntrinsicWidth(aRenderingContext, aData, nsLayoutUtils::PREF_WIDTH);
}

 nsSize
nsInlineFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
                           nsSize aCBSize, nscoord aAvailableWidth,
                           nsSize aMargin, nsSize aBorder, nsSize aPadding,
                           PRBool aShrinkWrap)
{
  
  return nsSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
}

nsRect
nsInlineFrame::ComputeTightBounds(gfxContext* aContext) const
{
  
  if (GetStyleContext()->HasTextDecorations())
    return GetVisualOverflowRect();
  return ComputeSimpleTightBounds(aContext);
}

void
nsInlineFrame::ReparentFloatsForInlineChild(nsIFrame* aOurLineContainer,
                                            nsIFrame* aFrame,
                                            PRBool aReparentSiblings)
{
  
  
  NS_ASSERTION(aOurLineContainer->GetNextContinuation() ||
               aOurLineContainer->GetPrevContinuation(),
               "Don't call this when we have no continuation, it's a waste");
  if (!aFrame) {
    NS_ASSERTION(aReparentSiblings, "Why did we get called?");
    return;
  }

  nsIFrame* ancestor = aFrame;
  nsIFrame* ancestorBlockChild;
  do {
    ancestorBlockChild = ancestor;
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

  const nsFrameList& blockChildren(ancestor->GetChildList(nsnull));
  PRBool isOverflow = !blockChildren.ContainsFrame(ancestorBlockChild);

  while (PR_TRUE) {
    ourBlock->ReparentFloats(aFrame, frameBlock, isOverflow, PR_FALSE);

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
  nsFrameManager *frameManager = aPresContext->FrameManager();

  for (nsFrameList::Enumerator e(aFrames); !e.AtEnd(); e.Next()) {
    NS_ASSERTION(e.get()->GetParent() == aParentFrame, "Bogus parentage");
    frameManager->ReparentStyleContext(e.get());
  }
}

NS_IMETHODIMP
nsInlineFrame::Reflow(nsPresContext*          aPresContext,
                      nsHTMLReflowMetrics&     aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsInlineFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
  if (nsnull == aReflowState.mLineLayout) {
    return NS_ERROR_INVALID_ARG;
  }

  PRBool  lazilySetParentPointer = PR_FALSE;

  nsIFrame* lineContainer = aReflowState.mLineLayout->GetLineContainerFrame();

   
  nsInlineFrame* prevInFlow = (nsInlineFrame*)GetPrevInFlow();
  if (nsnull != prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());

    if (prevOverflowFrames) {
      
      
      nsHTMLContainerFrame::ReparentFrameViewList(aPresContext,
                                                  *prevOverflowFrames,
                                                  prevInFlow, this);

      
      
      
      
      
      if ((GetStateBits() & NS_FRAME_FIRST_REFLOW) && mFrames.IsEmpty() &&
          !GetNextInFlow()) {
        
        
        
        
        mFrames.SetFrames(*prevOverflowFrames);
        lazilySetParentPointer = PR_TRUE;
      } else {
        
        if (lineContainer && lineContainer->GetPrevContinuation()) {
          ReparentFloatsForInlineChild(lineContainer,
                                       prevOverflowFrames->FirstChild(),
                                       PR_TRUE);
        }
        
        
        const nsFrameList::Slice& newFrames =
          mFrames.InsertFrames(this, nsnull, *prevOverflowFrames);
        
        
        
        
        
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
    nsAutoPtr<nsFrameList> overflowFrames(StealOverflowFrames());
    if (overflowFrames) {
      NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");

      
      
      
      mFrames.AppendFrames(this, *overflowFrames);
    }
  }

  if (IsFrameTreeTooDeep(aReflowState, aMetrics, aStatus)) {
    return NS_OK;
  }

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nsnull;
  irs.mLineContainer = lineContainer;
  irs.mLineLayout = aReflowState.mLineLayout;
  irs.mNextInFlow = (nsInlineFrame*) GetNextInFlow();
  irs.mSetParentPointer = lazilySetParentPointer;

  nsresult rv;
  if (mFrames.IsEmpty()) {
    
    
    PRBool complete;
    (void) PullOneFrame(aPresContext, irs, &complete);
  }

  rv = ReflowFrames(aPresContext, aReflowState, irs, aMetrics, aStatus);
  
  
  

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return rv;
}

 PRBool
nsInlineFrame::CanContinueTextRun() const
{
  
  return PR_TRUE;
}

 void
nsInlineFrame::PullOverflowsFromPrevInFlow()
{
  nsInlineFrame* prevInFlow = static_cast<nsInlineFrame*>(GetPrevInFlow());
  if (prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      nsHTMLContainerFrame::ReparentFrameViewList(PresContext(),
                                                  *prevOverflowFrames,
                                                  prevInFlow, this);
      mFrames.InsertFrames(this, nsnull, *prevOverflowFrames);
    }
  }
}

nsresult
nsInlineFrame::ReflowFrames(nsPresContext* aPresContext,
                            const nsHTMLReflowState& aReflowState,
                            InlineReflowState& irs,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus)
{
  nsresult rv = NS_OK;
  aStatus = NS_FRAME_COMPLETE;

  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  PRBool inFirstLine = aReflowState.mLineLayout->GetInFirstLine();
  nsFrameManager* frameManager = aPresContext->FrameManager();
  PRBool ltr = (NS_STYLE_DIRECTION_LTR == aReflowState.mStyleVisibility->mDirection);
  nscoord leftEdge = 0;
  
  
  if (!GetPrevContinuation() &&
      !nsLayoutUtils::FrameIsNonFirstInIBSplit(this)) {
    leftEdge = ltr ? aReflowState.mComputedBorderPadding.left
                   : aReflowState.mComputedBorderPadding.right;
  }
  nscoord availableWidth = aReflowState.availableWidth;
  NS_ASSERTION(availableWidth != NS_UNCONSTRAINEDSIZE,
               "should no longer use available widths");
  
  availableWidth -= leftEdge;
  availableWidth -= ltr ? aReflowState.mComputedBorderPadding.right
                        : aReflowState.mComputedBorderPadding.left;
  lineLayout->BeginSpan(this, &aReflowState, leftEdge, leftEdge + availableWidth);

  
  nsIFrame* frame = mFrames.FirstChild();
  PRBool done = PR_FALSE;
  while (nsnull != frame) {
    PRBool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();

    
    if (irs.mSetParentPointer) {
      PRBool havePrevBlock =
        irs.mLineContainer && irs.mLineContainer->GetPrevContinuation();
      
      
      if (havePrevBlock) {
        
        
        
        
        
        
        
        
        ReparentFloatsForInlineChild(irs.mLineContainer, frame, PR_FALSE);
      }
      frame->SetParent(this);
      if (inFirstLine) {
        frameManager->ReparentStyleContext(frame);
      }
      
      
      
      
      nsIFrame* nextInFlow = frame->GetNextInFlow();
      for ( ; nextInFlow; nextInFlow = nextInFlow->GetNextInFlow()) {
        
        
        
        
        NS_ASSERTION(mFrames.ContainsFrame(nextInFlow), "unexpected flow");
        if (havePrevBlock) {
          ReparentFloatsForInlineChild(irs.mLineContainer, nextInFlow, PR_FALSE);
        }
        nextInFlow->SetParent(this);
        if (inFirstLine) {
          frameManager->ReparentStyleContext(nextInFlow);
        }
      }

      
      
      nsIFrame* realFrame = nsPlaceholderFrame::GetRealFrameFor(frame);
      if (realFrame->GetType() == nsGkAtoms::letterFrame) {
        nsIFrame* child = realFrame->GetFirstChild(nsnull);
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
                frameManager->ReparentStyleContext(nextInFlow);
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
    rv = ReflowInlineFrame(aPresContext, aReflowState, irs, frame, aStatus);
    if (NS_FAILED(rv)) {
      done = PR_TRUE;
      break;
    }
    if (NS_INLINE_IS_BREAK(aStatus) || 
        (!reflowingFirstLetter && NS_FRAME_IS_NOT_COMPLETE(aStatus))) {
      done = PR_TRUE;
      break;
    }
    irs.mPrevFrame = frame;
    frame = frame->GetNextSibling();
  }

  
  if (!done && (nsnull != GetNextInFlow())) {
    while (!done) {
      PRBool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
      PRBool isComplete;
      if (!frame) { 
                    
        frame = PullOneFrame(aPresContext, irs, &isComplete);
      }
#ifdef NOISY_PUSHING
      printf("%p pulled up %p\n", this, frame);
#endif
      if (nsnull == frame) {
        if (!isComplete) {
          aStatus = NS_FRAME_NOT_COMPLETE;
        }
        break;
      }
      rv = ReflowInlineFrame(aPresContext, aReflowState, irs, frame, aStatus);
      if (NS_FAILED(rv)) {
        done = PR_TRUE;
        break;
      }
      if (NS_INLINE_IS_BREAK(aStatus) || 
          (!reflowingFirstLetter && NS_FRAME_IS_NOT_COMPLETE(aStatus))) {
        done = PR_TRUE;
        break;
      }
      irs.mPrevFrame = frame;
      frame = frame->GetNextSibling();
    }
  }
#ifdef DEBUG
  if (NS_FRAME_IS_COMPLETE(aStatus)) {
    
    NS_ASSERTION(!GetOverflowFrames(), "whoops");
  }
#endif

  
  
  
  
  
  
  
  aMetrics.width = lineLayout->EndSpan(this);

  

  
  
  
  if (!GetPrevContinuation() &&
      !nsLayoutUtils::FrameIsNonFirstInIBSplit(this)) {
    aMetrics.width += ltr ? aReflowState.mComputedBorderPadding.left
                          : aReflowState.mComputedBorderPadding.right;
  }

  






  if (NS_FRAME_IS_COMPLETE(aStatus) &&
      !GetLastInFlow()->GetNextContinuation() &&
      !nsLayoutUtils::FrameIsNonLastInIBSplit(this)) {
    aMetrics.width += ltr ? aReflowState.mComputedBorderPadding.right
                          : aReflowState.mComputedBorderPadding.left;
  }

  nsLayoutUtils::SetFontFromStyle(aReflowState.rendContext, mStyleContext);
  nsCOMPtr<nsIFontMetrics> fm = aReflowState.rendContext->GetFontMetrics();

  if (fm) {
    
    
    
    
    
    
    
    
    
    
    fm->GetMaxAscent(aMetrics.ascent);
    fm->GetHeight(aMetrics.height);
  } else {
    NS_WARNING("Cannot get font metrics - defaulting sizes to 0");
    aMetrics.ascent = aMetrics.height = 0;
  }
  aMetrics.ascent += aReflowState.mComputedBorderPadding.top;
  aMetrics.height += aReflowState.mComputedBorderPadding.top +
    aReflowState.mComputedBorderPadding.bottom;

  
  
  aMetrics.mOverflowAreas.Clear();

#ifdef NOISY_FINAL_SIZE
  ListTag(stdout);
  printf(": metrics=%d,%d ascent=%d\n",
         aMetrics.width, aMetrics.height, aMetrics.ascent);
#endif

  return rv;
}

nsresult
nsInlineFrame::ReflowInlineFrame(nsPresContext* aPresContext,
                                 const nsHTMLReflowState& aReflowState,
                                 InlineReflowState& irs,
                                 nsIFrame* aFrame,
                                 nsReflowStatus& aStatus)
{
  nsLineLayout* lineLayout = aReflowState.mLineLayout;
  PRBool reflowingFirstLetter = lineLayout->GetFirstLetterStyleOK();
  PRBool pushedFrame;
  nsresult rv =
    lineLayout->ReflowFrame(aFrame, aStatus, nsnull, pushedFrame);
  
  if (NS_FAILED(rv)) {
    return rv;
  }

  if (NS_INLINE_IS_BREAK_BEFORE(aStatus)) {
    if (aFrame != mFrames.FirstChild()) {
      
      
      
      aStatus = NS_FRAME_NOT_COMPLETE |
        NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |
        (aStatus & NS_INLINE_BREAK_TYPE_MASK);
      PushFrames(aPresContext, aFrame, irs.mPrevFrame, irs);
    }
    else {
      
      
      
      
      
      
      if (irs.mSetParentPointer) {
        if (irs.mLineContainer && irs.mLineContainer->GetPrevContinuation()) {
          ReparentFloatsForInlineChild(irs.mLineContainer, aFrame->GetNextSibling(),
                                       PR_TRUE);
        }
        for (nsIFrame* f = aFrame->GetNextSibling(); f; f = f->GetNextSibling()) {
          f->SetParent(this);
          if (lineLayout->GetInFirstLine()) {
            aPresContext->FrameManager()->ReparentStyleContext(f);
          }
        }
      }
    }
    return NS_OK;
  }

  
  if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus)) {
    nsIFrame* newFrame;
    rv = CreateNextInFlow(aPresContext, aFrame, newFrame);
    if (NS_FAILED(rv)) {
      return rv;
    }
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
    return NS_OK;
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(aStatus) && !reflowingFirstLetter) {
    nsIFrame* nextFrame = aFrame->GetNextSibling();
    if (nextFrame) {
      PushFrames(aPresContext, nextFrame, aFrame, irs);
    }
  }
  return NS_OK;
}

nsIFrame*
nsInlineFrame::PullOneFrame(nsPresContext* aPresContext,
                            InlineReflowState& irs,
                            PRBool* aIsComplete)
{
  PRBool isComplete = PR_TRUE;

  nsIFrame* frame = nsnull;
  nsInlineFrame* nextInFlow = irs.mNextInFlow;
  while (nsnull != nextInFlow) {
    frame = nextInFlow->mFrames.FirstChild();

    if (!frame) {
      
      
      nsAutoPtr<nsFrameList> overflowFrames(nextInFlow->StealOverflowFrames());
      if (overflowFrames) {
        nextInFlow->mFrames.SetFrames(*overflowFrames);
        frame = nextInFlow->mFrames.FirstChild();
      }
    }

    if (nsnull != frame) {
      
      
      
      if (irs.mLineContainer && irs.mLineContainer->GetNextContinuation()) {
        
        
        
        ReparentFloatsForInlineChild(irs.mLineContainer, frame, PR_FALSE);
      }
      nextInFlow->mFrames.RemoveFirstChild();

      
      
      if (!nextInFlow->mFrames.FirstChild()) {
        nsAutoPtr<nsFrameList> overflowFrames(nextInFlow->StealOverflowFrames());
        if (overflowFrames) {
          nextInFlow->mFrames.SetFrames(*overflowFrames);
        }
      }

      mFrames.InsertFrame(this, irs.mPrevFrame, frame);
      isComplete = PR_FALSE;
      if (irs.mLineLayout) {
        irs.mLineLayout->SetDirtyNextLine();
      }
      nsHTMLContainerFrame::ReparentFrameView(aPresContext, frame, nextInFlow, this);
      break;
    }
    nextInFlow = (nsInlineFrame*) nextInFlow->GetNextInFlow();
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

  
  
  SetOverflowFrames(aPresContext, mFrames.RemoveFramesAfter(aPrevSibling));
  if (aState.mLineLayout) {
    aState.mLineLayout->SetDirtyNextLine();
  }
}




PRIntn
nsInlineFrame::GetSkipSides() const
{
  PRIntn skip = 0;
  if (!IsLeftMost()) {
    nsInlineFrame* prev = (nsInlineFrame*) GetPrevContinuation();
    if ((GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET) ||
        (prev && (prev->mRect.height || prev->mRect.width))) {
      
      
      skip |= 1 << NS_SIDE_LEFT;
    }
    else {
      
      
    }
  }
  if (!IsRightMost()) {
    nsInlineFrame* next = (nsInlineFrame*) GetNextContinuation();
    if ((GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET) ||
        (next && (next->mRect.height || next->mRect.width))) {
      
      
      skip |= 1 << NS_SIDE_RIGHT;
    }
    else {
      
      
    }
  }

  if (GetStateBits() & NS_FRAME_IS_SPECIAL) {
    
    
    
    
    
    PRBool ltr = (NS_STYLE_DIRECTION_LTR == GetStyleVisibility()->mDirection);
    PRIntn startBit = (1 << (ltr ? NS_SIDE_LEFT : NS_SIDE_RIGHT));
    PRIntn endBit = (1 << (ltr ? NS_SIDE_RIGHT : NS_SIDE_LEFT));
    if (((startBit | endBit) & skip) != (startBit | endBit)) {
      
      
      nsIFrame* firstContinuation = GetFirstContinuation();
      if (nsLayoutUtils::FrameIsNonLastInIBSplit(firstContinuation)) {
        skip |= endBit;
      }
      if (nsLayoutUtils::FrameIsNonFirstInIBSplit(firstContinuation)) {
        skip |= startBit;
      }
    }
  }

  return skip;
}

nscoord
nsInlineFrame::GetBaseline() const
{
  nscoord ascent = 0;
  nsCOMPtr<nsIFontMetrics> fm;
  if (NS_SUCCEEDED(nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm)))) {
    fm->GetMaxAscent(ascent);
  }
  return NS_MIN(mRect.height, ascent + GetUsedBorderAndPadding().top);
}

#ifdef ACCESSIBILITY
already_AddRefed<nsAccessible>
nsInlineFrame::CreateAccessible()
{
  
  
  nsIAtom *tagAtom = mContent->Tag();
  if ((tagAtom == nsGkAtoms::img || tagAtom == nsGkAtoms::input || 
       tagAtom == nsGkAtoms::label) && mContent->IsHTML()) {
    

    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (!accService)
      return nsnull;
    if (tagAtom == nsGkAtoms::input)  
      return accService->CreateHTMLButtonAccessible(mContent,
                                                    PresContext()->PresShell());
    else if (tagAtom == nsGkAtoms::img)  
      return accService->CreateHTMLImageAccessible(mContent,
                                                   PresContext()->PresShell());
    else if (tagAtom == nsGkAtoms::label)  
      return accService->CreateHTMLLabelAccessible(mContent,
                                                   PresContext()->PresShell());
  }

  return nsnull;
}
#endif





nsIFrame*
NS_NewFirstLineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFirstLineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsFirstLineFrame)

#ifdef DEBUG
NS_IMETHODIMP
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
                               PRBool* aIsComplete)
{
  nsIFrame* frame = nsInlineFrame::PullOneFrame(aPresContext, irs, aIsComplete);
  if (frame && !GetPrevInFlow()) {
    
    
    NS_ASSERTION(frame->GetParent() == this, "Incorrect parent?");
    aPresContext->FrameManager()->ReparentStyleContext(frame);
  }
  return frame;
}

NS_IMETHODIMP
nsFirstLineFrame::Reflow(nsPresContext* aPresContext,
                         nsHTMLReflowMetrics& aMetrics,
                         const nsHTMLReflowState& aReflowState,
                         nsReflowStatus& aStatus)
{
  if (nsnull == aReflowState.mLineLayout) {
    return NS_ERROR_INVALID_ARG;
  }

  nsIFrame* lineContainer = aReflowState.mLineLayout->GetLineContainerFrame();

  
  nsFirstLineFrame* prevInFlow = (nsFirstLineFrame*)GetPrevInFlow();
  if (nsnull != prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      if (lineContainer && lineContainer->GetPrevContinuation()) {
        ReparentFloatsForInlineChild(lineContainer,
                                     prevOverflowFrames->FirstChild(),
                                     PR_TRUE);
      }
      const nsFrameList::Slice& newFrames =
        mFrames.InsertFrames(this, nsnull, *prevOverflowFrames);
      ReparentChildListStyle(aPresContext, newFrames, this);
    }
  }

  
  nsAutoPtr<nsFrameList> overflowFrames(StealOverflowFrames());
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");

    const nsFrameList::Slice& newFrames =
      mFrames.AppendFrames(nsnull, *overflowFrames);
    ReparentChildListStyle(aPresContext, newFrames, this);
  }

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nsnull;
  irs.mLineContainer = lineContainer;
  irs.mLineLayout = aReflowState.mLineLayout;
  irs.mNextInFlow = (nsInlineFrame*) GetNextInFlow();

  nsresult rv;
  PRBool wasEmpty = mFrames.IsEmpty();
  if (wasEmpty) {
    
    
    PRBool complete;
    PullOneFrame(aPresContext, irs, &complete);
  }

  if (nsnull == GetPrevInFlow()) {
    
    
    
    
    
    
    irs.mPrevFrame = mFrames.LastChild();
    for (;;) {
      PRBool complete;
      nsIFrame* frame = PullOneFrame(aPresContext, irs, &complete);
      if (!frame) {
        break;
      }
      irs.mPrevFrame = frame;
    }
    irs.mPrevFrame = nsnull;
  }
  else {

    
    
    
    
    nsFirstLineFrame* first = (nsFirstLineFrame*) GetFirstInFlow();
    if (mStyleContext == first->mStyleContext) {
      
      
      nsStyleContext* parentContext = first->GetParent()->GetStyleContext();
      if (parentContext) {
        
        
        
        
        nsRefPtr<nsStyleContext> newSC;
        newSC = aPresContext->StyleSet()->
          ResolveAnonymousBoxStyle(nsCSSAnonBoxes::mozLineFrame, parentContext);
        if (newSC) {
          
          SetStyleContext(newSC);

          
          ReparentChildListStyle(aPresContext, mFrames, this);
        }
      }
    }
  }

  NS_ASSERTION(!aReflowState.mLineLayout->GetInFirstLine(),
               "Nested first-line frames? BOGUS");
  aReflowState.mLineLayout->SetInFirstLine(PR_TRUE);
  rv = ReflowFrames(aPresContext, aReflowState, irs, aMetrics, aStatus);
  aReflowState.mLineLayout->SetInFirstLine(PR_FALSE);

  

  return rv;
}

 void
nsFirstLineFrame::PullOverflowsFromPrevInFlow()
{
  nsFirstLineFrame* prevInFlow = static_cast<nsFirstLineFrame*>(GetPrevInFlow());
  if (prevInFlow) {
    nsAutoPtr<nsFrameList> prevOverflowFrames(prevInFlow->StealOverflowFrames());
    if (prevOverflowFrames) {
      
      const nsFrameList::Slice& newFrames =
        mFrames.InsertFrames(this, nsnull, *prevOverflowFrames);
      ReparentChildListStyle(PresContext(), newFrames, this);
    }
  }
}



nsIFrame*
NS_NewPositionedInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPositionedInlineFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsPositionedInlineFrame)

void
nsPositionedInlineFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  mAbsoluteContainer.DestroyFrames(this, aDestructRoot);
  nsInlineFrame::DestroyFrom(aDestructRoot);
}

NS_IMETHODIMP
nsPositionedInlineFrame::SetInitialChildList(nsIAtom*        aListName,
                                             nsFrameList&    aChildList)
{
  nsresult  rv;

  if (nsGkAtoms::absoluteList == aListName) {
    rv = mAbsoluteContainer.SetInitialChildList(this, aListName, aChildList);
  } else {
    rv = nsInlineFrame::SetInitialChildList(aListName, aChildList);
  }

  return rv;
}

NS_IMETHODIMP
nsPositionedInlineFrame::AppendFrames(nsIAtom*        aListName,
                                      nsFrameList&    aFrameList)
{
  nsresult  rv;
  
  if (nsGkAtoms::absoluteList == aListName) {
    rv = mAbsoluteContainer.AppendFrames(this, aListName, aFrameList);
  } else {
    rv = nsInlineFrame::AppendFrames(aListName, aFrameList);
  }

  return rv;
}
  
NS_IMETHODIMP
nsPositionedInlineFrame::InsertFrames(nsIAtom*        aListName,
                                      nsIFrame*       aPrevFrame,
                                      nsFrameList&    aFrameList)
{
  nsresult  rv;

  if (nsGkAtoms::absoluteList == aListName) {
    rv = mAbsoluteContainer.InsertFrames(this, aListName, aPrevFrame,
                                         aFrameList);
  } else {
    rv = nsInlineFrame::InsertFrames(aListName, aPrevFrame, aFrameList);
  }

  return rv;
}
  
NS_IMETHODIMP
nsPositionedInlineFrame::RemoveFrame(nsIAtom*        aListName,
                                     nsIFrame*       aOldFrame)
{
  nsresult  rv;

  if (nsGkAtoms::absoluteList == aListName) {
    mAbsoluteContainer.RemoveFrame(this, aListName, aOldFrame);
    rv = NS_OK;
  } else {
    rv = nsInlineFrame::RemoveFrame(aListName, aOldFrame);
  }

  return rv;
}

NS_IMETHODIMP
nsPositionedInlineFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                          const nsRect&           aDirtyRect,
                                          const nsDisplayListSet& aLists)
{
  aBuilder->MarkFramesForDisplayList(this, mAbsoluteContainer.GetChildList(),
				     aDirtyRect);
  return nsHTMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}

nsIAtom*
nsPositionedInlineFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (0 == aIndex) {
    return nsGkAtoms::absoluteList;
  }
  return nsnull;
}

nsFrameList
nsPositionedInlineFrame::GetChildList(nsIAtom* aListName) const
{
  if (nsGkAtoms::absoluteList == aListName)
    return mAbsoluteContainer.GetChildList();

  return nsInlineFrame::GetChildList(aListName);
}

nsIAtom*
nsPositionedInlineFrame::GetType() const
{
  return nsGkAtoms::positionedInlineFrame;
}

NS_IMETHODIMP
nsPositionedInlineFrame::Reflow(nsPresContext*          aPresContext,
                                nsHTMLReflowMetrics&     aDesiredSize,
                                const nsHTMLReflowState& aReflowState,
                                nsReflowStatus&          aStatus)
{
  nsresult  rv = NS_OK;

  
  

  
  rv = nsInlineFrame::Reflow(aPresContext, aDesiredSize, aReflowState, aStatus);

  
  
  
  
  
  
  
  
  
  if (NS_SUCCEEDED(rv) &&
      mAbsoluteContainer.HasAbsoluteFrames()) {
    
    nsMargin computedBorder =
      aReflowState.mComputedBorderPadding - aReflowState.mComputedPadding;
    nscoord containingBlockWidth =
      aDesiredSize.width - computedBorder.LeftRight();
    nscoord containingBlockHeight =
      aDesiredSize.height - computedBorder.TopBottom();

    
    
    
    
    rv = mAbsoluteContainer.Reflow(this, aPresContext, aReflowState, aStatus,
                                   containingBlockWidth, containingBlockHeight,
                                   PR_TRUE, PR_TRUE, PR_TRUE, 
                                   &aDesiredSize.mOverflowAreas);
  }

  return rv;
}
