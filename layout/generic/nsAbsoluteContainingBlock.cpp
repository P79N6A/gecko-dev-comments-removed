









































#include "nsCOMPtr.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsContainerFrame.h"
#include "nsIPresShell.h"
#include "nsHTMLContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsPresContext.h"
#include "nsFrameManager.h"
#include "nsCSSFrameConstructor.h"

#ifdef DEBUG
#include "nsBlockFrame.h"
#endif

nsresult
nsAbsoluteContainingBlock::SetInitialChildList(nsIFrame*       aDelegatingFrame,
                                               nsIAtom*        aListName,
                                               nsFrameList&    aChildList)
{
  NS_PRECONDITION(GetChildListName() == aListName, "unexpected child list name");
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  mAbsoluteFrames.SetFrames(aChildList);
  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::AppendFrames(nsIFrame*      aDelegatingFrame,
                                        nsIAtom*       aListName,
                                        nsFrameList&   aFrameList)
{
  NS_ASSERTION(GetChildListName() == aListName, "unexpected child list");

  
#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.AppendFrames(nsnull, aFrameList);

  
  
  aDelegatingFrame->PresContext()->PresShell()->
    FrameNeedsReflow(aDelegatingFrame, nsIPresShell::eResize,
                     NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::InsertFrames(nsIFrame*      aDelegatingFrame,
                                        nsIAtom*       aListName,
                                        nsIFrame*      aPrevFrame,
                                        nsFrameList&   aFrameList)
{
  NS_ASSERTION(GetChildListName() == aListName, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == aDelegatingFrame,
               "inserting after sibling frame with different parent");

#ifdef NS_DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.InsertFrames(nsnull, aPrevFrame, aFrameList);

  
  
  aDelegatingFrame->PresContext()->PresShell()->
    FrameNeedsReflow(aDelegatingFrame, nsIPresShell::eResize,
                     NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}

void
nsAbsoluteContainingBlock::RemoveFrame(nsIFrame*       aDelegatingFrame,
                                       nsIAtom*        aListName,
                                       nsIFrame*       aOldFrame)
{
  NS_ASSERTION(GetChildListName() == aListName, "unexpected child list");
  nsIFrame* nif = aOldFrame->GetNextInFlow();
  if (nif) {
    static_cast<nsContainerFrame*>(nif->GetParent())
      ->DeleteNextInFlowChild(aOldFrame->PresContext(), nif, PR_FALSE);
  }

  mAbsoluteFrames.DestroyFrame(aOldFrame);
}

nsresult
nsAbsoluteContainingBlock::Reflow(nsContainerFrame*        aDelegatingFrame,
                                  nsPresContext*           aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nsReflowStatus&          aReflowStatus,
                                  nscoord                  aContainingBlockWidth,
                                  nscoord                  aContainingBlockHeight,
                                  PRBool                   aConstrainHeight,
                                  PRBool                   aCBWidthChanged,
                                  PRBool                   aCBHeightChanged,
                                  nsOverflowAreas*         aOverflowAreas)
{
  nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;

  PRBool reflowAll = aReflowState.ShouldReflowAllKids();

  nsIFrame* kidFrame;
  nsOverflowContinuationTracker tracker(aPresContext, aDelegatingFrame, PR_TRUE);
  for (kidFrame = mAbsoluteFrames.FirstChild(); kidFrame; kidFrame = kidFrame->GetNextSibling()) {
    PRBool kidNeedsReflow = reflowAll || NS_SUBTREE_DIRTY(kidFrame) ||
      FrameDependsOnContainer(kidFrame, aCBWidthChanged, aCBHeightChanged);
    if (kidNeedsReflow && !aPresContext->HasPendingInterrupt()) {
      
      nsReflowStatus  kidStatus = NS_FRAME_COMPLETE;
      ReflowAbsoluteFrame(aDelegatingFrame, aPresContext, aReflowState,
                          aContainingBlockWidth, aContainingBlockHeight,
                          aConstrainHeight, kidFrame, kidStatus,
                          aOverflowAreas);
      nsIFrame* nextFrame = kidFrame->GetNextInFlow();
      if (!NS_FRAME_IS_FULLY_COMPLETE(kidStatus)) {
        
        if (!nextFrame) {
          nsresult rv = aPresContext->PresShell()->FrameConstructor()->
            CreateContinuingFrame(aPresContext, kidFrame, aDelegatingFrame, &nextFrame);
          NS_ENSURE_SUCCESS(rv, rv);
        }
        
        
        
        
        tracker.Insert(nextFrame, kidStatus);
        NS_MergeReflowStatusInto(&reflowStatus, kidStatus);
      }
      else {
        
        if (nextFrame) {
          tracker.Finish(kidFrame);
          static_cast<nsContainerFrame*>(nextFrame->GetParent())
            ->DeleteNextInFlowChild(aPresContext, nextFrame, PR_TRUE);
        }
      }
    }
    else {
      tracker.Skip(kidFrame, reflowStatus);
      if (aOverflowAreas) {
        aDelegatingFrame->ConsiderChildOverflow(*aOverflowAreas, kidFrame);
      }
    }

    
    
    
    
    
    
    
    
    
    
    
    
    if (kidNeedsReflow && aPresContext->CheckForInterrupt(aDelegatingFrame)) {
      if (aDelegatingFrame->GetStateBits() & NS_FRAME_IS_DIRTY) {
        kidFrame->AddStateBits(NS_FRAME_IS_DIRTY);
      } else {
        kidFrame->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
      }
    }
  }

  
  
  if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus))
    NS_FRAME_SET_OVERFLOW_INCOMPLETE(reflowStatus);

  NS_MergeReflowStatusInto(&aReflowStatus, reflowStatus);
  return NS_OK;
}

static inline bool IsFixedPaddingSize(const nsStyleCoord& aCoord)
  { return aCoord.ConvertsToLength(); }
static inline bool IsFixedMarginSize(const nsStyleCoord& aCoord)
  { return aCoord.ConvertsToLength(); }
static inline bool IsFixedOffset(const nsStyleCoord& aCoord)
  { return aCoord.ConvertsToLength(); }

PRBool
nsAbsoluteContainingBlock::FrameDependsOnContainer(nsIFrame* f,
                                                   PRBool aCBWidthChanged,
                                                   PRBool aCBHeightChanged)
{
  const nsStylePosition* pos = f->GetStylePosition();
  
  
  
  
  
  
  
  
  
  
  
  if ((pos->mOffset.GetTopUnit() == eStyleUnit_Auto &&
       pos->mOffset.GetBottomUnit() == eStyleUnit_Auto) ||
      (pos->mOffset.GetLeftUnit() == eStyleUnit_Auto &&
       pos->mOffset.GetRightUnit() == eStyleUnit_Auto)) {
    return PR_TRUE;
  }
  if (!aCBWidthChanged && !aCBHeightChanged) {
    
    return PR_FALSE;
  }
  const nsStylePadding* padding = f->GetStylePadding();
  const nsStyleMargin* margin = f->GetStyleMargin();
  if (aCBWidthChanged) {
    
    
    
    
    
    
    if (pos->WidthDependsOnContainer() ||
        pos->MinWidthDependsOnContainer() ||
        pos->MaxWidthDependsOnContainer() ||
        !IsFixedPaddingSize(padding->mPadding.GetLeft()) ||
        !IsFixedPaddingSize(padding->mPadding.GetRight())) {
      return PR_TRUE;
    }

    
    
    
    if (!IsFixedMarginSize(margin->mMargin.GetLeft()) ||
        !IsFixedMarginSize(margin->mMargin.GetRight())) {
      return PR_TRUE;
    }
    if (f->GetStyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
      
      
      
      
      
      
      if (!IsFixedOffset(pos->mOffset.GetLeft()) ||
          pos->mOffset.GetRightUnit() != eStyleUnit_Auto) {
        return PR_TRUE;
      }
    } else {
      if (!IsFixedOffset(pos->mOffset.GetLeft())) {
        return PR_TRUE;
      }
    }
  }
  if (aCBHeightChanged) {
    
    
    
    
    
    
    if ((pos->HeightDependsOnContainer() &&
         !(pos->mHeight.GetUnit() == eStyleUnit_Auto &&
           pos->mOffset.GetBottomUnit() == eStyleUnit_Auto &&
           pos->mOffset.GetTopUnit() != eStyleUnit_Auto)) ||
        pos->MinHeightDependsOnContainer() ||
        pos->MaxHeightDependsOnContainer() ||
        !IsFixedPaddingSize(padding->mPadding.GetTop()) ||
        !IsFixedPaddingSize(padding->mPadding.GetBottom())) { 
      return PR_TRUE;
    }
      
    
    if (!IsFixedMarginSize(margin->mMargin.GetTop()) ||
        !IsFixedMarginSize(margin->mMargin.GetBottom())) {
      return PR_TRUE;
    }
    if (!IsFixedOffset(pos->mOffset.GetTop())) {
      return PR_TRUE;
    }
  }
  return PR_FALSE;
}

void
nsAbsoluteContainingBlock::DestroyFrames(nsIFrame* aDelegatingFrame,
                                         nsIFrame* aDestructRoot)
{
  mAbsoluteFrames.DestroyFramesFrom(aDestructRoot);
}

void
nsAbsoluteContainingBlock::MarkSizeDependentFramesDirty()
{
  DoMarkFramesDirty(PR_FALSE);
}

void
nsAbsoluteContainingBlock::MarkAllFramesDirty()
{
  DoMarkFramesDirty(PR_TRUE);
}

void
nsAbsoluteContainingBlock::DoMarkFramesDirty(PRBool aMarkAllDirty)
{
  for (nsIFrame* kidFrame = mAbsoluteFrames.FirstChild();
       kidFrame;
       kidFrame = kidFrame->GetNextSibling()) {
    if (aMarkAllDirty) {
      kidFrame->AddStateBits(NS_FRAME_IS_DIRTY);
    } else if (FrameDependsOnContainer(kidFrame, PR_TRUE, PR_TRUE)) {
      
      kidFrame->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
}









nsresult
nsAbsoluteContainingBlock::ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                                               nsPresContext*          aPresContext,
                                               const nsHTMLReflowState& aReflowState,
                                               nscoord                  aContainingBlockWidth,
                                               nscoord                  aContainingBlockHeight,
                                               PRBool                   aConstrainHeight,
                                               nsIFrame*                aKidFrame,
                                               nsReflowStatus&          aStatus,
                                               nsOverflowAreas*         aOverflowAreas)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout,nsBlockFrame::gNoiseIndent);
    printf("abs pos ");
    if (aKidFrame) {
      nsAutoString name;
      aKidFrame->GetFrameName(name);
      printf("%s ", NS_LossyConvertUTF16toASCII(name).get());
    }

    char width[16];
    char height[16];
    PrettyUC(aReflowState.availableWidth, width);
    PrettyUC(aReflowState.availableHeight, height);
    printf(" a=%s,%s ", width, height);
    PrettyUC(aReflowState.ComputedWidth(), width);
    PrettyUC(aReflowState.ComputedHeight(), height);
    printf("c=%s,%s \n", width, height);
  }
  AutoNoisyIndenter indent(nsBlockFrame::gNoisy);
#endif 

  
  
  nsRect oldOverflowRect(aKidFrame->GetVisualOverflowRect() +
                         aKidFrame->GetPosition());
  nsRect oldRect = aKidFrame->GetRect();

  nsresult  rv;
  
  const nsMargin& border = aReflowState.mStyleBorder->GetActualBorder();

  nscoord availWidth = aContainingBlockWidth;
  if (availWidth == -1) {
    NS_ASSERTION(aReflowState.ComputedWidth() != NS_UNCONSTRAINEDSIZE,
                 "Must have a useful width _somewhere_");
    availWidth =
      aReflowState.ComputedWidth() + aReflowState.mComputedPadding.LeftRight();
  }
    
  nsHTMLReflowMetrics kidDesiredSize;
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, aKidFrame,
                                   nsSize(availWidth, NS_UNCONSTRAINEDSIZE),
                                   aContainingBlockWidth,
                                   aContainingBlockHeight);

  
  aKidFrame->WillReflow(aPresContext);

  PRBool constrainHeight = (aReflowState.availableHeight != NS_UNCONSTRAINEDSIZE)
    && aConstrainHeight
       
    && (aDelegatingFrame->GetType() != nsGkAtoms::inlineFrame)
       
    && (aKidFrame->GetRect().y <= aReflowState.availableHeight);
       
       
       
  if (constrainHeight) {
    kidReflowState.availableHeight = aReflowState.availableHeight - border.top
                                     - kidReflowState.mComputedMargin.top;
    if (NS_AUTOOFFSET != kidReflowState.mComputedOffsets.top)
      kidReflowState.availableHeight -= kidReflowState.mComputedOffsets.top;
  }

  
  rv = aKidFrame->Reflow(aPresContext, kidDesiredSize, kidReflowState, aStatus);

  
  
  if ((NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) ||
      (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.top)) {
    if (-1 == aContainingBlockWidth) {
      
      kidReflowState.ComputeContainingBlockRectangle(aPresContext,
                                                     &aReflowState,
                                                     aContainingBlockWidth,
                                                     aContainingBlockHeight);
    }

    if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.left) {
      NS_ASSERTION(NS_AUTOOFFSET != kidReflowState.mComputedOffsets.right,
                   "Can't solve for both left and right");
      kidReflowState.mComputedOffsets.left = aContainingBlockWidth -
                                             kidReflowState.mComputedOffsets.right -
                                             kidReflowState.mComputedMargin.right -
                                             kidDesiredSize.width -
                                             kidReflowState.mComputedMargin.left;
    }
    if (NS_AUTOOFFSET == kidReflowState.mComputedOffsets.top) {
      kidReflowState.mComputedOffsets.top = aContainingBlockHeight -
                                            kidReflowState.mComputedOffsets.bottom -
                                            kidReflowState.mComputedMargin.bottom -
                                            kidDesiredSize.height -
                                            kidReflowState.mComputedMargin.top;
    }
  }

  
  nsRect  rect(border.left + kidReflowState.mComputedOffsets.left + kidReflowState.mComputedMargin.left,
               border.top + kidReflowState.mComputedOffsets.top + kidReflowState.mComputedMargin.top,
               kidDesiredSize.width, kidDesiredSize.height);
  aKidFrame->SetRect(rect);

  nsIView* view = aKidFrame->GetView();
  if (view) {
    
    
    nsContainerFrame::SyncFrameViewAfterReflow(aPresContext, aKidFrame, view,
                                               kidDesiredSize.VisualOverflow());
  } else {
    nsContainerFrame::PositionChildViews(aKidFrame);
  }

  if (oldRect.TopLeft() != rect.TopLeft() || 
      (aDelegatingFrame->GetStateBits() & NS_FRAME_FIRST_REFLOW)) {
    
    aKidFrame->GetParent()->Invalidate(oldOverflowRect);
    aKidFrame->InvalidateFrameSubtree();
  } else if (oldRect.Size() != rect.Size()) {
    
    nscoord innerWidth = NS_MIN(oldRect.width, rect.width);
    nscoord innerHeight = NS_MIN(oldRect.height, rect.height);
    nscoord outerWidth = NS_MAX(oldRect.width, rect.width);
    nscoord outerHeight = NS_MAX(oldRect.height, rect.height);
    aKidFrame->GetParent()->Invalidate(
        nsRect(rect.x + innerWidth, rect.y, outerWidth - innerWidth, outerHeight));
    
    aKidFrame->GetParent()->Invalidate(
        nsRect(rect.x, rect.y + innerHeight, outerWidth, outerHeight - innerHeight));
  }
  aKidFrame->DidReflow(aPresContext, &kidReflowState, NS_FRAME_REFLOW_FINISHED);

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout,nsBlockFrame::gNoiseIndent - 1);
    printf("abs pos ");
    if (aKidFrame) {
      nsAutoString name;
      aKidFrame->GetFrameName(name);
      printf("%s ", NS_LossyConvertUTF16toASCII(name).get());
    }
    printf("%p rect=%d,%d,%d,%d\n", static_cast<void*>(aKidFrame),
           rect.x, rect.y, rect.width, rect.height);
  }
#endif

  if (aOverflowAreas) {
    aOverflowAreas->UnionWith(kidDesiredSize.mOverflowAreas + rect.TopLeft());
  }

  return rv;
}

#ifdef DEBUG
 void nsAbsoluteContainingBlock::PrettyUC(nscoord aSize,
                        char*   aBuf)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  }
  else {
    if((PRInt32)0xdeadbeef == aSize)
    {
      strcpy(aBuf, "deadbeef");
    }
    else {
      sprintf(aBuf, "%d", aSize);
    }
  }
}
#endif
