






































#include "nsCOMPtr.h"
#include "nsInlineFrame.h"
#include "nsBlockFrame.h"
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
#include "nsIAccessibilityService.h"
#endif
#include "nsDisplayList.h"

#ifdef DEBUG
#undef NOISY_PUSHING
#endif


NS_DEFINE_IID(kInlineFrameCID, NS_INLINE_FRAME_CID);






nsIFrame*
NS_NewInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsInlineFrame(aContext);
}

NS_IMETHODIMP
nsInlineFrame::QueryInterface(const nsIID& aIID, void** aInstancePtr)
{
  NS_PRECONDITION(aInstancePtr, "null out param");

  if (aIID.Equals(kInlineFrameCID)) {
    *aInstancePtr = this;
    return NS_OK;
  }

  return nsInlineFrameSuper::QueryInterface(aIID, aInstancePtr);
}

void
nsInlineFrame::Destroy()
{
  if (mState & NS_FRAME_GENERATED_CONTENT) {
    
    
    
    
    

    
    
    
    
    nsContainerFrame::CleanupGeneratedContentIn(mContent, this);
  }
  nsInlineFrameSuper::Destroy();
}

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

inline PRBool
IsPaddingZero(nsStyleUnit aUnit, nsStyleCoord &aCoord)
{
    return ((aUnit == eStyleUnit_Coord && aCoord.GetCoordValue() == 0) ||
            (aUnit == eStyleUnit_Percent && aCoord.GetPercentValue() == 0.0));
}

inline PRBool
IsMarginZero(nsStyleUnit aUnit, nsStyleCoord &aCoord)
{
    return (aUnit == eStyleUnit_Auto ||
            (aUnit == eStyleUnit_Coord && aCoord.GetCoordValue() == 0) ||
            (aUnit == eStyleUnit_Percent && aCoord.GetPercentValue() == 0.0));
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
  nsStyleCoord coord;
  
  
  
  if (border->GetBorderWidth(NS_SIDE_RIGHT) != 0 ||
      border->GetBorderWidth(NS_SIDE_LEFT) != 0 ||
      !IsPaddingZero(padding->mPadding.GetRightUnit(),
                     padding->mPadding.GetRight(coord)) ||
      !IsPaddingZero(padding->mPadding.GetLeftUnit(),
                     padding->mPadding.GetLeft(coord)) ||
      !IsMarginZero(margin->mMargin.GetRightUnit(),
                    margin->mMargin.GetRight(coord)) ||
      !IsMarginZero(margin->mMargin.GetLeftUnit(),
                    margin->mMargin.GetLeft(coord))) {
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
nsInlineFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset)
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
    rv = DisplaySelectionOverlay(aBuilder, aLists);
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

void
nsInlineFrame::ReparentFloatsForInlineChild(nsIFrame* aOurLineContainer,
                                            nsIFrame* aFrame,
                                            PRBool aReparentSiblings)
{
  NS_ASSERTION(aOurLineContainer->GetNextContinuation() ||
               aOurLineContainer->GetPrevContinuation(),
               "Don't call this when we have no continuation, it's a waste");

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

  nsBlockFrame* ourBlock;
  nsresult rv = aOurLineContainer->QueryInterface(kBlockFrameCID, (void**)&ourBlock);
  NS_ASSERTION(NS_SUCCEEDED(rv), "Not a block, but broke vertically?");
  nsBlockFrame* frameBlock;
  rv = ancestor->QueryInterface(kBlockFrameCID, (void**)&frameBlock);
  NS_ASSERTION(NS_SUCCEEDED(rv), "ancestor not a block");

  nsFrameList blockChildren(ancestor->GetFirstChild(nsnull));
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
    nsIFrame* prevOverflowFrames = prevInFlow->GetOverflowFrames(aPresContext, PR_TRUE);

    if (prevOverflowFrames) {
      
      
      nsHTMLContainerFrame::ReparentFrameViewList(aPresContext, prevOverflowFrames,
                                                  prevInFlow, this);

      if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
        
        
        
        
        
        
        NS_ASSERTION(mFrames.IsEmpty(), "child list is not empty for initial reflow");
        mFrames.SetFrames(prevOverflowFrames);
        lazilySetParentPointer = PR_TRUE;
      } else {
        
        if (lineContainer && lineContainer->GetPrevContinuation()) {
          ReparentFloatsForInlineChild(lineContainer, prevOverflowFrames, PR_TRUE);
        }
        
        
        mFrames.InsertFrames(this, nsnull, prevOverflowFrames);
      }
    }
  }

  
