











































#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsBlockFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsIFrame.h"
#include "nsFrameManager.h"

#include "nsINameSpaceManager.h"


#ifdef DEBUG
#include "nsBlockDebugFlags.h"
#endif

nsBlockReflowState::nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                                       nsPresContext* aPresContext,
                                       nsBlockFrame* aFrame,
                                       const nsHTMLReflowMetrics& aMetrics,
                                       PRBool aTopMarginRoot,
                                       PRBool aBottomMarginRoot,
                                       PRBool aBlockNeedsFloatManager)
  : mBlock(aFrame),
    mPresContext(aPresContext),
    mReflowState(aReflowState),
    mOverflowTracker(nsnull),
    mPrevBottomMargin(),
    mLineNumber(0),
    mFlags(0),
    mFloatBreakType(NS_STYLE_CLEAR_NONE)
{
  SetFlag(BRS_ISFIRSTINFLOW, aFrame->GetPrevInFlow() == nsnull);
  SetFlag(BRS_ISOVERFLOWCONTAINER,
          IS_TRUE_OVERFLOW_CONTAINER(aFrame));

  const nsMargin& borderPadding = BorderPadding();

  if (aTopMarginRoot || 0 != aReflowState.mComputedBorderPadding.top) {
    SetFlag(BRS_ISTOPMARGINROOT, PR_TRUE);
  }
  if (aBottomMarginRoot || 0 != aReflowState.mComputedBorderPadding.bottom) {
    SetFlag(BRS_ISBOTTOMMARGINROOT, PR_TRUE);
  }
  if (GetFlag(BRS_ISTOPMARGINROOT)) {
    SetFlag(BRS_APPLYTOPMARGIN, PR_TRUE);
  }
  if (aBlockNeedsFloatManager) {
    SetFlag(BRS_FLOAT_MGR, PR_TRUE);
  }
  
  mFloatManager = aReflowState.mFloatManager;

  NS_ASSERTION(mFloatManager,
               "FloatManager should be set in nsBlockReflowState" );
  if (mFloatManager) {
    
    
    mFloatManager->Translate(borderPadding.left, borderPadding.top);
    mFloatManager->GetTranslation(mFloatManagerX, mFloatManagerY);
    mFloatManager->PushState(&mFloatManagerStateBefore); 
  }

  mReflowStatus = NS_FRAME_COMPLETE;

  mPresContext = aPresContext;
  mNextInFlow = static_cast<nsBlockFrame*>(mBlock->GetNextInFlow());

  NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aReflowState.ComputedWidth(),
               "no unconstrained widths should be present anymore");
  mContentArea.width = aReflowState.ComputedWidth();

  
  
  
  
  
  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight) {
    
    
    
    mBottomEdge = aReflowState.availableHeight - borderPadding.bottom;
    mContentArea.height = NS_MAX(0, mBottomEdge - borderPadding.top);
  }
  else {
    
    
    SetFlag(BRS_UNCONSTRAINEDHEIGHT, PR_TRUE);
    mContentArea.height = mBottomEdge = NS_UNCONSTRAINEDSIZE;
  }

  mY = borderPadding.top;

  mPrevChild = nsnull;
  mCurrentLine = aFrame->end_lines();

  mMinLineHeight = aReflowState.CalcLineHeight();
}

nsBlockReflowState::~nsBlockReflowState()
{
  NS_ASSERTION(mFloatContinuations.IsEmpty(),
               "Leaking float continuation frames");

  
  
  if (mFloatManager) {
    const nsMargin& borderPadding = BorderPadding();
    mFloatManager->Translate(-borderPadding.left, -borderPadding.top);
  }

  if (GetFlag(BRS_PROPTABLE_FLOATCLIST)) {
    mBlock->UnsetProperty(nsGkAtoms::floatContinuationProperty);
  }
}

nsLineBox*
nsBlockReflowState::NewLineBox(nsIFrame* aFrame,
                               PRInt32 aCount,
                               PRBool aIsBlock)
{
  return NS_NewLineBox(mPresContext->PresShell(), aFrame, aCount, aIsBlock);
}

void
nsBlockReflowState::FreeLineBox(nsLineBox* aLine)
{
  if (aLine) {
    aLine->Destroy(mPresContext->PresShell());
  }
}

