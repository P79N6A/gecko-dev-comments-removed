







#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsFloatManager.h"
#include "nsColumnSetFrame.h"
#include "nsContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsLayoutUtils.h"

using namespace mozilla;

#ifdef DEBUG
#undef  NOISY_MAX_ELEMENT_SIZE
#undef   REALLY_NOISY_MAX_ELEMENT_SIZE
#undef  NOISY_BLOCK_DIR_MARGINS
#else
#undef  NOISY_MAX_ELEMENT_SIZE
#undef   REALLY_NOISY_MAX_ELEMENT_SIZE
#undef  NOISY_BLOCK_DIR_MARGINS
#endif

nsBlockReflowContext::nsBlockReflowContext(nsPresContext* aPresContext,
                                           const nsHTMLReflowState& aParentRS)
  : mPresContext(aPresContext),
    mOuterReflowState(aParentRS),
    mSpace(aParentRS.GetWritingMode()),
    mMetrics(aParentRS)
{
}

static nsIFrame* DescendIntoBlockLevelFrame(nsIFrame* aFrame)
{
  nsIAtom* type = aFrame->GetType();
  if (type == nsGkAtoms::columnSetFrame) {
    static_cast<nsColumnSetFrame*>(aFrame)->DrainOverflowColumns();
    nsIFrame* child = aFrame->GetFirstPrincipalChild();
    if (child) {
      return DescendIntoBlockLevelFrame(child);
    }
  }
  return aFrame;
}

bool
nsBlockReflowContext::ComputeCollapsedBStartMargin(const nsHTMLReflowState& aRS,
                                                   nsCollapsingMargin* aMargin,
                                                   nsIFrame* aClearanceFrame,
                                                   bool* aMayNeedRetry,
                                                   bool* aBlockIsEmpty)
{
  WritingMode wm = aRS.GetWritingMode();
  WritingMode parentWM = mMetrics.GetWritingMode();

  
  aMargin->Include(aRS.ComputedLogicalMargin().ConvertTo(parentWM, wm).BStart(parentWM));

  
  
  

#ifdef NOISY_BLOCKDIR_MARGINS
  nsFrame::ListTag(stdout, aRS.frame);
  printf(": %d => %d\n", aRS.ComputedLogicalMargin().BStart(wm), aMargin->get());
#endif

  bool dirtiedLine = false;
  bool setBlockIsEmpty = false;

  
  
  
  
  
  nsIFrame* frame = DescendIntoBlockLevelFrame(aRS.frame);
  nsPresContext* prescontext = frame->PresContext();
  nsBlockFrame* block = nullptr;
  if (0 == aRS.ComputedLogicalBorderPadding().BStart(wm)) {
    block = nsLayoutUtils::GetAsBlock(frame);
    if (block) {
      bool bStartMarginRoot, unused;
      block->IsMarginRoot(&bStartMarginRoot, &unused);
      if (bStartMarginRoot) {
        block = nullptr;
      }
    }
  }

  
  
  
  
  
  
  for ( ;block; block = static_cast<nsBlockFrame*>(block->GetNextInFlow())) {
    for (int overflowLines = 0; overflowLines <= 1; ++overflowLines) {
      nsBlockFrame::line_iterator line;
      nsBlockFrame::line_iterator line_end;
      bool anyLines = true;
      if (overflowLines) {
        nsBlockFrame::FrameLines* frames = block->GetOverflowLines();
        nsLineList* lines = frames ? &frames->mLines : nullptr;
        if (!lines) {
          anyLines = false;
        } else {
          line = lines->begin();
          line_end = lines->end();
        }
      } else {
        line = block->begin_lines();
        line_end = block->end_lines();
      }
      for (; anyLines && line != line_end; ++line) {
        if (!aClearanceFrame && line->HasClearance()) {
          
          
          
          line->ClearHasClearance();
          line->MarkDirty();
          dirtiedLine = true;
        }

        bool isEmpty;
        if (line->IsInline()) {
          isEmpty = line->IsEmpty();
        } else {
          nsIFrame* kid = line->mFirstChild;
          if (kid == aClearanceFrame) {
            line->SetHasClearance();
            line->MarkDirty();
            dirtiedLine = true;
            goto done;
          }
          
          
          
          
          

          
          
          
          
          const nsHTMLReflowState* outerReflowState = &aRS;
          if (frame != aRS.frame) {
            NS_ASSERTION(frame->GetParent() == aRS.frame,
                         "Can only drill through one level of block wrapper");
            LogicalSize availSpace = aRS.ComputedSize(frame->GetWritingMode());
            outerReflowState = new nsHTMLReflowState(prescontext,
                                                     aRS, frame, availSpace);
          }
          {
            LogicalSize availSpace =
              outerReflowState->ComputedSize(kid->GetWritingMode());
            nsHTMLReflowState innerReflowState(prescontext,
                                               *outerReflowState, kid,
                                               availSpace);
            
            
            if (kid->StyleDisplay()->mBreakType != NS_STYLE_CLEAR_NONE) {
              *aMayNeedRetry = true;
            }
            if (ComputeCollapsedBStartMargin(innerReflowState, aMargin,
                                             aClearanceFrame, aMayNeedRetry,
                                             &isEmpty)) {
              line->MarkDirty();
              dirtiedLine = true;
            }
            if (isEmpty) {
              WritingMode innerWM = innerReflowState.GetWritingMode();
              LogicalMargin innerMargin =
                innerReflowState.ComputedLogicalMargin().ConvertTo(parentWM, innerWM);
              aMargin->Include(innerMargin.BEnd(parentWM));
            }
          }
          if (outerReflowState != &aRS) {
            delete const_cast<nsHTMLReflowState*>(outerReflowState);
          }
        }
        if (!isEmpty) {
          if (!setBlockIsEmpty && aBlockIsEmpty) {
            setBlockIsEmpty = true;
            *aBlockIsEmpty = false;
          }
          goto done;
        }
      }
      if (!setBlockIsEmpty && aBlockIsEmpty) {
        
        
        setBlockIsEmpty = true;
        
        *aBlockIsEmpty = aRS.frame->IsSelfEmpty();
      }
    }
  }
  done:

  if (!setBlockIsEmpty && aBlockIsEmpty) {
    *aBlockIsEmpty = aRS.frame->IsEmpty();
  }
  
#ifdef NOISY_BLOCKDIR_MARGINS
  nsFrame::ListTag(stdout, aRS.frame);
  printf(": => %d\n", aMargin->get());
#endif

  return dirtiedLine;
}