#ifdef DEBUG
  if (GetStateBits() & NS_FRAME_FIRST_REFLOW) {
    
    
    
    nsIFrame* overflowFrames = GetOverflowFrames(aPresContext, PR_FALSE);
    NS_ASSERTION(!overflowFrames, "overflow list is not empty for initial reflow");
  }
#endif
  if (!(GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    nsIFrame* overflowFrames = GetOverflowFrames(aPresContext, PR_TRUE);
    if (overflowFrames) {
      NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");

      
      
      
      mFrames.AppendFrames(this, overflowFrames);
    }
  }

  if (IsFrameTreeTooDeep(aReflowState, aMetrics)) {
#ifdef DEBUG_kipp
    {
      extern char* nsPresShell_ReflowStackPointerTop;
      char marker;
      char* newsp = (char*) &marker;
      printf("XXX: frame tree is too deep; approx stack size = %d\n",
             nsPresShell_ReflowStackPointerTop - newsp);
    }
#endif
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nsnull;
  irs.mLineContainer = lineContainer;
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
  PRBool ltr = (NS_STYLE_DIRECTION_LTR == aReflowState.mStyleVisibility->mDirection);
  nscoord leftEdge = 0;
  if (nsnull == GetPrevContinuation()) {
    leftEdge = ltr ? aReflowState.mComputedBorderPadding.left
                   : aReflowState.mComputedBorderPadding.right;
  }
  nscoord availableWidth = aReflowState.availableWidth;
  if (NS_UNCONSTRAINEDSIZE != availableWidth) {
    
    availableWidth -= leftEdge;
    availableWidth -= ltr ? aReflowState.mComputedBorderPadding.right
                          : aReflowState.mComputedBorderPadding.left;
    availableWidth = PR_MAX(0, availableWidth);
  }
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
      
      
      
      
      nsIFrame* nextInFlow = frame->GetNextInFlow();
      while (nextInFlow) {
        
        
        
        
        NS_ASSERTION(mFrames.ContainsFrame(nextInFlow), "unexpected flow");
        if (havePrevBlock) {
          ReparentFloatsForInlineChild(irs.mLineContainer, nextInFlow, PR_FALSE);
        }
        nextInFlow->SetParent(this);
        nextInFlow = nextInFlow->GetNextInFlow();
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
    
    nsIFrame* overflowFrames = GetOverflowFrames(aPresContext, PR_FALSE);
    NS_ASSERTION(!overflowFrames, "whoops");
  }
#endif

  
  
  
  
  
  
  
  nsSize size;
  lineLayout->EndSpan(this, size);

  
  aMetrics.width = size.width;
  if (nsnull == GetPrevContinuation()) {
    aMetrics.width += ltr ? aReflowState.mComputedBorderPadding.left
                          : aReflowState.mComputedBorderPadding.right;
  }
  if (NS_FRAME_IS_COMPLETE(aStatus) && (!GetNextContinuation() || GetNextInFlow())) {
    aMetrics.width += ltr ? aReflowState.mComputedBorderPadding.right
                          : aReflowState.mComputedBorderPadding.left;
  }

  nsLayoutUtils::SetFontFromStyle(aReflowState.rendContext, mStyleContext);
  nsCOMPtr<nsIFontMetrics> fm;
  aReflowState.rendContext->GetFontMetrics(*getter_AddRefs(fm));

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

  
  
  aMetrics.mOverflowArea.SetRect(0, 0, 0, 0);

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
  if (NS_INLINE_IS_BREAK(aStatus)) {
    if (NS_INLINE_IS_BREAK_BEFORE(aStatus)) {
      if (aFrame != mFrames.FirstChild()) {
        
        
        
        aStatus = NS_FRAME_NOT_COMPLETE |
          NS_INLINE_BREAK | NS_INLINE_BREAK_AFTER |
          (aStatus & NS_INLINE_BREAK_TYPE_MASK);
        PushFrames(aPresContext, aFrame, irs.mPrevFrame);
      }
      else {
        
        
        
        
        
        
        if (irs.mSetParentPointer) {
          if (irs.mLineContainer && irs.mLineContainer->GetPrevContinuation()) {
            ReparentFloatsForInlineChild(irs.mLineContainer, aFrame->GetNextSibling(),
                                         PR_TRUE);
          }
          for (nsIFrame* f = aFrame->GetNextSibling(); f; f = f->GetNextSibling()) {
            f->SetParent(this);
          }
        }
      }
    }
    else {
      
      if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
        nsIFrame* newFrame;
        rv = CreateNextInFlow(aPresContext, this, aFrame, newFrame);
        if (NS_FAILED(rv)) {
          return rv;
        }
      }
      nsIFrame* nextFrame = aFrame->GetNextSibling();
      if (nextFrame) {
        aStatus |= NS_FRAME_NOT_COMPLETE;
        PushFrames(aPresContext, nextFrame, aFrame);
      }
      else if (nsnull != GetNextInFlow()) {
        
        
        nsInlineFrame* nextInFlow = (nsInlineFrame*) GetNextInFlow();
        while (nsnull != nextInFlow) {
          if (nextInFlow->mFrames.NotEmpty()) {
            aStatus |= NS_FRAME_NOT_COMPLETE;
            break;
          }
          nextInFlow = (nsInlineFrame*) nextInFlow->GetNextInFlow();
        }
      }
    }
  }
  else if (NS_FRAME_IS_NOT_COMPLETE(aStatus)) {
    if (nsGkAtoms::placeholderFrame == aFrame->GetType()) {
      nsBlockReflowState* blockRS = lineLayout->mBlockRS;
      blockRS->mBlock->SplitPlaceholder(*blockRS, aFrame);
      
      aStatus = NS_FRAME_COMPLETE;
    }
    else {
      nsIFrame* newFrame;
      rv = CreateNextInFlow(aPresContext, this, aFrame, newFrame);
      if (NS_FAILED(rv)) {
        return rv;
      }
      if (!reflowingFirstLetter) {
        nsIFrame* nextFrame = aFrame->GetNextSibling();
        if (nextFrame) {
          PushFrames(aPresContext, nextFrame, aFrame);
        }
      }
    }
  }
  return rv;
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
    if (nsnull != frame) {
      
      
      
      if (irs.mLineContainer && irs.mLineContainer->GetNextContinuation()) {
        
        
        
        ReparentFloatsForInlineChild(irs.mLineContainer, frame, PR_FALSE);
      }
      nextInFlow->mFrames.RemoveFirstChild();
      mFrames.InsertFrame(this, irs.mPrevFrame, frame);
      isComplete = PR_FALSE;
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
                          nsIFrame* aPrevSibling)
{
  NS_PRECONDITION(nsnull != aFromChild, "null pointer");
  NS_PRECONDITION(nsnull != aPrevSibling, "pushing first child");
  NS_PRECONDITION(aPrevSibling->GetNextSibling() == aFromChild, "bad prev sibling");

#ifdef NOISY_PUSHING
      printf("%p pushing aFromChild %p, disconnecting from prev sib %p\n", 
             this, aFromChild, aPrevSibling);
#endif
  
  aPrevSibling->SetNextSibling(nsnull);

  
  
  SetOverflowFrames(aPresContext, aFromChild);
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
  return skip;
}

#ifdef ACCESSIBILITY
NS_IMETHODIMP nsInlineFrame::GetAccessible(nsIAccessible** aAccessible)
{
  
  
  *aAccessible = nsnull;
  nsIAtom *tagAtom = mContent->Tag();
  if ((tagAtom == nsGkAtoms::img || tagAtom == nsGkAtoms::input || 
       tagAtom == nsGkAtoms::label) && mContent->IsNodeOfType(nsINode::eHTML)) {
    
    nsCOMPtr<nsIAccessibilityService> accService(do_GetService("@mozilla.org/accessibilityService;1"));
    if (!accService)
      return NS_ERROR_FAILURE;
    if (tagAtom == nsGkAtoms::input)  
      return accService->CreateHTMLButtonAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
    else if (tagAtom == nsGkAtoms::img)  
      return accService->CreateHTMLImageAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
    else if (tagAtom == nsGkAtoms::label)  
      return accService->CreateHTMLLabelAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
  }

  return NS_ERROR_FAILURE;
}
#endif





static void
ReParentChildListStyle(nsPresContext* aPresContext,
                       nsFrameList& aFrameList,
                       nsIFrame* aParentFrame)
{
  nsFrameManager *frameManager = aPresContext->FrameManager();

  for (nsIFrame* kid = aFrameList.FirstChild(); kid;
       kid = kid->GetNextSibling()) {
    NS_ASSERTION(kid->GetParent() == aParentFrame, "Bogus parentage");
    frameManager->ReParentStyleContext(kid);
  }
}

nsIFrame*
NS_NewFirstLineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsFirstLineFrame(aContext);
}

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

