







#include "nsBlockReflowState.h"

#include "mozilla/DebugOnly.h"

#include "nsBlockFrame.h"
#include "nsLineLayout.h"
#include "nsPresContext.h"
#include "nsIFrameInlines.h"
#include "mozilla/AutoRestore.h"
#include "mozilla/Preferences.h"
#include <algorithm>

#ifdef DEBUG
#include "nsBlockDebugFlags.h"
#endif

using namespace mozilla;
using namespace mozilla::layout;

static bool sFloatFragmentsInsideColumnEnabled;
static bool sFloatFragmentsInsideColumnPrefCached;

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
  if (!sFloatFragmentsInsideColumnPrefCached) {
    sFloatFragmentsInsideColumnPrefCached = true;
    Preferences::AddBoolVarCache(&sFloatFragmentsInsideColumnEnabled,
                                 "layout.float-fragments-inside-column.enabled");
  }
  SetFlag(BRS_FLOAT_FRAGMENTS_INSIDE_COLUMN_ENABLED, sFloatFragmentsInsideColumnEnabled);
  
  WritingMode wm = aReflowState.GetWritingMode();
  SetFlag(BRS_ISFIRSTINFLOW, aFrame->GetPrevInFlow() == nullptr);
  SetFlag(BRS_ISOVERFLOWCONTAINER, IS_TRUE_OVERFLOW_CONTAINER(aFrame));

  nsIFrame::LogicalSides logicalSkipSides =
    aFrame->GetLogicalSkipSides(&aReflowState);
  mBorderPadding.ApplySkipSides(logicalSkipSides);

  
  
  
  
  
  
  
  
  
  mContainerSize.width = aReflowState.ComputedWidth();
  if (mContainerSize.width == NS_UNCONSTRAINEDSIZE) {
    mContainerSize.width = 0;
  }

  mContainerSize.width += mBorderPadding.LeftRight(wm);

  
  
  
  mContainerSize.height = aReflowState.ComputedHeight() +
                          mBorderPadding.TopBottom(wm);

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
    
    mFloatManager->GetTranslation(mFloatManagerI, mFloatManagerB);
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
nsBlockReflowState::ComputeReplacedBlockOffsetsForFloats(
                      nsIFrame* aFrame,
                      const LogicalRect& aFloatAvailableSpace,
                      nscoord& aIStartResult,
                      nscoord& aIEndResult)
{
  WritingMode wm = mReflowState.GetWritingMode();
  
  
  
  
  NS_ASSERTION(aFloatAvailableSpace.IStart(wm) >= mContentArea.IStart(wm),
               "bad avail space rect inline-coord");
  NS_ASSERTION(aFloatAvailableSpace.ISize(wm) == 0 ||
               aFloatAvailableSpace.IEnd(wm) <= mContentArea.IEnd(wm),
               "bad avail space rect inline-size");

  nscoord iStartOffset, iEndOffset;
  if (aFloatAvailableSpace.ISize(wm) == mContentArea.ISize(wm)) {
    
    iStartOffset = 0;
    iEndOffset = 0;
  } else {
    LogicalMargin frameMargin(wm);
    nsCSSOffsetState os(aFrame, mReflowState.rendContext,
                        mContentArea.ISize(wm));
    frameMargin =
      os.ComputedLogicalMargin().ConvertTo(wm, aFrame->GetWritingMode());

    nscoord iStartFloatIOffset =
      aFloatAvailableSpace.IStart(wm) - mContentArea.IStart(wm);
    iStartOffset = std::max(iStartFloatIOffset, frameMargin.IStart(wm)) -
                   frameMargin.IStart(wm);
    iStartOffset = std::max(iStartOffset, 0); 
    nscoord iEndFloatIOffset =
      mContentArea.IEnd(wm) - aFloatAvailableSpace.IEnd(wm);
    iEndOffset = std::max(iEndFloatIOffset, frameMargin.IEnd(wm)) -
                 frameMargin.IEnd(wm);
    iEndOffset = std::max(iEndOffset, 0); 
  }
  aIStartResult = iStartOffset;
  aIEndResult = iEndOffset;
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
                                           LogicalRect& aResult)
{
#ifdef REALLY_NOISY_REFLOW
  printf("CBAS frame=%p has floats %d\n",
         aFrame, aFloatAvailableSpace.mHasFloats);
#endif
  WritingMode wm = mReflowState.GetWritingMode();
  aResult.BStart(wm) = mBCoord;
  aResult.BSize(wm) = GetFlag(BRS_UNCONSTRAINEDBSIZE)
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
          
          
          aResult.IStart(wm) = mContentArea.IStart(wm);
          aResult.ISize(wm) = mContentArea.ISize(wm);
          break;
        case NS_STYLE_FLOAT_EDGE_MARGIN:
          {
            
            
            aResult.IStart(wm) = aFloatAvailableSpace.mRect.IStart(wm);
            aResult.ISize(wm) = aFloatAvailableSpace.mRect.ISize(wm);
          }
          break;
      }
    }
    else {
      
      
      
      aResult.IStart(wm) = mContentArea.IStart(wm);
      aResult.ISize(wm) = mContentArea.ISize(wm);
    }
  }
  else {
    nscoord iStartOffset, iEndOffset;
    ComputeReplacedBlockOffsetsForFloats(aFrame, aFloatAvailableSpace.mRect,
                                         iStartOffset, iEndOffset);
    aResult.IStart(wm) = mContentArea.IStart(wm) + iStartOffset;
    aResult.ISize(wm) = mContentArea.ISize(wm) - iStartOffset - iEndOffset;
  }