void
nsBlockReflowContext::ReflowBlock(const LogicalRect&  aSpace,
                                  bool                aApplyBStartMargin,
                                  nsCollapsingMargin& aPrevMargin,
                                  nscoord             aClearance,
                                  bool                aIsAdjacentWithBStart,
                                  nsLineBox*          aLine,
                                  nsHTMLReflowState&  aFrameRS,
                                  nsReflowStatus&     aFrameReflowStatus,
                                  nsBlockReflowState& aState)
{
  mFrame = aFrameRS.frame;
  mWritingMode = aState.mReflowState.GetWritingMode();
  mContainerWidth = aState.ContainerWidth();
  mSpace = aSpace;

  if (!aIsAdjacentWithBStart) {
    aFrameRS.mFlags.mIsTopOfPage = false;  
  }

  if (aApplyBStartMargin) {
    mBStartMargin = aPrevMargin;

#ifdef NOISY_BLOCKDIR_MARGINS
    nsFrame::ListTag(stdout, mOuterReflowState.frame);
    printf(": reflowing ");
    nsFrame::ListTag(stdout, mFrame);
    printf(" margin => %d, clearance => %d\n", mBStartMargin.get(), aClearance);
#endif

    
    
    if (mWritingMode.IsOrthogonalTo(mFrame->GetWritingMode())) {
      if (NS_UNCONSTRAINEDSIZE != aFrameRS.AvailableISize()) {
        aFrameRS.AvailableISize() -= mBStartMargin.get() + aClearance;
      }
    } else {
      if (NS_UNCONSTRAINEDSIZE != aFrameRS.AvailableBSize()) {
        aFrameRS.AvailableBSize() -= mBStartMargin.get() + aClearance;
      }
    }
  }

  nscoord tI = 0, tB = 0;
  
  
  
  
  
  if (aLine) {
    
    
    

    WritingMode frameWM = aFrameRS.GetWritingMode();
    LogicalMargin usedMargin =
      aFrameRS.ComputedLogicalMargin().ConvertTo(mWritingMode, frameWM);
    mICoord = mSpace.IStart(mWritingMode) + usedMargin.IStart(mWritingMode);
    mBCoord = mSpace.BStart(mWritingMode) + mBStartMargin.get() + aClearance;

    LogicalRect space(mWritingMode, mICoord, mBCoord,
                      mSpace.ISize(mWritingMode) -
                      usedMargin.IStartEnd(mWritingMode),
                      mSpace.BSize(mWritingMode) -
                      usedMargin.BStartEnd(mWritingMode));
    tI = space.LineLeft(mWritingMode, mContainerWidth);
    tB = mBCoord;

    if ((mFrame->GetStateBits() & NS_BLOCK_FLOAT_MGR) == 0)
      aFrameRS.mBlockDelta =
        mOuterReflowState.mBlockDelta + mBCoord - aLine->BStart();
  }

#ifdef DEBUG
  mMetrics.ISize(mWritingMode) = nscoord(0xdeadbeef);
  mMetrics.BSize(mWritingMode) = nscoord(0xdeadbeef);
#endif

  mOuterReflowState.mFloatManager->Translate(tI, tB);
  mFrame->Reflow(mPresContext, mMetrics, aFrameRS, aFrameReflowStatus);
  mOuterReflowState.mFloatManager->Translate(-tI, -tB);

#ifdef DEBUG
  if (!NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus)) {
    if (CRAZY_SIZE(mMetrics.ISize(mWritingMode)) ||
        CRAZY_SIZE(mMetrics.BSize(mWritingMode))) {
      printf("nsBlockReflowContext: ");
      nsFrame::ListTag(stdout, mFrame);
      printf(" metrics=%d,%d!\n",
             mMetrics.ISize(mWritingMode), mMetrics.BSize(mWritingMode));
    }
    if ((mMetrics.ISize(mWritingMode) == nscoord(0xdeadbeef)) ||
        (mMetrics.BSize(mWritingMode) == nscoord(0xdeadbeef))) {
      printf("nsBlockReflowContext: ");
      nsFrame::ListTag(stdout, mFrame);
      printf(" didn't set i/b %d,%d!\n",
             mMetrics.ISize(mWritingMode), mMetrics.BSize(mWritingMode));
    }
  }