void
nsBlockReflowState::ComputeReplacedBlockOffsetsForFloats(nsIFrame* aFrame,
                                                         const nsRect& aFloatAvailableSpace,
                                                         nscoord& aLeftResult,
                                                         nscoord& aRightResult,
                                                         nsBlockFrame::
                                                      ReplacedElementWidthToClear
                                                                 *aReplacedWidth)
{
  
  
  
  
  NS_ASSERTION(aFloatAvailableSpace.x >= 0, "bad avail space rect x");
  NS_ASSERTION(aFloatAvailableSpace.width == 0 ||
               aFloatAvailableSpace.XMost() <= mContentArea.width,
               "bad avail space rect width");

  nscoord leftOffset, rightOffset;
  if (aFloatAvailableSpace.width == mContentArea.width) {
    
    leftOffset = 0;
    rightOffset = 0;
  } else {
    
    
    
    
    
    nsCSSOffsetState os(aFrame, mReflowState.rendContext, mContentArea.width);
    NS_ASSERTION(!aReplacedWidth ||
                 aFrame->GetType() == nsGkAtoms::tableOuterFrame ||
                 (aReplacedWidth->marginLeft  == os.mComputedMargin.left &&
                  aReplacedWidth->marginRight == os.mComputedMargin.right),
                 "unexpected aReplacedWidth");

    nscoord leftFloatXOffset = aFloatAvailableSpace.x;
    leftOffset = NS_MAX(leftFloatXOffset, os.mComputedMargin.left) -
                 (aReplacedWidth ? aReplacedWidth->marginLeft
                                 : os.mComputedMargin.left);
    leftOffset = NS_MAX(leftOffset, 0); 
    nscoord rightFloatXOffset =
      mContentArea.width - aFloatAvailableSpace.XMost();
    rightOffset = NS_MAX(rightFloatXOffset, os.mComputedMargin.right) -
                  (aReplacedWidth ? aReplacedWidth->marginRight
                                  : os.mComputedMargin.right);
    rightOffset = NS_MAX(rightOffset, 0); 
  }
  aLeftResult = leftOffset;
  aRightResult = rightOffset;
}




void
nsBlockReflowState::ComputeBlockAvailSpace(nsIFrame* aFrame,
                                           const nsStyleDisplay* aDisplay,
                                           const nsFlowAreaRect& aFloatAvailableSpace,
                                           PRBool aBlockAvoidsFloats,
                                           nsRect& aResult)
{
#ifdef REALLY_NOISY_REFLOW
  printf("CBAS frame=%p has floats %d\n",
         aFrame, aFloatAvailableSpace.mHasFloats);
#endif
  aResult.y = mY;
  aResult.height = GetFlag(BRS_UNCONSTRAINEDHEIGHT)
    ? NS_UNCONSTRAINEDSIZE
    : NS_MAX(0, mReflowState.availableHeight - mY);
  
  
  

  const nsMargin& borderPadding = BorderPadding();

  
  
  
  
  
  
  
  
  
  
  
  
  NS_ASSERTION(nsBlockFrame::BlockCanIntersectFloats(aFrame) == 
                 !aBlockAvoidsFloats,
               "unexpected replaced width");
  if (!aBlockAvoidsFloats) {
    if (aFloatAvailableSpace.mHasFloats) {
      
      
      const nsStyleBorder* borderStyle = aFrame->GetStyleBorder();
      switch (borderStyle->mFloatEdge) {
        default:
        case NS_STYLE_FLOAT_EDGE_CONTENT:  
          
          
          aResult.x = borderPadding.left;
          aResult.width = mContentArea.width;
          break;
        case NS_STYLE_FLOAT_EDGE_MARGIN:
          {
            
            
            aResult.x = aFloatAvailableSpace.mRect.x + borderPadding.left;
            aResult.width = aFloatAvailableSpace.mRect.width;
          }
          break;
      }
    }
    else {
      
      
      
      aResult.x = borderPadding.left;
      aResult.width = mContentArea.width;
    }
  }
  else {
    nsBlockFrame::ReplacedElementWidthToClear replacedWidthStruct;
    nsBlockFrame::ReplacedElementWidthToClear *replacedWidth = nsnull;
    if (aFrame->GetType() == nsGkAtoms::tableOuterFrame) {
      replacedWidth = &replacedWidthStruct;
      replacedWidthStruct =
        nsBlockFrame::WidthToClearPastFloats(*this, aFloatAvailableSpace.mRect,
                                             aFrame);
    }

    nscoord leftOffset, rightOffset;
    ComputeReplacedBlockOffsetsForFloats(aFrame, aFloatAvailableSpace.mRect,
                                         leftOffset, rightOffset,
                                         replacedWidth);
    aResult.x = borderPadding.left + leftOffset;
    aResult.width = mContentArea.width - leftOffset - rightOffset;
  }

#ifdef REALLY_NOISY_REFLOW
  printf("  CBAS: result %d %d %d %d\n", aResult.x, aResult.y, aResult.width, aResult.height);
#endif
}

