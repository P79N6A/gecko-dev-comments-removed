











































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
    mOverflowTracker(aPresContext, aFrame, PR_FALSE),
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
  }

  mReflowStatus = NS_FRAME_COMPLETE;

  mPresContext = aPresContext;
  mNextInFlow = static_cast<nsBlockFrame*>(mBlock->GetNextInFlow());

  NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aReflowState.ComputedWidth(),
               "no unconstrained widths should be present anymore");
  mContentArea.width = aReflowState.ComputedWidth();

  
  
  
  
  
  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight) {
    
    
    
    mBottomEdge = aReflowState.availableHeight - borderPadding.bottom;
    mContentArea.height = PR_MAX(0, mBottomEdge - borderPadding.top);
  }
  else {
    
    
    SetFlag(BRS_UNCONSTRAINEDHEIGHT, PR_TRUE);
    mContentArea.height = mBottomEdge = NS_UNCONSTRAINEDSIZE;
  }

  mY = borderPadding.top;

  mPrevChild = nsnull;
  mCurrentLine = aFrame->end_lines();

  mMinLineHeight = nsHTMLReflowState::CalcLineHeight(aReflowState.frame);

  
  GetAvailableSpace();
  
  
  mOutsideBulletX =
    mReflowState.mStyleVisibility->mDirection == NS_STYLE_DIRECTION_LTR ?
      mAvailSpaceRect.x :
      PR_MIN(mReflowState.ComputedWidth(), mAvailSpaceRect.XMost()) +
        mReflowState.mComputedBorderPadding.LeftRight();
}

void
nsBlockReflowState::SetupOverflowPlaceholdersProperty()
{
  if (mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE ||
      !mOverflowPlaceholders.IsEmpty()) {
    mBlock->SetProperty(nsGkAtoms::overflowPlaceholdersProperty,
                        &mOverflowPlaceholders, nsnull);
    mBlock->AddStateBits(NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS);
  }
}

