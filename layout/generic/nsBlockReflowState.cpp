







#include "nsBlockReflowContext.h"
#include "nsBlockReflowState.h"
#include "nsBlockFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsIFrame.h"
#include "nsFrameManager.h"
#include "mozilla/AutoRestore.h"
#include "FrameLayerBuilder.h"

#include "nsINameSpaceManager.h"

#include "mozilla/Util.h" 

#ifdef DEBUG
#include "nsBlockDebugFlags.h"
#endif

using namespace mozilla;
using namespace mozilla::layout;

nsBlockReflowState::nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                                       nsPresContext* aPresContext,
                                       nsBlockFrame* aFrame,
                                       const nsHTMLReflowMetrics& aMetrics,
                                       bool aTopMarginRoot,
                                       bool aBottomMarginRoot,
                                       bool aBlockNeedsFloatManager)
  : mBlock(aFrame),
    mPresContext(aPresContext),
    mReflowState(aReflowState),
    mPushedFloats(nsnull),
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
    SetFlag(BRS_ISTOPMARGINROOT, true);
  }
  if (aBottomMarginRoot || 0 != aReflowState.mComputedBorderPadding.bottom) {
    SetFlag(BRS_ISBOTTOMMARGINROOT, true);
  }
  if (GetFlag(BRS_ISTOPMARGINROOT)) {
    SetFlag(BRS_APPLYTOPMARGIN, true);
  }
  if (aBlockNeedsFloatManager) {
    SetFlag(BRS_FLOAT_MGR, true);
  }
  
  mFloatManager = aReflowState.mFloatManager;

  NS_ASSERTION(mFloatManager,
               "FloatManager should be set in nsBlockReflowState" );
  if (mFloatManager) {
    
    mFloatManager->GetTranslation(mFloatManagerX, mFloatManagerY);
    mFloatManager->PushState(&mFloatManagerStateBefore); 
  }

  mReflowStatus = NS_FRAME_COMPLETE;

  mPresContext = aPresContext;
  mNextInFlow = static_cast<nsBlockFrame*>(mBlock->GetNextInFlow());

  NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aReflowState.ComputedWidth(),
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  mContentArea.width = aReflowState.ComputedWidth();

  
  
  
  
  
  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.availableHeight) {
    
    
    
    mBottomEdge = aReflowState.availableHeight - borderPadding.bottom;
    mContentArea.height = NS_MAX(0, mBottomEdge - borderPadding.top);
  }
  else {
    
    
    SetFlag(BRS_UNCONSTRAINEDHEIGHT, true);
    mContentArea.height = mBottomEdge = NS_UNCONSTRAINEDSIZE;
  }
  mContentArea.x = borderPadding.left;
  mY = mContentArea.y = borderPadding.top;

  mPrevChild = nsnull;
  mCurrentLine = aFrame->end_lines();

  mMinLineHeight = aReflowState.CalcLineHeight();
}

void
nsBlockReflowState::ComputeReplacedBlockOffsetsForFloats(nsIFrame* aFrame,
                                                         const nsRect& aFloatAvailableSpace,
                                                         nscoord& aLeftResult,
                                                         nscoord& aRightResult)
{
  
  
  
  
  NS_ASSERTION(aFloatAvailableSpace.x >= mContentArea.x, "bad avail space rect x");
  NS_ASSERTION(aFloatAvailableSpace.width == 0 ||
               aFloatAvailableSpace.XMost() <= mContentArea.XMost(),
               "bad avail space rect width");

  nscoord leftOffset, rightOffset;
  if (aFloatAvailableSpace.width == mContentArea.width) {
    
    leftOffset = 0;
    rightOffset = 0;
  } else {
    nsMargin frameMargin;
    nsCSSOffsetState os(aFrame, mReflowState.rendContext, mContentArea.width);
    frameMargin = os.mComputedMargin;

    nscoord leftFloatXOffset = aFloatAvailableSpace.x - mContentArea.x;
    leftOffset = NS_MAX(leftFloatXOffset, frameMargin.left) -
                 frameMargin.left;
    leftOffset = NS_MAX(leftOffset, 0); 
    nscoord rightFloatXOffset =
      mContentArea.XMost() - aFloatAvailableSpace.XMost();
    rightOffset = NS_MAX(rightFloatXOffset, frameMargin.right) -
                  frameMargin.right;
    rightOffset = NS_MAX(rightOffset, 0); 
  }
  aLeftResult = leftOffset;
  aRightResult = rightOffset;
}