nsFlowAreaRect
nsBlockReflowState::GetFloatAvailableSpaceWithState(
                      nscoord aY, PRBool aRelaxHeightConstraint,
                      nsFloatManager::SavedState *aState) const
{
#ifdef DEBUG
  
  nscoord wx, wy;
  mFloatManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mFloatManagerX) && (wy == mFloatManagerY),
               "bad coord system");
#endif

  nsFlowAreaRect result =
    mFloatManager->GetFlowArea(aY - BorderPadding().top, 
                               nsFloatManager::BAND_FROM_POINT,
                               aRelaxHeightConstraint ? nscoord_MAX
                                                      : mContentArea.height,
                               mContentArea.width, aState);
  
  if (result.mRect.width < 0)
    result.mRect.width = 0;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("GetAvailableSpace: band=%d,%d,%d,%d hasfloats=%d\n",
           result.mRect.x, result.mRect.y, result.mRect.width,
           result.mRect.height, result.mHasFloats);
  }
#endif
  return result;
}

nsFlowAreaRect
nsBlockReflowState::GetFloatAvailableSpaceForHeight(
                      nscoord aY, nscoord aHeight,
                      nsFloatManager::SavedState *aState) const
{
#ifdef DEBUG
  
  nscoord wx, wy;
  mFloatManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mFloatManagerX) && (wy == mFloatManagerY),
               "bad coord system");
#endif

  nsFlowAreaRect result =
    mFloatManager->GetFlowArea(aY - BorderPadding().top, 
                               nsFloatManager::WIDTH_WITHIN_HEIGHT,
                               aHeight, mContentArea.width, aState);
  
  if (result.mRect.width < 0)
    result.mRect.width = 0;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("GetAvailableSpaceForHeight: space=%d,%d,%d,%d hasfloats=%d\n",
           result.mRect.x, result.mRect.y, result.mRect.width,
           result.mRect.height, result.mHasFloats);
  }
#endif
  return result;
}














void
nsBlockReflowState::ReconstructMarginAbove(nsLineList::iterator aLine)
{
  mPrevBottomMargin.Zero();
  nsBlockFrame *block = mBlock;

  nsLineList::iterator firstLine = block->begin_lines();
  for (;;) {
    --aLine;
    if (aLine->IsBlock()) {
      mPrevBottomMargin = aLine->GetCarriedOutBottomMargin();
      break;
    }
    if (!aLine->IsEmpty()) {
      break;
    }
    if (aLine == firstLine) {
      
      
      if (!GetFlag(BRS_ISTOPMARGINROOT)) {
        mPrevBottomMargin.Zero();
      }
      break;
    }
  }
}

void
nsBlockReflowState::SetupFloatContinuationList()
{
  if (!GetFlag(BRS_PROPTABLE_FLOATCLIST)) {
    mBlock->SetProperty(nsGkAtoms::floatContinuationProperty,
                        &mFloatContinuations, nsnull);
    SetFlag(BRS_PROPTABLE_FLOATCLIST, PR_TRUE);
  }
}









