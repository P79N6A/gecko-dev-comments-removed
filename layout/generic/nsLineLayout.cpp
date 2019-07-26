






#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "plarena.h"

#include "mozilla/Util.h"
#include "nsCOMPtr.h"
#include "nsLineLayout.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsStyleConsts.h"
#include "nsContainerFrame.h"
#include "nsFloatManager.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsPlaceholderFrame.h"
#include "nsIContent.h"
#include "nsTextFragment.h"
#include "nsBidiUtils.h"
#include "nsLayoutUtils.h"
#include "nsTextFrame.h"
#include "nsCSSRendering.h"
#include <algorithm>

#ifdef DEBUG
#undef  NOISY_HORIZONTAL_ALIGN
#undef  NOISY_VERTICAL_ALIGN
#undef  REALLY_NOISY_VERTICAL_ALIGN
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
    mLastOptionalBreakPriority(eNoBreak),
    mLastOptionalBreakContentOffset(-1),
    mForceBreakContentOffset(-1),
    mMinLineHeight(0),
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
  NS_ASSERTION(aFloatManager || aOuterReflowState->frame->GetType() ==
                                  nsGkAtoms::letterFrame,
               "float manager should be present");
  MOZ_COUNT_CTOR(nsLineLayout);

  
  mStyleText = aOuterReflowState->frame->GetStyleText();
  mLineNumber = 0;
  mTotalPlacedFrames = 0;
  mTopEdge = 0;
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
nsLineLayout::BeginLineReflow(nscoord aX, nscoord aY,
                              nscoord aWidth, nscoord aHeight,
                              bool aImpactedByFloats,
                              bool aIsTopOfPage,
                              uint8_t aDirection)
{
  NS_ASSERTION(nullptr == mRootSpan, "bad linelayout user");
  NS_WARN_IF_FALSE(aWidth != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
#ifdef DEBUG
  if ((aWidth != NS_UNCONSTRAINEDSIZE) && CRAZY_WIDTH(aWidth)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": Init: bad caller: width WAS %d(0x%x)\n",
           aWidth, aWidth);
  }
  if ((aHeight != NS_UNCONSTRAINEDSIZE) && CRAZY_HEIGHT(aHeight)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": Init: bad caller: height WAS %d(0x%x)\n",
           aHeight, aHeight);
  }
#endif
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": BeginLineReflow: %d,%d,%d,%d impacted=%s %s\n",
         aX, aY, aWidth, aHeight,
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
  mMaxTopBoxHeight = mMaxBottomBoxHeight = 0;

  if (mGotLineBox) {
    mLineBox->ClearHasBullet();
  }

  PerSpanData* psd;
  NewPerSpanData(&psd);
  mCurrentSpan = mRootSpan = psd;
  psd->mReflowState = mBlockReflowState;
  psd->mLeftEdge = aX;
  psd->mX = aX;
  psd->mRightEdge = aX + aWidth;

  
  
  if (!(GetLineContainerFrame()->GetStateBits() &
      NS_FRAME_IN_CONSTRAINED_HEIGHT)) {

    
    
    
    nscoord maxLineBoxWidth =
      GetLineContainerFrame()->PresContext()->PresShell()->MaxLineBoxWidth();

    if (maxLineBoxWidth > 0 &&
        psd->mRightEdge - psd->mLeftEdge > maxLineBoxWidth) {
      psd->mRightEdge = psd->mLeftEdge + maxLineBoxWidth;
    }
  }

  mTopEdge = aY;

  psd->mNoWrap = !mStyleText->WhiteSpaceCanWrap();
  psd->mDirection = aDirection;
  psd->mChangedFrameDirection = false;

  
  

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

    if (NS_STYLE_DIRECTION_RTL == psd->mDirection) {
      psd->mRightEdge -= indent;
    }
    else {
      psd->mX += indent;
    }
  }
}

void
nsLineLayout::EndLineReflow()
{
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": EndLineReflow: width=%d\n", mRootSpan->mX - mRootSpan->mLeftEdge);
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
#ifdef REALLY_NOISY_REFLOW
  printf("nsLL::UpdateBand %d, %d, %d, %d, frame=%p\n  will set mImpacted to true\n",
         aNewAvailSpace.x, aNewAvailSpace.y,
         aNewAvailSpace.width, aNewAvailSpace.height,
         aFloatFrame);
#endif
#ifdef DEBUG
  if ((aNewAvailSpace.width != NS_UNCONSTRAINEDSIZE) && CRAZY_WIDTH(aNewAvailSpace.width)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": UpdateBand: bad caller: width WAS %d(0x%x)\n",
           aNewAvailSpace.width, aNewAvailSpace.width);
  }
  if ((aNewAvailSpace.height != NS_UNCONSTRAINEDSIZE) && CRAZY_HEIGHT(aNewAvailSpace.height)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": UpdateBand: bad caller: height WAS %d(0x%x)\n",
           aNewAvailSpace.height, aNewAvailSpace.height);
  }
#endif

  
  NS_WARN_IF_FALSE(mRootSpan->mRightEdge != NS_UNCONSTRAINEDSIZE &&
                   aNewAvailSpace.width != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  
  nscoord deltaX = aNewAvailSpace.x - mRootSpan->mLeftEdge;
  
  
  nscoord deltaWidth = aNewAvailSpace.width - (mRootSpan->mRightEdge - mRootSpan->mLeftEdge);
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": UpdateBand: %d,%d,%d,%d deltaWidth=%d deltaX=%d\n",
         aNewAvailSpace.x, aNewAvailSpace.y,
         aNewAvailSpace.width, aNewAvailSpace.height, deltaWidth, deltaX);
#endif

  
  mRootSpan->mLeftEdge += deltaX;
  mRootSpan->mRightEdge += deltaX;
  mRootSpan->mX += deltaX;

  
  
  for (PerSpanData* psd = mCurrentSpan; psd; psd = psd->mParent) {
    psd->mRightEdge += deltaWidth;
    psd->mContainsFloat = true;
    NS_ASSERTION(psd->mX - mTrimmableWidth <= psd->mRightEdge,
                 "We placed a float where there was no room!");
#ifdef NOISY_REFLOW
    printf("  span %p: oldRightEdge=%d newRightEdge=%d\n",
           psd, psd->mRightEdge - deltaRightEdge, psd->mRightEdge);
#endif
  }
  NS_ASSERTION(mRootSpan->mContainsFloat &&
               mRootSpan->mLeftEdge == aNewAvailSpace.x &&
               mRootSpan->mRightEdge == aNewAvailSpace.XMost(),
               "root span was updated incorrectly?");

  
  
  
  
  
  if (deltaX != 0) {
    for (PerFrameData* pfd = mRootSpan->mFirstFrame; pfd; pfd = pfd->mNext) {
      pfd->mBounds.x += deltaX;
    }
  }

  mTopEdge = aNewAvailSpace.y;
  mImpactedByFloats = true;

  mLastFloatWasLetterFrame = nsGkAtoms::letterFrame == aFloatFrame->GetType();
}