void
nsBlockReflowState::ComputeBlockAvailSpace(nsIFrame* aFrame,
                                           const nsStyleDisplay* aDisplay,
                                           const nsFlowAreaRect& aFloatAvailableSpace,
                                           bool aBlockAvoidsFloats,
                                           nsRect& aResult)
{
#ifdef REALLY_NOISY_REFLOW
  printf("CBAS frame=%p has floats %d\n",
         aFrame, aFloatAvailableSpace.mHasFloats);
#endif
  aResult.y = mY;
  aResult.height = GetFlag(BRS_UNCONSTRAINEDHEIGHT)
    ? NS_UNCONSTRAINEDSIZE
    : mReflowState.availableHeight - mY;
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  NS_ASSERTION(nsBlockFrame::BlockCanIntersectFloats(aFrame) == 
                 !aBlockAvoidsFloats,
               "unexpected replaced width");
  if (!aBlockAvoidsFloats) {
    if (aFloatAvailableSpace.mHasFloats) {
      
      
      const nsStyleBorder* borderStyle = aFrame->GetStyleBorder();
      switch (borderStyle->mFloatEdge) {
        default:
        case NS_STYLE_FLOAT_EDGE_CONTENT:  
          
          
          aResult.x = mContentArea.x;
          aResult.width = mContentArea.width;
          break;
        case NS_STYLE_FLOAT_EDGE_MARGIN:
          {
            
            
            aResult.x = aFloatAvailableSpace.mRect.x;
            aResult.width = aFloatAvailableSpace.mRect.width;
          }
          break;
      }
    }
    else {
      
      
      
      aResult.x = mContentArea.x;
      aResult.width = mContentArea.width;
    }
  }
  else {
    nscoord leftOffset, rightOffset;
    ComputeReplacedBlockOffsetsForFloats(aFrame, aFloatAvailableSpace.mRect,
                                         leftOffset, rightOffset);
    aResult.x = mContentArea.x + leftOffset;
    aResult.width = mContentArea.width - leftOffset - rightOffset;
  }

#ifdef REALLY_NOISY_REFLOW
  printf("  CBAS: result %d %d %d %d\n", aResult.x, aResult.y, aResult.width, aResult.height);
#endif
}

nsFlowAreaRect
nsBlockReflowState::GetFloatAvailableSpaceWithState(
                      nscoord aY,
                      nsFloatManager::SavedState *aState) const
{
#ifdef DEBUG
  
  nscoord wx, wy;
  mFloatManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mFloatManagerX) && (wy == mFloatManagerY),
               "bad coord system");
#endif

  nscoord height = (mContentArea.height == nscoord_MAX)
                     ? nscoord_MAX : NS_MAX(mContentArea.YMost() - aY, 0);
  nsFlowAreaRect result =
    mFloatManager->GetFlowArea(aY, nsFloatManager::BAND_FROM_POINT,
                               height, mContentArea, aState);
  
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
    mFloatManager->GetFlowArea(aY, nsFloatManager::WIDTH_WITHIN_HEIGHT,
                               aHeight, mContentArea, aState);
  
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
nsBlockReflowState::SetupPushedFloatList()
{
  NS_ABORT_IF_FALSE(!GetFlag(BRS_PROPTABLE_FLOATCLIST) == !mPushedFloats,
                    "flag mismatch");
  if (!GetFlag(BRS_PROPTABLE_FLOATCLIST)) {
    
    
    
    
    
    
    
    mPushedFloats = mBlock->EnsurePushedFloats();
    SetFlag(BRS_PROPTABLE_FLOATCLIST, true);
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
    RecoverFloats(aLine, aDeltaY);

#ifdef DEBUG
    if (nsBlockFrame::gNoisyReflow || nsBlockFrame::gNoisyFloatManager) {
      mFloatManager->List(stdout);
    }
#endif
  }
}











