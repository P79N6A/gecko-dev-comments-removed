








#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "nsLineLayout.h"

#include "SVGTextFrame.h"
#include "nsBlockFrame.h"
#include "nsStyleConsts.h"
#include "nsContainerFrame.h"
#include "nsFloatManager.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsLayoutUtils.h"
#include "nsTextFrame.h"
#include "nsStyleStructInlines.h"
#include "nsBidiPresUtils.h"
#include <algorithm>

#ifdef DEBUG
#undef  NOISY_INLINEDIR_ALIGN
#undef  NOISY_BLOCKDIR_ALIGN
#undef  REALLY_NOISY_BLOCKDIR_ALIGN
#undef  NOISY_REFLOW
#undef  REALLY_NOISY_REFLOW
#undef  NOISY_PUSHING
#undef  REALLY_NOISY_PUSHING
#undef  DEBUG_ADD_TEXT
#undef  NOISY_MAX_ELEMENT_SIZE
#undef  REALLY_NOISY_MAX_ELEMENT_SIZE
#undef  NOISY_CAN_PLACE_FRAME
#undef  NOISY_TRIM
#undef  REALLY_NOISY_TRIM
#endif

using namespace mozilla;



#define FIX_BUG_50257

nsLineLayout::nsLineLayout(nsPresContext* aPresContext,
                           nsFloatManager* aFloatManager,
                           const nsHTMLReflowState* aOuterReflowState,
                           const nsLineList::iterator* aLine)
  : mPresContext(aPresContext),
    mFloatManager(aFloatManager),
    mBlockReflowState(aOuterReflowState),
    mLastOptionalBreakContent(nullptr),
    mForceBreakContent(nullptr),
    mBlockRS(nullptr),
    mLastOptionalBreakPriority(gfxBreakPriority::eNoBreak),
    mLastOptionalBreakContentOffset(-1),
    mForceBreakContentOffset(-1),
    mMinLineBSize(0),
    mTextIndent(0),
    mFirstLetterStyleOK(false),
    mIsTopOfPage(false),
    mImpactedByFloats(false),
    mLastFloatWasLetterFrame(false),
    mLineIsEmpty(false),
    mLineEndsInBR(false),
    mNeedBackup(false),
    mInFirstLine(false),
    mGotLineBox(false),
    mInFirstLetter(false),
    mHasBullet(false),
    mDirtyNextLine(false),
    mLineAtStart(false)
{
  MOZ_ASSERT(aOuterReflowState, "aOuterReflowState must not be null");
  NS_ASSERTION(aFloatManager || aOuterReflowState->frame->GetType() ==
                                  nsGkAtoms::letterFrame,
               "float manager should be present");
  MOZ_COUNT_CTOR(nsLineLayout);

  
  nsBlockFrame* blockFrame = do_QueryFrame(aOuterReflowState->frame);
  if (blockFrame)
    mStyleText = blockFrame->StyleTextForLineLayout();
  else
    mStyleText = aOuterReflowState->frame->StyleText();

  mLineNumber = 0;
  mTotalPlacedFrames = 0;
  mBStartEdge = 0;
  mTrimmableWidth = 0;

  mInflationMinFontSize =
    nsLayoutUtils::InflationMinFontSizeFor(aOuterReflowState->frame);

  
  
  
  
  PL_INIT_ARENA_POOL(&mArena, "nsLineLayout", 1024);
  mFrameFreeList = nullptr;
  mSpanFreeList = nullptr;

  mCurrentSpan = mRootSpan = nullptr;
  mSpanDepth = 0;

  if (aLine) {
    mGotLineBox = true;
    mLineBox = *aLine;
  }
}

nsLineLayout::~nsLineLayout()
{
  MOZ_COUNT_DTOR(nsLineLayout);

  NS_ASSERTION(nullptr == mRootSpan, "bad line-layout user");

  PL_FinishArenaPool(&mArena);
}



inline bool
HasPrevInFlow(nsIFrame *aFrame)
{
  nsIFrame *prevInFlow = aFrame->GetPrevInFlow();
  return prevInFlow != nullptr;
}

void
nsLineLayout::BeginLineReflow(nscoord aICoord, nscoord aBCoord,
                              nscoord aISize, nscoord aBSize,
                              bool aImpactedByFloats,
                              bool aIsTopOfPage,
                              WritingMode aWritingMode,
                              nscoord aContainerWidth)
{
  NS_ASSERTION(nullptr == mRootSpan, "bad linelayout user");
  NS_WARN_IF_FALSE(aISize != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
#ifdef DEBUG
  if ((aISize != NS_UNCONSTRAINEDSIZE) && CRAZY_SIZE(aISize)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": Init: bad caller: width WAS %d(0x%x)\n",
           aISize, aISize);
  }
  if ((aBSize != NS_UNCONSTRAINEDSIZE) && CRAZY_SIZE(aBSize)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": Init: bad caller: height WAS %d(0x%x)\n",
           aBSize, aBSize);
  }
#endif
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": BeginLineReflow: %d,%d,%d,%d impacted=%s %s\n",
         aICoord, aBCoord, aISize, aBSize,
         aImpactedByFloats?"true":"false",
         aIsTopOfPage ? "top-of-page" : "");
#endif
#ifdef DEBUG
  mSpansAllocated = mSpansFreed = mFramesAllocated = mFramesFreed = 0;
#endif

  mFirstLetterStyleOK = false;
  mIsTopOfPage = aIsTopOfPage;
  mImpactedByFloats = aImpactedByFloats;
  mTotalPlacedFrames = 0;
  mLineIsEmpty = true;
  mLineAtStart = true;
  mLineEndsInBR = false;
  mSpanDepth = 0;
  mMaxStartBoxBSize = mMaxEndBoxBSize = 0;

  if (mGotLineBox) {
    mLineBox->ClearHasBullet();
  }

  PerSpanData* psd = NewPerSpanData();
  mCurrentSpan = mRootSpan = psd;
  psd->mReflowState = mBlockReflowState;
  psd->mIStart = aICoord;
  psd->mICoord = aICoord;
  psd->mIEnd = aICoord + aISize;
  mContainerWidth = aContainerWidth;

  
  
  if (!(LineContainerFrame()->GetStateBits() &
        NS_FRAME_IN_CONSTRAINED_HEIGHT)) {

    
    
    
    nscoord maxLineBoxWidth =
      LineContainerFrame()->PresContext()->PresShell()->MaxLineBoxWidth();

    if (maxLineBoxWidth > 0 &&
        psd->mIEnd - psd->mIStart > maxLineBoxWidth) {
      psd->mIEnd = psd->mIStart + maxLineBoxWidth;
    }
  }

  mBStartEdge = aBCoord;

  psd->mNoWrap =
    !mStyleText->WhiteSpaceCanWrapStyle() || LineContainerFrame()->IsSVGText();
  psd->mWritingMode = aWritingMode;

  
  

  if (0 == mLineNumber && !HasPrevInFlow(mBlockReflowState->frame)) {
    const nsStyleCoord &textIndent = mStyleText->mTextIndent;
    nscoord pctBasis = 0;
    if (textIndent.HasPercent()) {
      pctBasis =
        nsHTMLReflowState::GetContainingBlockContentWidth(mBlockReflowState);

      if (mGotLineBox) {
        mLineBox->DisableResizeReflowOptimization();
      }
    }
    nscoord indent = nsRuleNode::ComputeCoordPercentCalc(textIndent, pctBasis);

    mTextIndent = indent;

    psd->mICoord += indent;
  }
}

void
nsLineLayout::EndLineReflow()
{
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": EndLineReflow: width=%d\n", mRootSpan->mICoord - mRootSpan->mIStart);
#endif

  FreeSpan(mRootSpan);
  mCurrentSpan = mRootSpan = nullptr;

  NS_ASSERTION(mSpansAllocated == mSpansFreed, "leak");
  NS_ASSERTION(mFramesAllocated == mFramesFreed, "leak");

#if 0
  static int32_t maxSpansAllocated = NS_LINELAYOUT_NUM_SPANS;
  static int32_t maxFramesAllocated = NS_LINELAYOUT_NUM_FRAMES;
  if (mSpansAllocated > maxSpansAllocated) {
    printf("XXX: saw a line with %d spans\n", mSpansAllocated);
    maxSpansAllocated = mSpansAllocated;
  }
  if (mFramesAllocated > maxFramesAllocated) {
    printf("XXX: saw a line with %d frames\n", mFramesAllocated);
    maxFramesAllocated = mFramesAllocated;
  }
#endif
}






void
nsLineLayout::UpdateBand(const nsRect& aNewAvailSpace,
                         nsIFrame* aFloatFrame)
{
  WritingMode lineWM = mRootSpan->mWritingMode;
  LogicalRect availSpace(lineWM, aNewAvailSpace, mContainerWidth);
#ifdef REALLY_NOISY_REFLOW
  printf("nsLL::UpdateBand %d, %d, %d, %d, (logical %d, %d, %d, %d); frame=%p\n  will set mImpacted to true\n",
         aNewAvailSpace.x, aNewAvailSpace.y,
         aNewAvailSpace.width, aNewAvailSpace.height,
         availSpace.IStart(lineWM), availSpace.BStart(lineWM),
         availSpace.ISize(lineWM), availSpace.BSize(lineWM),
         aFloatFrame);
#endif
#ifdef DEBUG
  if ((availSpace.ISize(lineWM) != NS_UNCONSTRAINEDSIZE) &&
      CRAZY_SIZE(availSpace.ISize(lineWM))) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": UpdateBand: bad caller: ISize WAS %d(0x%x)\n",
           availSpace.ISize(lineWM), availSpace.ISize(lineWM));
  }
  if ((availSpace.BSize(lineWM) != NS_UNCONSTRAINEDSIZE) &&
      CRAZY_SIZE(availSpace.BSize(lineWM))) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": UpdateBand: bad caller: BSize WAS %d(0x%x)\n",
           availSpace.BSize(lineWM), availSpace.BSize(lineWM));
  }
#endif

  
  NS_WARN_IF_FALSE(mRootSpan->mIEnd != NS_UNCONSTRAINEDSIZE &&
                   aNewAvailSpace.width != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  
  nscoord deltaICoord = availSpace.IStart(lineWM) - mRootSpan->mIStart;
  
  
  nscoord deltaISize = availSpace.ISize(lineWM) -
                       (mRootSpan->mIEnd - mRootSpan->mIStart);
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": UpdateBand: %d,%d,%d,%d deltaISize=%d deltaICoord=%d\n",
         aNewAvailSpace.x, aNewAvailSpace.y,
         aNewAvailSpace.width, aNewAvailSpace.height, deltaISize, deltaICoord);
