








































#include "nsBlockReflowContext.h"
#include "nsLineLayout.h"
#include "nsFloatManager.h"
#include "nsIFontMetrics.h"
#include "nsPresContext.h"
#include "nsFrameManager.h"
#include "nsIContent.h"
#include "nsStyleContext.h"
#include "nsHTMLContainerFrame.h"
#include "nsBlockFrame.h"
#include "nsLineBox.h"
#include "nsIDOMHTMLTableCellElement.h"
#include "nsIDOMHTMLBodyElement.h"
#include "nsGkAtoms.h"
#include "nsCOMPtr.h"
#include "nsLayoutUtils.h"

#ifdef NS_DEBUG
#undef  NOISY_MAX_ELEMENT_SIZE
#undef   REALLY_NOISY_MAX_ELEMENT_SIZE
#undef  NOISY_VERTICAL_MARGINS
#else
#undef  NOISY_MAX_ELEMENT_SIZE
#undef   REALLY_NOISY_MAX_ELEMENT_SIZE
#undef  NOISY_VERTICAL_MARGINS
#endif

nsBlockReflowContext::nsBlockReflowContext(nsPresContext* aPresContext,
                                           const nsHTMLReflowState& aParentRS)
  : mPresContext(aPresContext),
    mOuterReflowState(aParentRS),
    mMetrics()
{
}

static nsIFrame* DescendIntoBlockLevelFrame(nsIFrame* aFrame)
{
  nsIAtom* type = aFrame->GetType();
  if (type == nsGkAtoms::columnSetFrame)
    return DescendIntoBlockLevelFrame(aFrame->GetFirstChild(nsnull));
  return aFrame;
}

PRBool
nsBlockReflowContext::ComputeCollapsedTopMargin(const nsHTMLReflowState& aRS,
  nsCollapsingMargin* aMargin, nsIFrame* aClearanceFrame,
  PRBool* aMayNeedRetry, PRBool* aBlockIsEmpty)
{
  
  aMargin->Include(aRS.mComputedMargin.top);

  
  
  

#ifdef NOISY_VERTICAL_MARGINS
  nsFrame::ListTag(stdout, aRS.frame);
  printf(": %d => %d\n", aRS.mComputedMargin.top, aMargin->get());
#endif

  PRBool dirtiedLine = PR_FALSE;
  PRBool setBlockIsEmpty = PR_FALSE;

  
  
  
  
  
  nsIFrame* frame = DescendIntoBlockLevelFrame(aRS.frame);
  nsPresContext* prescontext = frame->PresContext();
  if (0 == aRS.mComputedBorderPadding.top &&
      nsLayoutUtils::GetAsBlock(frame) &&
      !nsBlockFrame::BlockIsMarginRoot(frame)) {
    
    
    
    
    
    
    for (nsBlockFrame* block = static_cast<nsBlockFrame*>(frame);
         block; block = static_cast<nsBlockFrame*>(block->GetNextInFlow())) {
      for (PRBool overflowLines = PR_FALSE; overflowLines <= PR_TRUE; ++overflowLines) {
        nsBlockFrame::line_iterator line;
        nsBlockFrame::line_iterator line_end;
        PRBool anyLines = PR_TRUE;
        if (overflowLines) {
          nsLineList* lines = block->GetOverflowLines();
          if (!lines) {
            anyLines = PR_FALSE;
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
            dirtiedLine = PR_TRUE;
          }
          
          PRBool isEmpty;
          if (line->IsInline()) {
            isEmpty = line->IsEmpty();
          } else {
            nsIFrame* kid = line->mFirstChild;
            if (kid == aClearanceFrame) {
              line->SetHasClearance();
              line->MarkDirty();
              dirtiedLine = PR_TRUE;
              goto done;
            }
            
            
            
            
            
            
            
            
            
            
            const nsHTMLReflowState* outerReflowState = &aRS;
            if (frame != aRS.frame) {
              NS_ASSERTION(frame->GetParent() == aRS.frame,
                           "Can only drill through one level of block wrapper");
              nsSize availSpace(aRS.ComputedWidth(), aRS.ComputedHeight());
              outerReflowState = new nsHTMLReflowState(prescontext,
                                                       aRS, frame, availSpace);
              if (!outerReflowState)
                goto done;
            }
            {
              nsSize availSpace(outerReflowState->ComputedWidth(),
                                outerReflowState->ComputedHeight());
              nsHTMLReflowState innerReflowState(prescontext,
                                                 *outerReflowState, kid,
                                                 availSpace);
              
              
              if (kid->GetStyleDisplay()->mBreakType != NS_STYLE_CLEAR_NONE) {
                *aMayNeedRetry = PR_TRUE;
              }
              if (ComputeCollapsedTopMargin(innerReflowState, aMargin, aClearanceFrame, aMayNeedRetry, &isEmpty)) {
                line->MarkDirty();
                dirtiedLine = PR_TRUE;
              }
              if (isEmpty)
                aMargin->Include(innerReflowState.mComputedMargin.bottom);
            }
            if (outerReflowState != &aRS) {
              delete const_cast<nsHTMLReflowState*>(outerReflowState);
            }
          }
          if (!isEmpty) {
            if (!setBlockIsEmpty && aBlockIsEmpty) {
              setBlockIsEmpty = PR_TRUE;
              *aBlockIsEmpty = PR_FALSE;
            }
            goto done;
          }
        }
        if (!setBlockIsEmpty && aBlockIsEmpty) {
          
          
          setBlockIsEmpty = PR_TRUE;
          
          *aBlockIsEmpty = aRS.frame->IsSelfEmpty();
        }
      }
    }
  done:
    ;
  }

  if (!setBlockIsEmpty && aBlockIsEmpty) {
    *aBlockIsEmpty = aRS.frame->IsEmpty();
  }
  
#ifdef NOISY_VERTICAL_MARGINS
  nsFrame::ListTag(stdout, aRS.frame);
  printf(": => %d\n", aMargin->get());
#endif

  return dirtiedLine;
}

