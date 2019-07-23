








































#include "nsBlockReflowContext.h"
#include "nsLineLayout.h"
#include "nsSpaceManager.h"
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
  mStyleBorder = nsnull;
  mStyleMargin = nsnull;
  mStylePadding = nsnull;
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

  
  
  
  
  
  void* bf;
  nsIFrame* frame = DescendIntoBlockLevelFrame(aRS.frame);
  nsPresContext* prescontext = frame->GetPresContext();
  if (0 == aRS.mComputedBorderPadding.top &&
      NS_SUCCEEDED(frame->QueryInterface(kBlockFrameCID, &bf)) &&
      !nsBlockFrame::BlockIsMarginRoot(frame)) {
    
    
    
    
    
    
    for (nsBlockFrame* block = NS_STATIC_CAST(nsBlockFrame*, frame);
         block; block = NS_STATIC_CAST(nsBlockFrame*, block->GetNextInFlow())) {
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
              nsSize availSpace(aRS.ComputedWidth(), aRS.mComputedHeight);
              outerReflowState = new nsHTMLReflowState(prescontext,
                                                       aRS, frame, availSpace);
              if (!outerReflowState)
                goto done;
            }
            {
              nsSize availSpace(outerReflowState->ComputedWidth(),
                                outerReflowState->mComputedHeight);
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
              delete NS_CONST_CAST(nsHTMLReflowState*, outerReflowState);
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

static void
nsPointDtor(void *aFrame, nsIAtom *aPropertyName,
            void *aPropertyValue, void *aDtorData)
{
  nsPoint *point = NS_STATIC_CAST(nsPoint*, aPropertyValue);
  delete point;
}

nsresult
nsBlockReflowContext::ReflowBlock(const nsRect&       aSpace,
                                  PRBool              aApplyTopMargin,
                                  nsCollapsingMargin& aPrevMargin,
                                  nscoord             aClearance,
                                  PRBool              aIsAdjacentWithTop,
                                  nsMargin&           aComputedOffsets,
                                  nsHTMLReflowState&  aFrameRS,
                                  nsReflowStatus&     aFrameReflowStatus)
{
  nsresult rv = NS_OK;
  mFrame = aFrameRS.frame;
  mSpace = aSpace;

  const nsStyleDisplay* display = mFrame->GetStyleDisplay();

  aComputedOffsets = aFrameRS.mComputedOffsets;
  if (NS_STYLE_POSITION_RELATIVE == display->mPosition) {
    nsPropertyTable *propTable = mPresContext->PropertyTable();

    nsPoint *offsets = NS_STATIC_CAST(nsPoint*,
        propTable->GetProperty(mFrame, nsGkAtoms::computedOffsetProperty));

    if (offsets)
      offsets->MoveTo(aComputedOffsets.left, aComputedOffsets.top);
    else {
      offsets = new nsPoint(aComputedOffsets.left, aComputedOffsets.top);
      if (offsets)
        propTable->SetProperty(mFrame, nsGkAtoms::computedOffsetProperty,
                               offsets, nsPointDtor, nsnull);
    }
  }

  aFrameRS.mLineLayout = nsnull;
  if (!aIsAdjacentWithTop) {
    aFrameRS.mFlags.mIsTopOfPage = PR_FALSE;  
  }
  mComputedWidth = aFrameRS.ComputedWidth();

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

  
  
  
  mMargin = aFrameRS.mComputedMargin;
  mStyleBorder = aFrameRS.mStyleBorder;
  mStyleMargin = aFrameRS.mStyleMargin;
  mStylePadding = aFrameRS.mStylePadding;
  nscoord x;
  nscoord y = mSpace.y + mTopMargin.get() + aClearance;

  
  
  if (NS_STYLE_FLOAT_RIGHT == aFrameRS.mStyleDisplay->mFloats) {
    nscoord frameWidth;
     
    if (NS_UNCONSTRAINEDSIZE == aFrameRS.ComputedWidth()) {
      
      frameWidth = mFrame->GetSize().width;
    } else {
      frameWidth = aFrameRS.ComputedWidth() +
                   aFrameRS.mComputedBorderPadding.left +
                   aFrameRS.mComputedBorderPadding.right;
    }

    
    if (NS_UNCONSTRAINEDSIZE == mSpace.width)
      x = mSpace.x;
    else
      x = mSpace.XMost() - mMargin.right - frameWidth;

  } else {
    x = mSpace.x + mMargin.left;
  }
  mX = x;
  mY = y;

   
   
   
   
   
   
   
   nscoord tx = x - mOuterReflowState.mComputedBorderPadding.left;
   nscoord ty = y - mOuterReflowState.mComputedBorderPadding.top;
 
  
  if (NS_STYLE_POSITION_RELATIVE == aFrameRS.mStyleDisplay->mPosition) {
    x += aFrameRS.mComputedOffsets.left;
    y += aFrameRS.mComputedOffsets.top;
  }

  
  mFrame->WillReflow(mPresContext);

  
  
  
  
  mFrame->SetPosition(nsPoint(x, y));
  nsContainerFrame::PositionFrameView(mFrame);

#ifdef DEBUG
  mMetrics.width = nscoord(0xdeadbeef);
  mMetrics.height = nscoord(0xdeadbeef);
#endif

  mOuterReflowState.mSpaceManager->Translate(tx, ty);
  rv = mFrame->Reflow(mPresContext, mMetrics, aFrameRS, aFrameReflowStatus);
  mOuterReflowState.mSpaceManager->Translate(-tx, -ty);

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

  if (!(NS_FRAME_OUTSIDE_CHILDREN & mFrame->GetStateBits())) {
    
    mMetrics.mOverflowArea.x = 0;
    mMetrics.mOverflowArea.y = 0;
    mMetrics.mOverflowArea.width = mMetrics.width;
    mMetrics.mOverflowArea.height = mMetrics.height;
  }

  if (!NS_INLINE_IS_BREAK_BEFORE(aFrameReflowStatus) ||
      (mFrame->GetStateBits() & NS_FRAME_OUT_OF_FLOW)) {
    
    
    
    
    if (NS_FRAME_IS_COMPLETE(aFrameReflowStatus)) {
      nsIFrame* kidNextInFlow = mFrame->GetNextInFlow();
      if (nsnull != kidNextInFlow) {
        
        
        
        
        

        NS_STATIC_CAST(nsHTMLContainerFrame*, kidNextInFlow->GetParent())
          ->DeleteNextInFlowChild(mPresContext, kidNextInFlow);
      }
    }
  }

  return rv;
}






PRBool
nsBlockReflowContext::PlaceBlock(const nsHTMLReflowState& aReflowState,
                                 PRBool                   aForceFit,
                                 nsLineBox*               aLine,
                                 const nsMargin&          aComputedOffsets,
                                 nsCollapsingMargin&      aBottomMarginResult,
                                 nsRect&                  aInFlowBounds,
                                 nsRect&                  aCombinedRect,
                                 nsReflowStatus           aReflowStatus)
{
  
  if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
    aBottomMarginResult = mMetrics.mCarriedOutBottomMargin;
    aBottomMarginResult.Include(mMargin.bottom);
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
    x += aComputedOffsets.left;
    y += aComputedOffsets.top;
  }
  
  
  nsContainerFrame::FinishReflowChild(mFrame, mPresContext, &aReflowState, mMetrics, x, y, 0);
  
  aCombinedRect = mMetrics.mOverflowArea + nsPoint(x, y);

  return PR_TRUE;
}