#endif

  
  mRootSpan->mIStart += deltaICoord;
  mRootSpan->mIEnd += deltaICoord;
  mRootSpan->mICoord += deltaICoord;

  
  
  for (PerSpanData* psd = mCurrentSpan; psd; psd = psd->mParent) {
    psd->mIEnd += deltaISize;
    psd->mContainsFloat = true;
#ifdef NOISY_REFLOW
    printf("  span %p: oldIEnd=%d newIEnd=%d\n",
           psd, psd->mIEnd - deltaISize, psd->mIEnd);
#endif
  }
  NS_ASSERTION(mRootSpan->mContainsFloat &&
               mRootSpan->mIStart == availSpace.IStart(lineWM) &&
               mRootSpan->mIEnd == availSpace.IEnd(lineWM),
               "root span was updated incorrectly?");

  
  
  
  
  
  if (deltaICoord != 0) {
    for (PerFrameData* pfd = mRootSpan->mFirstFrame; pfd; pfd = pfd->mNext) {
      pfd->mBounds.IStart(lineWM) += deltaICoord;
    }
  }

  mBStartEdge = availSpace.BStart(lineWM);
  mImpactedByFloats = true;

  mLastFloatWasLetterFrame = nsGkAtoms::letterFrame == aFloatFrame->GetType();
}

nsLineLayout::PerSpanData*
nsLineLayout::NewPerSpanData()
{
  PerSpanData* psd = mSpanFreeList;
  if (!psd) {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &mArena, sizeof(PerSpanData));
    if (!mem) {
      NS_RUNTIMEABORT("OOM");
    }
    psd = reinterpret_cast<PerSpanData*>(mem);
  }
  else {
    mSpanFreeList = psd->mNextFreeSpan;
  }
  psd->mParent = nullptr;
  psd->mFrame = nullptr;
  psd->mFirstFrame = nullptr;
  psd->mLastFrame = nullptr;
  psd->mContainsFloat = false;
  psd->mZeroEffectiveSpanBox = false;
  psd->mHasNonemptyContent = false;

#ifdef DEBUG
  mSpansAllocated++;
#endif
  return psd;
}

void
nsLineLayout::BeginSpan(nsIFrame* aFrame,
                        const nsHTMLReflowState* aSpanReflowState,
                        nscoord aIStart, nscoord aIEnd,
                        nscoord* aBaseline)
{
  NS_ASSERTION(aIEnd != NS_UNCONSTRAINEDSIZE,
               "should no longer be using unconstrained sizes");
#ifdef NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth+1);
  nsFrame::ListTag(stdout, aFrame);
  printf(": BeginSpan leftEdge=%d rightEdge=%d\n", aIStart, aIEnd);
#endif

  PerSpanData* psd = NewPerSpanData();
  
  PerFrameData* pfd = mCurrentSpan->mLastFrame;
  NS_ASSERTION(pfd->mFrame == aFrame, "huh?");
  pfd->mSpan = psd;

  
  psd->mFrame = pfd;
  psd->mParent = mCurrentSpan;
  psd->mReflowState = aSpanReflowState;
  psd->mIStart = aIStart;
  psd->mICoord = aIStart;
  psd->mIEnd = aIEnd;
  psd->mBaseline = aBaseline;

  nsIFrame* frame = aSpanReflowState->frame;
  psd->mNoWrap = !frame->StyleText()->WhiteSpaceCanWrap(frame);
  psd->mWritingMode = aSpanReflowState->GetWritingMode();

  
  mCurrentSpan = psd;
  mSpanDepth++;
}

nscoord
nsLineLayout::EndSpan(nsIFrame* aFrame)
{
  NS_ASSERTION(mSpanDepth > 0, "end-span without begin-span");
#ifdef NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth);
  nsFrame::ListTag(stdout, aFrame);
  printf(": EndSpan width=%d\n", mCurrentSpan->mICoord - mCurrentSpan->mIStart);
#endif
  PerSpanData* psd = mCurrentSpan;
  nscoord iSizeResult = psd->mLastFrame ? (psd->mICoord - psd->mIStart) : 0;

  mSpanDepth--;
  mCurrentSpan->mReflowState = nullptr;  
  mCurrentSpan = mCurrentSpan->mParent;
  return iSizeResult;
}

int32_t
nsLineLayout::GetCurrentSpanCount() const
{
  NS_ASSERTION(mCurrentSpan == mRootSpan, "bad linelayout user");
  int32_t count = 0;
  PerFrameData* pfd = mRootSpan->mFirstFrame;
  while (nullptr != pfd) {
    count++;
    pfd = pfd->mNext;
  }
  return count;
}

void
nsLineLayout::SplitLineTo(int32_t aNewCount)
{
  NS_ASSERTION(mCurrentSpan == mRootSpan, "bad linelayout user");

#ifdef REALLY_NOISY_PUSHING
  printf("SplitLineTo %d (current count=%d); before:\n", aNewCount,
         GetCurrentSpanCount());
  DumpPerSpanData(mRootSpan, 1);
#endif
  PerSpanData* psd = mRootSpan;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    if (--aNewCount == 0) {
      
      PerFrameData* next = pfd->mNext;
      pfd->mNext = nullptr;
      psd->mLastFrame = pfd;

      
      pfd = next;
      while (nullptr != pfd) {
        next = pfd->mNext;
        pfd->mNext = mFrameFreeList;
        mFrameFreeList = pfd;
#ifdef DEBUG
        mFramesFreed++;
#endif
        if (nullptr != pfd->mSpan) {
          FreeSpan(pfd->mSpan);
        }
        pfd = next;
      }
      break;
    }
    pfd = pfd->mNext;
  }
#ifdef NOISY_PUSHING
  printf("SplitLineTo %d (current count=%d); after:\n", aNewCount,
         GetCurrentSpanCount());
  DumpPerSpanData(mRootSpan, 1);
#endif
}

void
nsLineLayout::PushFrame(nsIFrame* aFrame)
{
  PerSpanData* psd = mCurrentSpan;
  NS_ASSERTION(psd->mLastFrame->mFrame == aFrame, "pushing non-last frame");

#ifdef REALLY_NOISY_PUSHING
  nsFrame::IndentBy(stdout, mSpanDepth);
  printf("PushFrame %p, before:\n", psd);
  DumpPerSpanData(psd, 1);
#endif

  
  PerFrameData* pfd = psd->mLastFrame;
  if (pfd == psd->mFirstFrame) {
    
    psd->mFirstFrame = nullptr;
    psd->mLastFrame = nullptr;
  }
  else {
    PerFrameData* prevFrame = pfd->mPrev;
    prevFrame->mNext = nullptr;
    psd->mLastFrame = prevFrame;
  }

  
  pfd->mNext = mFrameFreeList;
  mFrameFreeList = pfd;
#ifdef DEBUG
  mFramesFreed++;
#endif
  if (nullptr != pfd->mSpan) {
    FreeSpan(pfd->mSpan);
  }
#ifdef NOISY_PUSHING
  nsFrame::IndentBy(stdout, mSpanDepth);
  printf("PushFrame: %p after:\n", psd);
  DumpPerSpanData(psd, 1);
#endif
}

void
nsLineLayout::FreeSpan(PerSpanData* psd)
{
  
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    if (nullptr != pfd->mSpan) {
      FreeSpan(pfd->mSpan);
    }
    PerFrameData* next = pfd->mNext;
    pfd->mNext = mFrameFreeList;
    mFrameFreeList = pfd;
#ifdef DEBUG
    mFramesFreed++;
#endif
    pfd = next;
  }

  
  psd->mNextFreeSpan = mSpanFreeList;
  mSpanFreeList = psd;
#ifdef DEBUG
  mSpansFreed++;
#endif
}

bool
nsLineLayout::IsZeroBSize()
{
  PerSpanData* psd = mCurrentSpan;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    if (0 != pfd->mBounds.BSize(psd->mWritingMode)) {
      return false;
    }
    pfd = pfd->mNext;
  }
  return true;
}

nsLineLayout::PerFrameData*
nsLineLayout::NewPerFrameData(nsIFrame* aFrame)
{
  PerFrameData* pfd = mFrameFreeList;
  if (!pfd) {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &mArena, sizeof(PerFrameData));
    if (!mem) {
      NS_RUNTIMEABORT("OOM");
    }
    pfd = reinterpret_cast<PerFrameData*>(mem);
  }
  else {
    mFrameFreeList = pfd->mNext;
  }
  pfd->mSpan = nullptr;
  pfd->mNext = nullptr;
  pfd->mPrev = nullptr;
  pfd->mFlags = 0;  
  pfd->mFrame = aFrame;

  WritingMode frameWM = aFrame->GetWritingMode();
  WritingMode lineWM = mRootSpan->mWritingMode;
  pfd->mBounds = LogicalRect(lineWM);
  pfd->mMargin = LogicalMargin(frameWM);
  pfd->mBorderPadding = LogicalMargin(frameWM);
  pfd->mOffsets = LogicalMargin(frameWM);

#ifdef DEBUG
  pfd->mBlockDirAlign = 0xFF;
  mFramesAllocated++;
#endif
  return pfd;
}

bool
nsLineLayout::LineIsBreakable() const
{
  
  
  if ((0 != mTotalPlacedFrames) || mImpactedByFloats) {
    return true;
  }
  return false;
}





static bool
HasPercentageUnitSide(const nsStyleSides& aSides)
{
  NS_FOR_CSS_SIDES(side) {
    if (aSides.Get(side).HasPercent())
      return true;
  }
  return false;
}