void
nsBlockReflowState::RecoverFloats(nsLineList::iterator aLine,
                                  nscoord aDeltaY)
{
  if (aLine->HasFloats()) {
    
    
    nsFloatCache* fc = aLine->GetFirstFloat();
    while (fc) {
      nsIFrame* floatFrame = fc->mFloat;
      if (aDeltaY != 0) {
        nsPoint p = floatFrame->GetPosition();
        floatFrame->SetPosition(nsPoint(p.x, p.y + aDeltaY));
        nsContainerFrame::PositionFrameView(floatFrame);
        nsContainerFrame::PositionChildViews(floatFrame);
      }
#ifdef DEBUG
      if (nsBlockFrame::gNoisyReflow || nsBlockFrame::gNoisyFloatManager) {
        nscoord tx, ty;
        mFloatManager->GetTranslation(tx, ty);
        nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
        printf("RecoverFloats: txy=%d,%d (%d,%d) ",
               tx, ty, mFloatManagerX, mFloatManagerY);
        nsFrame::ListTag(stdout, floatFrame);
        nsRect region = nsFloatManager::GetRegionFor(floatFrame);
        printf(" aDeltaY=%d region={%d,%d,%d,%d}\n",
               aDeltaY, region.x, region.y, region.width, region.height);
      }
#endif
      mFloatManager->AddFloat(floatFrame,
                              nsFloatManager::GetRegionFor(floatFrame));
      fc = fc->Next();
    }
  } else if (aLine->IsBlock()) {
    nsBlockFrame::RecoverFloatsFor(aLine->mFirstChild, *mFloatManager);
  }
}













void
nsBlockReflowState::RecoverStateFrom(nsLineList::iterator aLine,
                                     nscoord aDeltaY)
{
  
  mCurrentLine = aLine;

  
  if (aLine->HasFloats() || aLine->IsBlock()) {
    
    
    
    const nsMargin& bp = BorderPadding();
    mFloatManager->Translate(-bp.left, -bp.top);

    RecoverFloats(aLine, aDeltaY);

#ifdef DEBUG
    if (nsBlockFrame::gNoisyReflow || nsBlockFrame::gNoisyFloatManager) {
      mFloatManager->List(stdout);
    }
#endif
    
    mFloatManager->Translate(bp.left, bp.top);
  }
}











PRBool
nsBlockReflowState::AddFloat(nsLineLayout*       aLineLayout,
                             nsIFrame*           aFloat,
                             nscoord             aAvailableWidth,
                             nsReflowStatus&     aReflowStatus)
{
  NS_PRECONDITION(!aLineLayout || mBlock->end_lines() != mCurrentLine, "null ptr");
  NS_PRECONDITION(aFloat->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                  "aFloat must be an out-of-flow frame");

  
  aFloat->SetParent(mBlock);

  aReflowStatus = NS_FRAME_COMPLETE;

  
  
  
  
  
  nscoord ox, oy;
  mFloatManager->GetTranslation(ox, oy);
  nscoord dx = ox - mFloatManagerX;
  nscoord dy = oy - mFloatManagerY;
  mFloatManager->Translate(-dx, -dy);

  PRBool placed;

  
  
  
  
  
  nsRect floatAvailableSpace = GetFloatAvailableSpace().mRect;
  if (!aLineLayout ||
      (mBelowCurrentLineFloats.IsEmpty() &&
       (aLineLayout->LineIsEmpty() ||
        mBlock->ComputeFloatWidth(*this, floatAvailableSpace, aFloat)
        <= aAvailableWidth))) {
    
    
    
    PRBool forceFit = !aLineLayout ||
                      (IsAdjacentWithTop() && !aLineLayout->LineIsBreakable());
    placed = FlowAndPlaceFloat(aFloat, aReflowStatus, forceFit);
    NS_ASSERTION(placed || !forceFit,
                 "If we asked for force-fit, it should have been placed");
    if (forceFit || (placed && !NS_FRAME_IS_TRUNCATED(aReflowStatus))) {
      
      nsFlowAreaRect floatAvailSpace =
        GetFloatAvailableSpace(mY, forceFit);
      nsRect availSpace(nsPoint(floatAvailSpace.mRect.x + BorderPadding().left,
                                mY),
                        floatAvailSpace.mRect.Size());
      if (aLineLayout) {
        aLineLayout->UpdateBand(availSpace, aFloat);
        
        mCurrentLineFloats.Append(mFloatCacheFreeList.Alloc(aFloat));
      }
      
      
      aReflowStatus &= ~NS_FRAME_TRUNCATED;
    }
    else {
      if (IsAdjacentWithTop()) {
        
        
        NS_ASSERTION(aLineLayout->LineIsBreakable(),
                     "We can't get here unless forceFit is false");
        aReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
      } else {
        
        
        aReflowStatus |= NS_FRAME_TRUNCATED;
      }
    }
  }
  else {
    
    
    placed = PR_TRUE;
    
    
    mBelowCurrentLineFloats.Append(mFloatCacheFreeList.Alloc(aFloat));
  }

  
  mFloatManager->Translate(dx, dy);

  return placed;
}