nsresult
nsLineLayout::NewPerSpanData(PerSpanData** aResult)
{
  PerSpanData* psd = mSpanFreeList;
  if (nullptr == psd) {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &mArena, sizeof(PerSpanData));
    if (nullptr == mem) {
      return NS_ERROR_OUT_OF_MEMORY;
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
  *aResult = psd;
  return NS_OK;
}

nsresult
nsLineLayout::BeginSpan(nsIFrame* aFrame,
                        const nsHTMLReflowState* aSpanReflowState,
                        nscoord aLeftEdge,
                        nscoord aRightEdge,
                        nscoord* aBaseline)
{
  NS_ASSERTION(aRightEdge != NS_UNCONSTRAINEDSIZE,
               "should no longer be using unconstrained sizes");
#ifdef NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth+1);
  nsFrame::ListTag(stdout, aFrame);
  printf(": BeginSpan leftEdge=%d rightEdge=%d\n", aLeftEdge, aRightEdge);
#endif

  PerSpanData* psd;
  nsresult rv = NewPerSpanData(&psd);
  if (NS_SUCCEEDED(rv)) {
    
    PerFrameData* pfd = mCurrentSpan->mLastFrame;
    NS_ASSERTION(pfd->mFrame == aFrame, "huh?");
    pfd->mSpan = psd;

    
    psd->mFrame = pfd;
    psd->mParent = mCurrentSpan;
    psd->mReflowState = aSpanReflowState;
    psd->mLeftEdge = aLeftEdge;
    psd->mX = aLeftEdge;
    psd->mRightEdge = aRightEdge;
    psd->mBaseline = aBaseline;

    psd->mNoWrap =
      !aSpanReflowState->frame->GetStyleText()->WhiteSpaceCanWrap();
    psd->mDirection = aSpanReflowState->mStyleVisibility->mDirection;
    psd->mChangedFrameDirection = false;

    
    mCurrentSpan = psd;
    mSpanDepth++;
  }
  return rv;
}

nscoord
nsLineLayout::EndSpan(nsIFrame* aFrame)
{
  NS_ASSERTION(mSpanDepth > 0, "end-span without begin-span");
#ifdef NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth);
  nsFrame::ListTag(stdout, aFrame);
  printf(": EndSpan width=%d\n", mCurrentSpan->mX - mCurrentSpan->mLeftEdge);
#endif
  PerSpanData* psd = mCurrentSpan;
  nscoord widthResult = psd->mLastFrame ? (psd->mX - psd->mLeftEdge) : 0;

  mSpanDepth--;
  mCurrentSpan->mReflowState = nullptr;  
  mCurrentSpan = mCurrentSpan->mParent;
  return widthResult;
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
nsLineLayout::IsZeroHeight()
{
  PerSpanData* psd = mCurrentSpan;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    if (0 != pfd->mBounds.height) {
      return false;
    }
    pfd = pfd->mNext;
  }
  return true;
}

nsresult
nsLineLayout::NewPerFrameData(PerFrameData** aResult)
{
  PerFrameData* pfd = mFrameFreeList;
  if (nullptr == pfd) {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &mArena, sizeof(PerFrameData));
    if (nullptr == mem) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    pfd = reinterpret_cast<PerFrameData*>(mem);
  }
  else {
    mFrameFreeList = pfd->mNext;
  }
  pfd->mSpan = nullptr;
  pfd->mNext = nullptr;
  pfd->mPrev = nullptr;
  pfd->mFrame = nullptr;
  pfd->mFlags = 0;  

#ifdef DEBUG
  pfd->mVerticalAlign = 0xFF;
  mFramesAllocated++;
#endif
  *aResult = pfd;
  return NS_OK;
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

  
  
  
  

  const nsStyleMargin* margin = aFrame->GetStyleMargin();
  if (HasPercentageUnitSide(margin->mMargin)) {
    return true;
  }

  const nsStylePadding* padding = aFrame->GetStylePadding();
  if (HasPercentageUnitSide(padding->mPadding)) {
    return true;
  }

  

  const nsStylePosition* pos = aFrame->GetStylePosition();

  if ((pos->WidthDependsOnContainer() &&
       pos->mWidth.GetUnit() != eStyleUnit_Auto) ||
      pos->MaxWidthDependsOnContainer() ||
      pos->MinWidthDependsOnContainer() ||
      pos->OffsetHasPercent(NS_SIDE_RIGHT) ||
      pos->OffsetHasPercent(NS_SIDE_LEFT)) {
    return true;
  }

  if (eStyleUnit_Auto == pos->mWidth.GetUnit()) {
    
    
    const nsStyleDisplay* disp = aFrame->GetStyleDisplay();
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
      const nsIFrame::IntrinsicSize &intrinsicSize = f->GetIntrinsicSize();
      if (intrinsicSize.width.GetUnit() == eStyleUnit_None &&
          intrinsicSize.height.GetUnit() == eStyleUnit_None) {
        return true;
      }
    }
  }

  return false;
}