static bool
IsPercentageAware(const nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "null frame is not allowed");

  nsIAtom *fType = aFrame->GetType();
  if (fType == nsGkAtoms::textFrame) {
    
    return false;
  }

  
  
  
  

  const nsStyleMargin* margin = aFrame->StyleMargin();
  if (HasPercentageUnitSide(margin->mMargin)) {
    return true;
  }

  const nsStylePadding* padding = aFrame->StylePadding();
  if (HasPercentageUnitSide(padding->mPadding)) {
    return true;
  }

  

  const nsStylePosition* pos = aFrame->StylePosition();

  if ((pos->WidthDependsOnContainer() &&
       pos->mWidth.GetUnit() != eStyleUnit_Auto) ||
      pos->MaxWidthDependsOnContainer() ||
      pos->MinWidthDependsOnContainer() ||
      pos->OffsetHasPercent(NS_SIDE_RIGHT) ||
      pos->OffsetHasPercent(NS_SIDE_LEFT)) {
    return true;
  }

  if (eStyleUnit_Auto == pos->mWidth.GetUnit()) {
    
    
    const nsStyleDisplay* disp = aFrame->StyleDisplay();
    if (disp->mDisplay == NS_STYLE_DISPLAY_INLINE_BLOCK ||
        disp->mDisplay == NS_STYLE_DISPLAY_INLINE_TABLE ||
        fType == nsGkAtoms::HTMLButtonControlFrame ||
        fType == nsGkAtoms::gfxButtonControlFrame ||
        fType == nsGkAtoms::fieldSetFrame ||
        fType == nsGkAtoms::comboboxDisplayFrame) {
      return true;
    }

    
    
    
    
    
    
    
    nsIFrame *f = const_cast<nsIFrame*>(aFrame);
    if (f->GetIntrinsicRatio() != nsSize(0, 0) &&
        
        pos->mHeight.GetUnit() != eStyleUnit_Coord) {
      const IntrinsicSize &intrinsicSize = f->GetIntrinsicSize();
      if (intrinsicSize.width.GetUnit() == eStyleUnit_None &&
          intrinsicSize.height.GetUnit() == eStyleUnit_None) {
        return true;
      }
    }
  }

  return false;
}

void
nsLineLayout::ReflowFrame(nsIFrame* aFrame,
                          nsReflowStatus& aReflowStatus,
                          nsHTMLReflowMetrics* aMetrics,
                          bool& aPushedFrame)
{
  
  aPushedFrame = false;

  PerFrameData* pfd = NewPerFrameData(aFrame);
  PerSpanData* psd = mCurrentSpan;
  psd->AppendFrame(pfd);

#ifdef REALLY_NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth);
  printf("%p: Begin ReflowFrame pfd=%p ", psd, pfd);
  nsFrame::ListTag(stdout, aFrame);
  printf("\n");
#endif

  if (mCurrentSpan == mRootSpan) {
    pfd->mFrame->Properties().Remove(nsIFrame::LineBaselineOffset());
  } else {
#ifdef DEBUG
    bool hasLineOffset;
    pfd->mFrame->Properties().Get(nsIFrame::LineBaselineOffset(), &hasLineOffset);
    NS_ASSERTION(!hasLineOffset, "LineBaselineOffset was set but was not expected");
#endif
  }

  mTextJustificationNumSpaces = 0;
  mTextJustificationNumLetters = 0;

  
  
  WritingMode frameWM = aFrame->GetWritingMode();
  WritingMode lineWM = mRootSpan->mWritingMode;

  
  
  
  
  
  pfd->mBounds.IStart(lineWM) = psd->mICoord;
  pfd->mBounds.BStart(lineWM) = mBStartEdge;

  
  
  
  
  
  
  
  
  
  
  bool notSafeToBreak = LineIsEmpty() && !mImpactedByFloats;

  
  nsIAtom* frameType = aFrame->GetType();
  bool isText = frameType == nsGkAtoms::textFrame;
  
  
  
  
  NS_WARN_IF_FALSE(psd->mIEnd != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  nscoord availableSpaceOnLine = psd->mIEnd - psd->mICoord;

  
  Maybe<nsHTMLReflowState> reflowStateHolder;
  if (!isText) {
    
    
    
    LogicalSize availSize = mBlockReflowState->ComputedSize(frameWM);
    availSize.BSize(frameWM) = NS_UNCONSTRAINEDSIZE;
    reflowStateHolder.emplace(mPresContext, *psd->mReflowState,
                              aFrame, availSize);
    nsHTMLReflowState& reflowState = *reflowStateHolder;
    reflowState.mLineLayout = this;
    reflowState.mFlags.mIsTopOfPage = mIsTopOfPage;
    if (reflowState.ComputedWidth() == NS_UNCONSTRAINEDSIZE)
      reflowState.AvailableWidth() = availableSpaceOnLine;
    WritingMode stateWM = reflowState.GetWritingMode();
    pfd->mMargin =
      reflowState.ComputedLogicalMargin().ConvertTo(frameWM, stateWM);
    pfd->mBorderPadding =
      reflowState.ComputedLogicalBorderPadding().ConvertTo(frameWM, stateWM);
    pfd->SetFlag(PFD_RELATIVEPOS,
                 reflowState.mStyleDisplay->IsRelativelyPositionedStyle());
    if (pfd->GetFlag(PFD_RELATIVEPOS)) {
      pfd->mOffsets =
        reflowState.ComputedLogicalOffsets().ConvertTo(frameWM, stateWM);
    }

    
    
    
    
    AllowForStartMargin(pfd, reflowState);
  }
  
  

  
  
  
  
  
  
  
  if (mGotLineBox && IsPercentageAware(aFrame)) {
    mLineBox->DisableResizeReflowOptimization();
  }

  
  
  
  aFrame->WillReflow(mPresContext);

  
  nsHTMLReflowMetrics metrics(lineWM);
#ifdef DEBUG
  metrics.ISize(lineWM) = nscoord(0xdeadbeef);
  metrics.BSize(lineWM) = nscoord(0xdeadbeef);
#endif
  nsRect physicalBounds = pfd->mBounds.GetPhysicalRect(lineWM, mContainerWidth);
  nscoord tx = physicalBounds.x;
  nscoord ty = physicalBounds.y;
  mFloatManager->Translate(tx, ty);

  int32_t savedOptionalBreakOffset;
  gfxBreakPriority savedOptionalBreakPriority;
  nsIContent* savedOptionalBreakContent =
    GetLastOptionalBreakPosition(&savedOptionalBreakOffset,
                                 &savedOptionalBreakPriority);

  if (!isText) {
    aFrame->Reflow(mPresContext, metrics, *reflowStateHolder, aReflowStatus);
  } else {
    static_cast<nsTextFrame*>(aFrame)->
      ReflowText(*this, availableSpaceOnLine, psd->mReflowState->rendContext,
                 metrics, aReflowStatus);
  }
  
  pfd->mJustificationNumSpaces = mTextJustificationNumSpaces;
  pfd->mJustificationNumLetters = mTextJustificationNumLetters;

  
  
  
  bool placedFloat = false;
  bool isEmpty;
  if (!frameType) {
    isEmpty = pfd->mFrame->IsEmpty();
  } else {
    if (nsGkAtoms::placeholderFrame == frameType) {
      isEmpty = true;
      pfd->SetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE, true);
      nsIFrame* outOfFlowFrame = nsLayoutUtils::GetFloatFromPlaceholder(aFrame);
      if (outOfFlowFrame) {
        
        
        
        nscoord availableWidth = psd->mIEnd - (psd->mICoord - mTrimmableWidth);
        if (psd->mNoWrap) {
          
          
          
          
          
          
          
          
          
          availableWidth = 0;
        }
        placedFloat = AddFloat(outOfFlowFrame, availableWidth);
        NS_ASSERTION(!(outOfFlowFrame->GetType() == nsGkAtoms::letterFrame &&
                       GetFirstLetterStyleOK()),
                    "FirstLetterStyle set on line with floating first letter");
      }
    }
    else if (isText) {
      
      pfd->SetFlag(PFD_ISTEXTFRAME, true);
      nsTextFrame* textFrame = static_cast<nsTextFrame*>(pfd->mFrame);
      isEmpty = !textFrame->HasNoncollapsedCharacters();
      if (!isEmpty) {
        pfd->SetFlag(PFD_ISNONEMPTYTEXTFRAME, true);
        nsIContent* content = textFrame->GetContent();

        const nsTextFragment* frag = content->GetText();
        if (frag) {
          pfd->SetFlag(PFD_ISNONWHITESPACETEXTFRAME,
                       !content->TextIsOnlyWhitespace());
        }
      }
    }
    else if (nsGkAtoms::brFrame == frameType) {
      pfd->SetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE, true);
      isEmpty = false;
    } else {
      if (nsGkAtoms::letterFrame==frameType) {
        pfd->SetFlag(PFD_ISLETTERFRAME, true);
      }
      if (pfd->mSpan) {
        isEmpty = !pfd->mSpan->mHasNonemptyContent && pfd->mFrame->IsSelfEmpty();
      } else {
        isEmpty = pfd->mFrame->IsEmpty();
      }
    }
  }

  mFloatManager->Translate(-tx, -ty);

  NS_ASSERTION(metrics.ISize(lineWM) >= 0, "bad inline size");
  NS_ASSERTION(metrics.BSize(lineWM) >= 0,"bad block size");
  if (metrics.ISize(lineWM) < 0) {
    metrics.ISize(lineWM) = 0;
  }
  if (metrics.BSize(lineWM) < 0) {
    metrics.BSize(lineWM) = 0;
  }

#ifdef DEBUG
  
  
  if (!NS_INLINE_IS_BREAK_BEFORE(aReflowStatus)) {
    if (CRAZY_SIZE(metrics.ISize(lineWM)) ||
        CRAZY_SIZE(metrics.BSize(lineWM))) {
      printf("nsLineLayout: ");
      nsFrame::ListTag(stdout, aFrame);
      printf(" metrics=%d,%d!\n", metrics.Width(), metrics.Height());
    }
    if ((metrics.Width() == nscoord(0xdeadbeef)) ||
        (metrics.Height() == nscoord(0xdeadbeef))) {
      printf("nsLineLayout: ");
      nsFrame::ListTag(stdout, aFrame);
      printf(" didn't set w/h %d,%d!\n", metrics.Width(), metrics.Height());
    }
  }