PRBool
nsBlockReflowState::CanPlaceFloat(const nsSize& aFloatSize, PRUint8 aFloats,
                                  const nsFlowAreaRect& aFloatAvailableSpace,
                                  PRBool aForceFit)
{
  
  
  PRBool result = PR_TRUE;
  if (aFloatAvailableSpace.mHasFloats) {
    
    if (aFloatAvailableSpace.mRect.width < aFloatSize.width) {
      
      
      result = PR_FALSE;
    }
  }

  if (!result)
    return result;

  
  
  
  if (NSCoordGreaterThan(aFloatSize.height,
                         aFloatAvailableSpace.mRect.height)) {
    
    
    
    
    
    
    
    
    nscoord xa;
    if (NS_STYLE_FLOAT_LEFT == aFloats) {
      xa = aFloatAvailableSpace.mRect.x;
    }
    else {
      xa = aFloatAvailableSpace.mRect.XMost() - aFloatSize.width;

      
      
      
      if (xa < aFloatAvailableSpace.mRect.x) {
        xa = aFloatAvailableSpace.mRect.x;
      }
    }
    nscoord xb = xa + aFloatSize.width;

    
    
    const nsMargin& borderPadding = BorderPadding();
    nscoord ya = mY - borderPadding.top;
    if (ya < 0) {
      
      
      
      
      
      ya = 0;
    }
    nscoord yb = ya + aFloatSize.height;

    nscoord saveY = mY;
    nsFlowAreaRect floatAvailableSpace(aFloatAvailableSpace);
    for (;;) {
      
      if (floatAvailableSpace.mRect.height <= 0) {
        
        result = PR_FALSE;
        break;
      }

      mY += floatAvailableSpace.mRect.height;
      floatAvailableSpace = GetFloatAvailableSpace(mY, aForceFit);

      if (floatAvailableSpace.mHasFloats) {
        if (xa < floatAvailableSpace.mRect.x ||
            xb > floatAvailableSpace.mRect.XMost()) {
          
          result = PR_FALSE;
          break;
        }
      }

      
      if (yb <= mY + floatAvailableSpace.mRect.height) {
        
        
        break;
      }
    }

    
    mY = saveY;
  }

  return result;
}

PRBool
nsBlockReflowState::FlowAndPlaceFloat(nsIFrame*       aFloat,
                                      nsReflowStatus& aReflowStatus,
                                      PRBool          aForceFit)
{
  aReflowStatus = NS_FRAME_COMPLETE;
  
  
  
  
  
  nscoord saveY = mY;

  
  const nsStyleDisplay* floatDisplay = aFloat->GetStyleDisplay();

  
  nsRect oldRegion = nsFloatManager::GetRegionFor(aFloat);

  
  
  mY = NS_MAX(mFloatManager->GetLowestFloatTop() + BorderPadding().top, mY);

  
  
  
  if (NS_STYLE_CLEAR_NONE != floatDisplay->mBreakType) {
    
    mY = ClearFloats(mY, floatDisplay->mBreakType);
  }
    
  nsFlowAreaRect floatAvailableSpace = GetFloatAvailableSpace(mY, aForceFit);

  NS_ASSERTION(aFloat->GetParent() == mBlock,
               "Float frame has wrong parent");

  
  nsMargin floatMargin; 
  mBlock->ReflowFloat(*this, floatAvailableSpace.mRect, aFloat,
                      floatMargin, aReflowStatus);
  if (aFloat->GetPrevInFlow())
    floatMargin.top = 0;
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus))
    floatMargin.bottom = 0;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect region = aFloat->GetRect();
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("flowed float: ");
    nsFrame::ListTag(stdout, aFloat);
    printf(" (%d,%d,%d,%d)\n",
	   region.x, region.y, region.width, region.height);
  }