#ifdef REALLY_NOISY_REFLOW
  printf("  CBAS: result %d %d %d %d\n", aResult.IStart(wm), aResult.BStart(wm),
         aResult.ISize(wm), aResult.BSize(wm));
#endif
}

nsFlowAreaRect
nsBlockReflowState::GetFloatAvailableSpaceWithState(
                      nscoord aBCoord,
                      nsFloatManager::SavedState *aState) const
{
  WritingMode wm = mReflowState.GetWritingMode();
#ifdef DEBUG
  
  nscoord wI, wB;
  mFloatManager->GetTranslation(wI, wB);

  NS_ASSERTION((wI == mFloatManagerI) && (wB == mFloatManagerB),
               "bad coord system");
#endif

  nscoord blockSize = (mContentArea.BSize(wm) == nscoord_MAX)
    ? nscoord_MAX : std::max(mContentArea.BEnd(wm) - aBCoord, 0);
  nsFlowAreaRect result =
    mFloatManager->GetFlowArea(wm, aBCoord, nsFloatManager::BAND_FROM_POINT,
                               blockSize, mContentArea, aState,
                               ContainerWidth());
  
  if (result.mRect.ISize(wm) < 0) {
    result.mRect.ISize(wm) = 0;
  }

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("GetAvailableSpace: band=%d,%d,%d,%d hasfloats=%d\n",
           result.mRect.IStart(wm), result.mRect.BStart(wm),
           result.mRect.ISize(wm), result.mRect.BSize(wm), result.mHasFloats);
  }
#endif
  return result;
}