#endif

  
  
  
  
  
  pfd->mOverflowAreas = metrics.mOverflowAreas;

  pfd->mBounds.ISize(lineWM) = metrics.ISize(lineWM);
  pfd->mBounds.BSize(lineWM) = metrics.BSize(lineWM);

  
  aFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);

  
  aFrame->DidReflow(mPresContext,
                    isText ? nullptr : reflowStateHolder.ptr(),
                    nsDidReflowStatus::FINISHED);

  if (aMetrics) {
    *aMetrics = metrics;
  }

  if (!NS_INLINE_IS_BREAK_BEFORE(aReflowStatus)) {
    
    
    
    
    if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
      nsIFrame* kidNextInFlow = aFrame->GetNextInFlow();
      if (nullptr != kidNextInFlow) {
        
        
        
        kidNextInFlow->GetParent()->
          DeleteNextInFlowChild(kidNextInFlow, true);
      }
    }

    
    
    bool continuingTextRun = aFrame->CanContinueTextRun();
    
    
    if (!continuingTextRun && !pfd->GetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE)) {
      mTrimmableWidth = 0;
    }

    
    
    bool optionalBreakAfterFits;
    NS_ASSERTION(isText ||
                 !reflowStateHolder->IsFloating(),
                 "How'd we get a floated inline frame? "
                 "The frame ctor should've dealt with this.");
    if (CanPlaceFrame(pfd, notSafeToBreak, continuingTextRun,
                      savedOptionalBreakContent != nullptr, metrics,
                      aReflowStatus, &optionalBreakAfterFits)) {
      if (!isEmpty) {
        psd->mHasNonemptyContent = true;
        mLineIsEmpty = false;
        if (!pfd->mSpan) {
          
          mLineAtStart = false;
        }
      }

      
      
      
      PlaceFrame(pfd, metrics);
      PerSpanData* span = pfd->mSpan;
      if (span) {
        
        
        
        VerticalAlignFrames(span);
      }
      
      if (!continuingTextRun) {
        if (!psd->mNoWrap && (!LineIsEmpty() || placedFloat)) {
          
          
          
          if (NotifyOptionalBreakPosition(aFrame->GetContent(), INT32_MAX, optionalBreakAfterFits, gfxBreakPriority::eNormalBreak)) {
            
            aReflowStatus = NS_INLINE_LINE_BREAK_AFTER(aReflowStatus);
          }
        }
      }
    }
    else {
      PushFrame(aFrame);
      aPushedFrame = true;
      
      
      RestoreSavedBreakPosition(savedOptionalBreakContent,
                                savedOptionalBreakOffset,
                                savedOptionalBreakPriority);
    }
  }
  else {
    PushFrame(aFrame);
  }
  
#ifdef REALLY_NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth);
  printf("End ReflowFrame ");
  nsFrame::ListTag(stdout, aFrame);
  printf(" status=%x\n", aReflowStatus);
#endif
}

void
nsLineLayout::AllowForStartMargin(PerFrameData* pfd,
                                  nsHTMLReflowState& aReflowState)
{
  NS_ASSERTION(!aReflowState.IsFloating(),
               "How'd we get a floated inline frame? "
               "The frame ctor should've dealt with this.");

  WritingMode frameWM = pfd->mFrame->GetWritingMode();

  
  
  
  
  
  
  
  if ((pfd->mFrame->GetPrevContinuation() ||
       pfd->mFrame->FrameIsNonFirstInIBSplit()) &&
      aReflowState.mStyleBorder->mBoxDecorationBreak ==
        NS_STYLE_BOX_DECORATION_BREAK_SLICE) {
    
    
    pfd->mMargin.IStart(frameWM) = 0;
  } else {
    NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aReflowState.AvailableWidth(),
                     "have unconstrained width; this should only result from "
                     "very large sizes, not attempts at intrinsic width "
                     "calculation");
    if (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedWidth()) {
      
      
      
      
      aReflowState.AvailableWidth() -= pfd->mMargin.IStart(frameWM);
    }
  }
}

nscoord
nsLineLayout::GetCurrentFrameInlineDistanceFromBlock()
{
  PerSpanData* psd;
  nscoord x = 0;
  for (psd = mCurrentSpan; psd; psd = psd->mParent) {
    x += psd->mICoord;
  }
  return x;
}











bool
nsLineLayout::CanPlaceFrame(PerFrameData* pfd,
                            bool aNotSafeToBreak,
                            bool aFrameCanContinueTextRun,
                            bool aCanRollBackBeforeFrame,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus,
                            bool* aOptionalBreakAfterFits)
{
  NS_PRECONDITION(pfd && pfd->mFrame, "bad args, null pointers for frame data");

  *aOptionalBreakAfterFits = true;

  WritingMode frameWM = pfd->mFrame->GetWritingMode();
  WritingMode lineWM = mRootSpan->mWritingMode;
  
















  if ((NS_FRAME_IS_NOT_COMPLETE(aStatus) ||
       pfd->mFrame->LastInFlow()->GetNextContinuation() ||
       pfd->mFrame->FrameIsNonLastInIBSplit()) &&
      !pfd->GetFlag(PFD_ISLETTERFRAME) &&
      pfd->mFrame->StyleBorder()->mBoxDecorationBreak ==
        NS_STYLE_BOX_DECORATION_BREAK_SLICE) {
    pfd->mMargin.IEnd(frameWM) = 0;
  }

  
  
  LogicalMargin usedMargins = pfd->mMargin.ConvertTo(lineWM, frameWM);
  nscoord startMargin = usedMargins.IStart(lineWM);
  nscoord endMargin = usedMargins.IEnd(lineWM);

  pfd->mBounds.IStart(lineWM) += startMargin;

  PerSpanData* psd = mCurrentSpan;
  if (psd->mNoWrap) {
    
    return true;
  }

#ifdef NOISY_CAN_PLACE_FRAME
  if (nullptr != psd->mFrame) {
    nsFrame::ListTag(stdout, psd->mFrame->mFrame);
  }
  else {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
  } 
  printf(": aNotSafeToBreak=%s frame=", aNotSafeToBreak ? "true" : "false");
  nsFrame::ListTag(stdout, pfd->mFrame);
  printf(" frameWidth=%d, margins=%d,%d\n",
         pfd->mBounds.IEnd(lineWM) + endMargin - psd->mICoord,
         startMargin, endMargin);
#endif

  
  
  bool outside = pfd->mBounds.IEnd(lineWM) - mTrimmableWidth + endMargin >
                 psd->mIEnd;
  if (!outside) {
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> inside\n");
#endif
    return true;
  }
  *aOptionalBreakAfterFits = false;

  
  
  if (0 == startMargin + pfd->mBounds.ISize(lineWM) + endMargin) {
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> empty frame fits\n");
#endif
    return true;
  }

#ifdef FIX_BUG_50257
  
  if (nsGkAtoms::brFrame == pfd->mFrame->GetType()) {
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> BR frame fits\n");
#endif
    return true;
  }
#endif

  if (aNotSafeToBreak) {
    
    
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> not-safe and not-impacted fits: ");
    while (nullptr != psd) {
      printf("<psd=%p x=%d left=%d> ", psd, psd->mICoord, psd->mIStart);
      psd = psd->mParent;
    }
    printf("\n");
#endif
    return true;
  }
 
  
  if (pfd->mSpan && pfd->mSpan->mContainsFloat) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return true;
 }

  if (aFrameCanContinueTextRun) {
    
    
    
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> placing overflowing textrun, requesting backup\n");
#endif

    
    mNeedBackup = true;
    return true;
  }

#ifdef NOISY_CAN_PLACE_FRAME
  printf("   ==> didn't fit\n");
#endif
  aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  return false;
}




void
nsLineLayout::PlaceFrame(PerFrameData* pfd, nsHTMLReflowMetrics& aMetrics)
{
  WritingMode frameWM = pfd->mFrame->GetWritingMode();
  WritingMode lineWM = mRootSpan->mWritingMode;

  
  if (aMetrics.BlockStartAscent() == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    pfd->mAscent = pfd->mFrame->GetLogicalBaseline(lineWM);
  } else {
    pfd->mAscent = aMetrics.BlockStartAscent();
  }

  
  mCurrentSpan->mICoord = pfd->mBounds.IEnd(lineWM) +
                          pfd->mMargin.ConvertTo(lineWM, frameWM).IEnd(lineWM);

  
  if (pfd->mFrame->GetType() == nsGkAtoms::placeholderFrame) {
    NS_ASSERTION(pfd->mBounds.ISize(lineWM) == 0 &&
                 pfd->mBounds.BSize(lineWM) == 0,
                 "placeholders should have 0 width/height (checking "
                 "placeholders were never counted by the old code in "
                 "this function)");
  } else {
    mTotalPlacedFrames++;
  }
}

void
nsLineLayout::AddBulletFrame(nsIFrame* aFrame,
                             const nsHTMLReflowMetrics& aMetrics)
{
  NS_ASSERTION(mCurrentSpan == mRootSpan, "bad linelayout user");
  NS_ASSERTION(mGotLineBox, "must have line box");

  nsIFrame *blockFrame = mBlockReflowState->frame;
  NS_ASSERTION(blockFrame->IsFrameOfType(nsIFrame::eBlockFrame),
               "must be for block");
  if (!static_cast<nsBlockFrame*>(blockFrame)->BulletIsEmpty()) {
    mHasBullet = true;
    mLineBox->SetHasBullet();
  }

  WritingMode lineWM = mRootSpan->mWritingMode;
  PerFrameData* pfd = NewPerFrameData(aFrame);
  mRootSpan->AppendFrame(pfd);
  pfd->SetFlag(PFD_ISBULLET, true);
  if (aMetrics.BlockStartAscent() == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    pfd->mAscent = aFrame->GetLogicalBaseline(lineWM);
  } else {
    pfd->mAscent = aMetrics.BlockStartAscent();
  }

  
  pfd->mBounds = LogicalRect(lineWM, aFrame->GetRect(), mContainerWidth);
  pfd->mOverflowAreas = aMetrics.mOverflowAreas;
}

#ifdef DEBUG
void
nsLineLayout::DumpPerSpanData(PerSpanData* psd, int32_t aIndent)
{
  nsFrame::IndentBy(stdout, aIndent);
  printf("%p: left=%d x=%d right=%d\n", static_cast<void*>(psd),
         psd->mIStart, psd->mICoord, psd->mIEnd);
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    nsFrame::IndentBy(stdout, aIndent+1);
    nsFrame::ListTag(stdout, pfd->mFrame);
    nsRect rect = pfd->mBounds.GetPhysicalRect(psd->mWritingMode,
                                               mContainerWidth);
    printf(" %d,%d,%d,%d\n", rect.x, rect.y, rect.width, rect.height);
    if (pfd->mSpan) {
      DumpPerSpanData(pfd->mSpan, aIndent + 1);
    }
    pfd = pfd->mNext;
  }
}
#endif

#define VALIGN_OTHER  0
#define VALIGN_TOP    1
#define VALIGN_BOTTOM 2