#endif

  nsSize floatSize = aFloat->GetSize() +
                     nsSize(floatMargin.LeftRight(), floatMargin.TopBottom());

  
  
  
  NS_ASSERTION((NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) ||
	       (NS_STYLE_FLOAT_RIGHT == floatDisplay->mFloats),
	       "invalid float type");

  
  PRBool keepFloatOnSameLine = PR_FALSE;

  while (!CanPlaceFloat(floatSize, floatDisplay->mFloats, floatAvailableSpace,
                        aForceFit)) {
    if (floatAvailableSpace.mRect.height <= 0) {
      
      mY = saveY;
      return PR_FALSE;
    }

    
    if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
          eCompatibility_NavQuirks != mPresContext->CompatibilityMode() ) {

      mY += floatAvailableSpace.mRect.height;
      floatAvailableSpace = GetFloatAvailableSpace(mY, aForceFit);
    } else {
      
      

      
      nsFloatCache* fc = mCurrentLineFloats.Head();
      nsIFrame* prevFrame = nsnull;
      while (fc) {
        if (fc->mFloat == aFloat) {
          break;
        }
        prevFrame = fc->mFloat;
        fc = fc->Next();
      }
      
      if(prevFrame) {
        
        if (nsGkAtoms::tableOuterFrame == prevFrame->GetType()) {
          
          
          nsIContent* content = prevFrame->GetContent();
          if (content) {
            
            
            if (content->AttrValueIs(kNameSpaceID_None, nsGkAtoms::align,
                                     NS_LITERAL_STRING("left"), eIgnoreCase)) {
              keepFloatOnSameLine = PR_TRUE;
              
              
              
              break;
            }
          }
        }
      }

      
      mY += floatAvailableSpace.mRect.height;
      floatAvailableSpace = GetFloatAvailableSpace(mY, aForceFit);
      
      
      
      
      mBlock->ReflowFloat(*this, floatAvailableSpace.mRect, aFloat,
                          floatMargin, aReflowStatus);
      
      floatSize = aFloat->GetSize() +
                     nsSize(floatMargin.LeftRight(), floatMargin.TopBottom());
    }
  }
  

  
  

  
  
  
  
  nscoord floatX, floatY;
  if (NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) {
    floatX = floatAvailableSpace.mRect.x;
  }
  else {
    if (!keepFloatOnSameLine) {
      floatX = floatAvailableSpace.mRect.XMost() - floatSize.width;
    } 
    else {
      
      
      
      floatX = floatAvailableSpace.mRect.x;
    }
  }
  const nsMargin& borderPadding = BorderPadding();
  floatY = mY - borderPadding.top;
  if (floatY < 0) {
    
    
    
    
    
    floatY = 0;
  }

  
  
  
  
  nsPoint origin(borderPadding.left + floatMargin.left + floatX,
                 borderPadding.top + floatMargin.top + floatY);

  
  origin += aFloat->GetRelativeOffset(floatDisplay);

  
  
  
  aFloat->SetPosition(origin);
  nsContainerFrame::PositionFrameView(aFloat);
  nsContainerFrame::PositionChildViews(aFloat);

  
  nsRect combinedArea = aFloat->GetOverflowRect() + origin;

  
  mFloatCombinedArea.UnionRect(combinedArea, mFloatCombinedArea);

  
  
  nsRect region = nsFloatManager::CalculateRegionFor(aFloat, floatMargin);
  
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) &&
      (NS_UNCONSTRAINEDSIZE != mContentArea.height)) {
    region.height = NS_MAX(region.height, mContentArea.height - floatY);
  }
  nsresult rv =
  
  mFloatManager->AddFloat(aFloat,
                          region - nsPoint(borderPadding.left, borderPadding.top));
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "bad float placement");
  
  rv = nsFloatManager::StoreRegionFor(aFloat, region);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "float region storage failed");

  
  
  if (region != oldRegion) {
    
    
    
    
    nscoord top = NS_MIN(region.y, oldRegion.y) - borderPadding.top;
    nscoord bottom = NS_MAX(region.YMost(), oldRegion.YMost()) - borderPadding.left;
    mFloatManager->IncludeInDamage(top, bottom);
  }

