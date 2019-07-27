









#include "nsAbsoluteContainingBlock.h"

#include "nsContainerFrame.h"
#include "nsGkAtoms.h"
#include "nsIPresShell.h"
#include "nsHTMLReflowState.h"
#include "nsPresContext.h"
#include "nsCSSFrameConstructor.h"
#include "nsGridContainerFrame.h"

#include "mozilla/Snprintf.h"

#ifdef DEBUG
#include "nsBlockFrame.h"

static void PrettyUC(nscoord aSize, char* aBuf, int aBufSize)
{
  if (NS_UNCONSTRAINEDSIZE == aSize) {
    strcpy(aBuf, "UC");
  } else {
    if((int32_t)0xdeadbeef == aSize) {
      strcpy(aBuf, "deadbeef");
    } else {
      snprintf(aBuf, aBufSize, "%d", aSize);
    }
  }
}
#endif

using namespace mozilla;

void
nsAbsoluteContainingBlock::SetInitialChildList(nsIFrame*       aDelegatingFrame,
                                               ChildListID     aListID,
                                               nsFrameList&    aChildList)
{
  NS_PRECONDITION(mChildListID == aListID, "unexpected child list name");
#ifdef DEBUG
  nsFrame::VerifyDirtyBitSet(aChildList);
#endif
  mAbsoluteFrames.SetFrames(aChildList);
}

void
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
}