bool
nsBlockReflowState::AddFloat(nsLineLayout*       aLineLayout,
                             nsIFrame*           aFloat,
                             nscoord             aAvailableWidth)
{
  NS_PRECONDITION(aLineLayout, "must have line layout");
  NS_PRECONDITION(mBlock->end_lines() != mCurrentLine, "null ptr");
  NS_PRECONDITION(aFloat->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                  "aFloat must be an out-of-flow frame");

  NS_ABORT_IF_FALSE(aFloat->GetParent(), "float must have parent");
  NS_ABORT_IF_FALSE(aFloat->GetParent()->IsFrameOfType(nsIFrame::eBlockFrame),
                    "float's parent must be block");
  NS_ABORT_IF_FALSE(aFloat->GetParent() == mBlock ||
                    (aFloat->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT),
                    "float should be in this block unless it was marked as "
                    "pushed float");
  if (aFloat->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT) {
    
    
    
    
    
    nsBlockFrame *floatParent =
      static_cast<nsBlockFrame*>(aFloat->GetParent());
    floatParent->StealFrame(mPresContext, aFloat);

    aFloat->RemoveStateBits(NS_FRAME_IS_PUSHED_FLOAT);

    
    
    mBlock->mFloats.AppendFrame(mBlock, aFloat);
  }

  
  
  
  
  
  nscoord ox, oy;
  mFloatManager->GetTranslation(ox, oy);
  nscoord dx = ox - mFloatManagerX;
  nscoord dy = oy - mFloatManagerY;
  mFloatManager->Translate(-dx, -dy);

  bool placed;

  
  
  
  
  
  nsRect floatAvailableSpace = GetFloatAvailableSpace().mRect;
  if (mBelowCurrentLineFloats.IsEmpty() &&
      (aLineLayout->LineIsEmpty() ||
       mBlock->ComputeFloatWidth(*this, floatAvailableSpace, aFloat)
       <= aAvailableWidth)) {
    
    placed = FlowAndPlaceFloat(aFloat);
    if (placed) {
      
      nsFlowAreaRect floatAvailSpace = GetFloatAvailableSpace(mY);
      nsRect availSpace(nsPoint(floatAvailSpace.mRect.x, mY),
                        floatAvailSpace.mRect.Size());
      aLineLayout->UpdateBand(availSpace, aFloat);
      
      mCurrentLineFloats.Append(mFloatCacheFreeList.Alloc(aFloat));
    } else {
      (*aLineLayout->GetLine())->SetHadFloatPushed();
    }
  }
  else {
    
    
    placed = true;
    
    
    mBelowCurrentLineFloats.Append(mFloatCacheFreeList.Alloc(aFloat));
  }

  
  mFloatManager->Translate(dx, dy);

  return placed;
}

bool
nsBlockReflowState::CanPlaceFloat(nscoord aFloatWidth,
                                  const nsFlowAreaRect& aFloatAvailableSpace)
{
  
  
  
  
  return !aFloatAvailableSpace.mHasFloats ||
         aFloatAvailableSpace.mRect.width >= aFloatWidth;
}

static nscoord
FloatMarginWidth(const nsHTMLReflowState& aCBReflowState,
                 nscoord aFloatAvailableWidth,
                 nsIFrame *aFloat,
                 const nsCSSOffsetState& aFloatOffsetState)
{
  AutoMaybeDisableFontInflation an(aFloat);
  return aFloat->ComputeSize(
    aCBReflowState.rendContext,
    nsSize(aCBReflowState.ComputedWidth(),
           aCBReflowState.ComputedHeight()),
    aFloatAvailableWidth,
    nsSize(aFloatOffsetState.mComputedMargin.LeftRight(),
           aFloatOffsetState.mComputedMargin.TopBottom()),
    nsSize(aFloatOffsetState.mComputedBorderPadding.LeftRight() -
             aFloatOffsetState.mComputedPadding.LeftRight(),
           aFloatOffsetState.mComputedBorderPadding.TopBottom() -
             aFloatOffsetState.mComputedPadding.TopBottom()),
    nsSize(aFloatOffsetState.mComputedPadding.LeftRight(),
           aFloatOffsetState.mComputedPadding.TopBottom()),
    true).width +
  aFloatOffsetState.mComputedMargin.LeftRight() +
  aFloatOffsetState.mComputedBorderPadding.LeftRight();
}

