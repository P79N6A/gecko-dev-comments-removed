







#include "nsBlockReflowState.h"

#include "mozilla/DebugOnly.h"

#include "nsBlockFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsIFrameInlines.h"
#include "mozilla/AutoRestore.h"
#include <algorithm>

#ifdef DEBUG
#include "nsBlockDebugFlags.h"
#endif

using namespace mozilla;
using namespace mozilla::layout;

nsBlockReflowState::nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                                       nsPresContext* aPresContext,
                                       nsBlockFrame* aFrame,
                                       bool aBStartMarginRoot,
                                       bool aBEndMarginRoot,
                                       bool aBlockNeedsFloatManager,
                                       nscoord aConsumedBSize)
  : mBlock(aFrame),
    mPresContext(aPresContext),
    mReflowState(aReflowState),
    mContentArea(aReflowState.GetWritingMode()),
    mPushedFloats(nullptr),
    mOverflowTracker(nullptr),
    mBorderPadding(mReflowState.ComputedLogicalBorderPadding()),
    mPrevBEndMargin(),
    mLineNumber(0),
    mFlags(0),
    mFloatBreakType(NS_STYLE_CLEAR_NONE),
    mConsumedBSize(aConsumedBSize)
{
  WritingMode wm = aReflowState.GetWritingMode();
  SetFlag(BRS_ISFIRSTINFLOW, aFrame->GetPrevInFlow() == nullptr);
  SetFlag(BRS_ISOVERFLOWCONTAINER, IS_TRUE_OVERFLOW_CONTAINER(aFrame));

  nsIFrame::LogicalSides logicalSkipSides =
    aFrame->GetLogicalSkipSides(&aReflowState);
  mBorderPadding.ApplySkipSides(logicalSkipSides);

  
  mContainerWidth = aReflowState.ComputedWidth() + mBorderPadding.LeftRight(wm);

  if ((aBStartMarginRoot && !logicalSkipSides.BStart()) ||
      0 != mBorderPadding.BStart(wm)) {
    SetFlag(BRS_ISBSTARTMARGINROOT, true);
    SetFlag(BRS_APPLYBSTARTMARGIN, true);
  }
  if ((aBEndMarginRoot && !logicalSkipSides.BEnd()) ||
      0 != mBorderPadding.BEnd(wm)) {
    SetFlag(BRS_ISBENDMARGINROOT, true);
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

  mNextInFlow = static_cast<nsBlockFrame*>(mBlock->GetNextInFlow());

  NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aReflowState.ComputedISize(),
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  mContentArea.ISize(wm) = aReflowState.ComputedISize();

  
  
  
  
  
  
  if (NS_UNCONSTRAINEDSIZE != aReflowState.AvailableBSize()) {
    
    
    
    mBEndEdge = aReflowState.AvailableBSize() - mBorderPadding.BEnd(wm);
    mContentArea.BSize(wm) = std::max(0, mBEndEdge - mBorderPadding.BStart(wm));
  }
  else {
    
    
    SetFlag(BRS_UNCONSTRAINEDBSIZE, true);
    mContentArea.BSize(wm) = mBEndEdge = NS_UNCONSTRAINEDSIZE;
  }
  mContentArea.IStart(wm) = mBorderPadding.IStart(wm);
  mBCoord = mContentArea.BStart(wm) = mBorderPadding.BStart(wm);

  mPrevChild = nullptr;
  mCurrentLine = aFrame->end_lines();

  mMinLineHeight = aReflowState.CalcLineHeight();
}

nscoord
nsBlockReflowState::GetConsumedBSize()
{
  if (mConsumedBSize == NS_INTRINSICSIZE) {
    mConsumedBSize = mBlock->GetConsumedBSize();
  }

  return mConsumedBSize;
}

void
nsBlockReflowState::ComputeReplacedBlockOffsetsForFloats(nsIFrame* aFrame,
                                                         const nsRect& aFloatAvailableSpace,
                                                         nscoord& aLeftResult,
                                                         nscoord& aRightResult)
{
  nsRect contentArea =
    mContentArea.GetPhysicalRect(mReflowState.GetWritingMode(), mContainerWidth);
  
  
  
  
  NS_ASSERTION(aFloatAvailableSpace.x >= contentArea.x, "bad avail space rect x");
  NS_ASSERTION(aFloatAvailableSpace.width == 0 ||
               aFloatAvailableSpace.XMost() <= contentArea.XMost(),
               "bad avail space rect width");

  nscoord leftOffset, rightOffset;
  if (aFloatAvailableSpace.width == contentArea.width) {
    
    leftOffset = 0;
    rightOffset = 0;
  } else {
    nsMargin frameMargin;
    nsCSSOffsetState os(aFrame, mReflowState.rendContext, contentArea.width);
    frameMargin = os.ComputedPhysicalMargin();

    nscoord leftFloatXOffset = aFloatAvailableSpace.x - contentArea.x;
    leftOffset = std::max(leftFloatXOffset, frameMargin.left) -
                 frameMargin.left;
    leftOffset = std::max(leftOffset, 0); 
    nscoord rightFloatXOffset =
      contentArea.XMost() - aFloatAvailableSpace.XMost();
    rightOffset = std::max(rightFloatXOffset, frameMargin.right) -
                  frameMargin.right;
    rightOffset = std::max(rightOffset, 0); 
  }
  aLeftResult = leftOffset;
  aRightResult = rightOffset;
}

static nscoord
GetBEndMarginClone(nsIFrame* aFrame,
                   nsRenderingContext* aRenderingContext,
                   const LogicalRect& aContentArea,
                   WritingMode aWritingMode)
{
  if (aFrame->StyleBorder()->mBoxDecorationBreak ==
        NS_STYLE_BOX_DECORATION_BREAK_CLONE) {
    nsCSSOffsetState os(aFrame, aRenderingContext, aContentArea.Width(aWritingMode));
    return os.ComputedLogicalMargin().
                ConvertTo(aWritingMode,
                          aFrame->GetWritingMode()).BEnd(aWritingMode);
  }
  return 0;
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
  WritingMode wm = mReflowState.GetWritingMode();
  LogicalRect result(wm);
  LogicalRect floatAvailSpace = LogicalRect(wm,
                                            aFloatAvailableSpace.mRect,
                                            mContainerWidth); 
  result.BStart(wm) = mBCoord;
  result.BSize(wm) = GetFlag(BRS_UNCONSTRAINEDBSIZE)
    ? NS_UNCONSTRAINEDSIZE
    : mReflowState.AvailableBSize() - mBCoord
      - GetBEndMarginClone(aFrame, mReflowState.rendContext, mContentArea, wm);
  
  
  

  
  
  
  
  
  
  
  
  
  
  
  
  NS_ASSERTION(nsBlockFrame::BlockCanIntersectFloats(aFrame) == 
                 !aBlockAvoidsFloats,
               "unexpected replaced width");
  if (!aBlockAvoidsFloats) {
    if (aFloatAvailableSpace.mHasFloats) {
      
      
      const nsStyleBorder* borderStyle = aFrame->StyleBorder();
      switch (borderStyle->mFloatEdge) {
        default:
        case NS_STYLE_FLOAT_EDGE_CONTENT:  
          
          
          result.IStart(wm) = ContentIStart();
          result.ISize(wm) = ContentISize();
          break;
        case NS_STYLE_FLOAT_EDGE_MARGIN:
          {
            
            
            result.IStart(wm) = floatAvailSpace.IStart(wm);
            result.ISize(wm) = floatAvailSpace.ISize(wm);
          }
          break;
      }
    }
    else {
      
      
      
      result.IStart(wm) = ContentIStart();
      result.ISize(wm) = ContentISize();
    }
    aResult = result.GetPhysicalRect(wm, mContainerWidth);
  }
  else {
    aResult = result.GetPhysicalRect(wm, mContainerWidth);
    nsRect contentArea =
      mContentArea.GetPhysicalRect(wm, mContainerWidth);
    nscoord leftOffset, rightOffset;
    ComputeReplacedBlockOffsetsForFloats(aFrame, aFloatAvailableSpace.mRect,
                                         leftOffset, rightOffset);
    aResult.x = contentArea.x + leftOffset;
    aResult.width = contentArea.width - leftOffset - rightOffset;
  }

#ifdef REALLY_NOISY_REFLOW
  printf("  CBAS: result %d %d %d %d\n", aResult.x, aResult.y, aResult.width, aResult.height);
#endif
}

nsFlowAreaRect
nsBlockReflowState::GetFloatAvailableSpaceWithState(
                      nscoord aBCoord,
                      nsFloatManager::SavedState *aState) const
{
#ifdef DEBUG
  
  nscoord wx, wy;
  mFloatManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mFloatManagerX) && (wy == mFloatManagerY),
               "bad coord system");