void
nsFirstLineFrame::StealFramesFrom(nsIFrame* aFrame)
{
  nsIFrame* prevFrame = mFrames.GetPrevSiblingFor(aFrame);
  if (prevFrame) {
    prevFrame->SetNextSibling(nsnull);
  }
  else {
    mFrames.SetFrames(nsnull);
  }
}

nsIFrame*
nsFirstLineFrame::PullOneFrame(nsPresContext* aPresContext, InlineReflowState& irs,
                               PRBool* aIsComplete)
{
  nsIFrame* frame = nsInlineFrame::PullOneFrame(aPresContext, irs, aIsComplete);
  if (frame && !GetPrevInFlow()) {
    
    
    NS_ASSERTION(frame->GetParent() == this, "Incorrect parent?");
    aPresContext->FrameManager()->ReParentStyleContext(frame);
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
    nsIFrame* prevOverflowFrames = prevInFlow->GetOverflowFrames(aPresContext, PR_TRUE);
    if (prevOverflowFrames) {
      nsFrameList frames(prevOverflowFrames);
      
      
      if (lineContainer && lineContainer->GetPrevContinuation()) {
        ReparentFloatsForInlineChild(lineContainer, prevOverflowFrames, PR_TRUE);
      }
      mFrames.InsertFrames(this, nsnull, prevOverflowFrames);
      ReParentChildListStyle(aPresContext, frames, this);
    }
  }

  
  nsIFrame* overflowFrames = GetOverflowFrames(aPresContext, PR_TRUE);
  if (overflowFrames) {
    NS_ASSERTION(mFrames.NotEmpty(), "overflow list w/o frames");
    nsFrameList frames(overflowFrames);

    mFrames.AppendFrames(nsnull, overflowFrames);
    ReParentChildListStyle(aPresContext, frames, this);
  }

  
  
  InlineReflowState irs;
  irs.mPrevFrame = nsnull;
  irs.mLineContainer = lineContainer;
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
          ResolvePseudoStyleFor(nsnull,
                                nsCSSAnonBoxes::mozLineFrame, parentContext);
        if (newSC) {
          
          SetStyleContext(newSC);

          
          ReParentChildListStyle(aPresContext, mFrames, this);
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



nsIFrame*
NS_NewPositionedInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsPositionedInlineFrame(aContext);
}

void
nsPositionedInlineFrame::Destroy()
{
  mAbsoluteContainer.DestroyFrames(this);
  nsInlineFrame::Destroy();
}

NS_IMETHODIMP
nsPositionedInlineFrame::SetInitialChildList(nsIAtom*        aListName,
                                             nsIFrame*       aChildList)
{
  nsresult  rv;

  if (mAbsoluteContainer.GetChildListName() == aListName) {
    rv = mAbsoluteContainer.SetInitialChildList(this, aListName, aChildList);
  } else {
    rv = nsInlineFrame::SetInitialChildList(aListName, aChildList);
  }

  return rv;
}

NS_IMETHODIMP
nsPositionedInlineFrame::AppendFrames(nsIAtom*        aListName,
                                      nsIFrame*       aFrameList)
{
  nsresult  rv;
  
  if (mAbsoluteContainer.GetChildListName() == aListName) {
    rv = mAbsoluteContainer.AppendFrames(this, aListName, aFrameList);
  } else {
    rv = nsInlineFrame::AppendFrames(aListName, aFrameList);
  }

  return rv;
}
  
NS_IMETHODIMP
nsPositionedInlineFrame::InsertFrames(nsIAtom*        aListName,
                                      nsIFrame*       aPrevFrame,
                                      nsIFrame*       aFrameList)
{
  nsresult  rv;

  if (mAbsoluteContainer.GetChildListName() == aListName) {
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

  if (mAbsoluteContainer.GetChildListName() == aListName) {
    rv = mAbsoluteContainer.RemoveFrame(this, aListName, aOldFrame);
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
  aBuilder->MarkFramesForDisplayList(this, mAbsoluteContainer.GetFirstChild(), aDirtyRect);
  return nsHTMLContainerFrame::BuildDisplayList(aBuilder, aDirtyRect, aLists);
}

nsIAtom*
nsPositionedInlineFrame::GetAdditionalChildListName(PRInt32 aIndex) const
{
  if (0 == aIndex) {
    return mAbsoluteContainer.GetChildListName();
  }
  return nsnull;
}

nsIFrame*
nsPositionedInlineFrame::GetFirstChild(nsIAtom* aListName) const
{
  if (mAbsoluteContainer.GetChildListName() == aListName) {
    nsIFrame* result = nsnull;
    mAbsoluteContainer.FirstChild(this, aListName, &result);
    return result;
  }

  return nsInlineFrame::GetFirstChild(aListName);
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

    
    
    
    
    rv = mAbsoluteContainer.Reflow(this, aPresContext, aReflowState,
                                   containingBlockWidth, containingBlockHeight,
                                   PR_TRUE, PR_TRUE, 
                                   &aDesiredSize.mOverflowArea);
  }

  return rv;
}