bool
nsBlockReflowState::FlowAndPlaceFloat(nsIFrame* aFloat)
{
  
  
  
  
  
  AutoRestore<nscoord> restoreY(mY);
  
  const nscoord saveY = mY;

  
  const nsStyleDisplay* floatDisplay = aFloat->GetStyleDisplay();

  
  nsRect oldRegion = nsFloatManager::GetRegionFor(aFloat);

  
  
  mY = NS_MAX(mFloatManager->GetLowestFloatTop(), mY);

  
  
  
  if (NS_STYLE_CLEAR_NONE != floatDisplay->mBreakType) {
    
    mY = ClearFloats(mY, floatDisplay->mBreakType);
  }
    
  nsFlowAreaRect floatAvailableSpace = GetFloatAvailableSpace(mY);
  nsRect adjustedAvailableSpace = mBlock->AdjustFloatAvailableSpace(*this,
                                    floatAvailableSpace.mRect, aFloat);

  NS_ASSERTION(aFloat->GetParent() == mBlock,
               "Float frame has wrong parent");

  nsCSSOffsetState offsets(aFloat, mReflowState.rendContext,
                           mReflowState.ComputedWidth());

  nscoord floatMarginWidth = FloatMarginWidth(mReflowState,
                                              adjustedAvailableSpace.width,
                                              aFloat, offsets);

  nsMargin floatMargin; 
  nsReflowStatus reflowStatus;

  
  
  
  bool isLetter = aFloat->GetType() == nsGkAtoms::letterFrame;
  if (isLetter) {
    mBlock->ReflowFloat(*this, adjustedAvailableSpace, aFloat,
                        floatMargin, false, reflowStatus);
    floatMarginWidth = aFloat->GetSize().width + floatMargin.LeftRight();
    NS_ASSERTION(NS_FRAME_IS_COMPLETE(reflowStatus),
                 "letter frames shouldn't break, and if they do now, "
                 "then they're breaking at the wrong point");
  }

  
  
  
  NS_ASSERTION((NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) ||
	       (NS_STYLE_FLOAT_RIGHT == floatDisplay->mFloats),
	       "invalid float type");

  
  bool keepFloatOnSameLine = false;

  
  
  
  bool mustPlaceFloat =
    mReflowState.mFlags.mIsTopOfPage && IsAdjacentWithTop();

  for (;;) {
    if (mReflowState.availableHeight != NS_UNCONSTRAINEDSIZE &&
        floatAvailableSpace.mRect.height <= 0 &&
        !mustPlaceFloat) {
      
      PushFloatPastBreak(aFloat);
      return false;
    }

    if (CanPlaceFloat(floatMarginWidth, floatAvailableSpace)) {
      
      break;
    }

    
    if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
          eCompatibility_NavQuirks != mPresContext->CompatibilityMode() ) {

      mY += floatAvailableSpace.mRect.height;
      if (adjustedAvailableSpace.height != NS_UNCONSTRAINEDSIZE) {
        adjustedAvailableSpace.height -= floatAvailableSpace.mRect.height;
      }
      floatAvailableSpace = GetFloatAvailableSpace(mY);
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
              keepFloatOnSameLine = true;
              
              
              
              break;
            }
          }
        }
      }

      
      mY += floatAvailableSpace.mRect.height;
      
      
      floatAvailableSpace = GetFloatAvailableSpace(mY);
      adjustedAvailableSpace = mBlock->AdjustFloatAvailableSpace(*this,
                                 floatAvailableSpace.mRect, aFloat);
      floatMarginWidth = FloatMarginWidth(mReflowState,
                                          adjustedAvailableSpace.width,
                                          aFloat, offsets);
    }

    mustPlaceFloat = false;
  }

  

  
  

  
  nscoord floatX, floatY;
  if (NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) {
    floatX = floatAvailableSpace.mRect.x;
  }
  else {
    if (!keepFloatOnSameLine) {
      floatX = floatAvailableSpace.mRect.XMost() - floatMarginWidth;
    } 
    else {
      
      
      
      floatX = floatAvailableSpace.mRect.x;
    }
  }
  
  
  
  
  
  floatY = NS_MAX(mY, mContentArea.y);

  
  
  if (!isLetter) {
    bool pushedDown = mY != saveY;
    mBlock->ReflowFloat(*this, adjustedAvailableSpace, aFloat,
                        floatMargin, pushedDown, reflowStatus);
  }
  if (aFloat->GetPrevInFlow())
    floatMargin.top = 0;
  if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus))
    floatMargin.bottom = 0;

  
  
  
  
  
  
  
  if ((mContentArea.height != NS_UNCONSTRAINEDSIZE &&
       adjustedAvailableSpace.height == NS_UNCONSTRAINEDSIZE &&
       !mustPlaceFloat &&
       aFloat->GetSize().height + floatMargin.TopBottom() >
         mContentArea.YMost() - floatY) ||
      NS_FRAME_IS_TRUNCATED(reflowStatus)) {

    PushFloatPastBreak(aFloat);
    return false;
  }

  
  
  
  nsPoint origin(floatMargin.left + floatX,
                 floatMargin.top + floatY);

  
  origin += aFloat->GetRelativeOffset(floatDisplay);

  
  
  
  bool moved = aFloat->GetPosition() != origin;
  if (moved) {
    aFloat->SetPosition(origin);
    nsContainerFrame::PositionFrameView(aFloat);
    nsContainerFrame::PositionChildViews(aFloat);
  }

  
  
  mFloatOverflowAreas.UnionWith(aFloat->GetOverflowAreas() + origin);

  
  
  nsRect region = nsFloatManager::CalculateRegionFor(aFloat, floatMargin);
  
  if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus) &&
      (NS_UNCONSTRAINEDSIZE != mContentArea.height)) {
    region.height = NS_MAX(region.height, mContentArea.height - floatY);
  }
  nsresult rv =
  mFloatManager->AddFloat(aFloat, region);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "bad float placement");
  
  nsFloatManager::StoreRegionFor(aFloat, region);

  
  
  if (!region.IsEqualEdges(oldRegion)) {
    
    
    
    
    nscoord top = NS_MIN(region.y, oldRegion.y);
    nscoord bottom = NS_MAX(region.YMost(), oldRegion.YMost());
    mFloatManager->IncludeInDamage(top, bottom);
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(reflowStatus)) {
    mBlock->SplitFloat(*this, aFloat, reflowStatus);
  }