#endif

  nsRect contentArea =
    mContentArea.GetPhysicalRect(mReflowState.GetWritingMode(), mContainerWidth);
  nscoord height = (contentArea.height == nscoord_MAX)
                     ? nscoord_MAX : std::max(contentArea.YMost() - aBCoord, 0);
  nsFlowAreaRect result =
    mFloatManager->GetFlowArea(aBCoord, nsFloatManager::BAND_FROM_POINT,
                               height, contentArea, aState);
  
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
nsBlockReflowState::GetFloatAvailableSpaceForBSize(
                      nscoord aBCoord, nscoord aBSize,
                      nsFloatManager::SavedState *aState) const
{
#ifdef DEBUG
  
  nscoord wx, wy;
  mFloatManager->GetTranslation(wx, wy);
  NS_ASSERTION((wx == mFloatManagerX) && (wy == mFloatManagerY),
               "bad coord system");
#endif
  nsRect contentArea =
    mContentArea.GetPhysicalRect(mReflowState.GetWritingMode(), mContainerWidth);
  nsFlowAreaRect result =
    mFloatManager->GetFlowArea(aBCoord, nsFloatManager::WIDTH_WITHIN_HEIGHT,
                               aBSize, contentArea, aState);
  
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
nsBlockReflowState::ReconstructMarginBefore(nsLineList::iterator aLine)
{
  mPrevBEndMargin.Zero();
  nsBlockFrame *block = mBlock;

  nsLineList::iterator firstLine = block->begin_lines();
  for (;;) {
    --aLine;
    if (aLine->IsBlock()) {
      mPrevBEndMargin = aLine->GetCarriedOutBEndMargin();
      break;
    }
    if (!aLine->IsEmpty()) {
      break;
    }
    if (aLine == firstLine) {
      
      
      if (!GetFlag(BRS_ISBSTARTMARGINROOT)) {
        mPrevBEndMargin.Zero();
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
nsBlockReflowState::AppendPushedFloat(nsIFrame* aFloatCont)
{
  SetupPushedFloatList();
  aFloatCont->AddStateBits(NS_FRAME_IS_PUSHED_FLOAT);
  mPushedFloats->AppendFrame(mBlock, aFloatCont);
}









void
nsBlockReflowState::RecoverFloats(nsLineList::iterator aLine,
                                  nscoord aDeltaBCoord)
{
  if (aLine->HasFloats()) {
    
    
    nsFloatCache* fc = aLine->GetFirstFloat();
    while (fc) {
      nsIFrame* floatFrame = fc->mFloat;
      if (aDeltaBCoord != 0) {
        floatFrame->MovePositionBy(nsPoint(0, aDeltaBCoord));
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
        printf(" aDeltaBCoord=%d region={%d,%d,%d,%d}\n",
               aDeltaBCoord, region.x, region.y, region.width, region.height);
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
                                     nscoord aDeltaBCoord)
{
  
  mCurrentLine = aLine;

  
  if (aLine->HasFloats() || aLine->IsBlock()) {
    RecoverFloats(aLine, aDeltaBCoord);

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
    floatParent->StealFrame(aFloat);

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
      
      nsFlowAreaRect floatAvailSpace = GetFloatAvailableSpace(mBCoord);
      nsRect availSpace(nsPoint(floatAvailSpace.mRect.x, mBCoord),
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
  WritingMode fosWM = aFloatOffsetState.GetWritingMode();
  return aFloat->ComputeSize(
    aCBReflowState.rendContext,
    fosWM,
    aCBReflowState.ComputedSize(fosWM),
    aFloatAvailableWidth,
    aFloatOffsetState.ComputedLogicalMargin().Size(fosWM),
    aFloatOffsetState.ComputedLogicalBorderPadding().Size(fosWM) -
      aFloatOffsetState.ComputedLogicalPadding().Size(fosWM),
    aFloatOffsetState.ComputedLogicalPadding().Size(fosWM),
    true).Width(fosWM) +
  aFloatOffsetState.ComputedPhysicalMargin().LeftRight() +
  aFloatOffsetState.ComputedPhysicalBorderPadding().LeftRight();
}

bool
nsBlockReflowState::FlowAndPlaceFloat(nsIFrame* aFloat)
{
  
  
  
  
  
  AutoRestore<nscoord> restoreBCoord(mBCoord);
  
  const nscoord saveBCoord = mBCoord;

  
  const nsStyleDisplay* floatDisplay = aFloat->StyleDisplay();

  
  nsRect oldRegion = nsFloatManager::GetRegionFor(aFloat);

  
  
  mBCoord = std::max(mFloatManager->GetLowestFloatTop(), mBCoord);

  
  
  
  if (NS_STYLE_CLEAR_NONE != floatDisplay->mBreakType) {
    
    mBCoord = ClearFloats(mBCoord, floatDisplay->mBreakType);
  }
    
  nsFlowAreaRect floatAvailableSpace = GetFloatAvailableSpace(mBCoord);
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
  nsMargin floatOffsets;
  nsReflowStatus reflowStatus;

  
  
  
  bool isLetter = aFloat->GetType() == nsGkAtoms::letterFrame;
  if (isLetter) {
    mBlock->ReflowFloat(*this, adjustedAvailableSpace, aFloat, floatMargin,
                        floatOffsets, false, reflowStatus);
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
    if (mReflowState.AvailableHeight() != NS_UNCONSTRAINEDSIZE &&
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

      mBCoord += floatAvailableSpace.mRect.height;
      if (adjustedAvailableSpace.height != NS_UNCONSTRAINEDSIZE) {
        adjustedAvailableSpace.height -= floatAvailableSpace.mRect.height;
      }
      floatAvailableSpace = GetFloatAvailableSpace(mBCoord);
    } else {
      
      

      
      nsFloatCache* fc = mCurrentLineFloats.Head();
      nsIFrame* prevFrame = nullptr;
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

      
      mBCoord += floatAvailableSpace.mRect.height;
      
      
      floatAvailableSpace = GetFloatAvailableSpace(mBCoord);
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
  
  
  
  
  
  floatY = std::max(mBCoord, ContentBStart());

  
  
  if (!isLetter) {
    bool pushedDown = mBCoord != saveBCoord;
    mBlock->ReflowFloat(*this, adjustedAvailableSpace, aFloat, floatMargin,
                        floatOffsets, pushedDown, reflowStatus);
  }
  if (aFloat->GetPrevInFlow())
    floatMargin.top = 0;
  if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus))
    floatMargin.bottom = 0;

  
  
  
  
  
  
  
  if ((ContentBSize() != NS_UNCONSTRAINEDSIZE &&
       adjustedAvailableSpace.height == NS_UNCONSTRAINEDSIZE &&
       !mustPlaceFloat &&
       aFloat->GetSize().height + floatMargin.TopBottom() >
         ContentBEnd() - floatY) ||
      NS_FRAME_IS_TRUNCATED(reflowStatus) ||
      NS_INLINE_IS_BREAK_BEFORE(reflowStatus)) {
    PushFloatPastBreak(aFloat);
    return false;
  }

  
  
  
  if (ContentBSize() != NS_UNCONSTRAINEDSIZE &&
      !mustPlaceFloat && (!mReflowState.mFlags.mIsTopOfPage || floatY > 0) &&
      NS_STYLE_PAGE_BREAK_AVOID == aFloat->StyleDisplay()->mBreakInside &&
      (!NS_FRAME_IS_FULLY_COMPLETE(reflowStatus) ||
       aFloat->GetSize().height + floatMargin.TopBottom() >
       ContentBEnd() - floatY) &&
      !aFloat->GetPrevInFlow()) {
    PushFloatPastBreak(aFloat);
    return false;
  }

  
  
  
  nsPoint origin(floatMargin.left + floatX,
                 floatMargin.top + floatY);

  
  nsHTMLReflowState::ApplyRelativePositioning(aFloat, floatOffsets, &origin);

  
  
  
  bool moved = aFloat->GetPosition() != origin;
  if (moved) {
    aFloat->SetPosition(origin);
    nsContainerFrame::PositionFrameView(aFloat);
    nsContainerFrame::PositionChildViews(aFloat);
  }

  
  
  mFloatOverflowAreas.UnionWith(aFloat->GetOverflowAreas() + origin);

  
  
  nsRect region = nsFloatManager::CalculateRegionFor(aFloat, floatMargin);
  
  if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus) &&
      (NS_UNCONSTRAINEDSIZE != ContentBSize())) {
    region.height = std::max(region.height, ContentBSize() - floatY);
  }
  DebugOnly<nsresult> rv =
    mFloatManager->AddFloat(aFloat, region);
  NS_ABORT_IF_FALSE(NS_SUCCEEDED(rv), "bad float placement");
  
  nsFloatManager::StoreRegionFor(aFloat, region);

  
  
  if (!region.IsEqualEdges(oldRegion)) {
    
    
    
    
    nscoord top = std::min(region.y, oldRegion.y);
    nscoord bottom = std::max(region.YMost(), oldRegion.YMost());
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
  
  
  
  
  
  if (aFloat->StyleDisplay()->mFloats == NS_STYLE_FLOAT_LEFT) {
    mFloatManager->SetPushedLeftFloatPastBreak();
  } else {
    NS_ABORT_IF_FALSE(aFloat->StyleDisplay()->mFloats ==
                        NS_STYLE_FLOAT_RIGHT,
                      "unexpected float value");
    mFloatManager->SetPushedRightFloatPastBreak();
  }

  
  
  DebugOnly<nsresult> rv = mBlock->StealFrame(aFloat);
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
nsBlockReflowState::ClearFloats(nscoord aBCoord, uint8_t aBreakType,
                                nsIFrame *aReplacedBlock,
                                uint32_t aFlags)
{
#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: in: aBCoord=%d\n", aBCoord);
  }
#endif

#ifdef NOISY_FLOAT_CLEARING
  printf("nsBlockReflowState::ClearFloats: aBCoord=%d breakType=%d\n",
         aBCoord, aBreakType);
  mFloatManager->List(stdout);
#endif

  if (!mFloatManager->HasAnyFloats()) {
    return aBCoord;
  }

  nscoord newBCoord = aBCoord;

  if (aBreakType != NS_STYLE_CLEAR_NONE) {
    newBCoord = mFloatManager->ClearFloats(newBCoord, aBreakType, aFlags);
  }

  if (aReplacedBlock) {
    for (;;) {
      nsFlowAreaRect floatAvailableSpace = GetFloatAvailableSpace(newBCoord);
      if (!floatAvailableSpace.mHasFloats) {
        
        
        
        break;
      }
      nsBlockFrame::ReplacedElementWidthToClear replacedWidth =
        nsBlockFrame::WidthToClearPastFloats(*this, floatAvailableSpace.mRect,
                                             aReplacedBlock);
      if (std::max(floatAvailableSpace.mRect.x - ContentIStart(),
                   replacedWidth.marginLeft) +
            replacedWidth.borderBoxWidth +
            std::max(ContentIEnd() - floatAvailableSpace.mRect.XMost(),
                     replacedWidth.marginRight) <=
          ContentISize()) {
        break;
      }
      
      if (floatAvailableSpace.mRect.height > 0) {
        
        newBCoord += floatAvailableSpace.mRect.height;
      } else {
        if (mReflowState.AvailableHeight() != NS_UNCONSTRAINEDSIZE) {
          
          
          break;
        }
        NS_NOTREACHED("avail space rect with zero height!");
        newBCoord += 1;
      }
    }
  }

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("clear floats: out: y=%d\n", newBCoord);
  }
#endif

  return newBCoord;
}