#endif

  if (!mFrame->HasOverflowAreas()) {
    mMetrics.SetOverflowAreasToDesiredBounds();
  }

  if (!NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus) ||
      (mFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    
    
    
    
    if (NS_FRAME_IS_FULLY_COMPLETE(aFrameReflowStatus)) {
      nsIFrame* kidNextInFlow = mFrame->GetNextInFlow();
      if (nullptr != kidNextInFlow) {
        
        
        
        
        
        nsOverflowContinuationTracker::AutoFinish fini(aState.mOverflowTracker, mFrame);
        kidNextInFlow->GetParent()->DeleteNextInFlowChild(kidNextInFlow, true);
      }
    }
  }
}






bool
nsBlockReflowContext::PlaceBlock(const nsHTMLReflowState&  aReflowState,
                                 bool                      aForceFit,
                                 nsLineBox*                aLine,
                                 nsCollapsingMargin&       aBEndMarginResult,
                                 nsOverflowAreas&          aOverflowAreas,
                                 nsReflowStatus            aReflowStatus)
{
  
  WritingMode wm = aReflowState.GetWritingMode();
  WritingMode parentWM = mMetrics.GetWritingMode();
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    aBEndMarginResult = mMetrics.mCarriedOutBEndMargin;
    aBEndMarginResult.Include(aReflowState.ComputedLogicalMargin().
      ConvertTo(parentWM, wm).BEnd(parentWM));
  } else {
    
    aBEndMarginResult.Zero();
  }

  nscoord backupContainingBlockAdvance = 0;

  
  
  
  
  
  
  
  
  mFrame->RemoveStateBits(NS_FRAME_IS_DIRTY);
  bool empty = 0 == mMetrics.BSize(parentWM) && aLine->CachedIsEmpty();
  if (empty) {
    
    
    aBEndMarginResult.Include(mBStartMargin);

#ifdef NOISY_BLOCKDIR_MARGINS
    printf("  ");
    nsFrame::ListTag(stdout, mOuterReflowState.frame);
    printf(": ");
    nsFrame::ListTag(stdout, mFrame);
    printf(" -- collapsing block start & end margin together; BStart=%d spaceBStart=%d\n",
           mBCoord, mSpace.BStart(mWritingMode));
#endif
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    backupContainingBlockAdvance = mBStartMargin.get();
  }

  
  
  
  
  if (!empty && !aForceFit &&
      mSpace.BSize(mWritingMode) != NS_UNCONSTRAINEDSIZE) {
    nscoord bEnd = mBCoord -
                   backupContainingBlockAdvance + mMetrics.BSize(mWritingMode);
    if (bEnd > mSpace.BEnd(mWritingMode)) {
      
      mFrame->DidReflow(mPresContext, &aReflowState,
                        nsDidReflowStatus::FINISHED);
      return false;
    }
  }

  aLine->SetBounds(mWritingMode,
                   mICoord, mBCoord - backupContainingBlockAdvance,
                   mMetrics.ISize(mWritingMode), mMetrics.BSize(mWritingMode),
                   mContainerWidth);

  WritingMode frameWM = mFrame->GetWritingMode();
  LogicalPoint logPos =
    LogicalPoint(mWritingMode, mICoord, mBCoord).
      ConvertTo(frameWM, mWritingMode, mContainerWidth - mMetrics.Width());

  
  
  mFrame->SetSize(mWritingMode, mMetrics.Size(mWritingMode));
  aReflowState.ApplyRelativePositioning(&logPos, mContainerWidth);

  
  nsContainerFrame::FinishReflowChild(mFrame, mPresContext, mMetrics,
                                      &aReflowState, frameWM, logPos,
                                      mContainerWidth, 0);

  aOverflowAreas = mMetrics.mOverflowAreas + mFrame->GetPosition();

  return true;
}