nsresult
nsBlockReflowContext::ReflowBlock(const nsRect&       aSpace,
                                  PRBool              aApplyTopMargin,
                                  nsCollapsingMargin& aPrevMargin,
                                  nscoord             aClearance,
                                  PRBool              aIsAdjacentWithTop,
                                  nsLineBox*          aLine,
                                  nsHTMLReflowState&  aFrameRS,
                                  nsReflowStatus&     aFrameReflowStatus,
                                  nsBlockReflowState& aState)
{
  nsresult rv = NS_OK;
  mFrame = aFrameRS.frame;
  mSpace = aSpace;

  if (!aIsAdjacentWithTop) {
    aFrameRS.mFlags.mIsTopOfPage = PR_FALSE;  
  }

  if (aApplyTopMargin) {
    mTopMargin = aPrevMargin;

#ifdef NOISY_VERTICAL_MARGINS
    nsFrame::ListTag(stdout, mOuterReflowState.frame);
    printf(": reflowing ");
    nsFrame::ListTag(stdout, mFrame);
    printf(" margin => %d, clearance => %d\n", mTopMargin.get(), aClearance);
#endif

    
    
    if (NS_UNCONSTRAINEDSIZE != aFrameRS.availableHeight) {
      aFrameRS.availableHeight -= mTopMargin.get() + aClearance;
    }
  }

  nscoord tx = 0, ty = 0;
  
  
  
  
  if (aLine) {
    
    
    

    nscoord x = mSpace.x + aFrameRS.mComputedMargin.left;
    nscoord y = mSpace.y + mTopMargin.get() + aClearance;

    if ((mFrame->GetStateBits() & NS_BLOCK_FLOAT_MGR) == 0)
      aFrameRS.mBlockDelta = mOuterReflowState.mBlockDelta + y - aLine->mBounds.y;

    mX = x;
    mY = y;

    
    
    
    
    
    
    
    tx = x - mOuterReflowState.mComputedBorderPadding.left;
    ty = y - mOuterReflowState.mComputedBorderPadding.top;
  }

  
  mFrame->WillReflow(mPresContext);

#ifdef DEBUG
  mMetrics.width = nscoord(0xdeadbeef);
  mMetrics.height = nscoord(0xdeadbeef);
#endif

  mOuterReflowState.mFloatManager->Translate(tx, ty);
  rv = mFrame->Reflow(mPresContext, mMetrics, aFrameRS, aFrameReflowStatus);
  mOuterReflowState.mFloatManager->Translate(-tx, -ty);

#ifdef DEBUG
  if (!NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus)) {
    if (CRAZY_WIDTH(mMetrics.width) || CRAZY_HEIGHT(mMetrics.height)) {
      printf("nsBlockReflowContext: ");
      nsFrame::ListTag(stdout, mFrame);
      printf(" metrics=%d,%d!\n", mMetrics.width, mMetrics.height);
    }
    if ((mMetrics.width == nscoord(0xdeadbeef)) ||
        (mMetrics.height == nscoord(0xdeadbeef))) {
      printf("nsBlockReflowContext: ");
      nsFrame::ListTag(stdout, mFrame);
      printf(" didn't set w/h %d,%d!\n", mMetrics.width, mMetrics.height);
    }
  }