nsresult
nsLineLayout::ReflowFrame(nsIFrame* aFrame,
                          nsReflowStatus& aReflowStatus,
                          nsHTMLReflowMetrics* aMetrics,
                          bool& aPushedFrame)
{
  
  aPushedFrame = false;

  PerFrameData* pfd;
  nsresult rv = NewPerFrameData(&pfd);
  if (NS_FAILED(rv)) {
    return rv;
  }
  PerSpanData* psd = mCurrentSpan;
  psd->AppendFrame(pfd);

#ifdef REALLY_NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth);
  printf("%p: Begin ReflowFrame pfd=%p ", psd, pfd);
  nsFrame::ListTag(stdout, aFrame);
  printf("\n");
#endif

  mTextJustificationNumSpaces = 0;
  mTextJustificationNumLetters = 0;

  
  
  pfd->mFrame = aFrame;

  
  
  
  
  pfd->mBounds.x = psd->mX;
  pfd->mBounds.y = mTopEdge;

  
  
  
  
  
  
  
  
  
  
  bool notSafeToBreak = LineIsEmpty() && !mImpactedByFloats;

  
  nsIAtom* frameType = aFrame->GetType();
  bool isText = frameType == nsGkAtoms::textFrame;
  
  
  
  
  nsSize availSize(mBlockReflowState->ComputedWidth(), NS_UNCONSTRAINEDSIZE);

  
  
  
  NS_WARN_IF_FALSE(psd->mRightEdge != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  nscoord availableSpaceOnLine = psd->mRightEdge - psd->mX;

  
  Maybe<nsHTMLReflowState> reflowStateHolder;
  if (!isText) {
    reflowStateHolder.construct(mPresContext, *psd->mReflowState,
                                aFrame, availSize);
    nsHTMLReflowState& reflowState = reflowStateHolder.ref();
    reflowState.mLineLayout = this;
    reflowState.mFlags.mIsTopOfPage = mIsTopOfPage;
    if (reflowState.ComputedWidth() == NS_UNCONSTRAINEDSIZE)
      reflowState.availableWidth = availableSpaceOnLine;
    pfd->mMargin = reflowState.mComputedMargin;
    pfd->mBorderPadding = reflowState.mComputedBorderPadding;
    pfd->SetFlag(PFD_RELATIVEPOS,
                 (reflowState.mStyleDisplay->mPosition == NS_STYLE_POSITION_RELATIVE));
    if (pfd->GetFlag(PFD_RELATIVEPOS)) {
      pfd->mOffsets = reflowState.mComputedOffsets;
    }

    
    
    ApplyStartMargin(pfd, reflowState);
  } else {
    pfd->mMargin.SizeTo(0, 0, 0, 0);
    pfd->mBorderPadding.SizeTo(0, 0, 0, 0);
    pfd->mOffsets.SizeTo(0, 0, 0, 0);
    
    
  }

  
  
  
  
  
  
  
  if (mGotLineBox && IsPercentageAware(aFrame)) {
    mLineBox->DisableResizeReflowOptimization();
  }

  
  
  
  aFrame->WillReflow(mPresContext);

  
  nsHTMLReflowMetrics metrics;
#ifdef DEBUG
  metrics.width = nscoord(0xdeadbeef);
  metrics.height = nscoord(0xdeadbeef);
#endif
  nscoord tx = pfd->mBounds.x;
  nscoord ty = pfd->mBounds.y;
  mFloatManager->Translate(tx, ty);

  int32_t savedOptionalBreakOffset;
  gfxBreakPriority savedOptionalBreakPriority;
  nsIContent* savedOptionalBreakContent =
    GetLastOptionalBreakPosition(&savedOptionalBreakOffset,
                                 &savedOptionalBreakPriority);

  if (!isText) {
    rv = aFrame->Reflow(mPresContext, metrics, reflowStateHolder.ref(),
                        aReflowStatus);
    if (NS_FAILED(rv)) {
      NS_WARNING( "Reflow of frame failed in nsLineLayout" );
      return rv;
    }
  } else {
    static_cast<nsTextFrame*>(aFrame)->
      ReflowText(*this, availableSpaceOnLine, psd->mReflowState->rendContext,
                 psd->mReflowState->mFlags.mBlinks, metrics, aReflowStatus);
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
        
        
        
        nscoord availableWidth = psd->mRightEdge - (psd->mX - mTrimmableWidth);
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

  NS_ASSERTION(metrics.width>=0, "bad width");
  NS_ASSERTION(metrics.height>=0,"bad height");
  if (metrics.width<0) metrics.width=0;
  if (metrics.height<0) metrics.height=0;

#ifdef DEBUG
  
  
  if (!NS_INLINE_IS_BREAK_BEFORE(aReflowStatus)) {
    if (CRAZY_WIDTH(metrics.width) || CRAZY_HEIGHT(metrics.height)) {
      printf("nsLineLayout: ");
      nsFrame::ListTag(stdout, aFrame);
      printf(" metrics=%d,%d!\n", metrics.width, metrics.height);
    }
    if ((metrics.width == nscoord(0xdeadbeef)) ||
        (metrics.height == nscoord(0xdeadbeef))) {
      printf("nsLineLayout: ");
      nsFrame::ListTag(stdout, aFrame);
      printf(" didn't set w/h %d,%d!\n", metrics.width, metrics.height);
    }
  }
#endif

  
  
  
  
  
  pfd->mOverflowAreas = metrics.mOverflowAreas;

  pfd->mBounds.width = metrics.width;
  pfd->mBounds.height = metrics.height;

  
  aFrame->SetSize(nsSize(metrics.width, metrics.height));

  
  aFrame->DidReflow(mPresContext,
                    isText ? nullptr : reflowStateHolder.addr(),
                    nsDidReflowStatus::FINISHED);

  if (aMetrics) {
    *aMetrics = metrics;
  }

  if (!NS_INLINE_IS_BREAK_BEFORE(aReflowStatus)) {
    
    
    
    
    if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
      nsIFrame* kidNextInFlow = aFrame->GetNextInFlow();
      if (nullptr != kidNextInFlow) {
        
        
        
        nsContainerFrame* parent = static_cast<nsContainerFrame*>
                                                  (kidNextInFlow->GetParent());
        parent->DeleteNextInFlowChild(mPresContext, kidNextInFlow, true);
      }
    }

    
    
    bool continuingTextRun = aFrame->CanContinueTextRun();
    
    
    if (!continuingTextRun && !pfd->GetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE)) {
      mTrimmableWidth = 0;
    }

    
    
    bool optionalBreakAfterFits;
    NS_ASSERTION(isText ||
                 !reflowStateHolder.ref().IsFloating(),
                 "How'd we get a floated inline frame? "
                 "The frame ctor should've dealt with this.");
    
    
    
    uint8_t direction =
      isText ? psd->mReflowState->mStyleVisibility->mDirection :
               reflowStateHolder.ref().mStyleVisibility->mDirection;
    if (CanPlaceFrame(pfd, direction, notSafeToBreak, continuingTextRun,
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
          
          
          
          if (NotifyOptionalBreakPosition(aFrame->GetContent(), INT32_MAX, optionalBreakAfterFits, eNormalBreak)) {
            
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

  return rv;
}

void
nsLineLayout::ApplyStartMargin(PerFrameData* pfd,
                               nsHTMLReflowState& aReflowState)
{
  NS_ASSERTION(!aReflowState.IsFloating(),
               "How'd we get a floated inline frame? "
               "The frame ctor should've dealt with this.");

  
  bool ltr = (NS_STYLE_DIRECTION_LTR == aReflowState.mStyleVisibility->mDirection);

  
  
  
  
  
  if (pfd->mFrame->GetPrevContinuation() ||
      nsLayoutUtils::FrameIsNonFirstInIBSplit(pfd->mFrame)) {
    
    
    if (ltr)
      pfd->mMargin.left = 0;
    else
      pfd->mMargin.right = 0;
  }
  else {
    pfd->mBounds.x += ltr ? pfd->mMargin.left : pfd->mMargin.right;

    NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aReflowState.availableWidth,
                     "have unconstrained width; this should only result from "
                     "very large sizes, not attempts at intrinsic width "
                     "calculation");
    if (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedWidth()) {
      
      
      
      
      aReflowState.availableWidth -= ltr ? pfd->mMargin.left : pfd->mMargin.right;
    }
  }
}

nscoord
nsLineLayout::GetCurrentFrameXDistanceFromBlock()
{
  PerSpanData* psd;
  nscoord x = 0;
  for (psd = mCurrentSpan; psd; psd = psd->mParent) {
    x += psd->mX;
  }
  return x;
}











bool
nsLineLayout::CanPlaceFrame(PerFrameData* pfd,
                            uint8_t aFrameDirection,
                            bool aNotSafeToBreak,
                            bool aFrameCanContinueTextRun,
                            bool aCanRollBackBeforeFrame,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus,
                            bool* aOptionalBreakAfterFits)
{
  NS_PRECONDITION(pfd && pfd->mFrame, "bad args, null pointers for frame data");
  
  *aOptionalBreakAfterFits = true;
  
  if (0 != pfd->mBounds.width) {
    
    bool ltr = (NS_STYLE_DIRECTION_LTR == aFrameDirection);

    













    if ((NS_FRAME_IS_NOT_COMPLETE(aStatus) ||
         pfd->mFrame->GetLastInFlow()->GetNextContinuation() ||
         nsLayoutUtils::FrameIsNonLastInIBSplit(pfd->mFrame))
        && !pfd->GetFlag(PFD_ISLETTERFRAME)) {
      if (ltr)
        pfd->mMargin.right = 0;
      else
        pfd->mMargin.left = 0;
    }
  }
  else {
    
    pfd->mMargin.left = pfd->mMargin.right = 0;
  }

  PerSpanData* psd = mCurrentSpan;
  if (psd->mNoWrap) {
    
    return true;
  }

  bool ltr = NS_STYLE_DIRECTION_LTR == aFrameDirection;
  nscoord endMargin = ltr ? pfd->mMargin.right : pfd->mMargin.left;

#ifdef NOISY_CAN_PLACE_FRAME
  if (nullptr != psd->mFrame) {
    nsFrame::ListTag(stdout, psd->mFrame->mFrame);
  }
  else {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
  } 
  printf(": aNotSafeToBreak=%s frame=", aNotSafeToBreak ? "true" : "false");
  nsFrame::ListTag(stdout, pfd->mFrame);
  printf(" frameWidth=%d\n", pfd->mBounds.XMost() + endMargin - psd->mX);
#endif

  
  
  bool outside = pfd->mBounds.XMost() - mTrimmableWidth + endMargin > psd->mRightEdge;
  if (!outside) {
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> inside\n");
#endif
    return true;
  }
  *aOptionalBreakAfterFits = false;

  
  
  if (0 == pfd->mMargin.left + pfd->mBounds.width + pfd->mMargin.right) {
    
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
      printf("<psd=%p x=%d left=%d> ", psd, psd->mX, psd->mLeftEdge);
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
  
  PerSpanData* psd = mCurrentSpan;
  bool emptyFrame = false;
  if ((0 == pfd->mBounds.width) && (0 == pfd->mBounds.height)) {
    pfd->mBounds.x = psd->mX;
    pfd->mBounds.y = mTopEdge;
    emptyFrame = true;
  }

  
  if (aMetrics.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE)
    pfd->mAscent = pfd->mFrame->GetBaseline();
  else
    pfd->mAscent = aMetrics.ascent;

  bool ltr = (NS_STYLE_DIRECTION_LTR == pfd->mFrame->GetStyleVisibility()->mDirection);
  
  psd->mX = pfd->mBounds.XMost() + (ltr ? pfd->mMargin.right : pfd->mMargin.left);

  
  if (!emptyFrame) {
    mTotalPlacedFrames++;
  }
}

nsresult
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

  PerFrameData* pfd;
  nsresult rv = NewPerFrameData(&pfd);
  if (NS_SUCCEEDED(rv)) {
    mRootSpan->AppendFrame(pfd);
    pfd->mFrame = aFrame;
    pfd->mMargin.SizeTo(0, 0, 0, 0);
    pfd->mBorderPadding.SizeTo(0, 0, 0, 0);
    pfd->mFlags = 0;  
    pfd->SetFlag(PFD_ISBULLET, true);
    if (aMetrics.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE)
      pfd->mAscent = aFrame->GetBaseline();
    else
      pfd->mAscent = aMetrics.ascent;

    
    pfd->mBounds = aFrame->GetRect();
    pfd->mOverflowAreas = aMetrics.mOverflowAreas;
  }
  return rv;
}

#ifdef DEBUG
void
nsLineLayout::DumpPerSpanData(PerSpanData* psd, int32_t aIndent)
{
  nsFrame::IndentBy(stdout, aIndent);
  printf("%p: left=%d x=%d right=%d\n", static_cast<void*>(psd),
         psd->mLeftEdge, psd->mX, psd->mRightEdge);
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    nsFrame::IndentBy(stdout, aIndent+1);
    nsFrame::ListTag(stdout, pfd->mFrame);
    printf(" %d,%d,%d,%d\n", pfd->mBounds.x, pfd->mBounds.y,
           pfd->mBounds.width, pfd->mBounds.height);
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
  
  PerFrameData rootPFD;
  rootPFD.mFrame = mBlockReflowState->frame;
  rootPFD.mAscent = 0;
  mRootSpan->mFrame = &rootPFD;

  
  
  
  PerSpanData* psd = mRootSpan;
  VerticalAlignFrames(psd);

  
  
  
  
  
  
  
  
  
  
  nscoord lineHeight = psd->mMaxY - psd->mMinY;

  
  
  
  nscoord baselineY;
  if (psd->mMinY < 0) {
    baselineY = mTopEdge - psd->mMinY;
  }
  else {
    baselineY = mTopEdge;
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  if (lineHeight < mMaxBottomBoxHeight) {
    
    nscoord extra = mMaxBottomBoxHeight - lineHeight;
    baselineY += extra;
    lineHeight = mMaxBottomBoxHeight;
  }
  if (lineHeight < mMaxTopBoxHeight) {
    lineHeight = mMaxTopBoxHeight;
  }
#ifdef NOISY_VERTICAL_ALIGN
  printf("  [line]==> lineHeight=%d baselineY=%d\n", lineHeight, baselineY);
#endif

  
  
  
  
  
  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    if (pfd->mVerticalAlign == VALIGN_OTHER) {
      pfd->mBounds.y += baselineY;
      pfd->mFrame->SetRect(pfd->mBounds);
    }
  }
  PlaceTopBottomFrames(psd, -mTopEdge, lineHeight);

  
  
  
  
  
  if (rootPFD.mFrame->StyleContext()->HasTextDecorationLines()) {
    for (const PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      const nsIFrame *const f = pfd->mFrame;
      if (f->VerticalAlignEnum() != NS_STYLE_VERTICAL_ALIGN_BASELINE) {
        const nscoord offset = baselineY - pfd->mBounds.y;
        f->Properties().Set(nsIFrame::LineBaselineOffset(),
                            NS_INT32_TO_PTR(offset));
      }
    }
  }

  
  mLineBox->mBounds.x = psd->mLeftEdge;
  mLineBox->mBounds.y = mTopEdge;
  mLineBox->mBounds.width = psd->mX - psd->mLeftEdge;
  mLineBox->mBounds.height = lineHeight;
  mFinalLineHeight = lineHeight;
  mLineBox->SetAscent(baselineY - mTopEdge);
#ifdef NOISY_VERTICAL_ALIGN
  printf(
    "  [line]==> bounds{x,y,w,h}={%d,%d,%d,%d} lh=%d a=%d\n",
    mLineBox->mBounds.x, mLineBox->mBounds.y,
    mLineBox->mBounds.width, mLineBox->mBounds.height,
    mFinalLineHeight, mLineBox->GetAscent());
#endif

  
  mRootSpan->mFrame = nullptr;
}

void
nsLineLayout::PlaceTopBottomFrames(PerSpanData* psd,
                                   nscoord aDistanceFromTop,
                                   nscoord aLineHeight)
{
  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    PerSpanData* span = pfd->mSpan;
#ifdef DEBUG
    NS_ASSERTION(0xFF != pfd->mVerticalAlign, "umr");
#endif
    switch (pfd->mVerticalAlign) {
      case VALIGN_TOP:
        if (span) {
          pfd->mBounds.y = -aDistanceFromTop - span->mMinY;
        }
        else {
          pfd->mBounds.y = -aDistanceFromTop + pfd->mMargin.top;
        }
        pfd->mFrame->SetRect(pfd->mBounds);
#ifdef NOISY_VERTICAL_ALIGN
        printf("    ");
        nsFrame::ListTag(stdout, pfd->mFrame);
        printf(": y=%d dTop=%d [bp.top=%d topLeading=%d]\n",
               pfd->mBounds.y, aDistanceFromTop,
               span ? pfd->mBorderPadding.top : 0,
               span ? span->mTopLeading : 0);
#endif
        break;
      case VALIGN_BOTTOM:
        if (span) {
          
          pfd->mBounds.y = -aDistanceFromTop + aLineHeight - span->mMaxY;
        }
        else {
          pfd->mBounds.y = -aDistanceFromTop + aLineHeight -
            pfd->mMargin.bottom - pfd->mBounds.height;
        }
        pfd->mFrame->SetRect(pfd->mBounds);
#ifdef NOISY_VERTICAL_ALIGN
        printf("    ");
        nsFrame::ListTag(stdout, pfd->mFrame);
        printf(": y=%d\n", pfd->mBounds.y);
#endif
        break;
    }
    if (span) {
      nscoord distanceFromTop = aDistanceFromTop + pfd->mBounds.y;
      PlaceTopBottomFrames(span, distanceFromTop, aLineHeight);
    }
  }
}

#define VERTICAL_ALIGN_FRAMES_NO_MINIMUM nscoord_MAX
#define VERTICAL_ALIGN_FRAMES_NO_MAXIMUM nscoord_MIN





void
nsLineLayout::VerticalAlignFrames(PerSpanData* psd)
{
  
  PerFrameData* spanFramePFD = psd->mFrame;
  nsIFrame* spanFrame = spanFramePFD->mFrame;

  
  nsRefPtr<nsFontMetrics> fm;
  float inflation =
    nsLayoutUtils::FontSizeInflationInner(spanFrame, mInflationMinFontSize);
  nsLayoutUtils::GetFontMetricsForFrame(spanFrame, getter_AddRefs(fm),
                                        inflation);
  mBlockReflowState->rendContext->SetFont(fm);

  bool preMode = mStyleText->WhiteSpaceIsSignificant();

  
  
  
  
  bool emptyContinuation = psd != mRootSpan &&
    spanFrame->GetPrevInFlow() && !spanFrame->GetNextInFlow() &&
    (0 == spanFramePFD->mBounds.width) && (0 == spanFramePFD->mBounds.height);

#ifdef NOISY_VERTICAL_ALIGN
  printf("[%sSpan]", (psd == mRootSpan)?"Root":"");
  nsFrame::ListTag(stdout, spanFrame);
  printf(": preMode=%s strictMode=%s w/h=%d,%d emptyContinuation=%s",
         preMode ? "yes" : "no",
         mPresContext->CompatibilityMode() != eCompatibility_NavQuirks ? "yes" : "no",
         spanFramePFD->mBounds.width, spanFramePFD->mBounds.height,
         emptyContinuation ? "yes" : "no");
  if (psd != mRootSpan) {
    printf(" bp=%d,%d,%d,%d margin=%d,%d,%d,%d",
           spanFramePFD->mBorderPadding.top,
           spanFramePFD->mBorderPadding.right,
           spanFramePFD->mBorderPadding.bottom,
           spanFramePFD->mBorderPadding.left,
           spanFramePFD->mMargin.top,
           spanFramePFD->mMargin.right,
           spanFramePFD->mMargin.bottom,
           spanFramePFD->mMargin.left);
  }
  printf("\n");
#endif

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool zeroEffectiveSpanBox = false;
  
  
  
  
  if ((emptyContinuation ||
       mPresContext->CompatibilityMode() != eCompatibility_FullStandards) &&
      ((psd == mRootSpan) ||
       ((0 == spanFramePFD->mBorderPadding.top) &&
        (0 == spanFramePFD->mBorderPadding.right) &&
        (0 == spanFramePFD->mBorderPadding.bottom) &&
        (0 == spanFramePFD->mBorderPadding.left) &&
        (0 == spanFramePFD->mMargin.top) &&
        (0 == spanFramePFD->mMargin.right) &&
        (0 == spanFramePFD->mMargin.bottom) &&
        (0 == spanFramePFD->mMargin.left)))) {
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    zeroEffectiveSpanBox = true;
    for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      if (pfd->GetFlag(PFD_ISTEXTFRAME) &&
          (pfd->GetFlag(PFD_ISNONWHITESPACETEXTFRAME) || preMode ||
           pfd->mBounds.width != 0)) {
        zeroEffectiveSpanBox = false;
        break;
      }
    }
  }
  psd->mZeroEffectiveSpanBox = zeroEffectiveSpanBox;

  
  nscoord baselineY, minY, maxY;
  if (psd == mRootSpan) {
    
    
    
    
    
    
    baselineY = 0;
    minY = VERTICAL_ALIGN_FRAMES_NO_MINIMUM;
    maxY = VERTICAL_ALIGN_FRAMES_NO_MAXIMUM;
#ifdef NOISY_VERTICAL_ALIGN
    printf("[RootSpan]");
    nsFrame::ListTag(stdout, spanFrame);
    printf(": pass1 valign frames: topEdge=%d minLineHeight=%d zeroEffectiveSpanBox=%s\n",
           mTopEdge, mMinLineHeight,
           zeroEffectiveSpanBox ? "yes" : "no");
#endif
  }
  else {
    
    
    
    float inflation =
      nsLayoutUtils::FontSizeInflationInner(spanFrame, mInflationMinFontSize);
    nscoord logicalHeight = nsHTMLReflowState::
      CalcLineHeight(spanFrame->StyleContext(),
                     mBlockReflowState->ComputedHeight(),
                     inflation);
    nscoord contentHeight = spanFramePFD->mBounds.height -
      spanFramePFD->mBorderPadding.top - spanFramePFD->mBorderPadding.bottom;

    
    
    if (spanFramePFD->GetFlag(PFD_ISLETTERFRAME) &&
        !spanFrame->GetPrevInFlow() &&
        spanFrame->GetStyleText()->mLineHeight.GetUnit() == eStyleUnit_Normal) {
      logicalHeight = spanFramePFD->mBounds.height;
    }

    nscoord leading = logicalHeight - contentHeight;
    psd->mTopLeading = leading / 2;
    psd->mBottomLeading = leading - psd->mTopLeading;
    psd->mLogicalHeight = logicalHeight;

    if (zeroEffectiveSpanBox) {
      
      
      
      

      
      
      minY = VERTICAL_ALIGN_FRAMES_NO_MINIMUM;
      maxY = VERTICAL_ALIGN_FRAMES_NO_MAXIMUM;
    }
    else {

      
      
      
      
      
      minY = spanFramePFD->mBorderPadding.top - psd->mTopLeading;
      maxY = minY + psd->mLogicalHeight;
    }

    
    
    
    *psd->mBaseline = baselineY = spanFramePFD->mAscent;


#ifdef NOISY_VERTICAL_ALIGN
    printf("[%sSpan]", (psd == mRootSpan)?"Root":"");
    nsFrame::ListTag(stdout, spanFrame);
    printf(": baseLine=%d logicalHeight=%d topLeading=%d h=%d bp=%d,%d zeroEffectiveSpanBox=%s\n",
           baselineY, psd->mLogicalHeight, psd->mTopLeading,
           spanFramePFD->mBounds.height,
           spanFramePFD->mBorderPadding.top, spanFramePFD->mBorderPadding.bottom,
           zeroEffectiveSpanBox ? "yes" : "no");
#endif
  }

  nscoord maxTopBoxHeight = 0;
  nscoord maxBottomBoxHeight = 0;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    nsIFrame* frame = pfd->mFrame;

    
    NS_ASSERTION(frame, "null frame in PerFrameData - something is very very bad");
    if (!frame) {
      return;
    }

    
    nscoord logicalHeight;
    PerSpanData* frameSpan = pfd->mSpan;
    if (frameSpan) {
      
      
      logicalHeight = frameSpan->mLogicalHeight;
    }
    else {
      
      
      logicalHeight = pfd->mBounds.height + pfd->mMargin.top +
        pfd->mMargin.bottom;
    }

    
    const nsStyleCoord& verticalAlign =
      frame->GetStyleTextReset()->mVerticalAlign;
    uint8_t verticalAlignEnum = frame->VerticalAlignEnum();
#ifdef NOISY_VERTICAL_ALIGN
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
          
          
          pfd->mBounds.y = baselineY - pfd->mAscent;
          pfd->mVerticalAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_SUB:
        {
          
          
          
          
          nscoord parentSubscript = fm->SubscriptOffset();
          nscoord revisedBaselineY = baselineY + parentSubscript;
          pfd->mBounds.y = revisedBaselineY - pfd->mAscent;
          pfd->mVerticalAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_SUPER:
        {
          
          
          
          
          nscoord parentSuperscript = fm->SuperscriptOffset();
          nscoord revisedBaselineY = baselineY - parentSuperscript;
          pfd->mBounds.y = revisedBaselineY - pfd->mAscent;
          pfd->mVerticalAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TOP:
        {
          pfd->mVerticalAlign = VALIGN_TOP;
          nscoord subtreeHeight = logicalHeight;
          if (frameSpan) {
            subtreeHeight = frameSpan->mMaxY - frameSpan->mMinY;
            NS_ASSERTION(subtreeHeight >= logicalHeight,
                         "unexpected subtree height");
          }
          if (subtreeHeight > maxTopBoxHeight) {
            maxTopBoxHeight = subtreeHeight;
          }
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
        {
          pfd->mVerticalAlign = VALIGN_BOTTOM;
          nscoord subtreeHeight = logicalHeight;
          if (frameSpan) {
            subtreeHeight = frameSpan->mMaxY - frameSpan->mMinY;
            NS_ASSERTION(subtreeHeight >= logicalHeight,
                         "unexpected subtree height");
          }
          if (subtreeHeight > maxBottomBoxHeight) {
            maxBottomBoxHeight = subtreeHeight;
          }
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
        {
          
          
          nscoord parentXHeight = fm->XHeight();
          if (frameSpan) {
            pfd->mBounds.y = baselineY -
              (parentXHeight + pfd->mBounds.height)/2;
          }
          else {
            pfd->mBounds.y = baselineY - (parentXHeight + logicalHeight)/2 +
              pfd->mMargin.top;
          }
          pfd->mVerticalAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TEXT_TOP:
        {
          
          
          nscoord parentAscent = fm->MaxAscent();
          if (frameSpan) {
            pfd->mBounds.y = baselineY - parentAscent -
              pfd->mBorderPadding.top + frameSpan->mTopLeading;
          }
          else {
            pfd->mBounds.y = baselineY - parentAscent + pfd->mMargin.top;
          }
          pfd->mVerticalAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM:
        {
          
          
          nscoord parentDescent = fm->MaxDescent();
          if (frameSpan) {
            pfd->mBounds.y = baselineY + parentDescent -
              pfd->mBounds.height + pfd->mBorderPadding.bottom -
              frameSpan->mBottomLeading;
          }
          else {
            pfd->mBounds.y = baselineY + parentDescent -
              pfd->mBounds.height - pfd->mMargin.bottom;
          }
          pfd->mVerticalAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_MIDDLE_WITH_BASELINE:
        {
          
          if (frameSpan) {
            pfd->mBounds.y = baselineY - pfd->mBounds.height/2;
          }
          else {
            pfd->mBounds.y = baselineY - logicalHeight/2 + pfd->mMargin.top;
          }
          pfd->mVerticalAlign = VALIGN_OTHER;
          break;
        }
      }
    } else {
      
      nscoord pctBasis = 0;
      if (verticalAlign.HasPercent()) {
        
        
        float inflation =
          nsLayoutUtils::FontSizeInflationInner(frame, mInflationMinFontSize);
        pctBasis = nsHTMLReflowState::CalcLineHeight(
          frame->StyleContext(), mBlockReflowState->ComputedHeight(),
          inflation);
      }
      nscoord offset =
        nsRuleNode::ComputeCoordPercentCalc(verticalAlign, pctBasis);
      
      
      
      
      
      nscoord revisedBaselineY = baselineY - offset;
      pfd->mBounds.y = revisedBaselineY - pfd->mAscent;
      pfd->mVerticalAlign = VALIGN_OTHER;
    }

    
    
    if (pfd->mVerticalAlign == VALIGN_OTHER) {
      
      
      
      
      
      
      
      
      
      
      
      
#if 0
      if (!pfd->GetFlag(PFD_ISTEXTFRAME)) {
#else

      bool canUpdate = !pfd->GetFlag(PFD_ISTEXTFRAME);
      if (!canUpdate && pfd->GetFlag(PFD_ISNONWHITESPACETEXTFRAME)) {
        canUpdate =
          frame->GetStyleText()->mLineHeight.GetUnit() == eStyleUnit_Normal;
      }
      if (canUpdate) {
#endif
        nscoord yTop, yBottom;
        if (frameSpan) {
          
          
          
          yTop = pfd->mBounds.y + frameSpan->mMinY;
          yBottom = pfd->mBounds.y + frameSpan->mMaxY;
        }
        else {
          yTop = pfd->mBounds.y - pfd->mMargin.top;
          yBottom = yTop + logicalHeight;
        }
        if (!preMode &&
            mPresContext->CompatibilityMode() != eCompatibility_FullStandards &&
            !logicalHeight) {
          
          
          
          if (nsGkAtoms::brFrame == frame->GetType()) {
            yTop = VERTICAL_ALIGN_FRAMES_NO_MINIMUM;
            yBottom = VERTICAL_ALIGN_FRAMES_NO_MAXIMUM;
          }
        }
        if (yTop < minY) minY = yTop;
        if (yBottom > maxY) maxY = yBottom;
#ifdef NOISY_VERTICAL_ALIGN
        printf("     [frame]raw: a=%d h=%d bp=%d,%d logical: h=%d leading=%d y=%d minY=%d maxY=%d\n",
               pfd->mAscent, pfd->mBounds.height,
               pfd->mBorderPadding.top, pfd->mBorderPadding.bottom,
               logicalHeight,
               frameSpan ? frameSpan->mTopLeading : 0,
               pfd->mBounds.y, minY, maxY);
#endif
      }
      if (psd != mRootSpan) {
        frame->SetRect(pfd->mBounds);
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
#ifdef NOISY_VERTICAL_ALIGN
        printf("  [span]==> adjusting min/maxY: currentValues: %d,%d", minY, maxY);
#endif
        nscoord minimumLineHeight = mMinLineHeight;
        nscoord yTop =
          -nsLayoutUtils::GetCenteredFontBaseline(fm, minimumLineHeight);
        nscoord yBottom = yTop + minimumLineHeight;

        if (yTop < minY) minY = yTop;
        if (yBottom > maxY) maxY = yBottom;

#ifdef NOISY_VERTICAL_ALIGN
        printf(" new values: %d,%d\n", minY, maxY);
#endif
#ifdef NOISY_VERTICAL_ALIGN
        printf("            Used mMinLineHeight: %d, yTop: %d, yBottom: %d\n", mMinLineHeight, yTop, yBottom);
#endif
      }
      else {
        
        
        
        

        
#ifdef NOISY_VERTICAL_ALIGN
        printf("  [span]==> zapping min/maxY: currentValues: %d,%d newValues: 0,0\n",
               minY, maxY);
#endif
        minY = maxY = 0;
      }
    }
  }

  if ((minY == VERTICAL_ALIGN_FRAMES_NO_MINIMUM) ||
      (maxY == VERTICAL_ALIGN_FRAMES_NO_MAXIMUM)) {
    minY = maxY = baselineY;
  }

  if ((psd != mRootSpan) && (psd->mZeroEffectiveSpanBox)) {
#ifdef NOISY_VERTICAL_ALIGN
    printf("   [span]adjusting for zeroEffectiveSpanBox\n");
    printf("     Original: minY=%d, maxY=%d, height=%d, ascent=%d, logicalHeight=%d, topLeading=%d, bottomLeading=%d\n",
           minY, maxY, spanFramePFD->mBounds.height,
           spanFramePFD->mAscent,
           psd->mLogicalHeight, psd->mTopLeading, psd->mBottomLeading);
#endif
    nscoord goodMinY = spanFramePFD->mBorderPadding.top - psd->mTopLeading;
    nscoord goodMaxY = goodMinY + psd->mLogicalHeight;

    
    
    
    
    
    
    
    
    
    
    
    if (maxTopBoxHeight > maxY - minY) {
      
      
      
      nscoord distribute = maxTopBoxHeight - (maxY - minY);
      nscoord ascentSpace = std::max(minY - goodMinY, 0);
      if (distribute > ascentSpace) {
        distribute -= ascentSpace;
        minY -= ascentSpace;
        nscoord descentSpace = std::max(goodMaxY - maxY, 0);
        if (distribute > descentSpace) {
          maxY += descentSpace;
        } else {
          maxY += distribute;
        }
      } else {
        minY -= distribute;
      }
    }
    if (maxBottomBoxHeight > maxY - minY) {
      
      nscoord distribute = maxBottomBoxHeight - (maxY - minY);
      nscoord descentSpace = std::max(goodMaxY - maxY, 0);
      if (distribute > descentSpace) {
        distribute -= descentSpace;
        maxY += descentSpace;
        nscoord ascentSpace = std::max(minY - goodMinY, 0);
        if (distribute > ascentSpace) {
          minY -= ascentSpace;
        } else {
          minY -= distribute;
        }
      } else {
        maxY += distribute;
      }
    }

    if (minY > goodMinY) {
      nscoord adjust = minY - goodMinY; 

      
      psd->mLogicalHeight -= adjust;
      psd->mTopLeading -= adjust;
    }
    if (maxY < goodMaxY) {
      nscoord adjust = goodMaxY - maxY;
      psd->mLogicalHeight -= adjust;
      psd->mBottomLeading -= adjust;
    }
    if (minY > 0) {

      
      
      
      spanFramePFD->mAscent -= minY; 
      spanFramePFD->mBounds.height -= minY; 
      psd->mTopLeading += minY;
      *psd->mBaseline -= minY;

      pfd = psd->mFirstFrame;
      while (nullptr != pfd) {
        pfd->mBounds.y -= minY; 
        pfd->mFrame->SetRect(pfd->mBounds);
        pfd = pfd->mNext;
      }
      maxY -= minY; 
      minY = 0;
    }
    if (maxY < spanFramePFD->mBounds.height) {
      nscoord adjust = spanFramePFD->mBounds.height - maxY;
      spanFramePFD->mBounds.height -= adjust; 
      psd->mBottomLeading += adjust;
    }
#ifdef NOISY_VERTICAL_ALIGN
    printf("     New: minY=%d, maxY=%d, height=%d, ascent=%d, logicalHeight=%d, topLeading=%d, bottomLeading=%d\n",
           minY, maxY, spanFramePFD->mBounds.height,
           spanFramePFD->mAscent,
           psd->mLogicalHeight, psd->mTopLeading, psd->mBottomLeading);
#endif
  }

  psd->mMinY = minY;
  psd->mMaxY = maxY;
#ifdef NOISY_VERTICAL_ALIGN
  printf("  [span]==> minY=%d maxY=%d delta=%d maxTopBoxHeight=%d maxBottomBoxHeight=%d\n",
         minY, maxY, maxY - minY, maxTopBoxHeight, maxBottomBoxHeight);
#endif
  if (maxTopBoxHeight > mMaxTopBoxHeight) {
    mMaxTopBoxHeight = maxTopBoxHeight;
  }
  if (maxBottomBoxHeight > mMaxBottomBoxHeight) {
    mMaxBottomBoxHeight = maxBottomBoxHeight;
  }
}

static void SlideSpanFrameRect(nsIFrame* aFrame, nscoord aDeltaWidth)
{
  nsRect r = aFrame->GetRect();
  r.x -= aDeltaWidth;
  aFrame->SetRect(r);
}

bool
nsLineLayout::TrimTrailingWhiteSpaceIn(PerSpanData* psd,
                                       nscoord* aDeltaWidth)
{
#ifndef IBMBIDI

  if (NS_STYLE_DIRECTION_RTL == psd->mDirection) {
    *aDeltaWidth = 0;
    return true;
  }
#endif

  PerFrameData* pfd = psd->mFirstFrame;
  if (!pfd) {
    *aDeltaWidth = 0;
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
    if (childSpan) {
      
      if (TrimTrailingWhiteSpaceIn(childSpan, aDeltaWidth)) {
        nscoord deltaWidth = *aDeltaWidth;
        if (deltaWidth) {
          
          pfd->mBounds.width -= deltaWidth;
          if (psd != mRootSpan) {
            
            
            
            
            
            
            nsIFrame* f = pfd->mFrame;
            nsRect r = f->GetRect();
            r.width -= deltaWidth;
            f->SetRect(r);
          }

          
          psd->mX -= deltaWidth;

          
          
          
          
          while (pfd->mNext) {
            pfd = pfd->mNext;
            pfd->mBounds.x -= deltaWidth;
            if (psd != mRootSpan) {
              
              
              
              
              
              
              SlideSpanFrameRect(pfd->mFrame, deltaWidth);
            }
          }
        }
        return true;
      }
    }
    else if (!pfd->GetFlag(PFD_ISTEXTFRAME) &&
             !pfd->GetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE)) {
      
      
      *aDeltaWidth = 0;
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
        pfd->mBounds.width -= trimOutput.mDeltaWidth;

        
        if (psd != mRootSpan) {
          
          
          pfd->mFrame->SetRect(pfd->mBounds);
        }

        
        psd->mX -= trimOutput.mDeltaWidth;

        
        
        
        
        while (pfd->mNext) {
          pfd = pfd->mNext;
          pfd->mBounds.x -= trimOutput.mDeltaWidth;
          if (psd != mRootSpan) {
            
            
            
            
            
            
            SlideSpanFrameRect(pfd->mFrame, trimOutput.mDeltaWidth);
          }
        }
      }

      if (pfd->GetFlag(PFD_ISNONEMPTYTEXTFRAME) || trimOutput.mChanged) {
        
        *aDeltaWidth = trimOutput.mDeltaWidth;
        return true;
      }
    }
    pfd = pfd->mPrev;
  }

  *aDeltaWidth = 0;
  return false;
}

bool
nsLineLayout::TrimTrailingWhiteSpace()
{
  PerSpanData* psd = mRootSpan;
  nscoord deltaWidth;
  TrimTrailingWhiteSpaceIn(psd, &deltaWidth);
  return 0 != deltaWidth;
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

  nscoord deltaX = 0;
  for (PerFrameData* pfd = aPSD->mFirstFrame; pfd != nullptr; pfd = pfd->mNext) {
    
    if (!pfd->GetFlag(PFD_ISBULLET)) {
      nscoord dw = 0;
      
      pfd->mBounds.x += deltaX;
      
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
      
      pfd->mBounds.width += dw;

      deltaX += dw;
      pfd->mFrame->SetRect(pfd->mBounds);
    }
  }
  return deltaX;
}

void
nsLineLayout::HorizontalAlignFrames(nsRect& aLineBounds,
                                    bool aIsLastLine)
{
  



  PerSpanData* psd = mRootSpan;
  NS_WARN_IF_FALSE(psd->mRightEdge != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained width; this should only result from "
                   "very large sizes, not attempts at intrinsic width "
                   "calculation");
  nscoord availWidth = psd->mRightEdge - psd->mLeftEdge;
  nscoord remainingWidth = availWidth - aLineBounds.width;
#ifdef NOISY_HORIZONTAL_ALIGN
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": availWidth=%d lineWidth=%d delta=%d\n",
           availWidth, aLineBounds.width, remainingWidth);
#endif
  nscoord dx = 0;

  if (remainingWidth > 0 &&
      !(mBlockReflowState->frame->IsSVGText())) {
    uint8_t textAlign = mStyleText->mTextAlign;

    






    if (aIsLastLine) {
      if (mStyleText->mTextAlignLast == NS_STYLE_TEXT_ALIGN_AUTO) {
        if (textAlign == NS_STYLE_TEXT_ALIGN_JUSTIFY) {
          textAlign = NS_STYLE_TEXT_ALIGN_DEFAULT;
        }
      } else {
        textAlign = mStyleText->mTextAlignLast;
      }
    }

    switch (textAlign) {
      case NS_STYLE_TEXT_ALIGN_JUSTIFY:
        int32_t numSpaces;
        int32_t numLetters;
            
        ComputeJustificationWeights(psd, &numSpaces, &numLetters);

        if (numSpaces > 0) {
          FrameJustificationState state =
            { numSpaces, numLetters, remainingWidth, 0, 0, 0, 0, 0 };

          
          
          aLineBounds.width += ApplyFrameJustification(psd, &state);
          remainingWidth = availWidth - aLineBounds.width;
          break;
        }
        
        

      case NS_STYLE_TEXT_ALIGN_DEFAULT:
        if (NS_STYLE_DIRECTION_LTR == psd->mDirection) {
          
          break;
        }
        
        

      case NS_STYLE_TEXT_ALIGN_RIGHT:
      case NS_STYLE_TEXT_ALIGN_MOZ_RIGHT:
        dx = remainingWidth;
        break;

      case NS_STYLE_TEXT_ALIGN_END:
        if (NS_STYLE_DIRECTION_LTR == psd->mDirection) {
          
          dx = remainingWidth;
          break;
        }
        
        

      case NS_STYLE_TEXT_ALIGN_LEFT:
      case NS_STYLE_TEXT_ALIGN_MOZ_LEFT:
        break;

      case NS_STYLE_TEXT_ALIGN_CENTER:
      case NS_STYLE_TEXT_ALIGN_MOZ_CENTER:
        dx = remainingWidth / 2;
        break;
    }
  }
  else if (remainingWidth < 0) {
    if (NS_STYLE_DIRECTION_RTL == psd->mDirection) {
      dx = remainingWidth;
      psd->mX += dx;
      psd->mLeftEdge += dx;
    }
  }

  if (NS_STYLE_DIRECTION_RTL == psd->mDirection &&
      !psd->mChangedFrameDirection) {
    if (psd->mLastFrame->GetFlag(PFD_ISBULLET) ) {
      PerFrameData* bulletPfd = psd->mLastFrame;
      bulletPfd->mBounds.x -= remainingWidth;
      bulletPfd->mFrame->SetRect(bulletPfd->mBounds);
    }
    psd->mChangedFrameDirection = true;
  }

  if (dx) {
    for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      pfd->mBounds.x += dx;
      pfd->mFrame->SetRect(pfd->mBounds);
    }
    aLineBounds.x += dx;
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
  if (nullptr != psd->mFrame) {
    
    
    
    
    
    
    
    
    
    
    nsRect adjustedBounds(nsPoint(0, 0), psd->mFrame->mFrame->GetSize());

    overflowAreas.ScrollableOverflow().UnionRect(
      psd->mFrame->mOverflowAreas.ScrollableOverflow(), adjustedBounds);
    overflowAreas.VisualOverflow().UnionRect(
      psd->mFrame->mOverflowAreas.VisualOverflow(), adjustedBounds);
  }
  else {
    
    
    
    
    overflowAreas.VisualOverflow().x = psd->mLeftEdge;
    
    
    overflowAreas.VisualOverflow().width =
      psd->mX - overflowAreas.VisualOverflow().x;
    overflowAreas.VisualOverflow().y = mTopEdge;
    overflowAreas.VisualOverflow().height = mFinalLineHeight;

    overflowAreas.ScrollableOverflow() = overflowAreas.VisualOverflow();
  }

  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    nsIFrame* frame = pfd->mFrame;
    nsPoint origin = frame->GetPosition();

    
    if (pfd->GetFlag(PFD_RELATIVEPOS)) {
      
      
      nsPoint change(pfd->mOffsets.left, pfd->mOffsets.top);
      origin += change;
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