#ifdef NOISY_FLOATMANAGER
  nscoord tx, ty;
  mFloatManager->GetTranslation(tx, ty);
  nsFrame::ListTag(stdout, mBlock);
  printf(": FlowAndPlaceFloat: AddFloat: txy=%d,%d (%d,%d) {%d,%d,%d,%d}\n",
         tx, ty, mFloatManagerX, mFloatManagerY,
         region.x, region.y, region.width, region.height);
#endif

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsRect r = aFloat->GetRect();
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("placed float: ");
    nsFrame::ListTag(stdout, aFloat);
    printf(" %d,%d,%d,%d\n", r.x, r.y, r.width, r.height);
  }
#endif

  return true;
}

void
nsBlockReflowState::PushFloatPastBreak(nsIFrame *aFloat)
{
  
  
  
  
  
  if (aFloat->GetStyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
    mFloatManager->SetPushedLeftFloatPastBreak();
  } else {
    NS_ABORT_IF_FALSE(aFloat->GetStyleDisplay()->mFloats ==
                        NS_STYLE_FLOAT_RIGHT,
                      "unexpected float value");
    mFloatManager->SetPushedRightFloatPastBreak();
  }

  
  
  DebugOnly<nsresult> rv = mBlock->StealFrame(mPresContext, aFloat);
  NS_ASSERTION(NS_SUCCEEDED(rv), "StealFrame should succeed");
  AppendPushedFloat(aFloat);

  NS_FRAME_SET_OVERFLOW_INCOMPLETE(mReflowStatus);
}




void
nsBlockReflowState::PlaceBelowCurrentLineFloats(nsFloatCacheFreeList& aList,
                                                nsLineBox* aLine)
{
  nsFloatCache* fc = aList.Head();
  while (fc) {
#ifdef DEBUG
    if (nsBlockFrame::gNoisyReflow) {
      nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
      printf("placing bcl float: ");
      nsFrame::ListTag(stdout, fc->mFloat);
      printf("\n");
    }
#endif
    
    bool placed = FlowAndPlaceFloat(fc->mFloat);
    nsFloatCache *next = fc->Next();
    if (!placed) {
      aList.Remove(fc);
      delete fc;
      aLine->SetHadFloatPushed();
    }
    fc = next;
  }
}

nscoord
nsBlockReflowState::ClearFloats(nscoord aY, PRUint8 aBreakType,
                                nsIFrame *aReplacedBlock,
                                PRUint32 aFlags)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: in: aY=%d\n", aY);
  }
#endif

#ifdef NOISY_FLOAT_CLEARING
  printf("nsBlockReflowState::ClearFloats: aY=%d breakType=%d\n",
         aY, aBreakType);
  mFloatManager->List(stdout);
#endif
  
  nscoord newY = aY;

  if (aBreakType != NS_STYLE_CLEAR_NONE) {
    newY = mFloatManager->ClearFloats(newY, aBreakType, aFlags);
  }

  if (aReplacedBlock) {
    for (;;) {
      nsFlowAreaRect floatAvailableSpace = GetFloatAvailableSpace(newY);
      nsBlockFrame::ReplacedElementWidthToClear replacedWidth =
        nsBlockFrame::WidthToClearPastFloats(*this, floatAvailableSpace.mRect,
                                             aReplacedBlock);
      if (!floatAvailableSpace.mHasFloats ||
          NS_MAX(floatAvailableSpace.mRect.x - mContentArea.x,
                 replacedWidth.marginLeft) +
            replacedWidth.borderBoxWidth +
            NS_MAX(mContentArea.XMost() - floatAvailableSpace.mRect.XMost(),
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
    printf("clear floats: out: y=%d\n", newY);
  }
#endif

  return newY;
}