nsFlowAreaRect
nsBlockReflowState::GetFloatAvailableSpaceForBSize(
                      nscoord aBCoord, nscoord aBSize,
                      nsFloatManager::SavedState *aState) const
{
  WritingMode wm = mReflowState.GetWritingMode();
#ifdef DEBUG
  
  nscoord wI, wB;
  mFloatManager->GetTranslation(wI, wB);

  NS_ASSERTION((wI == mFloatManagerI) && (wB == mFloatManagerB),
               "bad coord system");
#endif
  nsFlowAreaRect result =
    mFloatManager->GetFlowArea(wm, aBCoord, nsFloatManager::WIDTH_WITHIN_HEIGHT,
                               aBSize, mContentArea, aState, ContainerWidth());
  
  if (result.mRect.ISize(wm) < 0) {
    result.mRect.ISize(wm) = 0;
  }

#ifdef DEBUG
  if (nsBlockFrame::gNoisyReflow) {
    nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
    printf("GetAvailableSpaceForHeight: space=%d,%d,%d,%d hasfloats=%d\n",
           result.mRect.IStart(wm), result.mRect.BStart(wm),
           result.mRect.ISize(wm), result.mRect.BSize(wm), result.mHasFloats);
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
  MOZ_ASSERT(!GetFlag(BRS_PROPTABLE_FLOATCLIST) == !mPushedFloats,
             "flag mismatch");
  if (!GetFlag(BRS_PROPTABLE_FLOATCLIST)) {
    
    
    
    
    
    
    
    mPushedFloats = mBlock->EnsurePushedFloats();
    SetFlag(BRS_PROPTABLE_FLOATCLIST, true);
  }
}

void
nsBlockReflowState::AppendPushedFloatChain(nsIFrame* aFloatCont)
{
  SetupPushedFloatList();
  while (true) {
    aFloatCont->AddStateBits(NS_FRAME_IS_PUSHED_FLOAT);
    mPushedFloats->AppendFrame(mBlock, aFloatCont);
    aFloatCont = aFloatCont->GetNextInFlow();
    if (!aFloatCont || aFloatCont->GetParent() != mBlock) {
      break;
    }
    DebugOnly<nsresult> rv = mBlock->StealFrame(aFloatCont);
    NS_ASSERTION(NS_SUCCEEDED(rv), "StealFrame should succeed");
  }
}









void
nsBlockReflowState::RecoverFloats(nsLineList::iterator aLine,
                                  nscoord aDeltaBCoord)
{
  WritingMode wm = mReflowState.GetWritingMode();
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
        nscoord tI, tB;
        mFloatManager->GetTranslation(tI, tB);
        nsFrame::IndentBy(stdout, nsBlockFrame::gNoiseIndent);
        printf("RecoverFloats: tIB=%d,%d (%d,%d) ",
               tI, tB, mFloatManagerI, mFloatManagerB);
        nsFrame::ListTag(stdout, floatFrame);
        LogicalRect region = nsFloatManager::GetRegionFor(wm, floatFrame,
                                                          ContainerWidth());
        printf(" aDeltaBCoord=%d region={%d,%d,%d,%d}\n",
               aDeltaBCoord, region.IStart(wm), region.BStart(wm),
               region.ISize(wm), region.BSize(wm));
      }
#endif
      mFloatManager->AddFloat(floatFrame,
                              nsFloatManager::GetRegionFor(wm, floatFrame,
                                                           ContainerWidth()),
                              wm, ContainerWidth());
      fc = fc->Next();
    }
  } else if (aLine->IsBlock()) {
    nsBlockFrame::RecoverFloatsFor(aLine->mFirstChild, *mFloatManager, wm,
                                   ContainerWidth());
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
                             nscoord             aAvailableISize)
{
  NS_PRECONDITION(aLineLayout, "must have line layout");
  NS_PRECONDITION(mBlock->end_lines() != mCurrentLine, "null ptr");
  NS_PRECONDITION(aFloat->GetStateBits() & NS_FRAME_OUT_OF_FLOW,
                  "aFloat must be an out-of-flow frame");

  MOZ_ASSERT(aFloat->GetParent(), "float must have parent");
  MOZ_ASSERT(aFloat->GetParent()->IsFrameOfType(nsIFrame::eBlockFrame),
             "float's parent must be block");
  MOZ_ASSERT(aFloat->GetParent() == mBlock ||
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

  
  
  
  
  
  nscoord oI, oB;
  mFloatManager->GetTranslation(oI, oB);
  nscoord dI = oI - mFloatManagerI;
  nscoord dB = oB - mFloatManagerB;
  mFloatManager->Translate(-dI, -dB);

  bool placed;

  
  
  
  
  
  LogicalRect floatAvailableSpace = GetFloatAvailableSpace().mRect;
  if (mBelowCurrentLineFloats.IsEmpty() &&
      (aLineLayout->LineIsEmpty() ||
       mBlock->ComputeFloatISize(*this, floatAvailableSpace, aFloat)
       <= aAvailableISize)) {
    
    placed = FlowAndPlaceFloat(aFloat);
    if (placed) {
      
      WritingMode wm = mReflowState.GetWritingMode();
      nsFlowAreaRect floatAvailSpace = GetFloatAvailableSpace(mBCoord);
      LogicalRect availSpace(wm, floatAvailSpace.mRect.IStart(wm), mBCoord,
                             floatAvailSpace.mRect.ISize(wm),
                             floatAvailSpace.mRect.BSize(wm));
      aLineLayout->UpdateBand(wm, availSpace, aFloat);
      
      mCurrentLineFloats.Append(mFloatCacheFreeList.Alloc(aFloat));
    } else {
      (*aLineLayout->GetLine())->SetHadFloatPushed();
    }
  }
  else {
    
    
    placed = true;
    
    
    mBelowCurrentLineFloats.Append(mFloatCacheFreeList.Alloc(aFloat));
  }

  
  mFloatManager->Translate(dI, dB);

  return placed;
}

bool
nsBlockReflowState::CanPlaceFloat(nscoord aFloatISize,
                                  const nsFlowAreaRect& aFloatAvailableSpace)
{
  
  
  
  
  
  return !aFloatAvailableSpace.mHasFloats ||
    aFloatAvailableSpace.mRect.ISize(mReflowState.GetWritingMode()) >=
      aFloatISize;
}



static nscoord
FloatMarginISize(const nsHTMLReflowState& aCBReflowState,
                 nscoord aFloatAvailableISize,
                 nsIFrame *aFloat,
                 const nsCSSOffsetState& aFloatOffsetState)
{
  AutoMaybeDisableFontInflation an(aFloat);
  WritingMode wm = aFloatOffsetState.GetWritingMode();

  LogicalSize floatSize =
    aFloat->ComputeSize(
              aCBReflowState.rendContext,
              wm,
              aCBReflowState.ComputedSize(wm),
              aFloatAvailableISize,
              aFloatOffsetState.ComputedLogicalMargin().Size(wm),
              aFloatOffsetState.ComputedLogicalBorderPadding().Size(wm) -
                aFloatOffsetState.ComputedLogicalPadding().Size(wm),
              aFloatOffsetState.ComputedLogicalPadding().Size(wm),
              nsIFrame::ComputeSizeFlags::eShrinkWrap);

  floatSize += aFloatOffsetState.ComputedLogicalMargin().Size(wm);
  floatSize += aFloatOffsetState.ComputedLogicalBorderPadding().Size(wm);

  WritingMode cbwm = aCBReflowState.GetWritingMode();
  return floatSize.ConvertTo(cbwm, wm).ISize(cbwm);
}

bool
nsBlockReflowState::FlowAndPlaceFloat(nsIFrame* aFloat)
{
  WritingMode wm = mReflowState.GetWritingMode();
  
  
  
  
  
  AutoRestore<nscoord> restoreBCoord(mBCoord);
  
  const nscoord saveBCoord = mBCoord;

  
  const nsStyleDisplay* floatDisplay = aFloat->StyleDisplay();

  
  LogicalRect oldRegion = nsFloatManager::GetRegionFor(wm, aFloat,
                                                       ContainerWidth());

  
  
  mBCoord = std::max(mFloatManager->GetLowestFloatTop(), mBCoord);

  
  
  
  if (NS_STYLE_CLEAR_NONE != floatDisplay->mBreakType) {
    
    mBCoord = ClearFloats(mBCoord, floatDisplay->mBreakType);
  }
    
  nsFlowAreaRect floatAvailableSpace = GetFloatAvailableSpace(mBCoord);
  LogicalRect adjustedAvailableSpace =
    mBlock->AdjustFloatAvailableSpace(*this, floatAvailableSpace.mRect, aFloat);

  NS_ASSERTION(aFloat->GetParent() == mBlock,
               "Float frame has wrong parent");

  nsCSSOffsetState offsets(aFloat, mReflowState.rendContext,
                           mReflowState.ComputedWidth());

  nscoord floatMarginISize = FloatMarginISize(mReflowState,
                                              adjustedAvailableSpace.ISize(wm),
                                              aFloat, offsets);

  LogicalMargin floatMargin(wm); 
  LogicalMargin floatOffsets(wm);
  nsReflowStatus reflowStatus;

  
  
  
  bool isLetter = aFloat->GetType() == nsGkAtoms::letterFrame;
  if (isLetter) {
    mBlock->ReflowFloat(*this, adjustedAvailableSpace, aFloat, floatMargin,
                        floatOffsets, false, reflowStatus);
    floatMarginISize = aFloat->ISize(wm) + floatMargin.IStartEnd(wm);
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
        floatAvailableSpace.mRect.BSize(wm) <= 0 &&
        !mustPlaceFloat) {
      
      PushFloatPastBreak(aFloat);
      return false;
    }

    if (CanPlaceFloat(floatMarginISize, floatAvailableSpace)) {
      
      break;
    }

    
    if (NS_STYLE_DISPLAY_TABLE != floatDisplay->mDisplay ||
          eCompatibility_NavQuirks != mPresContext->CompatibilityMode() ) {

      mBCoord += floatAvailableSpace.mRect.BSize(wm);
      if (adjustedAvailableSpace.BSize(wm) != NS_UNCONSTRAINEDSIZE) {
        adjustedAvailableSpace.BSize(wm) -= floatAvailableSpace.mRect.BSize(wm);
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

      
      mBCoord += floatAvailableSpace.mRect.BSize(wm);
      
      
      floatAvailableSpace = GetFloatAvailableSpace(mBCoord);
      adjustedAvailableSpace = mBlock->AdjustFloatAvailableSpace(*this,
                                 floatAvailableSpace.mRect, aFloat);
      floatMarginISize = FloatMarginISize(mReflowState,
                                          adjustedAvailableSpace.ISize(wm),
                                          aFloat, offsets);
    }

    mustPlaceFloat = false;
  }

  

  
  

  
  
  
  LogicalPoint floatPos(wm);
  if ((NS_STYLE_FLOAT_LEFT == floatDisplay->mFloats) == wm.IsBidiLTR()) {
    floatPos.I(wm) = floatAvailableSpace.mRect.IStart(wm);
  }
  else {
    if (!keepFloatOnSameLine) {
      floatPos.I(wm) = floatAvailableSpace.mRect.IEnd(wm) - floatMarginISize;
    }
    else {
      
      
      
      floatPos.I(wm) = floatAvailableSpace.mRect.IStart(wm);
    }
  }
  
  
  
  
  
  floatPos.B(wm) = std::max(mBCoord, ContentBStart());

  
  
  if (!isLetter) {
    bool pushedDown = mBCoord != saveBCoord;
    mBlock->ReflowFloat(*this, adjustedAvailableSpace, aFloat, floatMargin,
                        floatOffsets, pushedDown, reflowStatus);
  }
  if (aFloat->GetPrevInFlow())
    floatMargin.BStart(wm) = 0;
  if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus))
    floatMargin.BEnd(wm) = 0;

  
  
  
  
  
  
  
  if ((ContentBSize() != NS_UNCONSTRAINEDSIZE &&
       !GetFlag(BRS_FLOAT_FRAGMENTS_INSIDE_COLUMN_ENABLED) &&
       adjustedAvailableSpace.BSize(wm) == NS_UNCONSTRAINEDSIZE &&
       !mustPlaceFloat &&
       aFloat->BSize(wm) + floatMargin.BStartEnd(wm) >
       ContentBEnd() - floatPos.B(wm)) ||
      NS_FRAME_IS_TRUNCATED(reflowStatus) ||
      NS_INLINE_IS_BREAK_BEFORE(reflowStatus)) {
    PushFloatPastBreak(aFloat);
    return false;
  }

  
  
  
  if (ContentBSize() != NS_UNCONSTRAINEDSIZE &&
      !mustPlaceFloat &&
      (!mReflowState.mFlags.mIsTopOfPage || floatPos.B(wm) > 0) &&
      NS_STYLE_PAGE_BREAK_AVOID == aFloat->StyleDisplay()->mBreakInside &&
      (!NS_FRAME_IS_FULLY_COMPLETE(reflowStatus) ||
       aFloat->BSize(wm) + floatMargin.BStartEnd(wm) >
       ContentBEnd() - floatPos.B(wm)) &&
      !aFloat->GetPrevInFlow()) {
    PushFloatPastBreak(aFloat);
    return false;
  }

  
  
  
  LogicalPoint origin(wm, floatMargin.IStart(wm) + floatPos.I(wm),
                      floatMargin.BStart(wm) + floatPos.B(wm));

  
  nsHTMLReflowState::ApplyRelativePositioning(aFloat, wm, floatOffsets,
                                              &origin, ContainerWidth());

  
  
  
  bool moved = aFloat->GetLogicalPosition(wm, ContainerWidth()) != origin;
  if (moved) {
    aFloat->SetPosition(wm, origin, ContainerWidth());
    nsContainerFrame::PositionFrameView(aFloat);
    nsContainerFrame::PositionChildViews(aFloat);
  }

  
  
  mFloatOverflowAreas.UnionWith(aFloat->GetOverflowAreas() +
                                aFloat->GetPosition());

  
  
  LogicalRect region =
    nsFloatManager::CalculateRegionFor(wm, aFloat, floatMargin,
                                       ContainerWidth());
  
  if (NS_FRAME_IS_NOT_COMPLETE(reflowStatus) &&
      (NS_UNCONSTRAINEDSIZE != ContentBSize())) {
    region.BSize(wm) = std::max(region.BSize(wm),
                                ContentBSize() - floatPos.B(wm));
  }
  DebugOnly<nsresult> rv = mFloatManager->AddFloat(aFloat, region, wm,
                                                   ContainerWidth());
  MOZ_ASSERT(NS_SUCCEEDED(rv), "bad float placement");
  
  nsFloatManager::StoreRegionFor(wm, aFloat, region, ContainerWidth());

  
  
  if (!region.IsEqualEdges(oldRegion)) {
    
    
    
    
    nscoord blockStart = std::min(region.BStart(wm), oldRegion.BStart(wm));
    nscoord blockEnd = std::max(region.BEnd(wm), oldRegion.BEnd(wm));
    mFloatManager->IncludeInDamage(wm, blockStart, blockEnd);
  }

  if (!NS_FRAME_IS_FULLY_COMPLETE(reflowStatus)) {
    mBlock->SplitFloat(*this, aFloat, reflowStatus);
  } else {
    MOZ_ASSERT(!aFloat->GetNextInFlow());
  }