#ifdef NOISY_FLOATMANAGER
  nscoord tx, ty;
  mFloatManager->GetTranslation(tx, ty);
  nsFrame::ListTag(stdout, mBlock);
  printf(": FlowAndPlaceFloat: AddFloat: txy=%d,%d (%d,%d) {%d,%d,%d,%d}\n",
         tx, ty, mFloatManagerX, mFloatManagerY,
         region.x, region.y, region.width, region.height);
#endif

  
  mY = saveY;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect r = aFloat->GetRect();
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("placed float: ");
    nsFrame::ListTag(stdout, aFloat);
    printf(" %d,%d,%d,%d\n", r.x, r.y, r.width, r.height);
  }
#endif

  return PR_TRUE;
}




PRBool
nsBlockReflowState::PlaceBelowCurrentLineFloats(nsFloatCacheFreeList& aList, PRBool aForceFit)
{
  nsFloatCache* fc = aList.Head();
  while (fc) {
    {
#ifdef DEBUG
      if (nsBlockFrame::gNoisyReflow) {
        nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
        printf("placing bcl float: ");
        nsFrame::ListTag(stdout, fc->mFloat);
        printf("\n");
      }
#endif
      
      nsReflowStatus reflowStatus;
      PRBool placed = FlowAndPlaceFloat(fc->mFloat, reflowStatus, aForceFit);
      NS_ASSERTION(placed || !aForceFit,
                   "If we're in force-fit mode, we should have placed the float");

      if (!placed || (NS_FRAME_IS_TRUNCATED(reflowStatus) && !aForceFit)) {
        
        return PR_FALSE;
      }
      else if (!NS_FRAME_IS_FULLY_COMPLETE(reflowStatus)) {
        
        nsresult rv = mBlock->SplitFloat(*this, fc->mFloat, reflowStatus);
        if (NS_FAILED(rv))
          return PR_FALSE;
      } else {
        
        
        NS_WARN_IF_FALSE(!NS_FRAME_IS_TRUNCATED(reflowStatus),
                         "This situation currently leads to data not printing");
        
      }
    }
    fc = fc->Next();
  }
  return PR_TRUE;
}

nscoord
nsBlockReflowState::ClearFloats(nscoord aY, PRUint8 aBreakType,
                                nsIFrame *aReplacedBlock)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: in: aY=%d(%d)\n",
           aY, aY - BorderPadding().top);
  }
#endif

#ifdef NOISY_FLOAT_CLEARING
  printf("nsBlockReflowState::ClearFloats: aY=%d breakType=%d\n",
         aY, aBreakType);
  mFloatManager->List(stdout);
#endif
  
  const nsMargin& bp = BorderPadding();
  nscoord newY = aY;

  if (aBreakType != NS_STYLE_CLEAR_NONE) {
    newY = bp.top + mFloatManager->ClearFloats(newY - bp.top, aBreakType);
  }

  if (aReplacedBlock) {
    for (;;) {
      nsFlowAreaRect floatAvailableSpace = 
        GetFloatAvailableSpace(newY, PR_FALSE);
      nsBlockFrame::ReplacedElementWidthToClear replacedWidth =
        nsBlockFrame::WidthToClearPastFloats(*this, floatAvailableSpace.mRect,
                                             aReplacedBlock);
      if (!floatAvailableSpace.mHasFloats ||
          NS_MAX(floatAvailableSpace.mRect.x, replacedWidth.marginLeft) +
            replacedWidth.borderBoxWidth +
            NS_MAX(mContentArea.width -
                     NS_MIN(mContentArea.width,
                            floatAvailableSpace.mRect.XMost()),
                   replacedWidth.marginRight) <=
          mContentArea.width) {
        break;
      }
      
      if (floatAvailableSpace.mRect.height > 0) {
        
        newY += floatAvailableSpace.mRect.height;
      } else {
        if (mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE) {
          
          
          break;
        }
        NS_NOTREACHED("avail space rect with zero height!");
        newY += 1;
      }
    }
  }

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: out: y=%d(%d)\n", newY, newY - bp.top);
  }
#endif

  return newY;
}

