









#include "nsAbsoluteContainingBlock.h"

#include "nsContainerFrame.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsHTMLReflowState.h"
#include "nsPresContext.h"
#include "nsCSSFrameConstructor.h"

#ifdef DEBUG
#include "nsBlockFrame.h"

static void PrettyUC(nscoord aSize, char* aBuf)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  } else {
    if((int32_t)0xdeadbeef == aSize) {
      strcpy(aBuf, "deadbeef");
    } else {
      sprintf(aBuf, "%d", aSize);
    }
  }
}
#endif

nsresult
nsAbsoluteContainingBlock::SetInitialChildList(nsIFrame*       aDelegatingFrame,
                                               ChildListID     aListID,
                                               nsFrameList&    aChildList)
{
  NS_PRECONDITION(mChildListID == aListID, "unexpected child list name");
#ifdef DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  mAbsoluteFrames.SetFrames(aChildList);
  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::AppendFrames(nsIFrame*      aDelegatingFrame,
                                        ChildListID    aListID,
                                        nsFrameList&   aFrameList)
{
  NS_ASSERTION(mChildListID == aListID, "unexpected child list");

  
#ifdef DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.AppendFrames(nullptr, aFrameList);

  
  
  aDelegatingFrame->PresContext()->PresShell()->
    FrameNeedsReflow(aDelegatingFrame, nsIPresShell::eResize,
                     NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}

nsresult
nsAbsoluteContainingBlock::InsertFrames(nsIFrame*      aDelegatingFrame,
                                        ChildListID    aListID,
                                        nsIFrame*      aPrevFrame,
                                        nsFrameList&   aFrameList)
{
  NS_ASSERTION(mChildListID == aListID, "unexpected child list");
  NS_ASSERTION(!aPrevFrame || aPrevFrame->GetParent() == aDelegatingFrame,
               "inserting after sibling frame with different parent");

#ifdef DEBUG
  nsFrame::VerifyDirtyBitSet(aFrameList);
#endif
  mAbsoluteFrames.InsertFrames(nullptr, aPrevFrame, aFrameList);

  
  
  aDelegatingFrame->PresContext()->PresShell()->
    FrameNeedsReflow(aDelegatingFrame, nsIPresShell::eResize,
                     NS_FRAME_HAS_DIRTY_CHILDREN);

  return NS_OK;
}

void
nsAbsoluteContainingBlock::RemoveFrame(nsIFrame*       aDelegatingFrame,
                                       ChildListID     aListID,
                                       nsIFrame*       aOldFrame)
{
  NS_ASSERTION(mChildListID == aListID, "unexpected child list");
  nsIFrame* nif = aOldFrame->GetNextInFlow();
  if (nif) {
    nif->GetParent()->DeleteNextInFlowChild(nif, false);
  }

  mAbsoluteFrames.DestroyFrame(aOldFrame);
}

void
nsAbsoluteContainingBlock::Reflow(nsContainerFrame*        aDelegatingFrame,
                                  nsPresContext*           aPresContext,
                                  const nsHTMLReflowState& aReflowState,
                                  nsReflowStatus&          aReflowStatus,
                                  const nsRect&            aContainingBlock,
                                  bool                     aConstrainHeight,
                                  bool                     aCBWidthChanged,
                                  bool                     aCBHeightChanged,
                                  nsOverflowAreas*         aOverflowAreas)
{
  nsReflowStatus reflowStatus = NS_FRAME_COMPLETE;

  bool reflowAll = aReflowState.ShouldReflowAllKids();

  nsIFrame* kidFrame;
  nsOverflowContinuationTracker tracker(aDelegatingFrame, true);
  for (kidFrame = mAbsoluteFrames.FirstChild(); kidFrame; kidFrame = kidFrame->GetNextSibling()) {
    bool kidNeedsReflow = reflowAll || NS_SUBTREE_DIRTY(kidFrame) ||
      FrameDependsOnContainer(kidFrame, aCBWidthChanged, aCBHeightChanged);
    if (kidNeedsReflow && !aPresContext->HasPendingInterrupt()) {
      
      nsReflowStatus  kidStatus = NS_FRAME_COMPLETE;
      ReflowAbsoluteFrame(aDelegatingFrame, aPresContext, aReflowState,
                          aContainingBlock,
                          aConstrainHeight, kidFrame, kidStatus,
                          aOverflowAreas);
      nsIFrame* nextFrame = kidFrame->GetNextInFlow();
      if (!NS_FRAME_IS_FULLY_COMPLETE(kidStatus)) {
        
        if (!nextFrame) {
          nextFrame =
            aPresContext->PresShell()->FrameConstructor()->
              CreateContinuingFrame(aPresContext, kidFrame, aDelegatingFrame);
        }
        
        
        
        
        tracker.Insert(nextFrame, kidStatus);
        NS_MergeReflowStatusInto(&reflowStatus, kidStatus);
      }
      else {
        
        if (nextFrame) {
          nsOverflowContinuationTracker::AutoFinish fini(&tracker, kidFrame);
          nextFrame->GetParent()->DeleteNextInFlowChild(nextFrame, true);
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
}

static inline bool IsFixedPaddingSize(const nsStyleCoord& aCoord)
  { return aCoord.ConvertsToLength(); }
static inline bool IsFixedMarginSize(const nsStyleCoord& aCoord)
  { return aCoord.ConvertsToLength(); }
static inline bool IsFixedOffset(const nsStyleCoord& aCoord)
  { return aCoord.ConvertsToLength(); }

bool
nsAbsoluteContainingBlock::FrameDependsOnContainer(nsIFrame* f,
                                                   bool aCBWidthChanged,
                                                   bool aCBHeightChanged)
{
  const nsStylePosition* pos = f->StylePosition();
  
  
  
  
  
  
  
  
  
  
  
  if ((pos->mOffset.GetTopUnit() == eStyleUnit_Auto &&
       pos->mOffset.GetBottomUnit() == eStyleUnit_Auto) ||
      (pos->mOffset.GetLeftUnit() == eStyleUnit_Auto &&
       pos->mOffset.GetRightUnit() == eStyleUnit_Auto)) {
    return true;
  }
  if (!aCBWidthChanged && !aCBHeightChanged) {
    
    return false;
  }
  const nsStylePadding* padding = f->StylePadding();
  const nsStyleMargin* margin = f->StyleMargin();
  if (aCBWidthChanged) {
    
    
    
    
    
    
    if (pos->WidthDependsOnContainer() ||
        pos->MinWidthDependsOnContainer() ||
        pos->MaxWidthDependsOnContainer() ||
        !IsFixedPaddingSize(padding->mPadding.GetLeft()) ||
        !IsFixedPaddingSize(padding->mPadding.GetRight())) {
      return true;
    }

    
    
    
    if (!IsFixedMarginSize(margin->mMargin.GetLeft()) ||
        !IsFixedMarginSize(margin->mMargin.GetRight())) {
      return true;
    }
    if (f->StyleVisibility()->mDirection == NS_STYLE_DIRECTION_RTL) {
      
      
      
      
      
      
      if (!IsFixedOffset(pos->mOffset.GetLeft()) ||
          pos->mOffset.GetRightUnit() != eStyleUnit_Auto) {
        return true;
      }
    } else {
      if (!IsFixedOffset(pos->mOffset.GetLeft())) {
        return true;
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
      return true;
    }
      
    
    if (!IsFixedMarginSize(margin->mMargin.GetTop()) ||
        !IsFixedMarginSize(margin->mMargin.GetBottom())) {
      return true;
    }
    if (!IsFixedOffset(pos->mOffset.GetTop())) {
      return true;
    }
  }
  return false;
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
  DoMarkFramesDirty(false);
}

void
nsAbsoluteContainingBlock::MarkAllFramesDirty()
{
  DoMarkFramesDirty(true);
}

void
nsAbsoluteContainingBlock::DoMarkFramesDirty(bool aMarkAllDirty)
{
  for (nsIFrame* kidFrame = mAbsoluteFrames.FirstChild();
       kidFrame;
       kidFrame = kidFrame->GetNextSibling()) {
    if (aMarkAllDirty) {
      kidFrame->AddStateBits(NS_FRAME_IS_DIRTY);
    } else if (FrameDependsOnContainer(kidFrame, true, true)) {
      
      kidFrame->AddStateBits(NS_FRAME_HAS_DIRTY_CHILDREN);
    }
  }
}









void
nsAbsoluteContainingBlock::ReflowAbsoluteFrame(nsIFrame*                aDelegatingFrame,
                                               nsPresContext*           aPresContext,
                                               const nsHTMLReflowState& aReflowState,
                                               const nsRect&            aContainingBlock,
                                               bool                     aConstrainHeight,
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
    PrettyUC(aReflowState.AvailableWidth(), width);
    PrettyUC(aReflowState.AvailableHeight(), height);
    printf(" a=%s,%s ", width, height);
    PrettyUC(aReflowState.ComputedWidth(), width);
    PrettyUC(aReflowState.ComputedHeight(), height);
    printf("c=%s,%s \n", width, height);
  }
  AutoNoisyIndenter indent(nsBlockFrame::gNoisy);
#endif 

  nscoord availWidth = aContainingBlock.width;
  if (availWidth == -1) {
    NS_ASSERTION(aReflowState.ComputedWidth() != NS_UNCONSTRAINEDSIZE,
                 "Must have a useful width _somewhere_");
    availWidth =
      aReflowState.ComputedWidth() + aReflowState.ComputedPhysicalPadding().LeftRight();
  }

  nsHTMLReflowMetrics kidDesiredSize(aReflowState);
  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, aKidFrame,
                                   nsSize(availWidth, NS_UNCONSTRAINEDSIZE),
                                   aContainingBlock.width,
                                   aContainingBlock.height);

  
  aKidFrame->WillReflow(aPresContext);

  
  const nsMargin& border = aReflowState.mStyleBorder->GetComputedBorder();

  bool constrainHeight = (aReflowState.AvailableHeight() != NS_UNCONSTRAINEDSIZE)
    && aConstrainHeight
       
    && (aDelegatingFrame->GetType() != nsGkAtoms::inlineFrame)
       
    && (aKidFrame->GetRect().y <= aReflowState.AvailableHeight());
       
       
       
  if (constrainHeight) {
    kidReflowState.AvailableHeight() = aReflowState.AvailableHeight() - border.top
                                     - kidReflowState.ComputedPhysicalMargin().top;
    if (NS_AUTOOFFSET != kidReflowState.ComputedPhysicalOffsets().top)
      kidReflowState.AvailableHeight() -= kidReflowState.ComputedPhysicalOffsets().top;
  }

  
  aKidFrame->Reflow(aPresContext, kidDesiredSize, kidReflowState, aStatus);

  
  
  if ((NS_AUTOOFFSET == kidReflowState.ComputedPhysicalOffsets().left) ||
      (NS_AUTOOFFSET == kidReflowState.ComputedPhysicalOffsets().top)) {
    nscoord aContainingBlockWidth = aContainingBlock.width;
    nscoord aContainingBlockHeight = aContainingBlock.height;

    if (-1 == aContainingBlockWidth) {
      
      kidReflowState.ComputeContainingBlockRectangle(aPresContext,
                                                     &aReflowState,
                                                     aContainingBlockWidth,
                                                     aContainingBlockHeight);
    }

    if (NS_AUTOOFFSET == kidReflowState.ComputedPhysicalOffsets().left) {
      NS_ASSERTION(NS_AUTOOFFSET != kidReflowState.ComputedPhysicalOffsets().right,
                   "Can't solve for both left and right");
      kidReflowState.ComputedPhysicalOffsets().left = aContainingBlockWidth -
                                             kidReflowState.ComputedPhysicalOffsets().right -
                                             kidReflowState.ComputedPhysicalMargin().right -
                                             kidDesiredSize.Width() -
                                             kidReflowState.ComputedPhysicalMargin().left;
    }
    if (NS_AUTOOFFSET == kidReflowState.ComputedPhysicalOffsets().top) {
      kidReflowState.ComputedPhysicalOffsets().top = aContainingBlockHeight -
                                            kidReflowState.ComputedPhysicalOffsets().bottom -
                                            kidReflowState.ComputedPhysicalMargin().bottom -
                                            kidDesiredSize.Height() -
                                            kidReflowState.ComputedPhysicalMargin().top;
    }
  }

  
  nsRect  rect(border.left + kidReflowState.ComputedPhysicalOffsets().left + kidReflowState.ComputedPhysicalMargin().left,
               border.top + kidReflowState.ComputedPhysicalOffsets().top + kidReflowState.ComputedPhysicalMargin().top,
               kidDesiredSize.Width(), kidDesiredSize.Height());

  
  
  
  if (aContainingBlock.TopLeft() != nsPoint(0, 0)) {
    if (!(kidReflowState.mStylePosition->mOffset.GetLeftUnit() == eStyleUnit_Auto &&
          kidReflowState.mStylePosition->mOffset.GetRightUnit() == eStyleUnit_Auto)) {
      rect.x += aContainingBlock.x;
    }
    if (!(kidReflowState.mStylePosition->mOffset.GetTopUnit() == eStyleUnit_Auto &&
          kidReflowState.mStylePosition->mOffset.GetBottomUnit() == eStyleUnit_Auto)) {
      rect.y += aContainingBlock.y;
    }
  }

  aKidFrame->SetRect(rect);

  nsView* view = aKidFrame->GetView();
  if (view) {
    
    
    nsContainerFrame::SyncFrameViewAfterReflow(aPresContext, aKidFrame, view,
                                               kidDesiredSize.VisualOverflow());
  } else {
    nsContainerFrame::PositionChildViews(aKidFrame);
  }

  aKidFrame->DidReflow(aPresContext, &kidReflowState, nsDidReflowStatus::FINISHED);

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
}