void
nsLineLayout::VerticalAlignLine()
{
  
  PerFrameData rootPFD(mBlockReflowState->frame->GetWritingMode());
  rootPFD.mFrame = mBlockReflowState->frame;
  rootPFD.mAscent = 0;
  mRootSpan->mFrame = &rootPFD;

  
  
  
  PerSpanData* psd = mRootSpan;
  VerticalAlignFrames(psd);

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord lineBSize = psd->mMaxBCoord - psd->mMinBCoord;

  
  
  
  nscoord baselineBCoord;
  if (psd->mMinBCoord < 0) {
    baselineBCoord = mBStartEdge - psd->mMinBCoord;
  }
  else {
    baselineBCoord = mBStartEdge;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (lineBSize < mMaxEndBoxBSize) {
    
    nscoord extra = mMaxEndBoxBSize - lineBSize;
    baselineBCoord += extra;
    lineBSize = mMaxEndBoxBSize;
  }
  if (lineBSize < mMaxStartBoxBSize) {
    lineBSize = mMaxStartBoxBSize;
  }
#ifdef NOISY_BLOCKDIR_ALIGN
  printf("  [line]==> lineBSize=%d baselineBCoord=%d\n", lineBSize, baselineBCoord);
#endif

  
  
  
  
  
  WritingMode lineWM = psd->mWritingMode;
  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    if (pfd->mBlockDirAlign == VALIGN_OTHER) {
      pfd->mBounds.BStart(lineWM) += baselineBCoord;
      pfd->mFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
    }
  }
  PlaceTopBottomFrames(psd, -mBStartEdge, lineBSize);

  
  mLineBox->SetBounds(lineWM,
                      psd->mIStart, mBStartEdge,
                      psd->mICoord - psd->mIStart, lineBSize,
                      mContainerWidth);

  mFinalLineBSize = lineBSize;
  mLineBox->SetLogicalAscent(baselineBCoord - mBStartEdge);
#ifdef NOISY_BLOCKDIR_ALIGN
  printf(
    "  [line]==> bounds{x,y,w,h}={%d,%d,%d,%d} lh=%d a=%d\n",
    mLineBox->GetBounds().IStart(lineWM), mLineBox->GetBounds().BStart(lineWM),
    mLineBox->GetBounds().ISize(lineWM), mLineBox->GetBounds().BSize(lineWM),
    mFinalLineBSize, mLineBox->GetLogicalAscent());
#endif

  
  mRootSpan->mFrame = nullptr;
}


void
nsLineLayout::PlaceTopBottomFrames(PerSpanData* psd,
                                   nscoord aDistanceFromStart,
                                   nscoord aLineBSize)
{
  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    PerSpanData* span = pfd->mSpan;
#ifdef DEBUG
    NS_ASSERTION(0xFF != pfd->mBlockDirAlign, "umr");
#endif
    WritingMode frameWM = pfd->mFrame->GetWritingMode();
    WritingMode lineWM = mRootSpan->mWritingMode;
    switch (pfd->mBlockDirAlign) {
      case VALIGN_TOP:
        if (span) {
          pfd->mBounds.BStart(lineWM) = -aDistanceFromStart - span->mMinBCoord;
        }
        else {
          pfd->mBounds.BStart(lineWM) =
            -aDistanceFromStart + pfd->mMargin.BStart(frameWM);
        }
        pfd->mFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
#ifdef NOISY_BLOCKDIR_ALIGN
        printf("    ");
        nsFrame::ListTag(stdout, pfd->mFrame);
        printf(": y=%d dTop=%d [bp.top=%d topLeading=%d]\n",
               pfd->mBounds.BStart(lineWM), aDistanceFromStart,
               span ? pfd->mBorderPadding.BStart(frameWM) : 0,
               span ? span->mBStartLeading : 0);
#endif
        break;
      case VALIGN_BOTTOM:
        if (span) {
          
          pfd->mBounds.BStart(lineWM) =
            -aDistanceFromStart + aLineBSize - span->mMaxBCoord;
        }
        else {
          pfd->mBounds.BStart(lineWM) = -aDistanceFromStart + aLineBSize -
            pfd->mMargin.BEnd(frameWM) - pfd->mBounds.BSize(lineWM);
        }
        pfd->mFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
#ifdef NOISY_BLOCKDIR_ALIGN
        printf("    ");
        nsFrame::ListTag(stdout, pfd->mFrame);
        printf(": y=%d\n", pfd->mBounds.BStart(lineWM));
#endif
        break;
    }
    if (span) {
      nscoord fromStart = aDistanceFromStart + pfd->mBounds.BStart(lineWM);
      PlaceTopBottomFrames(span, fromStart, aLineBSize);
    }
  }
}

static float
GetInflationForBlockDirAlignment(nsIFrame* aFrame,
                                 nscoord aInflationMinFontSize)
{
  if (aFrame->IsSVGText()) {
    const nsIFrame* container =
      nsLayoutUtils::GetClosestFrameOfType(aFrame, nsGkAtoms::svgTextFrame);
    NS_ASSERTION(container, "expected to find an ancestor SVGTextFrame");
    return
      static_cast<const SVGTextFrame*>(container)->GetFontSizeScaleFactor();
  }
  return nsLayoutUtils::FontSizeInflationInner(aFrame, aInflationMinFontSize);
}

#define BLOCKDIR_ALIGN_FRAMES_NO_MINIMUM nscoord_MAX
#define BLOCKDIR_ALIGN_FRAMES_NO_MAXIMUM nscoord_MIN






void
nsLineLayout::VerticalAlignFrames(PerSpanData* psd)
{
  
  PerFrameData* spanFramePFD = psd->mFrame;
  nsIFrame* spanFrame = spanFramePFD->mFrame;

  
  nsRefPtr<nsFontMetrics> fm;
  float inflation =
    GetInflationForBlockDirAlignment(spanFrame, mInflationMinFontSize);
  nsLayoutUtils::GetFontMetricsForFrame(spanFrame, getter_AddRefs(fm),
                                        inflation);
  mBlockReflowState->rendContext->SetFont(fm);

  bool preMode = mStyleText->WhiteSpaceIsSignificant();

  
  
  
  
  WritingMode frameWM = spanFramePFD->mFrame->GetWritingMode();
  WritingMode lineWM = mRootSpan->mWritingMode;
  bool emptyContinuation = psd != mRootSpan &&
    spanFrame->GetPrevInFlow() && !spanFrame->GetNextInFlow() &&
    spanFramePFD->mBounds.IsZeroSize();

#ifdef NOISY_BLOCKDIR_ALIGN
  printf("[%sSpan]", (psd == mRootSpan)?"Root":"");
  nsFrame::ListTag(stdout, spanFrame);
  printf(": preMode=%s strictMode=%s w/h=%d,%d emptyContinuation=%s",
         preMode ? "yes" : "no",
         mPresContext->CompatibilityMode() != eCompatibility_NavQuirks ? "yes" : "no",
         spanFramePFD->mBounds.ISize(lineWM),
         spanFramePFD->mBounds.BSize(lineWM),
         emptyContinuation ? "yes" : "no");
  if (psd != mRootSpan) {
    WritingMode frameWM = spanFramePFD->mFrame->GetWritingMode();
    printf(" bp=%d,%d,%d,%d margin=%d,%d,%d,%d",
           spanFramePFD->mBorderPadding.Top(frameWM),
           spanFramePFD->mBorderPadding.Right(frameWM),
           spanFramePFD->mBorderPadding.Bottom(frameWM),
           spanFramePFD->mBorderPadding.Left(frameWM),
           spanFramePFD->mMargin.Top(frameWM),
           spanFramePFD->mMargin.Right(frameWM),
           spanFramePFD->mMargin.Bottom(frameWM),
           spanFramePFD->mMargin.Left(frameWM));
  }
  printf("\n");
#endif

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool zeroEffectiveSpanBox = false;
  
  
  
  
  if ((emptyContinuation ||
       mPresContext->CompatibilityMode() != eCompatibility_FullStandards) &&
      ((psd == mRootSpan) ||
       (spanFramePFD->mBorderPadding.IsEmpty() &&
        spanFramePFD->mMargin.IsEmpty()))) {
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    zeroEffectiveSpanBox = true;
    for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      if (pfd->GetFlag(PFD_ISTEXTFRAME) &&
          (pfd->GetFlag(PFD_ISNONWHITESPACETEXTFRAME) || preMode ||
           pfd->mBounds.ISize(mRootSpan->mWritingMode) != 0)) {
        zeroEffectiveSpanBox = false;
        break;
      }
    }
  }
  psd->mZeroEffectiveSpanBox = zeroEffectiveSpanBox;

  
  nscoord baselineBCoord, minBCoord, maxBCoord;
  if (psd == mRootSpan) {
    
    
    
    
    
    
    baselineBCoord = 0;
    minBCoord = BLOCKDIR_ALIGN_FRAMES_NO_MINIMUM;
    maxBCoord = BLOCKDIR_ALIGN_FRAMES_NO_MAXIMUM;
#ifdef NOISY_BLOCKDIR_ALIGN
    printf("[RootSpan]");
    nsFrame::ListTag(stdout, spanFrame);
    printf(": pass1 valign frames: topEdge=%d minLineBSize=%d zeroEffectiveSpanBox=%s\n",
           mBStartEdge, mMinLineBSize,
           zeroEffectiveSpanBox ? "yes" : "no");
#endif
  }
  else {
    
    
    
    float inflation =
      GetInflationForBlockDirAlignment(spanFrame, mInflationMinFontSize);
    nscoord logicalBSize = nsHTMLReflowState::
      CalcLineHeight(spanFrame->GetContent(), spanFrame->StyleContext(),
                     mBlockReflowState->ComputedHeight(),
                     inflation);
    nscoord contentBSize = spanFramePFD->mBounds.BSize(lineWM) -
      spanFramePFD->mBorderPadding.BStart(frameWM) -
      spanFramePFD->mBorderPadding.BEnd(frameWM);

    
    
    if (spanFramePFD->GetFlag(PFD_ISLETTERFRAME) &&
        !spanFrame->GetPrevInFlow() &&
        spanFrame->StyleText()->mLineHeight.GetUnit() == eStyleUnit_Normal) {
      logicalBSize = spanFramePFD->mBounds.BSize(lineWM);
    }

    nscoord leading = logicalBSize - contentBSize;
    psd->mBStartLeading = leading / 2;
    psd->mBEndLeading = leading - psd->mBStartLeading;
    psd->mLogicalBSize = logicalBSize;

    if (zeroEffectiveSpanBox) {
      
      
      
      

      
      
      minBCoord = BLOCKDIR_ALIGN_FRAMES_NO_MINIMUM;
      maxBCoord = BLOCKDIR_ALIGN_FRAMES_NO_MAXIMUM;
    }
    else {

      
      
      
      
      
      minBCoord = spanFramePFD->mBorderPadding.BStart(frameWM) -
                  psd->mBStartLeading;
      maxBCoord = minBCoord + psd->mLogicalBSize;
    }

    
    
    
    *psd->mBaseline = baselineBCoord = spanFramePFD->mAscent;


#ifdef NOISY_BLOCKDIR_ALIGN
    printf("[%sSpan]", (psd == mRootSpan)?"Root":"");
    nsFrame::ListTag(stdout, spanFrame);
    printf(": baseLine=%d logicalBSize=%d topLeading=%d h=%d bp=%d,%d zeroEffectiveSpanBox=%s\n",
           baselineBCoord, psd->mLogicalBSize, psd->mBStartLeading,
           spanFramePFD->mBounds.BSize(lineWM),
           spanFramePFD->mBorderPadding.Top(frameWM),
           spanFramePFD->mBorderPadding.Bottom(frameWM),
           zeroEffectiveSpanBox ? "yes" : "no");
#endif
  }

  nscoord maxStartBoxBSize = 0;
  nscoord maxEndBoxBSize = 0;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    nsIFrame* frame = pfd->mFrame;
    WritingMode frameWM = frame->GetWritingMode();

    
    NS_ASSERTION(frame, "null frame in PerFrameData - something is very very bad");
    if (!frame) {
      return;
    }

    
    nscoord logicalBSize;
    PerSpanData* frameSpan = pfd->mSpan;
    if (frameSpan) {
      
      
      logicalBSize = frameSpan->mLogicalBSize;
    }
    else {
      
      
      logicalBSize = pfd->mBounds.BSize(lineWM) +
                     pfd->mMargin.BStartEnd(frameWM);
      if (logicalBSize < 0 &&
          mPresContext->CompatibilityMode() == eCompatibility_NavQuirks) {
        pfd->mAscent -= logicalBSize;
        logicalBSize = 0;
      }
    }

    
    
    const nsStyleCoord& verticalAlign =
      frame->StyleTextReset()->mVerticalAlign;
    uint8_t verticalAlignEnum = frame->VerticalAlignEnum();