#endif

  if (!mFrame->HasOverflowRect()) {
    
    mMetrics.mOverflowArea.x = 0;
    mMetrics.mOverflowArea.y = 0;
    mMetrics.mOverflowArea.width = mMetrics.width;
    mMetrics.mOverflowArea.height = mMetrics.height;
  }

  if (!NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus) ||
      (mFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    
    
    
    
    if (NS_FRAME_IS_FULLY_COMPLETE(aFrameReflowStatus)) {
      nsIFrame* kidNextInFlow = mFrame->GetNextInFlow();
      if (nsnull != kidNextInFlow) {
        
        
        
        
        

        aState.mOverflowTracker->Finish(mFrame);
        static_cast<nsHTMLContainerFrame*>(kidNextInFlow->GetParent())
          ->DeleteNextInFlowChild(mPresContext, kidNextInFlow, PR_TRUE);
      }
    }
  }

  return rv;
}






PRBool
nsBlockReflowContext::PlaceBlock(const nsHTMLReflowState& aReflowState,
                                 PRBool                   aForceFit,
                                 nsLineBox*               aLine,
                                 nsCollapsingMargin&      aBottomMarginResult,
                                 nsRect&                  aInFlowBounds,
                                 nsRect&                  aCombinedRect,
                                 nsReflowStatus           aReflowStatus)
{
  
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    aBottomMarginResult = mMetrics.mCarriedOutBottomMargin;
    aBottomMarginResult.Include(aReflowState.mComputedMargin.bottom);
  } else {
    
    aBottomMarginResult.Zero();
  }

  nscoord x = mX;
  nscoord y = mY;
  nscoord backupContainingBlockAdvance = 0;

  
  
  
  
  
  
  
  
  mFrame->RemoveStateBits(NS_FRAME_IS_DIRTY);
  PRBool empty = 0 == mMetrics.height && aLine->CachedIsEmpty();
  if (empty) {
    
    
    aBottomMarginResult.Include(mTopMargin);

#ifdef NOISY_VERTICAL_MARGINS
    printf("  ");
    nsFrame::ListTag(stdout, mOuterReflowState.frame);
    printf(": ");
    nsFrame::ListTag(stdout, mFrame);
    printf(" -- collapsing top & bottom margin together; y=%d spaceY=%d\n",
           y, mSpace.y);
#endif
    
    
    
    
    
    
    
    

    
    
    
    
    
    
    
    
    backupContainingBlockAdvance = mTopMargin.get();
  }

  
  
  
  
  if (!empty && !aForceFit && mSpace.height != NS_UNCONSTRAINEDSIZE) {
    nscoord yMost = y - backupContainingBlockAdvance + mMetrics.height;
    if (yMost > mSpace.YMost()) {
      
      mFrame->DidReflow(mPresContext, &aReflowState, NS_FRAME_REFLOW_FINISHED);
      return PR_FALSE;
    }
  }

  aInFlowBounds = nsRect(x, y - backupContainingBlockAdvance,
                         mMetrics.width, mMetrics.height);
  
  
  const nsStyleDisplay* styleDisp = mFrame->GetStyleDisplay();
  if (NS_STYLE_POSITION_RELATIVE == styleDisp->mPosition) {
    x += aReflowState.mComputedOffsets.left;
    y += aReflowState.mComputedOffsets.top;
  }
  
  
  nsContainerFrame::FinishReflowChild(mFrame, mPresContext, &aReflowState, mMetrics, x, y, 0);
  
  aCombinedRect = mMetrics.mOverflowArea + nsPoint(x, y);

  return PR_TRUE;
}