nsBlockReflowState::~nsBlockReflowState()
{
  NS_ASSERTION(mOverflowPlaceholders.IsEmpty(),
               "Leaking overflow placeholder frames");

  
  
  if (mFloatManager) {
    const nsMargin& borderPadding = BorderPadding();
    mFloatManager->Translate(-borderPadding.left, -borderPadding.top);
  }

  if (mBlock->GetStateBits() & NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS) {
    mBlock->UnsetProperty(nsGkAtoms::overflowPlaceholdersProperty);
    mBlock->RemoveStateBits(NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS);
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
                                                         nscoord& aLeftResult,
                                                         nscoord& aRightResult,
                                                         nsBlockFrame::
                                                      ReplacedElementWidthToClear
                                                                 *aReplacedWidth)
{
  
  
  
  
  NS_ASSERTION(mAvailSpaceRect.x >= 0, "bad avail space rect x");
  NS_ASSERTION(mAvailSpaceRect.width == 0 ||
               mAvailSpaceRect.XMost() <= mContentArea.width,
               "bad avail space rect width");

  nscoord leftOffset, rightOffset;
  if (mAvailSpaceRect.width == mContentArea.width) {
    
    leftOffset = 0;
    rightOffset = 0;
  } else {
    
    
    
    
    
    nsCSSOffsetState os(aFrame, mReflowState.rendContext, mContentArea.width);
    NS_ASSERTION(!aReplacedWidth ||
                 aFrame->GetType() == nsGkAtoms::tableOuterFrame ||
                 (aReplacedWidth->marginLeft  == os.mComputedMargin.left &&
                  aReplacedWidth->marginRight == os.mComputedMargin.right),
                 "unexpected aReplacedWidth");

    nscoord leftFloatXOffset = mAvailSpaceRect.x;
    leftOffset = PR_MAX(leftFloatXOffset, os.mComputedMargin.left) -
                 (aReplacedWidth ? aReplacedWidth->marginLeft
                                 : os.mComputedMargin.left);
    leftOffset = PR_MAX(leftOffset, 0); 
    nscoord rightFloatXOffset = mContentArea.width - mAvailSpaceRect.XMost();
    rightOffset = PR_MAX(rightFloatXOffset, os.mComputedMargin.right) -
                  (aReplacedWidth ? aReplacedWidth->marginRight
                                  : os.mComputedMargin.right);
    rightOffset = PR_MAX(rightOffset, 0); 
  }
  aLeftResult = leftOffset;
  aRightResult = rightOffset;
}




void
nsBlockReflowState::ComputeBlockAvailSpace(nsIFrame* aFrame,
                                           const nsStyleDisplay* aDisplay,
                                           PRBool aBlockAvoidsFloats,
                                           nsRect& aResult)
{
#ifdef REALLY_NOISY_REFLOW
  printf("CBAS frame=%p has floats %d\n", aFrame, mBandHasFloats);
#endif
  aResult.y = mY;
  aResult.height = GetFlag(BRS_UNCONSTRAINEDHEIGHT)
    ? NS_UNCONSTRAINEDSIZE
    : PR_MAX(0, mReflowState.availableHeight - mY);
  
  
  

  const nsMargin& borderPadding = BorderPadding();

  
  
  
  
  
  
  
  
  
  
  
  
  NS_ASSERTION(nsBlockFrame::BlockCanIntersectFloats(aFrame) == 
                 !aBlockAvoidsFloats,
               "unexpected replaced width");
  if (!aBlockAvoidsFloats) {
    if (mBandHasFloats) {
      
      
      const nsStyleBorder* borderStyle = aFrame->GetStyleBorder();
      switch (borderStyle->mFloatEdge) {
        default:
        case NS_STYLE_FLOAT_EDGE_CONTENT:  
          
          
          aResult.x = borderPadding.left;
          aResult.width = mContentArea.width;
          break;
        case NS_STYLE_FLOAT_EDGE_MARGIN:
          {
            
            
            aResult.x = mAvailSpaceRect.x + borderPadding.left;
            aResult.width = mAvailSpaceRect.width;
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
      replacedWidthStruct = nsBlockFrame::WidthToClearPastFloats(*this, aFrame);
    }

    nscoord leftOffset, rightOffset;
    ComputeReplacedBlockOffsetsForFloats(aFrame, leftOffset, rightOffset,
                                         replacedWidth);
    aResult.x = borderPadding.left + leftOffset;
    aResult.width = mContentArea.width - leftOffset - rightOffset;
  }

#ifdef REALLY_NOISY_REFLOW
  printf("  CBAS: result %d %d %d %d\n", aResult.x, aResult.y, aResult.width, aResult.height);
#endif
}

PRBool
nsBlockReflowState::GetFloatAvailableSpace(nscoord aY,
                                           PRBool aRelaxHeightConstraint,
                                           nsRect& aResult) const
{
#ifdef DEBUG
  
  nscoord wx, wy;
  mFloatManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mFloatManagerX) && (wy == mFloatManagerY),
               "bad coord system");
#endif

  PRBool hasFloats;
  aResult = 
    mFloatManager->GetBand(aY - BorderPadding().top, 
                           aRelaxHeightConstraint ? nscoord_MAX
                                                  : mContentArea.height,
                           mContentArea.width,
                           &hasFloats);
  
  if (aResult.width < 0)
    aResult.width = 0;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("GetAvailableSpace: band=%d,%d,%d,%d hasfloats=%d\n",
           aResult.x, aResult.y, aResult.width, aResult.height, hasFloats);
  }
#endif
  return hasFloats;
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
nsBlockReflowState::RecoverFloats(nsLineList::iterator aLine,
                                  nscoord aDeltaY)
{
  if (aLine->HasFloats()) {
    
    
    nsFloatCache* fc = aLine->GetFirstFloat();
    while (fc) {
      nsIFrame* floatFrame = fc->mPlaceholder->GetOutOfFlowFrame();
      if (aDeltaY != 0) {
        fc->mRegion.y += aDeltaY;
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
        printf(" aDeltaY=%d region={%d,%d,%d,%d}\n",
               aDeltaY, fc->mRegion.x, fc->mRegion.y,
               fc->mRegion.width, fc->mRegion.height);
      }
#endif
      mFloatManager->AddFloat(floatFrame, fc->mRegion);
      fc = fc->Next();
    }
  } else if (aLine->IsBlock()) {
    nsBlockFrame *kid = nsLayoutUtils::GetAsBlock(aLine->mFirstChild);
    
    
    
    if (kid && !nsBlockFrame::BlockNeedsFloatManager(kid)) {
      nscoord tx = kid->mRect.x, ty = kid->mRect.y;

      
      
      
      if (NS_STYLE_POSITION_RELATIVE == kid->GetStyleDisplay()->mPosition) {
        nsPoint *offsets = static_cast<nsPoint*>
                                      (mPresContext->PropertyTable()->GetProperty(kid,
                                       nsGkAtoms::computedOffsetProperty));

        if (offsets) {
          tx -= offsets->x;
          ty -= offsets->y;
        }
      }
 
      mFloatManager->Translate(tx, ty);
      for (nsBlockFrame::line_iterator line = kid->begin_lines(),
                                   line_end = kid->end_lines();
           line != line_end;
           ++line)
        
        
        
        RecoverFloats(line, 0);
      mFloatManager->Translate(-tx, -ty);
    }
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
nsBlockReflowState::IsImpactedByFloat() const
{
#ifdef REALLY_NOISY_REFLOW
  printf("nsBlockReflowState::IsImpactedByFloat %p returned %d\n", 
         this, mBandHasFloats);
#endif
  return mBandHasFloats;
}


PRBool
nsBlockReflowState::InitFloat(nsLineLayout&       aLineLayout,
                              nsPlaceholderFrame* aPlaceholder,
                              nscoord             aAvailableWidth,
                              nsReflowStatus&     aReflowStatus)
{
  
  nsIFrame* floatFrame = aPlaceholder->GetOutOfFlowFrame();
  floatFrame->SetParent(mBlock);

  
  
  return AddFloat(aLineLayout, aPlaceholder, PR_TRUE,
                  aAvailableWidth, aReflowStatus);
}











PRBool
nsBlockReflowState::AddFloat(nsLineLayout&       aLineLayout,
                             nsPlaceholderFrame* aPlaceholder,
                             PRBool              aInitialReflow,
                             nscoord             aAvailableWidth,
                             nsReflowStatus&     aReflowStatus)
{
  NS_PRECONDITION(mBlock->end_lines() != mCurrentLine, "null ptr");

  aReflowStatus = NS_FRAME_COMPLETE;
  
  nsFloatCache* fc = mFloatCacheFreeList.Alloc();
  fc->mPlaceholder = aPlaceholder;

  PRBool placed;

  
  
  
  
  
  if (mBelowCurrentLineFloats.IsEmpty() &&
      (aLineLayout.LineIsEmpty() ||
       mBlock->ComputeFloatWidth(*this, aPlaceholder) <= aAvailableWidth)) {
    
    
    
    
    
    nscoord ox, oy;
    mFloatManager->GetTranslation(ox, oy);
    nscoord dx = ox - mFloatManagerX;
    nscoord dy = oy - mFloatManagerY;
    mFloatManager->Translate(-dx, -dy);

    
    PRBool isLeftFloat;
    
    
    PRBool forceFit = IsAdjacentWithTop() && !aLineLayout.LineIsBreakable();
    placed = FlowAndPlaceFloat(fc, &isLeftFloat, aReflowStatus, forceFit);
    NS_ASSERTION(placed || !forceFit,
                 "If we asked for force-fit, it should have been placed");
    if (forceFit || (placed && !NS_FRAME_IS_TRUNCATED(aReflowStatus))) {
      
      nsRect floatAvailSpace;
      GetFloatAvailableSpace(mY, forceFit, floatAvailSpace);
      nsRect availSpace(nsPoint(floatAvailSpace.x + BorderPadding().left, mY),
                        floatAvailSpace.Size());
      aLineLayout.UpdateBand(availSpace, isLeftFloat,
                             aPlaceholder->GetOutOfFlowFrame());
      
      
      mCurrentLineFloats.Append(fc);
      
      
      aReflowStatus &= ~NS_FRAME_TRUNCATED;
    }
    else {
      if (IsAdjacentWithTop()) {
        
        
        NS_ASSERTION(aLineLayout.LineIsBreakable(),
                     "We can't get here unless forceFit is false");
        aReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
      } else {
        
        
        aReflowStatus |= NS_FRAME_TRUNCATED;
      }
      delete fc;
    }

    
    mFloatManager->Translate(dx, dy);
  }
  else {
    
    
    placed = PR_TRUE;
    
    
    mBelowCurrentLineFloats.Append(fc);
    if (aPlaceholder->GetNextInFlow()) {
      
      
      
      
      
      
      
      if (aPlaceholder->GetSplittableType() != NS_FRAME_NOT_SPLITTABLE) {
        aReflowStatus = NS_FRAME_NOT_COMPLETE;
      }
    }
  }
  return placed;
}

PRBool
nsBlockReflowState::CanPlaceFloat(const nsSize& aFloatSize,
                                  PRUint8 aFloats, PRBool aForceFit)
{
  
  
  PRBool result = PR_TRUE;
  if (mBandHasFloats) {
    
    if (mAvailSpaceRect.width < aFloatSize.width) {
      
      
      result = PR_FALSE;
    }
  }

  if (!result)
    return result;

  
  
  
  if (NSCoordGreaterThan(aFloatSize.height, mAvailSpaceRect.height)) {
    
    
    
    
    
    
    
    
    nscoord xa;
    if (NS_STYLE_FLOAT_LEFT == aFloats) {
      xa = mAvailSpaceRect.x;
    }
    else {
      xa = mAvailSpaceRect.XMost() - aFloatSize.width;

      
      
      
      if (xa < mAvailSpaceRect.x) {
        xa = mAvailSpaceRect.x;
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
    for (;;) {
      
      if (mAvailSpaceRect.height <= 0) {
        
        result = PR_FALSE;
        break;
      }

      mY += mAvailSpaceRect.height;
      GetAvailableSpace(mY, aForceFit);

      if (mBandHasFloats) {
        if ((xa < mAvailSpaceRect.x) || (xb > mAvailSpaceRect.XMost())) {
          
          result = PR_FALSE;
          break;
        }
      }

      
      if (yb <= mY + mAvailSpaceRect.height) {
        
        
        break;
      }
    }

    
    
    mY = saveY;
    GetAvailableSpace(mY, aForceFit);
  }

  return result;
}

PRBool
nsBlockReflowState::FlowAndPlaceFloat(nsFloatCache*   aFloatCache,
                                      PRBool*         aIsLeftFloat,
                                      nsReflowStatus& aReflowStatus,
                                      PRBool          aForceFit)
{
  aReflowStatus = NS_FRAME_COMPLETE;
  
  
  
  
  
  nscoord saveY = mY;

  nsPlaceholderFrame* placeholder = aFloatCache->mPlaceholder;
  nsIFrame*           floatFrame = placeholder->GetOutOfFlowFrame();

  
  const nsStyleDisplay* floatDisplay = floatFrame->GetStyleDisplay();

  
  nsRect oldRegion = aFloatCache->mRegion;

  
  
  mY = NS_MAX(mFloatManager->GetLowestFloatTop() + BorderPadding().top, mY);

  
  
  
  if (NS_STYLE_CLEAR_NONE != floatDisplay->mBreakType) {
    
    mY = ClearFloats(mY, floatDisplay->mBreakType);
  }
    
  GetAvailableSpace(mY, aForceFit);

  NS_ASSERTION(floatFrame->GetParent() == mBlock,
               "Float frame has wrong parent");

  
  nsMargin floatMargin;
  mBlock->ReflowFloat(*this, placeholder, floatMargin, aReflowStatus);

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect region = floatFrame->GetRect();
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("flowed float: ");
    nsFrame::ListTag(stdout, floatFrame);
    printf(" (%d,%d,%d,%d)\n",
	   region.x, region.y, region.width, region.height);
  }
#endif

  nsSize floatSize = floatFrame->GetSize() +
                     nsSize(floatMargin.LeftRight(), floatMargin.TopBottom());

  
  
  
  NS_ASSERTION((NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) ||
	       (NS_STYLE_FLOAT_RIGHT == floatDisplay->mFloats),
	       "invalid float type");

  
  PRBool keepFloatOnSameLine = PR_FALSE;

  while (!CanPlaceFloat(floatSize, floatDisplay->mFloats, aForceFit)) {
    if (mAvailSpaceRect.height <= 0) {
      
      mY = saveY;
      return PR_FALSE;
    }

    
    if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
          eCompatibility_NavQuirks != mPresContext->CompatibilityMode() ) {

      mY += mAvailSpaceRect.height;
      GetAvailableSpace(mY, aForceFit);
    } else {
      
      

      
      nsFloatCache* fc = mCurrentLineFloats.Head();
      nsIFrame* prevFrame = nsnull;
      while (fc) {
        if (fc->mPlaceholder->GetOutOfFlowFrame() == floatFrame) {
          break;
        }
        prevFrame = fc->mPlaceholder->GetOutOfFlowFrame();
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

      
      mY += mAvailSpaceRect.height;
      GetAvailableSpace(mY, aForceFit);
      
      
      
      
      mBlock->ReflowFloat(*this, placeholder, floatMargin, aReflowStatus);
      
      floatSize = floatFrame->GetSize() +
                     nsSize(floatMargin.LeftRight(), floatMargin.TopBottom());
    }
  }
  

  
  

  
  
  
  
  PRBool isLeftFloat;
  nscoord floatX, floatY;
  if (NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) {
    isLeftFloat = PR_TRUE;
    floatX = mAvailSpaceRect.x;
  }
  else {
    isLeftFloat = PR_FALSE;
    if (!keepFloatOnSameLine) {
      floatX = mAvailSpaceRect.XMost() - floatSize.width;
    } 
    else {
      
      
      
      floatX = mAvailSpaceRect.x;
    }
  }
  *aIsLeftFloat = isLeftFloat;
  const nsMargin& borderPadding = BorderPadding();
  floatY = mY - borderPadding.top;
  if (floatY < 0) {
    
    
    
    
    
    floatY = 0;
  }

  
  
  if (NS_FRAME_IS_NOT_COMPLETE(aReflowStatus) && 
      (NS_UNCONSTRAINEDSIZE != mContentArea.height)) {
    floatSize.height = PR_MAX(floatSize.height, mContentArea.height - floatY);
  }

  nsRect region(floatX, floatY, floatSize.width, floatSize.height);
  
  
  
  if (region.width < 0) {
    
    
    if (isLeftFloat) {
      region.x = region.XMost();
    }
    region.width = 0;
  }
  if (region.height < 0) {
    region.height = 0;
  }
#ifdef DEBUG
  nsresult rv =
#endif
  mFloatManager->AddFloat(floatFrame, region);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "bad float placement");

  
  
  
  
  
  
  
  aFloatCache->mRegion = region +
                         nsPoint(borderPadding.left, borderPadding.top);

  
  
  if (aFloatCache->mRegion != oldRegion) {
    
    
    
    
    nscoord top = NS_MIN(region.y, oldRegion.y);
    nscoord bottom = NS_MAX(region.YMost(), oldRegion.YMost());
    mFloatManager->IncludeInDamage(top, bottom);
  }

#ifdef NOISY_FLOATMANAGER
  nscoord tx, ty;
  mFloatManager->GetTranslation(tx, ty);
  nsFrame::ListTag(stdout, mBlock);
  printf(": FlowAndPlaceFloat: AddFloat: txy=%d,%d (%d,%d) {%d,%d,%d,%d}\n",
         tx, ty, mFloatManagerX, mFloatManagerY,
         aFloatCache->mRegion.x, aFloatCache->mRegion.y,
         aFloatCache->mRegion.width, aFloatCache->mRegion.height);
#endif

  
  
  
  
  nsPoint origin(borderPadding.left + floatMargin.left + floatX,
                 borderPadding.top + floatMargin.top + floatY);

  
  origin += floatFrame->GetRelativeOffset(floatDisplay);

  
  
  
  floatFrame->SetPosition(origin);
  nsContainerFrame::PositionFrameView(floatFrame);
  nsContainerFrame::PositionChildViews(floatFrame);

  
  nsRect combinedArea = floatFrame->GetOverflowRect() + origin;

  
  mFloatCombinedArea.UnionRect(combinedArea, mFloatCombinedArea);

  
  mY = saveY;

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect r = floatFrame->GetRect();
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("placed float: ");
    nsFrame::ListTag(stdout, floatFrame);
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
        nsFrame::ListTag(stdout, fc->mPlaceholder->GetOutOfFlowFrame());
        printf("\n");
      }
#endif
      
      PRBool isLeftFloat;
      nsReflowStatus reflowStatus;
      PRBool placed = FlowAndPlaceFloat(fc, &isLeftFloat, reflowStatus, aForceFit);
      NS_ASSERTION(placed || !aForceFit,
                   "If we're in force-fit mode, we should have placed the float");

      if (!placed || (NS_FRAME_IS_TRUNCATED(reflowStatus) && !aForceFit)) {
        
        return PR_FALSE;
      }
      else if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus)) {
        
        nsresult rv = mBlock->SplitPlaceholder(*this, fc->mPlaceholder);
        if (NS_FAILED(rv)) 
          return PR_FALSE;
      } else {
        
        
        NS_WARN_IF_FALSE(!NS_FRAME_IS_TRUNCATED(reflowStatus),
                         "This situation currently leads to data not printing");

        
        nsIFrame* nextPlaceholder = fc->mPlaceholder->GetNextInFlow();
        if (nextPlaceholder) {
          nsHTMLContainerFrame* parent =
            static_cast<nsHTMLContainerFrame*>(nextPlaceholder->GetParent());
          parent->DeleteNextInFlowChild(mPresContext, nextPlaceholder, PR_TRUE);
        }
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
      GetAvailableSpace(newY, PR_FALSE);
      nsBlockFrame::ReplacedElementWidthToClear replacedWidth =
        nsBlockFrame::WidthToClearPastFloats(*this, aReplacedBlock);
      if (!mBandHasFloats ||
          PR_MAX(mAvailSpaceRect.x, replacedWidth.marginLeft) +
            replacedWidth.borderBoxWidth +
            PR_MAX(mContentArea.width -
                     PR_MIN(mContentArea.width, mAvailSpaceRect.XMost()),
                   replacedWidth.marginRight) <=
          mContentArea.width) {
        break;
      }
      
      if (mAvailSpaceRect.height > 0) {
        
        newY += mAvailSpaceRect.height;
      } else {
        if (mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE) {
          
          
          break;
        }
        NS_NOTREACHED("avail space rect with zero height!");
        newY += 1;
      }
    }
    
    
    
    
    GetAvailableSpace();
  }

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: out: y=%d(%d)\n", newY, newY - bp.top);
  }
#endif

  return newY;
}