#ifdef NOISY_BLOCKDIR_ALIGN
    printf("  [frame]");
    nsFrame::ListTag(stdout, frame);
    printf(": verticalAlignUnit=%d (enum == %d",
           verticalAlign.GetUnit(),
           ((eStyleUnit_Enumerated == verticalAlign.GetUnit())
            ? verticalAlign.GetIntValue()
            : -1));
    if (verticalAlignEnum != nsIFrame::eInvalidVerticalAlign) {
      printf(", after SVG dominant-baseline conversion == %d",
             verticalAlignEnum);
    }
    printf(")\n");
#endif

    if (verticalAlignEnum != nsIFrame::eInvalidVerticalAlign) {
      switch (verticalAlignEnum) {
        default:
        case NS_STYLE_VERTICAL_ALIGN_BASELINE:
        {
          
          
          pfd->mBounds.BStart(lineWM) = baselineBCoord - pfd->mAscent;
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_SUB:
        {
          
          
          
          
          nscoord parentSubscript = fm->SubscriptOffset();
          nscoord revisedBaselineBCoord = baselineBCoord + parentSubscript;
          pfd->mBounds.BStart(lineWM) = revisedBaselineBCoord - pfd->mAscent;
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_SUPER:
        {
          
          
          
          
          nscoord parentSuperscript = fm->SuperscriptOffset();
          nscoord revisedBaselineBCoord = baselineBCoord - parentSuperscript;
          pfd->mBounds.BStart(lineWM) = revisedBaselineBCoord - pfd->mAscent;
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TOP:
        {
          pfd->mBlockDirAlign = VALIGN_TOP;
          nscoord subtreeBSize = logicalBSize;
          if (frameSpan) {
            subtreeBSize = frameSpan->mMaxBCoord - frameSpan->mMinBCoord;
            NS_ASSERTION(subtreeBSize >= logicalBSize,
                         "unexpected subtree block size");
          }
          if (subtreeBSize > maxStartBoxBSize) {
            maxStartBoxBSize = subtreeBSize;
          }
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
        {
          pfd->mBlockDirAlign = VALIGN_BOTTOM;
          nscoord subtreeBSize = logicalBSize;
          if (frameSpan) {
            subtreeBSize = frameSpan->mMaxBCoord - frameSpan->mMinBCoord;
            NS_ASSERTION(subtreeBSize >= logicalBSize,
                         "unexpected subtree block size");
          }
          if (subtreeBSize > maxEndBoxBSize) {
            maxEndBoxBSize = subtreeBSize;
          }
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
        {
          
          
          nscoord parentXHeight = fm->XHeight();
          if (frameSpan) {
            pfd->mBounds.BStart(lineWM) = baselineBCoord -
              (parentXHeight + pfd->mBounds.BSize(lineWM))/2;
          }
          else {
            pfd->mBounds.BStart(lineWM) = baselineBCoord -
              (parentXHeight + logicalBSize)/2 +
              pfd->mMargin.BStart(frameWM);
          }
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TEXT_TOP:
        {
          
          
          
          
          nscoord parentAscent = fm->MaxAscent();
          if (frameSpan) {
            pfd->mBounds.BStart(lineWM) = baselineBCoord - parentAscent -
              pfd->mBorderPadding.BStart(frameWM) + frameSpan->mBStartLeading;
          }
          else {
            pfd->mBounds.BStart(lineWM) = baselineBCoord - parentAscent +
                                          pfd->mMargin.BStart(frameWM);
          }
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM:
        {
          
          
          nscoord parentDescent = fm->MaxDescent();
          if (frameSpan) {
            pfd->mBounds.BStart(lineWM) = baselineBCoord + parentDescent -
                                          pfd->mBounds.BSize(lineWM) +
                                          pfd->mBorderPadding.BEnd(frameWM) -
                                          frameSpan->mBEndLeading;
          }
          else {
            pfd->mBounds.BStart(lineWM) = baselineBCoord + parentDescent -
                                          pfd->mBounds.BSize(lineWM) -
                                          pfd->mMargin.BEnd(frameWM);
          }
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_MIDDLE_WITH_BASELINE:
        {
          
          if (frameSpan) {
            pfd->mBounds.BStart(lineWM) = baselineBCoord -
                                          pfd->mBounds.BSize(lineWM)/2;
          }
          else {
            pfd->mBounds.BStart(lineWM) = baselineBCoord - logicalBSize/2 +
                                          pfd->mMargin.BStart(frameWM);
          }
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }
      }
    } else {
      
      nscoord pctBasis = 0;
      if (verticalAlign.HasPercent()) {
        
        
        float inflation =
          GetInflationForBlockDirAlignment(frame, mInflationMinFontSize);
        pctBasis = nsHTMLReflowState::CalcLineHeight(frame->GetContent(),
          frame->StyleContext(), mBlockReflowState->ComputedBSize(),
          inflation);
      }
      nscoord offset =
        nsRuleNode::ComputeCoordPercentCalc(verticalAlign, pctBasis);
      
      
      
      
      
      nscoord revisedBaselineBCoord = baselineBCoord - offset;
      pfd->mBounds.BStart(lineWM) = revisedBaselineBCoord - pfd->mAscent;
      pfd->mBlockDirAlign = VALIGN_OTHER;
    }

    
    
    if (pfd->mBlockDirAlign == VALIGN_OTHER) {
      
      
      
      
      
      
      
      
      
      
      
      
#if 0
      if (!pfd->GetFlag(PFD_ISTEXTFRAME)) {
#else

      bool canUpdate = !pfd->GetFlag(PFD_ISTEXTFRAME);
      if (!canUpdate && pfd->GetFlag(PFD_ISNONWHITESPACETEXTFRAME)) {
        canUpdate =
          frame->StyleText()->mLineHeight.GetUnit() == eStyleUnit_Normal;
      }
      if (canUpdate) {
#endif
        nscoord blockStart, blockEnd;
        if (frameSpan) {
          
          
          
          blockStart = pfd->mBounds.BStart(lineWM) + frameSpan->mMinBCoord;
          blockEnd = pfd->mBounds.BStart(lineWM) + frameSpan->mMaxBCoord;
        }
        else {
          blockStart = pfd->mBounds.BStart(lineWM) -
                       pfd->mMargin.BStart(frameWM);
          blockEnd = blockStart + logicalBSize;
        }
        if (!preMode &&
            mPresContext->CompatibilityMode() != eCompatibility_FullStandards &&
            !logicalBSize) {
          
          
          
          if (nsGkAtoms::brFrame == frame->GetType()) {
            blockStart = BLOCKDIR_ALIGN_FRAMES_NO_MINIMUM;
            blockEnd = BLOCKDIR_ALIGN_FRAMES_NO_MAXIMUM;
          }
        }
        if (blockStart < minBCoord) minBCoord = blockStart;
        if (blockEnd > maxBCoord) maxBCoord = blockEnd;
#ifdef NOISY_BLOCKDIR_ALIGN
        printf("     [frame]raw: a=%d h=%d bp=%d,%d logical: h=%d leading=%d y=%d minBCoord=%d maxBCoord=%d\n",
               pfd->mAscent, pfd->mBounds.BSize(lineWM),
               pfd->mBorderPadding.Top(frameWM),
               pfd->mBorderPadding.Bottom(frameWM),
               logicalBSize,
               frameSpan ? frameSpan->mBStartLeading : 0,
               pfd->mBounds.BStart(lineWM), minBCoord, maxBCoord);
#endif
      }
      if (psd != mRootSpan) {
        frame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
      }
    }
    pfd = pfd->mNext;
  }

  
  
  if (psd == mRootSpan) {
    
    
    
    
    
    
    
    
    
    

    
    bool applyMinLH = !psd->mZeroEffectiveSpanBox || mHasBullet;
    bool isLastLine = (!mLineBox->IsLineWrapped() && !mLineEndsInBR);
    if (!applyMinLH && isLastLine) {
      nsIContent* blockContent = mRootSpan->mFrame->mFrame->GetContent();
      if (blockContent) {
        nsIAtom *blockTagAtom = blockContent->Tag();
        
        if (blockTagAtom == nsGkAtoms::li ||
            blockTagAtom == nsGkAtoms::dt ||
            blockTagAtom == nsGkAtoms::dd) {
          applyMinLH = true;
        }
      }
    }
    if (applyMinLH) {
      if (psd->mHasNonemptyContent || preMode || mHasBullet) {
#ifdef NOISY_BLOCKDIR_ALIGN
        printf("  [span]==> adjusting min/maxBCoord: currentValues: %d,%d", minBCoord, maxBCoord);
#endif
        nscoord minimumLineBSize = mMinLineBSize;
        nscoord blockStart =
          -nsLayoutUtils::GetCenteredFontBaseline(fm, minimumLineBSize);
        nscoord blockEnd = blockStart + minimumLineBSize;

        if (blockStart < minBCoord) minBCoord = blockStart;
        if (blockEnd > maxBCoord) maxBCoord = blockEnd;

#ifdef NOISY_BLOCKDIR_ALIGN
        printf(" new values: %d,%d\n", minBCoord, maxBCoord);
#endif
#ifdef NOISY_BLOCKDIR_ALIGN
        printf("            Used mMinLineBSize: %d, blockStart: %d, blockEnd: %d\n", mMinLineBSize, blockStart, blockEnd);
#endif
      }
      else {
        
        
        
        

        
#ifdef NOISY_BLOCKDIR_ALIGN
        printf("  [span]==> zapping min/maxBCoord: currentValues: %d,%d newValues: 0,0\n",
               minBCoord, maxBCoord);
#endif
        minBCoord = maxBCoord = 0;
      }
    }
  }

  if ((minBCoord == BLOCKDIR_ALIGN_FRAMES_NO_MINIMUM) ||
      (maxBCoord == BLOCKDIR_ALIGN_FRAMES_NO_MAXIMUM)) {
    minBCoord = maxBCoord = baselineBCoord;
  }

  if ((psd != mRootSpan) && (psd->mZeroEffectiveSpanBox)) {
#ifdef NOISY_BLOCKDIR_ALIGN
    printf("   [span]adjusting for zeroEffectiveSpanBox\n");
    printf("     Original: minBCoord=%d, maxBCoord=%d, bSize=%d, ascent=%d, logicalBSize=%d, topLeading=%d, bottomLeading=%d\n",
           minBCoord, maxBCoord, spanFramePFD->mBounds.BSize(frameWM),
           spanFramePFD->mAscent,
           psd->mLogicalBSize, psd->mBStartLeading, psd->mBEndLeading);
#endif
    nscoord goodMinBCoord = spanFramePFD->mBorderPadding.BStart(frameWM) -
                            psd->mBStartLeading;
    nscoord goodMaxBCoord = goodMinBCoord + psd->mLogicalBSize;

    
    
    
    
    
    
    
    
    
    
    
    if (maxStartBoxBSize > maxBCoord - minBCoord) {
      
      
      
      nscoord distribute = maxStartBoxBSize - (maxBCoord - minBCoord);
      nscoord ascentSpace = std::max(minBCoord - goodMinBCoord, 0);
      if (distribute > ascentSpace) {
        distribute -= ascentSpace;
        minBCoord -= ascentSpace;
        nscoord descentSpace = std::max(goodMaxBCoord - maxBCoord, 0);
        if (distribute > descentSpace) {
          maxBCoord += descentSpace;
        } else {
          maxBCoord += distribute;
        }
      } else {
        minBCoord -= distribute;
      }
    }
    if (maxEndBoxBSize > maxBCoord - minBCoord) {
      
      nscoord distribute = maxEndBoxBSize - (maxBCoord - minBCoord);
      nscoord descentSpace = std::max(goodMaxBCoord - maxBCoord, 0);
      if (distribute > descentSpace) {
        distribute -= descentSpace;
        maxBCoord += descentSpace;
        nscoord ascentSpace = std::max(minBCoord - goodMinBCoord, 0);
        if (distribute > ascentSpace) {
          minBCoord -= ascentSpace;
        } else {
          minBCoord -= distribute;
        }
      } else {
        maxBCoord += distribute;
      }
    }

    if (minBCoord > goodMinBCoord) {
      nscoord adjust = minBCoord - goodMinBCoord; 

      
      psd->mLogicalBSize -= adjust;
      psd->mBStartLeading -= adjust;
    }
    if (maxBCoord < goodMaxBCoord) {
      nscoord adjust = goodMaxBCoord - maxBCoord;
      psd->mLogicalBSize -= adjust;
      psd->mBEndLeading -= adjust;
    }
    if (minBCoord > 0) {

      
      
      
      spanFramePFD->mAscent -= minBCoord; 
      spanFramePFD->mBounds.BSize(lineWM) -= minBCoord; 
      psd->mBStartLeading += minBCoord;
      *psd->mBaseline -= minBCoord;

      pfd = psd->mFirstFrame;
      while (nullptr != pfd) {
        pfd->mBounds.BStart(lineWM) -= minBCoord; 
                                                  
        pfd->mFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
        pfd = pfd->mNext;
      }
      maxBCoord -= minBCoord; 
                              
      minBCoord = 0;
    }
    if (maxBCoord < spanFramePFD->mBounds.BSize(lineWM)) {
      nscoord adjust = spanFramePFD->mBounds.BSize(lineWM) - maxBCoord;
      spanFramePFD->mBounds.BSize(lineWM) -= adjust; 
      psd->mBEndLeading += adjust;
    }
#ifdef NOISY_BLOCKDIR_ALIGN
    printf("     New: minBCoord=%d, maxBCoord=%d, bSize=%d, ascent=%d, logicalBSize=%d, topLeading=%d, bottomLeading=%d\n",
           minBCoord, maxBCoord, spanFramePFD->mBounds.BSize(lineWM),
           spanFramePFD->mAscent,
           psd->mLogicalBSize, psd->mBStartLeading, psd->mBEndLeading);
#endif
  }

  psd->mMinBCoord = minBCoord;
  psd->mMaxBCoord = maxBCoord;
#ifdef NOISY_BLOCKDIR_ALIGN
  printf("  [span]==> minBCoord=%d maxBCoord=%d delta=%d maxStartBoxBSize=%d maxEndBoxBSize=%d\n",
         minBCoord, maxBCoord, maxBCoord - minBCoord, maxStartBoxBSize, maxEndBoxBSize);
#endif
  if (maxStartBoxBSize > mMaxStartBoxBSize) {
    mMaxStartBoxBSize = maxStartBoxBSize;
  }
  if (maxEndBoxBSize > mMaxEndBoxBSize) {
    mMaxEndBoxBSize = maxEndBoxBSize;
  }
}

static void SlideSpanFrameRect(nsIFrame* aFrame, nscoord aDeltaWidth)
{
  
  
  
  
  nsPoint p = aFrame->GetPosition();
  p.x -= aDeltaWidth;
  aFrame->SetPosition(p);
}

bool
nsLineLayout::TrimTrailingWhiteSpaceIn(PerSpanData* psd,
                                       nscoord* aDeltaISize)
{
  PerFrameData* pfd = psd->mFirstFrame;
  if (!pfd) {
    *aDeltaISize = 0;
    return false;
  }
  pfd = pfd->Last();
  while (nullptr != pfd) {
#ifdef REALLY_NOISY_TRIM
    nsFrame::ListTag(stdout, (psd == mRootSpan
                              ? mBlockReflowState->frame
                              : psd->mFrame->mFrame));
    printf(": attempting trim of ");
    nsFrame::ListTag(stdout, pfd->mFrame);
    printf("\n");
#endif
    PerSpanData* childSpan = pfd->mSpan;
    WritingMode lineWM = mRootSpan->mWritingMode;
    if (childSpan) {
      
      if (TrimTrailingWhiteSpaceIn(childSpan, aDeltaISize)) {
        nscoord deltaISize = *aDeltaISize;
        if (deltaISize) {
          
          pfd->mBounds.ISize(lineWM) -= deltaISize;
          if (psd != mRootSpan) {
            
            
            
            
            
            
            nsIFrame* f = pfd->mFrame;
            LogicalRect r(lineWM, f->GetRect(), mContainerWidth);
            r.ISize(lineWM) -= deltaISize;
            f->SetRect(lineWM, r, mContainerWidth);
          }

          
          psd->mICoord -= deltaISize;

          
          
          
          
          while (pfd->mNext) {
            pfd = pfd->mNext;
            pfd->mBounds.IStart(lineWM) -= deltaISize;
            if (psd != mRootSpan) {
              
              
              
              
              
              
              SlideSpanFrameRect(pfd->mFrame, deltaISize);
            }
          }
        }
        return true;
      }
    }
    else if (!pfd->GetFlag(PFD_ISTEXTFRAME) &&
             !pfd->GetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE)) {
      
      
      *aDeltaISize = 0;
      return true;
    }
    else if (pfd->GetFlag(PFD_ISTEXTFRAME)) {
      
      
      
      nsTextFrame::TrimOutput trimOutput = static_cast<nsTextFrame*>(pfd->mFrame)->
          TrimTrailingWhiteSpace(mBlockReflowState->rendContext);
#ifdef NOISY_TRIM
      nsFrame::ListTag(stdout, (psd == mRootSpan
                                ? mBlockReflowState->frame
                                : psd->mFrame->mFrame));
      printf(": trim of ");
      nsFrame::ListTag(stdout, pfd->mFrame);
      printf(" returned %d\n", trimOutput.mDeltaWidth);
#endif
      if (trimOutput.mLastCharIsJustifiable && pfd->mJustificationNumSpaces > 0) {
        pfd->mJustificationNumSpaces--;
      }
      
      if (trimOutput.mChanged) {
        pfd->SetFlag(PFD_RECOMPUTEOVERFLOW, true);
      }

      if (trimOutput.mDeltaWidth) {
        pfd->mBounds.ISize(lineWM) -= trimOutput.mDeltaWidth;

        
        if (psd != mRootSpan) {
          
          
          pfd->mFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
        }

        
        psd->mICoord -= trimOutput.mDeltaWidth;

        
        
        
        
        while (pfd->mNext) {
          pfd = pfd->mNext;
          pfd->mBounds.IStart(lineWM) -= trimOutput.mDeltaWidth;
          if (psd != mRootSpan) {
            
            
            
            
            
            
            SlideSpanFrameRect(pfd->mFrame, trimOutput.mDeltaWidth);
          }
        }
      }

      if (pfd->GetFlag(PFD_ISNONEMPTYTEXTFRAME) || trimOutput.mChanged) {
        
        *aDeltaISize = trimOutput.mDeltaWidth;
        return true;
      }
    }
    pfd = pfd->mPrev;
  }

  *aDeltaISize = 0;
  return false;
}

bool
nsLineLayout::TrimTrailingWhiteSpace()
{
  PerSpanData* psd = mRootSpan;
  nscoord deltaISize;
  TrimTrailingWhiteSpaceIn(psd, &deltaISize);
  return 0 != deltaISize;
}

void
nsLineLayout::ComputeJustificationWeights(PerSpanData* aPSD,
                                          int32_t* aNumSpaces,
                                          int32_t* aNumLetters)
{
  NS_ASSERTION(aPSD, "null arg");
  NS_ASSERTION(aNumSpaces, "null arg");
  NS_ASSERTION(aNumLetters, "null arg");
  int32_t numSpaces = 0;
  int32_t numLetters = 0;

  for (PerFrameData* pfd = aPSD->mFirstFrame; pfd != nullptr; pfd = pfd->mNext) {

    if (true == pfd->GetFlag(PFD_ISTEXTFRAME)) {
      numSpaces += pfd->mJustificationNumSpaces;
      numLetters += pfd->mJustificationNumLetters;
    }
    else if (pfd->mSpan != nullptr) {
      int32_t spanSpaces;
      int32_t spanLetters;

      ComputeJustificationWeights(pfd->mSpan, &spanSpaces, &spanLetters);

      numSpaces += spanSpaces;
      numLetters += spanLetters;
    }
  }

  *aNumSpaces = numSpaces;
  *aNumLetters = numLetters;
}

nscoord 
nsLineLayout::ApplyFrameJustification(PerSpanData* aPSD, FrameJustificationState* aState)
{
  NS_ASSERTION(aPSD, "null arg");
  NS_ASSERTION(aState, "null arg");

  nscoord deltaICoord = 0;
  for (PerFrameData* pfd = aPSD->mFirstFrame; pfd != nullptr; pfd = pfd->mNext) {
    
    if (!pfd->GetFlag(PFD_ISBULLET)) {
      nscoord dw = 0;
      WritingMode lineWM = mRootSpan->mWritingMode;

      pfd->mBounds.IStart(lineWM) += deltaICoord;

      if (true == pfd->GetFlag(PFD_ISTEXTFRAME)) {
        if (aState->mTotalWidthForSpaces > 0 &&
            aState->mTotalNumSpaces > 0) {
          aState->mNumSpacesProcessed += pfd->mJustificationNumSpaces;

          nscoord newAllocatedWidthForSpaces =
            (aState->mTotalWidthForSpaces*aState->mNumSpacesProcessed)
              /aState->mTotalNumSpaces;
          
          dw += newAllocatedWidthForSpaces - aState->mWidthForSpacesProcessed;

          aState->mWidthForSpacesProcessed = newAllocatedWidthForSpaces;
        }

        if (aState->mTotalWidthForLetters > 0 &&
            aState->mTotalNumLetters > 0) {
          aState->mNumLettersProcessed += pfd->mJustificationNumLetters;

          nscoord newAllocatedWidthForLetters =
            (aState->mTotalWidthForLetters*aState->mNumLettersProcessed)
              /aState->mTotalNumLetters;
          
          dw += newAllocatedWidthForLetters - aState->mWidthForLettersProcessed;

          aState->mWidthForLettersProcessed = newAllocatedWidthForLetters;
        }
        
        if (dw) {
          pfd->SetFlag(PFD_RECOMPUTEOVERFLOW, true);
        }
      }
      else {
        if (nullptr != pfd->mSpan) {
          dw += ApplyFrameJustification(pfd->mSpan, aState);
        }
      }

      pfd->mBounds.ISize(lineWM) += dw;

      deltaICoord += dw;
      pfd->mFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
    }
  }
  return deltaICoord;
}



void
nsLineLayout::TextAlignLine(nsLineBox* aLine,
                            bool aIsLastLine)
{
  



  PerSpanData* psd = mRootSpan;
  WritingMode lineWM = psd->mWritingMode;
  NS_WARN_IF_FALSE(psd->mIEnd != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  nscoord availISize = psd->mIEnd - psd->mIStart;
  nscoord remainingISize = availISize - aLine->ISize();
#ifdef NOISY_INLINEDIR_ALIGN
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": availISize=%d lineBounds.IStart=%d lineISize=%d delta=%d\n",
         availISize, aLine->IStart(), aLine->ISize(), remainingISize);
#endif

  
  
  
  
  
  
  nscoord dx = 0;
  uint8_t textAlign = mStyleText->mTextAlign;
  bool textAlignTrue = mStyleText->mTextAlignTrue;
  if (aIsLastLine) {
    textAlignTrue = mStyleText->mTextAlignLastTrue;
    if (mStyleText->mTextAlignLast == NS_STYLE_TEXT_ALIGN_AUTO) {
      if (textAlign == NS_STYLE_TEXT_ALIGN_JUSTIFY) {
        textAlign = NS_STYLE_TEXT_ALIGN_DEFAULT;
      }
    } else {
      textAlign = mStyleText->mTextAlignLast;
    }
  }

  if ((remainingISize > 0 || textAlignTrue) &&
      !(mBlockReflowState->frame->IsSVGText())) {

    switch (textAlign) {
      case NS_STYLE_TEXT_ALIGN_JUSTIFY:
        int32_t numSpaces;
        int32_t numLetters;

        ComputeJustificationWeights(psd, &numSpaces, &numLetters);

        if (numSpaces > 0) {
          FrameJustificationState state =
            { numSpaces, numLetters, remainingISize, 0, 0, 0, 0, 0 };

          
          
          aLine->ExpandBy(ApplyFrameJustification(psd, &state),
                          mContainerWidth);
          break;
        }
        
        

      case NS_STYLE_TEXT_ALIGN_DEFAULT:
        
        break;

      case NS_STYLE_TEXT_ALIGN_LEFT:
      case NS_STYLE_TEXT_ALIGN_MOZ_LEFT:
        if (!lineWM.IsBidiLTR()) {
          dx = remainingISize;
        }
        break;

      case NS_STYLE_TEXT_ALIGN_RIGHT:
      case NS_STYLE_TEXT_ALIGN_MOZ_RIGHT:
        if (lineWM.IsBidiLTR()) {
          dx = remainingISize;
        }
        break;

      case NS_STYLE_TEXT_ALIGN_END:
        dx = remainingISize;
        break;

      case NS_STYLE_TEXT_ALIGN_CENTER:
      case NS_STYLE_TEXT_ALIGN_MOZ_CENTER:
        dx = remainingISize / 2;
        break;
    }
  }

  if (dx) {
    for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      pfd->mBounds.IStart(lineWM) += dx;
      pfd->mFrame->SetRect(lineWM, pfd->mBounds, mContainerWidth);
    }
    aLine->IndentBy(dx, mContainerWidth);
  }

  if (mPresContext->BidiEnabled() &&
      (!mPresContext->IsVisualMode() || !lineWM.IsBidiLTR())) {
    nsBidiPresUtils::ReorderFrames(psd->mFirstFrame->mFrame,
                                   aLine->GetChildCount(),
                                   lineWM, mContainerWidth);
  }
}

void
nsLineLayout::RelativePositionFrames(nsOverflowAreas& aOverflowAreas)
{
  RelativePositionFrames(mRootSpan, aOverflowAreas);
}

void
nsLineLayout::RelativePositionFrames(PerSpanData* psd, nsOverflowAreas& aOverflowAreas)
{
  nsOverflowAreas overflowAreas;
  WritingMode wm = psd->mWritingMode;
  if (nullptr != psd->mFrame) {
    
    
    
    
    
    
    
    
    
    
    nsRect adjustedBounds(nsPoint(0, 0), psd->mFrame->mFrame->GetSize());

    overflowAreas.ScrollableOverflow().UnionRect(
      psd->mFrame->mOverflowAreas.ScrollableOverflow(), adjustedBounds);
    overflowAreas.VisualOverflow().UnionRect(
      psd->mFrame->mOverflowAreas.VisualOverflow(), adjustedBounds);
  }
  else {
    LogicalRect rect(wm, psd->mIStart, mBStartEdge,
                     psd->mICoord - psd->mIStart, mFinalLineBSize);
    
    
    
    
    overflowAreas.VisualOverflow() = rect.GetPhysicalRect(wm, mContainerWidth);
    overflowAreas.ScrollableOverflow() = overflowAreas.VisualOverflow();
  }

  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    nsIFrame* frame = pfd->mFrame;
    nsPoint origin = frame->GetPosition();

    
    if (pfd->GetFlag(PFD_RELATIVEPOS)) {
      
      nsMargin physicalOffsets =
        pfd->mOffsets.GetPhysicalMargin(pfd->mFrame->GetWritingMode());
      
      
      nsHTMLReflowState::ApplyRelativePositioning(pfd->mFrame,
                                                  physicalOffsets,
                                                  &origin);
      frame->SetPosition(origin);
    }

    
    
    
    if (frame->HasView())
      nsContainerFrame::SyncFrameViewAfterReflow(mPresContext, frame,
        frame->GetView(), pfd->mOverflowAreas.VisualOverflow(),
        NS_FRAME_NO_SIZE_VIEW);

    
    
    
    
    nsOverflowAreas r;
    if (pfd->mSpan) {
      
      
      RelativePositionFrames(pfd->mSpan, r);
    } else {
      r = pfd->mOverflowAreas;
      if (pfd->GetFlag(PFD_ISTEXTFRAME)) {
        
        
        
        
        if (pfd->GetFlag(PFD_RECOMPUTEOVERFLOW) ||
            frame->StyleContext()->HasTextDecorationLines()) {
          nsTextFrame* f = static_cast<nsTextFrame*>(frame);
          r = f->RecomputeOverflow(*mBlockReflowState);
        }
        frame->FinishAndStoreOverflow(r, frame->GetSize());
      }

      
      
      
      
      
      
      
      nsContainerFrame::PositionChildViews(frame);
    }

    
    
    
    if (frame->HasView())
      nsContainerFrame::SyncFrameViewAfterReflow(mPresContext, frame,
                                                 frame->GetView(),
                                                 r.VisualOverflow(),
                                                 NS_FRAME_NO_MOVE_VIEW);

    overflowAreas.UnionWith(r + origin);
  }

  
  
  if (psd->mFrame) {
    PerFrameData* spanPFD = psd->mFrame;
    nsIFrame* frame = spanPFD->mFrame;
    frame->FinishAndStoreOverflow(overflowAreas, frame->GetSize());
  }
  aOverflowAreas = overflowAreas;
}

void
nsLineLayout::AdvanceICoord(nscoord aAmount)
{
  mCurrentSpan->mICoord += aAmount;
}

WritingMode
nsLineLayout::GetWritingMode()
{
  return mRootSpan->mWritingMode;
}

nscoord
nsLineLayout::GetCurrentICoord()
{
  return mCurrentSpan->mICoord;
}