void
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

  
  
  
  
  const bool isGrid =
    aContainingBlock.width == nsGridContainerFrame::VERY_LIKELY_A_GRID_CONTAINER &&
    aDelegatingFrame->GetType() == nsGkAtoms::gridContainerFrame;

  nsIFrame* kidFrame;
  nsOverflowContinuationTracker tracker(aDelegatingFrame, true);
  for (kidFrame = mAbsoluteFrames.FirstChild(); kidFrame; kidFrame = kidFrame->GetNextSibling()) {
    bool kidNeedsReflow = reflowAll || NS_SUBTREE_DIRTY(kidFrame) ||
      FrameDependsOnContainer(kidFrame, aCBWidthChanged, aCBHeightChanged);
    if (kidNeedsReflow && !aPresContext->HasPendingInterrupt()) {
      
      nsReflowStatus  kidStatus = NS_FRAME_COMPLETE;
      const nsRect& cb = isGrid ? nsGridContainerFrame::GridItemCB(kidFrame)
                                : aContainingBlock;
      ReflowAbsoluteFrame(aDelegatingFrame, aPresContext, aReflowState, cb,
                          aConstrainHeight, kidFrame, kidStatus,
                          aOverflowAreas);
      nsIFrame* nextFrame = kidFrame->GetNextInFlow();
      if (!NS_FRAME_IS_FULLY_COMPLETE(kidStatus) &&
          aDelegatingFrame->IsFrameOfType(nsIFrame::eCanContainOverflowContainers)) {
        
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
  WritingMode wm = f->GetWritingMode();
  if (wm.IsVertical() ? aCBHeightChanged : aCBWidthChanged) {
    
    
    
    
    
    
    if (pos->ISizeDependsOnContainer(wm) ||
        pos->MinISizeDependsOnContainer(wm) ||
        pos->MaxISizeDependsOnContainer(wm) ||
        !IsFixedPaddingSize(padding->mPadding.GetIStart(wm)) ||
        !IsFixedPaddingSize(padding->mPadding.GetIEnd(wm))) {
      return true;
    }

    
    
    
    if (!IsFixedMarginSize(margin->mMargin.GetIStart(wm)) ||
        !IsFixedMarginSize(margin->mMargin.GetIEnd(wm))) {
      return true;
    }
    if (!wm.IsBidiLTR()) {
      
      
      
      
      
      
      if (!IsFixedOffset(pos->mOffset.GetIStart(wm)) ||
          pos->mOffset.GetIEndUnit(wm) != eStyleUnit_Auto) {
        return true;
      }
    } else {
      if (!IsFixedOffset(pos->mOffset.GetIStart(wm))) {
        return true;
      }
    }
  }
  if (wm.IsVertical() ? aCBWidthChanged : aCBHeightChanged) {
    
    
    
    
    
    
    if ((pos->BSizeDependsOnContainer(wm) &&
         !(pos->BSize(wm).GetUnit() == eStyleUnit_Auto &&
           pos->mOffset.GetBEndUnit(wm) == eStyleUnit_Auto &&
           pos->mOffset.GetBStartUnit(wm) != eStyleUnit_Auto)) ||
        pos->MinBSizeDependsOnContainer(wm) ||
        pos->MaxBSizeDependsOnContainer(wm) ||
        !IsFixedPaddingSize(padding->mPadding.GetBStart(wm)) ||
        !IsFixedPaddingSize(padding->mPadding.GetBEnd(wm))) {
      return true;
    }
      
    
    if (!IsFixedMarginSize(margin->mMargin.GetBStart(wm)) ||
        !IsFixedMarginSize(margin->mMargin.GetBEnd(wm))) {
      return true;
    }
    if (!IsFixedOffset(pos->mOffset.GetBStart(wm))) {
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
                                               bool                     aConstrainBSize,
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
    PrettyUC(aReflowState.AvailableWidth(), width, 16);
    PrettyUC(aReflowState.AvailableHeight(), height, 16);
    printf(" a=%s,%s ", width, height);
    PrettyUC(aReflowState.ComputedWidth(), width, 16);
    PrettyUC(aReflowState.ComputedHeight(), height, 16);
    printf("c=%s,%s \n", width, height);
  }
  AutoNoisyIndenter indent(nsBlockFrame::gNoisy);
#endif 

  WritingMode wm = aKidFrame->GetWritingMode();
  LogicalSize logicalCBSize(wm, aContainingBlock.Size());
  nscoord availISize = logicalCBSize.ISize(wm);
  if (availISize == -1) {
    NS_ASSERTION(aReflowState.ComputedSize(wm).ISize(wm) !=
                   NS_UNCONSTRAINEDSIZE,
                 "Must have a useful inline-size _somewhere_");
    availISize =
      aReflowState.ComputedSizeWithPadding(wm).ISize(wm);
  }

  nsHTMLReflowState kidReflowState(aPresContext, aReflowState, aKidFrame,
                                   LogicalSize(wm, availISize,
                                               NS_UNCONSTRAINEDSIZE),
                                   &logicalCBSize);

  
  WritingMode outerWM = aReflowState.GetWritingMode();
  const LogicalMargin border(outerWM,
                             aReflowState.mStyleBorder->GetComputedBorder());
  const LogicalMargin margin =
    kidReflowState.ComputedLogicalMargin().ConvertTo(outerWM, wm);
  bool constrainBSize = (aReflowState.AvailableBSize() != NS_UNCONSTRAINEDSIZE)
    && aConstrainBSize
       
    && (aDelegatingFrame->GetType() != nsGkAtoms::inlineFrame)
       
    && (aKidFrame->GetLogicalRect(aContainingBlock.width).BStart(wm) <=
        aReflowState.AvailableBSize());
       
       
       
  if (constrainBSize) {
    kidReflowState.AvailableBSize() =
      aReflowState.AvailableBSize() - border.ConvertTo(wm, outerWM).BStart(wm) -
      kidReflowState.ComputedLogicalMargin().BStart(wm);
    if (NS_AUTOOFFSET != kidReflowState.ComputedLogicalOffsets().BStart(wm)) {
      kidReflowState.AvailableBSize() -=
        kidReflowState.ComputedLogicalOffsets().BStart(wm);
    }
  }

  
  nsHTMLReflowMetrics kidDesiredSize(kidReflowState);
  aKidFrame->Reflow(aPresContext, kidDesiredSize, kidReflowState, aStatus);

  const LogicalSize kidSize = kidDesiredSize.Size(wm).ConvertTo(outerWM, wm);

  LogicalMargin offsets =
    kidReflowState.ComputedLogicalOffsets().ConvertTo(outerWM, wm);

  
  
  if ((NS_AUTOOFFSET == offsets.IStart(outerWM)) ||
      (NS_AUTOOFFSET == offsets.BStart(outerWM))) {
    if (-1 == logicalCBSize.ISize(wm)) {
      
      logicalCBSize =
        kidReflowState.ComputeContainingBlockRectangle(aPresContext,
                                                       &aReflowState);
    }

    if (NS_AUTOOFFSET == offsets.IStart(outerWM)) {
      NS_ASSERTION(NS_AUTOOFFSET != offsets.IEnd(outerWM),
                   "Can't solve for both start and end");
      offsets.IStart(outerWM) =
        logicalCBSize.ConvertTo(outerWM, wm).ISize(outerWM) -
        offsets.IEnd(outerWM) - margin.IStartEnd(outerWM) -
        kidSize.ISize(outerWM);
    }
    if (NS_AUTOOFFSET == offsets.BStart(outerWM)) {
      offsets.BStart(outerWM) =
        logicalCBSize.ConvertTo(outerWM, wm).BSize(outerWM) -
        offsets.BEnd(outerWM) - margin.BStartEnd(outerWM) -
        kidSize.BSize(outerWM);
    }
    kidReflowState.SetComputedLogicalOffsets(offsets.ConvertTo(wm, outerWM));
  }

  
  LogicalRect rect(outerWM,
                   border.IStart(outerWM) + offsets.IStart(outerWM) +
                     margin.IStart(outerWM),
                   border.BStart(outerWM) + offsets.BStart(outerWM) +
                     margin.BStart(outerWM),
                   kidSize.ISize(outerWM), kidSize.BSize(outerWM));
  nsRect r =
    rect.GetPhysicalRect(outerWM, logicalCBSize.Width(wm) +
                                  border.LeftRight(outerWM));
  
  if (outerWM.IsVertical() && !outerWM.IsBidiLTR()) {
    r.y = logicalCBSize.Height(wm) + border.TopBottom(outerWM) - r.YMost();
  }

  
  
  
  if (aContainingBlock.TopLeft() != nsPoint(0, 0)) {
    const nsStyleSides& offsets = kidReflowState.mStylePosition->mOffset;
    if (!(offsets.GetLeftUnit() == eStyleUnit_Auto &&
          offsets.GetRightUnit() == eStyleUnit_Auto)) {
      r.x += aContainingBlock.x;
    }
    if (!(offsets.GetTopUnit() == eStyleUnit_Auto &&
          offsets.GetBottomUnit() == eStyleUnit_Auto)) {
      r.y += aContainingBlock.y;
    }
  }

  aKidFrame->SetRect(r);

  nsView* view = aKidFrame->GetView();
  if (view) {
    
    
    nsContainerFrame::SyncFrameViewAfterReflow(aPresContext, aKidFrame, view,
                                               kidDesiredSize.VisualOverflow());
  } else {
    nsContainerFrame::PositionChildViews(aKidFrame);
  }

  aKidFrame->DidReflow(aPresContext, &kidReflowState,
                       nsDidReflowStatus::FINISHED);

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
           r.x, r.y, r.width, r.height);
  }
#endif

  if (aOverflowAreas) {
    aOverflowAreas->UnionWith(kidDesiredSize.mOverflowAreas + r.TopLeft());
  }
}