#ifdef NOISY_FLOATMANAGER
  nscoord tI, tB;
  mFloatManager->GetTranslation(tI, tB);
  nsIFrame::ListTag(stdout, mBlock);
  printf(": FlowAndPlaceFloat: AddFloat: tIB=%d,%d (%d,%d) {%d,%d,%d,%d}\n",
         tI, tB, mFloatManagerI, mFloatManagerB,
         region.IStart(wm), region.BStart(wm),
         region.ISize(wm), region.BSize(wm));
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
    MOZ_ASSERT(aFloat->StyleDisplay()->mFloats == NS_STYLE_FLOAT_RIGHT,
               "unexpected float value");
    mFloatManager->SetPushedRightFloatPastBreak();
  }

  
  
  DebugOnly<nsresult> rv = mBlock->StealFrame(aFloat);
  NS_ASSERTION(NS_SUCCEEDED(rv), "StealFrame should succeed");
  AppendPushedFloatChain(aFloat);
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
  WritingMode wm = mReflowState.GetWritingMode();

  if (aBreakType != NS_STYLE_CLEAR_NONE) {
    newBCoord = mFloatManager->ClearFloats(newBCoord, aBreakType, aFlags);
  }

  if (aReplacedBlock) {
    for (;;) {
      nsFlowAreaRect floatAvailableSpace = GetFloatAvailableSpace(newBCoord);
      if (!floatAvailableSpace.mHasFloats) {
        
        
        
        break;
      }
      nsBlockFrame::ReplacedElementISizeToClear replacedISize =
        nsBlockFrame::ISizeToClearPastFloats(*this, floatAvailableSpace.mRect,
                                             aReplacedBlock);
      if (std::max(floatAvailableSpace.mRect.IStart(wm) -
                    mContentArea.IStart(wm),
                   replacedISize.marginIStart) +
            replacedISize.borderBoxISize +
            std::max(mContentArea.IEnd(wm) -
                     floatAvailableSpace.mRect.IEnd(wm),
                     replacedISize.marginIEnd) <=
          mContentArea.ISize(wm)) {
        break;
      }
      
      if (floatAvailableSpace.mRect.BSize(wm) > 0) {
        
        newBCoord += floatAvailableSpace.mRect.BSize(wm);
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

