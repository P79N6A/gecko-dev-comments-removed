








#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "nsLineLayout.h"

#include "SVGTextFrame.h"
#include "nsBlockFrame.h"
#include "nsFontMetrics.h"
#include "nsStyleConsts.h"
#include "nsContainerFrame.h"
#include "nsFloatManager.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsGkAtoms.h"
#include "nsIContent.h"
#include "nsLayoutUtils.h"
#include "nsTextFrame.h"
#include "nsStyleStructInlines.h"
#include "nsBidiPresUtils.h"
#include "nsRubyFrame.h"
#include "nsRubyTextFrame.h"
#include "RubyUtils.h"
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
                           const nsLineList::iterator* aLine,
                           nsLineLayout* aBaseLineLayout)
  : mPresContext(aPresContext),
    mFloatManager(aFloatManager),
    mBlockReflowState(aOuterReflowState),
    mBaseLineLayout(aBaseLineLayout),
    mLastOptionalBreakFrame(nullptr),
    mForceBreakFrame(nullptr),
    mBlockRS(nullptr),
    mLastOptionalBreakPriority(gfxBreakPriority::eNoBreak),
    mLastOptionalBreakFrameOffset(-1),
    mForceBreakFrameOffset(-1),
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
    mLineAtStart(false),
    mHasRuby(false),
    mSuppressLineWrap(aOuterReflowState->frame->IsSVGText())
{
  MOZ_ASSERT(aOuterReflowState, "aOuterReflowState must not be null");
  NS_ASSERTION(aFloatManager || aOuterReflowState->frame->GetType() ==
                                  nsGkAtoms::letterFrame,
               "float manager should be present");
  MOZ_ASSERT((!!mBaseLineLayout) ==
             (aOuterReflowState->frame->GetType() ==
              nsGkAtoms::rubyTextContainerFrame),
             "Only ruby text container frames have "
             "a different base line layout");
  MOZ_COUNT_CTOR(nsLineLayout);

  
  nsBlockFrame* blockFrame = do_QueryFrame(aOuterReflowState->frame);
  if (blockFrame)
    mStyleText = blockFrame->StyleTextForLineLayout();
  else
    mStyleText = aOuterReflowState->frame->StyleText();

  mLineNumber = 0;
  mTotalPlacedFrames = 0;
  mBStartEdge = 0;
  mTrimmableISize = 0;

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
                              const nsSize& aContainerSize)
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
  if (!mBaseLineLayout) {
    mLineIsEmpty = true;
    mLineAtStart = true;
  } else {
    mLineIsEmpty = false;
    mLineAtStart = false;
  }
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
  mContainerSize = aContainerSize;

  
  
  if (!(LineContainerFrame()->GetStateBits() &
        NS_FRAME_IN_CONSTRAINED_BSIZE)) {

    
    
    
    nscoord maxLineBoxWidth =
      LineContainerFrame()->PresContext()->PresShell()->MaxLineBoxWidth();

    if (maxLineBoxWidth > 0 &&
        psd->mIEnd - psd->mIStart > maxLineBoxWidth) {
      psd->mIEnd = psd->mIStart + maxLineBoxWidth;
    }
  }

  mBStartEdge = aBCoord;

  psd->mNoWrap = !mStyleText->WhiteSpaceCanWrapStyle() || mSuppressLineWrap;
  psd->mWritingMode = aWritingMode;

  
  

  if (0 == mLineNumber && !HasPrevInFlow(mBlockReflowState->frame)) {
    const nsStyleCoord &textIndent = mStyleText->mTextIndent;
    nscoord pctBasis = 0;
    if (textIndent.HasPercent()) {
      pctBasis =
        nsHTMLReflowState::GetContainingBlockContentWidth(mBlockReflowState);
    }
    nscoord indent = nsRuleNode::ComputeCoordPercentCalc(textIndent, pctBasis);

    mTextIndent = indent;

    psd->mICoord += indent;
  }

  PerFrameData* pfd = NewPerFrameData(mBlockReflowState->frame);
  pfd->mAscent = 0;
  pfd->mSpan = psd;
  psd->mFrame = pfd;
  nsIFrame* frame = mBlockReflowState->frame;
  if (frame->GetType() == nsGkAtoms::rubyTextContainerFrame) {
    
    
    MOZ_ASSERT(mBaseLineLayout != this);
    pfd->mRelativePos =
      mBlockReflowState->mStyleDisplay->IsRelativelyPositionedStyle();
    if (pfd->mRelativePos) {
      MOZ_ASSERT(
        mBlockReflowState->GetWritingMode() == frame->GetWritingMode(),
        "mBlockReflowState->frame == frame, "
        "hence they should have identical writing mode");
      pfd->mOffsets = mBlockReflowState->ComputedLogicalOffsets();
    }
  }
}

void
nsLineLayout::EndLineReflow()
{
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": EndLineReflow: width=%d\n", mRootSpan->mICoord - mRootSpan->mIStart);
#endif

  NS_ASSERTION(!mBaseLineLayout ||
               (!mSpansAllocated && !mSpansFreed && !mSpanFreeList &&
                !mFramesAllocated && !mFramesFreed && !mFrameFreeList),
               "Allocated frames or spans on non-base line layout?");

  UnlinkFrame(mRootSpan->mFrame);
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
nsLineLayout::UpdateBand(WritingMode aWM,
                         const LogicalRect& aNewAvailSpace,
                         nsIFrame* aFloatFrame)
{
  WritingMode lineWM = mRootSpan->mWritingMode;
  
  
  LogicalRect availSpace = aNewAvailSpace.ConvertTo(lineWM, aWM,
                                                    ContainerWidth());
#ifdef REALLY_NOISY_REFLOW
  printf("nsLL::UpdateBand %d, %d, %d, %d, (converted to %d, %d, %d, %d); frame=%p\n  will set mImpacted to true\n",
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
                   availSpace.ISize(lineWM) != NS_UNCONSTRAINEDSIZE,
                   "have unconstrained inline size; this should only result "
                   "from very large sizes, not attempts at intrinsic width "
                   "calculation");
  
  nscoord deltaICoord = availSpace.IStart(lineWM) - mRootSpan->mIStart;
  
  
  nscoord deltaISize = availSpace.ISize(lineWM) -
                       (mRootSpan->mIEnd - mRootSpan->mIStart);
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": UpdateBand: %d,%d,%d,%d deltaISize=%d deltaICoord=%d\n",
         availSpace.IStart(lineWM), availSpace.BStart(lineWM),
         availSpace.ISize(lineWM), availSpace.BSize(lineWM),
         deltaISize, deltaICoord);
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
  nsLineLayout* outerLineLayout = GetOutermostLineLayout();
  PerSpanData* psd = outerLineLayout->mSpanFreeList;
  if (!psd) {
    void *mem;
    size_t sz = sizeof(PerSpanData);
    PL_ARENA_ALLOCATE(mem, &outerLineLayout->mArena, sz);
    if (!mem) {
      NS_ABORT_OOM(sz);
    }
    psd = reinterpret_cast<PerSpanData*>(mem);
  }
  else {
    outerLineLayout->mSpanFreeList = psd->mNextFreeSpan;
  }
  psd->mParent = nullptr;
  psd->mFrame = nullptr;
  psd->mFirstFrame = nullptr;
  psd->mLastFrame = nullptr;
  psd->mContainsFloat = false;
  psd->mHasNonemptyContent = false;

#ifdef DEBUG
  outerLineLayout->mSpansAllocated++;
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
  psd->mNoWrap = !frame->StyleText()->WhiteSpaceCanWrap(frame) ||
                 mSuppressLineWrap ||
                 frame->StyleContext()->ShouldSuppressLineBreak();
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

void
nsLineLayout::AttachFrameToBaseLineLayout(PerFrameData* aFrame)
{
  NS_PRECONDITION(mBaseLineLayout,
                  "This method must not be called in a base line layout.");

  PerFrameData* baseFrame = mBaseLineLayout->LastFrame();
  MOZ_ASSERT(aFrame && baseFrame);
  MOZ_ASSERT(!aFrame->mIsLinkedToBase,
             "The frame must not have been linked with the base");
#ifdef DEBUG
  nsIAtom* baseType = baseFrame->mFrame->GetType();
  nsIAtom* annotationType = aFrame->mFrame->GetType();
  MOZ_ASSERT((baseType == nsGkAtoms::rubyBaseContainerFrame &&
              annotationType == nsGkAtoms::rubyTextContainerFrame) ||
             (baseType == nsGkAtoms::rubyBaseFrame &&
              annotationType == nsGkAtoms::rubyTextFrame));
#endif

  aFrame->mNextAnnotation = baseFrame->mNextAnnotation;
  baseFrame->mNextAnnotation = aFrame;
  aFrame->mIsLinkedToBase = true;
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

      
      UnlinkFrame(next);
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

  
  MOZ_ASSERT(!pfd->mNext);
  UnlinkFrame(pfd);
#ifdef NOISY_PUSHING
  nsFrame::IndentBy(stdout, mSpanDepth);
  printf("PushFrame: %p after:\n", psd);
  DumpPerSpanData(psd, 1);
#endif
}

void
nsLineLayout::UnlinkFrame(PerFrameData* pfd)
{
  while (nullptr != pfd) {
    PerFrameData* next = pfd->mNext;
    if (pfd->mIsLinkedToBase) {
      
      
      
      pfd->mNext = pfd->mPrev = nullptr;
      pfd = next;
      continue;
    }

    
    
    PerFrameData* annotationPFD = pfd->mNextAnnotation;
    while (annotationPFD) {
      PerFrameData* nextAnnotation = annotationPFD->mNextAnnotation;
      MOZ_ASSERT(annotationPFD->mNext == nullptr &&
                 annotationPFD->mPrev == nullptr,
                 "PFD in annotations should have been unlinked.");
      FreeFrame(annotationPFD);
      annotationPFD = nextAnnotation;
    }

    FreeFrame(pfd);
    pfd = next;
  }
}

void
nsLineLayout::FreeFrame(PerFrameData* pfd)
{
  if (nullptr != pfd->mSpan) {
    FreeSpan(pfd->mSpan);
  }
  nsLineLayout* outerLineLayout = GetOutermostLineLayout();
  pfd->mNext = outerLineLayout->mFrameFreeList;
  outerLineLayout->mFrameFreeList = pfd;
#ifdef DEBUG
  outerLineLayout->mFramesFreed++;
#endif
}

void
nsLineLayout::FreeSpan(PerSpanData* psd)
{
  
  UnlinkFrame(psd->mFirstFrame);

  nsLineLayout* outerLineLayout = GetOutermostLineLayout();
  
  psd->mNextFreeSpan = outerLineLayout->mSpanFreeList;
  outerLineLayout->mSpanFreeList = psd;
#ifdef DEBUG
  outerLineLayout->mSpansFreed++;
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
  nsLineLayout* outerLineLayout = GetOutermostLineLayout();
  PerFrameData* pfd = outerLineLayout->mFrameFreeList;
  if (!pfd) {
    void *mem;
    size_t sz = sizeof(PerFrameData);
    PL_ARENA_ALLOCATE(mem, &outerLineLayout->mArena, sz);
    if (!mem) {
      NS_ABORT_OOM(sz);
    }
    pfd = reinterpret_cast<PerFrameData*>(mem);
  }
  else {
    outerLineLayout->mFrameFreeList = pfd->mNext;
  }
  pfd->mSpan = nullptr;
  pfd->mNext = nullptr;
  pfd->mPrev = nullptr;
  pfd->mNextAnnotation = nullptr;
  pfd->mFrame = aFrame;

  
  pfd->mRelativePos = false;
  pfd->mIsTextFrame = false;
  pfd->mIsNonEmptyTextFrame = false;
  pfd->mIsNonWhitespaceTextFrame = false;
  pfd->mIsLetterFrame = false;
  pfd->mRecomputeOverflow = false;
  pfd->mIsBullet = false;
  pfd->mSkipWhenTrimmingWhitespace = false;
  pfd->mIsEmpty = false;
  pfd->mIsLinkedToBase = false;

  WritingMode frameWM = aFrame->GetWritingMode();
  WritingMode lineWM = mRootSpan->mWritingMode;
  pfd->mBounds = LogicalRect(lineWM);
  pfd->mOverflowAreas.Clear();
  pfd->mMargin = LogicalMargin(lineWM);
  pfd->mBorderPadding = LogicalMargin(lineWM);
  pfd->mOffsets = LogicalMargin(frameWM);

  pfd->mJustificationInfo = JustificationInfo();
  pfd->mJustificationAssignment = JustificationAssignment();

#ifdef DEBUG
  pfd->mBlockDirAlign = 0xFF;
  outerLineLayout->mFramesAllocated++;
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

  mJustificationInfo = JustificationInfo();

  
  
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
    if (reflowState.ComputedISize() == NS_UNCONSTRAINEDSIZE) {
      reflowState.AvailableISize() = availableSpaceOnLine;
    }
    WritingMode stateWM = reflowState.GetWritingMode();
    pfd->mMargin =
      reflowState.ComputedLogicalMargin().ConvertTo(lineWM, stateWM);
    pfd->mBorderPadding =
      reflowState.ComputedLogicalBorderPadding().ConvertTo(lineWM, stateWM);
    pfd->mRelativePos =
      reflowState.mStyleDisplay->IsRelativelyPositionedStyle();
    if (pfd->mRelativePos) {
      pfd->mOffsets =
        reflowState.ComputedLogicalOffsets().ConvertTo(frameWM, stateWM);
    }

    
    
    
    
    AllowForStartMargin(pfd, reflowState);
  }
  
  

  
  
  
  
  
  
  
  if (mGotLineBox && IsPercentageAware(aFrame)) {
    mLineBox->DisableResizeReflowOptimization();
  }

  
  

  
  nsHTMLReflowMetrics metrics(lineWM);
#ifdef DEBUG
  metrics.ISize(lineWM) = nscoord(0xdeadbeef);
  metrics.BSize(lineWM) = nscoord(0xdeadbeef);
#endif
  nscoord tI = pfd->mBounds.LineLeft(lineWM, ContainerWidth());
  nscoord tB = pfd->mBounds.BStart(lineWM);
  mFloatManager->Translate(tI, tB);

  int32_t savedOptionalBreakOffset;
  gfxBreakPriority savedOptionalBreakPriority;
  nsIFrame* savedOptionalBreakFrame =
    GetLastOptionalBreakPosition(&savedOptionalBreakOffset,
                                 &savedOptionalBreakPriority);

  if (!isText) {
    aFrame->Reflow(mPresContext, metrics, *reflowStateHolder, aReflowStatus);
  } else {
    static_cast<nsTextFrame*>(aFrame)->
      ReflowText(*this, availableSpaceOnLine, psd->mReflowState->rendContext,
                 metrics, aReflowStatus);
  }

  pfd->mJustificationInfo = mJustificationInfo;
  mJustificationInfo = JustificationInfo();

  
  
  
  bool placedFloat = false;
  bool isEmpty;
  if (!frameType) {
    isEmpty = pfd->mFrame->IsEmpty();
  } else {
    if (nsGkAtoms::placeholderFrame == frameType) {
      isEmpty = true;
      pfd->mSkipWhenTrimmingWhitespace = true;
      nsIFrame* outOfFlowFrame = nsLayoutUtils::GetFloatFromPlaceholder(aFrame);
      if (outOfFlowFrame) {
        
        
        
        nscoord availableISize = psd->mIEnd - (psd->mICoord - mTrimmableISize);
        if (psd->mNoWrap) {
          
          
          
          
          
          
          
          
          
          availableISize = 0;
        }
        placedFloat = GetOutermostLineLayout()->
          AddFloat(outOfFlowFrame, availableISize);
        NS_ASSERTION(!(outOfFlowFrame->GetType() == nsGkAtoms::letterFrame &&
                       GetFirstLetterStyleOK()),
                    "FirstLetterStyle set on line with floating first letter");
      }
    }
    else if (isText) {
      
      pfd->mIsTextFrame = true;
      nsTextFrame* textFrame = static_cast<nsTextFrame*>(pfd->mFrame);
      isEmpty = !textFrame->HasNoncollapsedCharacters();
      if (!isEmpty) {
        pfd->mIsNonEmptyTextFrame = true;
        nsIContent* content = textFrame->GetContent();

        const nsTextFragment* frag = content->GetText();
        if (frag) {
          pfd->mIsNonWhitespaceTextFrame = !content->TextIsOnlyWhitespace();
        }
      }
    }
    else if (nsGkAtoms::brFrame == frameType) {
      pfd->mSkipWhenTrimmingWhitespace = true;
      isEmpty = false;
    } else {
      if (nsGkAtoms::letterFrame==frameType) {
        pfd->mIsLetterFrame = true;
      }
      if (pfd->mSpan) {
        isEmpty = !pfd->mSpan->mHasNonemptyContent && pfd->mFrame->IsSelfEmpty();
      } else {
        isEmpty = pfd->mFrame->IsEmpty();
      }
    }
  }
  pfd->mIsEmpty = isEmpty;

  mFloatManager->Translate(-tI, -tB);

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

  
  aFrame->SetRect(lineWM, pfd->mBounds, ContainerWidthForSpan(psd));

  
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
    
    
    if (!continuingTextRun && !pfd->mSkipWhenTrimmingWhitespace) {
      mTrimmableISize = 0;
    }

    
    
    bool optionalBreakAfterFits;
    NS_ASSERTION(isText ||
                 !reflowStateHolder->IsFloating(),
                 "How'd we get a floated inline frame? "
                 "The frame ctor should've dealt with this.");
    if (CanPlaceFrame(pfd, notSafeToBreak, continuingTextRun,
                      savedOptionalBreakFrame != nullptr, metrics,
                      aReflowStatus, &optionalBreakAfterFits)) {
      if (!isEmpty) {
        psd->mHasNonemptyContent = true;
        mLineIsEmpty = false;
        if (!pfd->mSpan) {
          
          mLineAtStart = false;
        }
        if (nsGkAtoms::rubyFrame == frameType) {
          mHasRuby = true;
          SyncAnnotationBounds(pfd);
        }
      }

      
      
      
      PlaceFrame(pfd, metrics);
      PerSpanData* span = pfd->mSpan;
      if (span) {
        
        
        
        VerticalAlignFrames(span);
      }
      
      if (!continuingTextRun) {
        if (!psd->mNoWrap && (!LineIsEmpty() || placedFloat)) {
          
          
          
          if (NotifyOptionalBreakPosition(aFrame, INT32_MAX,
                                          optionalBreakAfterFits,
                                          gfxBreakPriority::eNormalBreak)) {
            
            aReflowStatus = NS_INLINE_LINE_BREAK_AFTER(aReflowStatus);
          }
        }
      }
    }
    else {
      PushFrame(aFrame);
      aPushedFrame = true;
      
      
      RestoreSavedBreakPosition(savedOptionalBreakFrame,
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

  WritingMode lineWM = mRootSpan->mWritingMode;

  
  
  
  
  
  
  
  if ((pfd->mFrame->GetPrevContinuation() ||
       pfd->mFrame->FrameIsNonFirstInIBSplit()) &&
      aReflowState.mStyleBorder->mBoxDecorationBreak ==
        NS_STYLE_BOX_DECORATION_BREAK_SLICE) {
    
    
    pfd->mMargin.IStart(lineWM) = 0;
  } else {
    NS_WARN_IF_FALSE(NS_UNCONSTRAINEDSIZE != aReflowState.AvailableISize(),
                     "have unconstrained inline-size; this should only result "
                     "from very large sizes, not attempts at intrinsic "
                     "inline-size calculation");
    if (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedISize()) {
      
      
      
      
      WritingMode wm = aReflowState.GetWritingMode();
      aReflowState.AvailableISize() -=
          pfd->mMargin.ConvertTo(wm, lineWM).IStart(wm);
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









void
nsLineLayout::SyncAnnotationBounds(PerFrameData* aRubyFrame)
{
  MOZ_ASSERT(aRubyFrame->mFrame->GetType() == nsGkAtoms::rubyFrame);
  MOZ_ASSERT(aRubyFrame->mSpan);

  PerSpanData* span = aRubyFrame->mSpan;
  WritingMode lineWM = mRootSpan->mWritingMode;
  for (PerFrameData* pfd = span->mFirstFrame; pfd; pfd = pfd->mNext) {
    for (PerFrameData* rtc = pfd->mNextAnnotation;
         rtc; rtc = rtc->mNextAnnotation) {
      
      
      
      LogicalRect rtcBounds(lineWM, rtc->mFrame->GetRect(), 0);
      rtc->mBounds = rtcBounds;
      nscoord rtcWidth = rtcBounds.Width(lineWM);
      for (PerFrameData* rt = rtc->mSpan->mFirstFrame; rt; rt = rt->mNext) {
        LogicalRect rtBounds = rt->mFrame->GetLogicalRect(lineWM, rtcWidth);
        MOZ_ASSERT(rt->mBounds.Size(lineWM) == rtBounds.Size(lineWM),
                   "Size of the annotation should not have been changed");
        rt->mBounds.SetOrigin(lineWM, rtBounds.Origin(lineWM));
      }
    }
  }
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

  WritingMode lineWM = mRootSpan->mWritingMode;
  
















  if ((NS_FRAME_IS_NOT_COMPLETE(aStatus) ||
       pfd->mFrame->LastInFlow()->GetNextContinuation() ||
       pfd->mFrame->FrameIsNonLastInIBSplit()) &&
      !pfd->mIsLetterFrame &&
      pfd->mFrame->StyleBorder()->mBoxDecorationBreak ==
        NS_STYLE_BOX_DECORATION_BREAK_SLICE) {
    pfd->mMargin.IEnd(lineWM) = 0;
  }

  
  nscoord startMargin = pfd->mMargin.IStart(lineWM);
  nscoord endMargin = pfd->mMargin.IEnd(lineWM);

  pfd->mBounds.IStart(lineWM) += startMargin;

  PerSpanData* psd = mCurrentSpan;
  if (psd->mNoWrap) {
    
    return true;
  }

#ifdef NOISY_CAN_PLACE_FRAME
  if (nullptr != psd->mFrame) {
    nsFrame::ListTag(stdout, psd->mFrame->mFrame);
  }
  printf(": aNotSafeToBreak=%s frame=", aNotSafeToBreak ? "true" : "false");
  nsFrame::ListTag(stdout, pfd->mFrame);
  printf(" frameWidth=%d, margins=%d,%d\n",
         pfd->mBounds.IEnd(lineWM) + endMargin - psd->mICoord,
         startMargin, endMargin);
#endif

  
  
  bool outside = pfd->mBounds.IEnd(lineWM) - mTrimmableISize + endMargin >
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
  WritingMode lineWM = mRootSpan->mWritingMode;

  
  
  
  if (pfd->mFrame->GetWritingMode().GetBlockDir() != lineWM.GetBlockDir()) {
    pfd->mAscent = lineWM.IsLineInverted() ? 0 : aMetrics.BSize(lineWM);
  } else {
    if (aMetrics.BlockStartAscent() == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
      pfd->mAscent = pfd->mFrame->GetLogicalBaseline(lineWM);
    } else {
      pfd->mAscent = aMetrics.BlockStartAscent();
    }
  }

  
  mCurrentSpan->mICoord = pfd->mBounds.IEnd(lineWM) +
                          pfd->mMargin.IEnd(lineWM);

  
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
  pfd->mIsBullet = true;
  if (aMetrics.BlockStartAscent() == nsHTMLReflowMetrics::ASK_FOR_BASELINE) {
    pfd->mAscent = aFrame->GetLogicalBaseline(lineWM);
  } else {
    pfd->mAscent = aMetrics.BlockStartAscent();
  }

  
  pfd->mBounds = LogicalRect(lineWM, aFrame->GetRect(), ContainerWidth());
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
                                               ContainerWidth());
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
      pfd->mFrame->SetRect(lineWM, pfd->mBounds, ContainerWidth());
    }
  }
  PlaceTopBottomFrames(psd, -mBStartEdge, lineBSize);

  mFinalLineBSize = lineBSize;
  if (mGotLineBox) {
    
    mLineBox->SetBounds(lineWM,
                        psd->mIStart, mBStartEdge,
                        psd->mICoord - psd->mIStart, lineBSize,
                        ContainerWidth());

    mLineBox->SetLogicalAscent(baselineBCoord - mBStartEdge);
#ifdef NOISY_BLOCKDIR_ALIGN
    printf(
      "  [line]==> bounds{x,y,w,h}={%d,%d,%d,%d} lh=%d a=%d\n",
      mLineBox->GetBounds().IStart(lineWM), mLineBox->GetBounds().BStart(lineWM),
      mLineBox->GetBounds().ISize(lineWM), mLineBox->GetBounds().BSize(lineWM),
      mFinalLineBSize, mLineBox->GetLogicalAscent());
#endif
  }
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
    WritingMode lineWM = mRootSpan->mWritingMode;
    nscoord containerWidth = ContainerWidthForSpan(psd);
    switch (pfd->mBlockDirAlign) {
      case VALIGN_TOP:
        if (span) {
          pfd->mBounds.BStart(lineWM) = -aDistanceFromStart - span->mMinBCoord;
        }
        else {
          pfd->mBounds.BStart(lineWM) =
            -aDistanceFromStart + pfd->mMargin.BStart(lineWM);
        }
        pfd->mFrame->SetRect(lineWM, pfd->mBounds, containerWidth);
#ifdef NOISY_BLOCKDIR_ALIGN
        printf("    ");
        nsFrame::ListTag(stdout, pfd->mFrame);
        printf(": y=%d dTop=%d [bp.top=%d topLeading=%d]\n",
               pfd->mBounds.BStart(lineWM), aDistanceFromStart,
               span ? pfd->mBorderPadding.BStart(lineWM) : 0,
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
            pfd->mMargin.BEnd(lineWM) - pfd->mBounds.BSize(lineWM);
        }
        pfd->mFrame->SetRect(lineWM, pfd->mBounds, containerWidth);
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

  bool preMode = mStyleText->WhiteSpaceIsSignificant();

  
  
  
  
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
           spanFramePFD->mBorderPadding.Top(lineWM),
           spanFramePFD->mBorderPadding.Right(lineWM),
           spanFramePFD->mBorderPadding.Bottom(lineWM),
           spanFramePFD->mBorderPadding.Left(lineWM),
           spanFramePFD->mMargin.Top(lineWM),
           spanFramePFD->mMargin.Right(lineWM),
           spanFramePFD->mMargin.Bottom(lineWM),
           spanFramePFD->mMargin.Left(lineWM));
  }
  printf("\n");
#endif

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  bool zeroEffectiveSpanBox = false;
  
  
  
  
  if ((emptyContinuation ||
       mPresContext->CompatibilityMode() != eCompatibility_FullStandards) &&
      ((psd == mRootSpan) ||
       (spanFramePFD->mBorderPadding.IsAllZero() &&
        spanFramePFD->mMargin.IsAllZero()))) {
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    zeroEffectiveSpanBox = true;
    for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      if (pfd->mIsTextFrame &&
          (pfd->mIsNonWhitespaceTextFrame || preMode ||
           pfd->mBounds.ISize(mRootSpan->mWritingMode) != 0)) {
        zeroEffectiveSpanBox = false;
        break;
      }
    }
  }

  
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
      spanFramePFD->mBorderPadding.BStartEnd(lineWM);

    
    
    if (spanFramePFD->mIsLetterFrame &&
        !spanFrame->GetPrevInFlow() &&
        spanFrame->StyleText()->mLineHeight.GetUnit() == eStyleUnit_Normal) {
      logicalBSize = spanFramePFD->mBounds.BSize(lineWM);
    }

    nscoord leading = logicalBSize - contentBSize;
    psd->mBStartLeading = leading / 2;
    psd->mBEndLeading = leading - psd->mBStartLeading;
    psd->mLogicalBSize = logicalBSize;
    if (spanFrame->GetType() == nsGkAtoms::rubyFrame) {
      
      
      
      auto rubyFrame = static_cast<nsRubyFrame*>(spanFrame);
      nscoord startLeading, endLeading;
      rubyFrame->GetBlockLeadings(startLeading, endLeading);
      nscoord deltaLeading = startLeading + endLeading - leading;
      if (deltaLeading > 0) {
        
        
        
        if (startLeading < psd->mBStartLeading) {
          psd->mBEndLeading += deltaLeading;
        } else if (endLeading < psd->mBEndLeading) {
          psd->mBStartLeading += deltaLeading;
        } else {
          psd->mBStartLeading = startLeading;
          psd->mBEndLeading = endLeading;
        }
        psd->mLogicalBSize += deltaLeading;
        
        
        zeroEffectiveSpanBox = false;
      }
    }

    if (zeroEffectiveSpanBox) {
      
      
      
      

      
      
      minBCoord = BLOCKDIR_ALIGN_FRAMES_NO_MINIMUM;
      maxBCoord = BLOCKDIR_ALIGN_FRAMES_NO_MAXIMUM;
    }
    else {

      
      
      
      
      
      minBCoord = spanFramePFD->mBorderPadding.BStart(lineWM) -
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
           spanFramePFD->mBorderPadding.Top(lineWM),
           spanFramePFD->mBorderPadding.Bottom(lineWM),
           zeroEffectiveSpanBox ? "yes" : "no");
#endif
  }

  nscoord maxStartBoxBSize = 0;
  nscoord maxEndBoxBSize = 0;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nullptr != pfd) {
    nsIFrame* frame = pfd->mFrame;

    
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
                     pfd->mMargin.BStartEnd(lineWM);
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
      if (lineWM.IsVertical()) {
        if (verticalAlignEnum == NS_STYLE_VERTICAL_ALIGN_MIDDLE) {
          
          
          
          
          if (!lineWM.IsSideways()) {
            verticalAlignEnum = NS_STYLE_VERTICAL_ALIGN_MIDDLE_WITH_BASELINE;
          }
        } else if (lineWM.IsLineInverted()) {
          
          
          switch (verticalAlignEnum) {
            case NS_STYLE_VERTICAL_ALIGN_TOP:
              verticalAlignEnum = NS_STYLE_VERTICAL_ALIGN_BOTTOM;
              break;
            case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
              verticalAlignEnum = NS_STYLE_VERTICAL_ALIGN_TOP;
              break;
            case NS_STYLE_VERTICAL_ALIGN_TEXT_TOP:
              verticalAlignEnum = NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM;
              break;
            case NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM:
              verticalAlignEnum = NS_STYLE_VERTICAL_ALIGN_TEXT_TOP;
              break;
          }
        }
      }

      
      nscoord revisedBaselineBCoord = baselineBCoord;

      
      
      if (verticalAlignEnum == NS_STYLE_VERTICAL_ALIGN_SUB ||
          verticalAlignEnum == NS_STYLE_VERTICAL_ALIGN_SUPER) {
        revisedBaselineBCoord += lineWM.FlowRelativeToLineRelativeFactor() *
          (verticalAlignEnum == NS_STYLE_VERTICAL_ALIGN_SUB
            ? fm->SubscriptOffset() : -fm->SuperscriptOffset());
        verticalAlignEnum = NS_STYLE_VERTICAL_ALIGN_BASELINE;
      }

      switch (verticalAlignEnum) {
        default:
        case NS_STYLE_VERTICAL_ALIGN_BASELINE:
          if (lineWM.IsVertical() && !lineWM.IsSideways()) {
            if (frameSpan) {
              pfd->mBounds.BStart(lineWM) = revisedBaselineBCoord -
                                            pfd->mBounds.BSize(lineWM)/2;
            } else {
              pfd->mBounds.BStart(lineWM) = revisedBaselineBCoord -
                                            logicalBSize/2 +
                                            pfd->mMargin.BStart(lineWM);
            }
          } else {
            pfd->mBounds.BStart(lineWM) = revisedBaselineBCoord - pfd->mAscent;
          }
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;

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
          
          
          nscoord parentXHeight =
            lineWM.FlowRelativeToLineRelativeFactor() * fm->XHeight();
          if (frameSpan) {
            pfd->mBounds.BStart(lineWM) = baselineBCoord -
              (parentXHeight + pfd->mBounds.BSize(lineWM))/2;
          }
          else {
            pfd->mBounds.BStart(lineWM) = baselineBCoord -
              (parentXHeight + logicalBSize)/2 +
              pfd->mMargin.BStart(lineWM);
          }
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TEXT_TOP:
        {
          
          
          
          
          nscoord parentAscent =
            lineWM.IsLineInverted() ? fm->MaxDescent() : fm->MaxAscent();
          if (frameSpan) {
            pfd->mBounds.BStart(lineWM) = baselineBCoord - parentAscent -
              pfd->mBorderPadding.BStart(lineWM) + frameSpan->mBStartLeading;
          }
          else {
            pfd->mBounds.BStart(lineWM) = baselineBCoord - parentAscent +
                                          pfd->mMargin.BStart(lineWM);
          }
          pfd->mBlockDirAlign = VALIGN_OTHER;
          break;
        }

        case NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM:
        {
          
          
          nscoord parentDescent =
            lineWM.IsLineInverted() ? fm->MaxAscent() : fm->MaxDescent();
          if (frameSpan) {
            pfd->mBounds.BStart(lineWM) = baselineBCoord + parentDescent -
                                          pfd->mBounds.BSize(lineWM) +
                                          pfd->mBorderPadding.BEnd(lineWM) -
                                          frameSpan->mBEndLeading;
          }
          else {
            pfd->mBounds.BStart(lineWM) = baselineBCoord + parentDescent -
                                          pfd->mBounds.BSize(lineWM) -
                                          pfd->mMargin.BEnd(lineWM);
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
                                          pfd->mMargin.BStart(lineWM);
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
      
      
      
      
      
      
      nscoord revisedBaselineBCoord = baselineBCoord - offset *
        lineWM.FlowRelativeToLineRelativeFactor();
      if (lineWM.IsVertical() && !lineWM.IsSideways()) {
        
        
        pfd->mBounds.BStart(lineWM) =
          revisedBaselineBCoord - pfd->mBounds.BSize(lineWM)/2;
      } else {
        pfd->mBounds.BStart(lineWM) = revisedBaselineBCoord - pfd->mAscent;
      }
      pfd->mBlockDirAlign = VALIGN_OTHER;
    }

    
    
    if (pfd->mBlockDirAlign == VALIGN_OTHER) {
      
      
      
      
      
      
      
      
      
      
      
      
#if 0
      if (!pfd->mIsTextFrame) {
#else

      bool canUpdate = !pfd->mIsTextFrame;
      if (!canUpdate && pfd->mIsNonWhitespaceTextFrame) {
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
                       pfd->mMargin.BStart(lineWM);
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
               pfd->mBorderPadding.Top(lineWM),
               pfd->mBorderPadding.Bottom(lineWM),
               logicalBSize,
               frameSpan ? frameSpan->mBStartLeading : 0,
               pfd->mBounds.BStart(lineWM), minBCoord, maxBCoord);
#endif
      }
      if (psd != mRootSpan) {
        frame->SetRect(lineWM, pfd->mBounds, ContainerWidthForSpan(psd));
      }
    }
    pfd = pfd->mNext;
  }

  
  
  if (psd == mRootSpan) {
    
    
    
    
    
    
    
    
    
    

    
    bool applyMinLH = !zeroEffectiveSpanBox || mHasBullet;
    bool isLastLine = !mGotLineBox ||
      (!mLineBox->IsLineWrapped() && !mLineEndsInBR);
    if (!applyMinLH && isLastLine) {
      nsIContent* blockContent = mRootSpan->mFrame->mFrame->GetContent();
      if (blockContent) {
        
        if (blockContent->IsAnyOfHTMLElements(nsGkAtoms::li,
                                              nsGkAtoms::dt,
                                              nsGkAtoms::dd)) {
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
          -nsLayoutUtils::GetCenteredFontBaseline(fm, minimumLineBSize,
                                                  lineWM.IsLineInverted());
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

  if (psd != mRootSpan && zeroEffectiveSpanBox) {
#ifdef NOISY_BLOCKDIR_ALIGN
    printf("   [span]adjusting for zeroEffectiveSpanBox\n");
    printf("     Original: minBCoord=%d, maxBCoord=%d, bSize=%d, ascent=%d, logicalBSize=%d, topLeading=%d, bottomLeading=%d\n",
           minBCoord, maxBCoord, spanFramePFD->mBounds.BSize(frameWM),
           spanFramePFD->mAscent,
           psd->mLogicalBSize, psd->mBStartLeading, psd->mBEndLeading);
#endif
    nscoord goodMinBCoord =
      spanFramePFD->mBorderPadding.BStart(lineWM) - psd->mBStartLeading;
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
                                                  
        pfd->mFrame->SetRect(lineWM, pfd->mBounds, ContainerWidthForSpan(psd));
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
    nsFrame::ListTag(stdout, psd->mFrame->mFrame);
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
            
            
            
            
            
            
            nscoord containerWidth = ContainerWidthForSpan(childSpan);
            nsIFrame* f = pfd->mFrame;
            LogicalRect r(lineWM, f->GetRect(), containerWidth);
            r.ISize(lineWM) -= deltaISize;
            f->SetRect(lineWM, r, containerWidth);
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
    else if (!pfd->mIsTextFrame && !pfd->mSkipWhenTrimmingWhitespace) {
      
      
      *aDeltaISize = 0;
      return true;
    }
    else if (pfd->mIsTextFrame) {
      
      
      
      nsTextFrame::TrimOutput trimOutput = static_cast<nsTextFrame*>(pfd->mFrame)->
          TrimTrailingWhiteSpace(mBlockReflowState->rendContext);
#ifdef NOISY_TRIM
      nsFrame::ListTag(stdout, psd->mFrame->mFrame);
      printf(": trim of ");
      nsFrame::ListTag(stdout, pfd->mFrame);
      printf(" returned %d\n", trimOutput.mDeltaWidth);
#endif

      if (trimOutput.mChanged) {
        pfd->mRecomputeOverflow = true;
      }

      
      
      if (trimOutput.mDeltaWidth) {
        pfd->mBounds.ISize(lineWM) -= trimOutput.mDeltaWidth;

        
        
        pfd->mJustificationInfo.CancelOpportunityForTrimmedSpace();

        
        if (psd != mRootSpan) {
          
          
          pfd->mFrame->SetRect(lineWM, pfd->mBounds,
                               ContainerWidthForSpan(psd));
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

      if (pfd->mIsNonEmptyTextFrame || trimOutput.mChanged) {
        
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

bool
nsLineLayout::PerFrameData::ParticipatesInJustification() const
{
  if (mIsBullet || mIsEmpty || mSkipWhenTrimmingWhitespace) {
    
    return false;
  }
  if (mIsTextFrame && !mIsNonWhitespaceTextFrame &&
      static_cast<nsTextFrame*>(mFrame)->IsAtEndOfLine()) {
    
    return false;
  }
  return true;
}

struct nsLineLayout::JustificationComputationState
{
  PerFrameData* mFirstParticipant;
  PerFrameData* mLastParticipant;
  
  
  
  PerFrameData* mLastExitedRubyBase;
  PerFrameData* mLastEnteredRubyBase;

  JustificationComputationState()
    : mFirstParticipant(nullptr)
    , mLastParticipant(nullptr)
    , mLastExitedRubyBase(nullptr)
    , mLastEnteredRubyBase(nullptr) { }
};

static bool
IsRubyAlignSpaceAround(nsIFrame* aRubyBase)
{
  return aRubyBase->StyleText()->mRubyAlign == NS_STYLE_RUBY_ALIGN_SPACE_AROUND;
}





 int
nsLineLayout::AssignInterframeJustificationGaps(
  PerFrameData* aFrame, JustificationComputationState& aState)
{
  PerFrameData* prev = aState.mLastParticipant;
  MOZ_ASSERT(prev);

  auto& assign = aFrame->mJustificationAssignment;
  auto& prevAssign = prev->mJustificationAssignment;

  if (aState.mLastExitedRubyBase || aState.mLastEnteredRubyBase) {
    PerFrameData* exitedRubyBase = aState.mLastExitedRubyBase;
    if (!exitedRubyBase || IsRubyAlignSpaceAround(exitedRubyBase->mFrame)) {
      prevAssign.mGapsAtEnd = 1;
    } else {
      exitedRubyBase->mJustificationAssignment.mGapsAtEnd = 1;
    }

    PerFrameData* enteredRubyBase = aState.mLastEnteredRubyBase;
    if (!enteredRubyBase || IsRubyAlignSpaceAround(enteredRubyBase->mFrame)) {
      assign.mGapsAtStart = 1;
    } else {
      enteredRubyBase->mJustificationAssignment.mGapsAtStart = 1;
    }

    
    aState.mLastExitedRubyBase = nullptr;
    aState.mLastEnteredRubyBase = nullptr;
    return 1;
  }

  const auto& info = aFrame->mJustificationInfo;
  const auto& prevInfo = prev->mJustificationInfo;
  if (!info.mIsStartJustifiable && !prevInfo.mIsEndJustifiable) {
    return 0;
  }

  if (!info.mIsStartJustifiable) {
    prevAssign.mGapsAtEnd = 2;
    assign.mGapsAtStart = 0;
  } else if (!prevInfo.mIsEndJustifiable) {
    prevAssign.mGapsAtEnd = 0;
    assign.mGapsAtStart = 2;
  } else {
    prevAssign.mGapsAtEnd = 1;
    assign.mGapsAtStart = 1;
  }
  return 1;
}






int32_t
nsLineLayout::ComputeFrameJustification(PerSpanData* aPSD,
                                        JustificationComputationState& aState)
{
  NS_ASSERTION(aPSD, "null arg");
  NS_ASSERTION(!aState.mLastParticipant || !aState.mLastParticipant->mSpan,
               "Last participant shall always be a leaf frame");
  bool firstChild = true;
  int32_t& innerOpportunities =
    aPSD->mFrame->mJustificationInfo.mInnerOpportunities;
  MOZ_ASSERT(innerOpportunities == 0,
             "Justification info should not have been set yet.");
  int32_t outerOpportunities = 0;

  for (PerFrameData* pfd = aPSD->mFirstFrame; pfd; pfd = pfd->mNext) {
    if (!pfd->ParticipatesInJustification()) {
      continue;
    }

    bool isRubyBase = pfd->mFrame->GetType() == nsGkAtoms::rubyBaseFrame;
    PerFrameData* outerRubyBase = aState.mLastEnteredRubyBase;
    if (isRubyBase) {
      aState.mLastEnteredRubyBase = pfd;
    }

    int extraOpportunities = 0;
    if (pfd->mSpan) {
      PerSpanData* span = pfd->mSpan;
      extraOpportunities = ComputeFrameJustification(span, aState);
      innerOpportunities += pfd->mJustificationInfo.mInnerOpportunities;
    } else {
      if (pfd->mIsTextFrame) {
        innerOpportunities += pfd->mJustificationInfo.mInnerOpportunities;
      }

      if (!aState.mLastParticipant) {
        aState.mFirstParticipant = pfd;
        
        
        
        aState.mLastEnteredRubyBase = nullptr;
      } else {
        extraOpportunities = AssignInterframeJustificationGaps(pfd, aState);
      }

      aState.mLastParticipant = pfd;
    }

    if (isRubyBase) {
      if (aState.mLastEnteredRubyBase == pfd) {
        
        
        
        aState.mLastEnteredRubyBase = outerRubyBase;
      } else {
        aState.mLastExitedRubyBase = pfd;
      }
    }

    if (firstChild) {
      outerOpportunities = extraOpportunities;
      firstChild = false;
    } else {
      innerOpportunities += extraOpportunities;
    }
  }

  return outerOpportunities;
}

void
nsLineLayout::AdvanceAnnotationInlineBounds(PerFrameData* aPFD,
                                            nscoord aContainerWidth,
                                            nscoord aDeltaICoord,
                                            nscoord aDeltaISize)
{
  nsIFrame* frame = aPFD->mFrame;
  nsIAtom* frameType = frame->GetType();
  MOZ_ASSERT(frameType == nsGkAtoms::rubyTextFrame ||
             frameType == nsGkAtoms::rubyTextContainerFrame);
  MOZ_ASSERT(aPFD->mSpan, "rt and rtc should have span.");

  PerSpanData* psd = aPFD->mSpan;
  WritingMode lineWM = mRootSpan->mWritingMode;
  aPFD->mBounds.IStart(lineWM) += aDeltaICoord;

  
  
  
  
  
  
  
  
  if (frameType == nsGkAtoms::rubyTextFrame ||
      
      (psd->mFirstFrame == psd->mLastFrame && psd->mFirstFrame &&
       !psd->mFirstFrame->mIsLinkedToBase)) {
    
    
    if (frameType != nsGkAtoms::rubyTextFrame ||
        !static_cast<nsRubyTextFrame*>(frame)->IsAutoHidden()) {
      nscoord reservedISize = RubyUtils::GetReservedISize(frame);
      RubyUtils::SetReservedISize(frame, reservedISize + aDeltaISize);
    }
  } else {
    
    
    aPFD->mBounds.ISize(lineWM) += aDeltaISize;
  }
  aPFD->mFrame->SetRect(lineWM, aPFD->mBounds, aContainerWidth);
}





void
nsLineLayout::ApplyLineJustificationToAnnotations(PerFrameData* aPFD,
                                                  nscoord aDeltaICoord,
                                                  nscoord aDeltaISize)
{
  PerFrameData* pfd = aPFD->mNextAnnotation;
  while (pfd) {
    nscoord containerWidth = pfd->mFrame->GetParent()->GetRect().Width();
    AdvanceAnnotationInlineBounds(pfd, containerWidth,
                                  aDeltaICoord, aDeltaISize);

    
    
    
    
    
    
    
    PerFrameData* sibling = pfd->mNext;
    while (sibling && !sibling->mIsLinkedToBase) {
      AdvanceAnnotationInlineBounds(sibling, containerWidth,
                                    aDeltaICoord + aDeltaISize, 0);
      sibling = sibling->mNext;
    }

    pfd = pfd->mNextAnnotation;
  }
}

nscoord 
nsLineLayout::ApplyFrameJustification(PerSpanData* aPSD,
                                      JustificationApplicationState& aState)
{
  NS_ASSERTION(aPSD, "null arg");

  nscoord deltaICoord = 0;
  for (PerFrameData* pfd = aPSD->mFirstFrame; pfd != nullptr; pfd = pfd->mNext) {
    
    if (!pfd->mIsBullet) {
      nscoord dw = 0;
      WritingMode lineWM = mRootSpan->mWritingMode;
      const auto& assign = pfd->mJustificationAssignment;

      if (true == pfd->mIsTextFrame) {
        if (aState.IsJustifiable()) {
          
          
          const auto& info = pfd->mJustificationInfo;
          auto textFrame = static_cast<nsTextFrame*>(pfd->mFrame);
          textFrame->AssignJustificationGaps(assign);
          dw = aState.Consume(JustificationUtils::CountGaps(info, assign));
        }

        if (dw) {
          pfd->mRecomputeOverflow = true;
        }
      }
      else {
        if (nullptr != pfd->mSpan) {
          dw = ApplyFrameJustification(pfd->mSpan, aState);
        }
      }

      pfd->mBounds.ISize(lineWM) += dw;
      nscoord gapsAtEnd = 0;
      if (!pfd->mIsTextFrame && assign.TotalGaps()) {
        
        
        deltaICoord += aState.Consume(assign.mGapsAtStart);
        gapsAtEnd = aState.Consume(assign.mGapsAtEnd);
        dw += gapsAtEnd;
      }
      pfd->mBounds.IStart(lineWM) += deltaICoord;

      
      
      ApplyLineJustificationToAnnotations(pfd, deltaICoord, dw - gapsAtEnd);
      deltaICoord += dw;
      pfd->mFrame->SetRect(lineWM, pfd->mBounds, ContainerWidthForSpan(aPSD));
    }
  }
  return deltaICoord;
}

static nsIFrame*
FindNearestRubyBaseAncestor(nsIFrame* aFrame)
{
  MOZ_ASSERT(aFrame->StyleContext()->ShouldSuppressLineBreak());
  while (aFrame && aFrame->GetType() != nsGkAtoms::rubyBaseFrame) {
    aFrame = aFrame->GetParent();
  }
  
  
  
  NS_ASSERTION(aFrame, "No ruby base ancestor?");
  return aFrame;
}




void
nsLineLayout::ExpandRubyBox(PerFrameData* aFrame, nscoord aReservedISize,
                            nscoord aContainerWidth)
{
  WritingMode lineWM = mRootSpan->mWritingMode;
  auto rubyAlign = aFrame->mFrame->StyleText()->mRubyAlign;
  switch (rubyAlign) {
    case NS_STYLE_RUBY_ALIGN_START:
      
      break;
    case NS_STYLE_RUBY_ALIGN_SPACE_BETWEEN:
    case NS_STYLE_RUBY_ALIGN_SPACE_AROUND: {
      int32_t opportunities = aFrame->mJustificationInfo.mInnerOpportunities;
      int32_t gaps = opportunities * 2;
      if (rubyAlign == NS_STYLE_RUBY_ALIGN_SPACE_AROUND) {
        
        
        
        
        gaps += 2;
      }
      if (gaps > 0) {
        JustificationApplicationState state(gaps, aReservedISize);
        ApplyFrameJustification(aFrame->mSpan, state);
        break;
      }
      
      
    }
    case NS_STYLE_RUBY_ALIGN_CENTER:
      
      for (PerFrameData* child = aFrame->mSpan->mFirstFrame;
           child; child = child->mNext) {
        child->mBounds.IStart(lineWM) += aReservedISize / 2;
        child->mFrame->SetRect(lineWM, child->mBounds, aContainerWidth);
      }
      break;
    default:
      MOZ_ASSERT_UNREACHABLE("Unknown ruby-align value");
  }

  aFrame->mBounds.ISize(lineWM) += aReservedISize;
  aFrame->mFrame->SetRect(lineWM, aFrame->mBounds, aContainerWidth);
}






void
nsLineLayout::ExpandRubyBoxWithAnnotations(PerFrameData* aFrame,
                                           nscoord aContainerWidth)
{
  nscoord reservedISize = RubyUtils::GetReservedISize(aFrame->mFrame);
  if (reservedISize) {
    ExpandRubyBox(aFrame, reservedISize, aContainerWidth);
  }

  WritingMode lineWM = mRootSpan->mWritingMode;
  bool isLevelContainer =
    aFrame->mFrame->GetType() == nsGkAtoms::rubyBaseContainerFrame;
  for (PerFrameData* annotation = aFrame->mNextAnnotation;
       annotation; annotation = annotation->mNextAnnotation) {
    if (isLevelContainer) {
      nsIFrame* rtcFrame = annotation->mFrame;
      MOZ_ASSERT(rtcFrame->GetType() == nsGkAtoms::rubyTextContainerFrame);
      
      
      
      
      MOZ_ASSERT(
        rtcFrame->GetLogicalSize(lineWM) == annotation->mBounds.Size(lineWM));
      rtcFrame->SetPosition(lineWM, annotation->mBounds.Origin(lineWM),
                            aContainerWidth);
    }

    nscoord reservedISize = RubyUtils::GetReservedISize(annotation->mFrame);
    if (!reservedISize) {
      continue;
    }

    MOZ_ASSERT(annotation->mSpan);
    JustificationComputationState computeState;
    ComputeFrameJustification(annotation->mSpan, computeState);
    if (!computeState.mFirstParticipant) {
      continue;
    }
    if (IsRubyAlignSpaceAround(annotation->mFrame)) {
      
      computeState.mFirstParticipant->mJustificationAssignment.mGapsAtStart = 1;
      computeState.mLastParticipant->mJustificationAssignment.mGapsAtEnd = 1;
    }
    nsIFrame* parentFrame = annotation->mFrame->GetParent();
    nscoord containerWidth = parentFrame->GetRect().Width();
    MOZ_ASSERT(containerWidth == aContainerWidth ||
               parentFrame->GetType() == nsGkAtoms::rubyTextContainerFrame,
               "Container width should only be different when the current "
               "annotation is a ruby text frame, whose parent is not same "
               "as its base frame.");
    ExpandRubyBox(annotation, reservedISize, containerWidth);
    ExpandInlineRubyBoxes(annotation->mSpan);
  }
}





void
nsLineLayout::ExpandInlineRubyBoxes(PerSpanData* aSpan)
{
  nscoord containerWidth = ContainerWidthForSpan(aSpan);
  for (PerFrameData* pfd = aSpan->mFirstFrame; pfd; pfd = pfd->mNext) {
    if (RubyUtils::IsExpandableRubyBox(pfd->mFrame)) {
      ExpandRubyBoxWithAnnotations(pfd, containerWidth);
    }
    if (pfd->mSpan) {
      ExpandInlineRubyBoxes(pfd->mSpan);
    }
  }
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

  bool isSVG = mBlockReflowState->frame->IsSVGText();
  bool doTextAlign = remainingISize > 0 || textAlignTrue;

  int32_t additionalGaps = 0;
  if (!isSVG && (mHasRuby || (doTextAlign &&
                              textAlign == NS_STYLE_TEXT_ALIGN_JUSTIFY))) {
    JustificationComputationState computeState;
    ComputeFrameJustification(psd, computeState);
    if (mHasRuby && computeState.mFirstParticipant) {
      PerFrameData* firstFrame = computeState.mFirstParticipant;
      if (firstFrame->mFrame->StyleContext()->ShouldSuppressLineBreak()) {
        MOZ_ASSERT(!firstFrame->mJustificationAssignment.mGapsAtStart);
        nsIFrame* rubyBase = FindNearestRubyBaseAncestor(firstFrame->mFrame);
        if (rubyBase && IsRubyAlignSpaceAround(rubyBase)) {
          firstFrame->mJustificationAssignment.mGapsAtStart = 1;
          additionalGaps++;
        }
      }
      PerFrameData* lastFrame = computeState.mLastParticipant;
      if (lastFrame->mFrame->StyleContext()->ShouldSuppressLineBreak()) {
        MOZ_ASSERT(!lastFrame->mJustificationAssignment.mGapsAtEnd);
        nsIFrame* rubyBase = FindNearestRubyBaseAncestor(lastFrame->mFrame);
        if (rubyBase && IsRubyAlignSpaceAround(rubyBase)) {
          lastFrame->mJustificationAssignment.mGapsAtEnd = 1;
          additionalGaps++;
        }
      }
    }
  }

  if (!isSVG && doTextAlign) {
    switch (textAlign) {
      case NS_STYLE_TEXT_ALIGN_JUSTIFY: {
        int32_t opportunities =
          psd->mFrame->mJustificationInfo.mInnerOpportunities;
        if (opportunities > 0) {
          int32_t gaps = opportunities * 2 + additionalGaps;
          JustificationApplicationState applyState(gaps, remainingISize);

          
          
          aLine->ExpandBy(ApplyFrameJustification(psd, applyState),
                          ContainerWidthForSpan(psd));

          MOZ_ASSERT(applyState.mGaps.mHandled == applyState.mGaps.mCount,
                     "Unprocessed justification gaps");
          MOZ_ASSERT(applyState.mWidth.mConsumed == applyState.mWidth.mAvailable,
                     "Unprocessed justification width");
          break;
        }
        
        
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

  if (mHasRuby) {
    ExpandInlineRubyBoxes(mRootSpan);
  }

  if (mPresContext->BidiEnabled() &&
      (!mPresContext->IsVisualMode() || !lineWM.IsBidiLTR())) {
    nsBidiPresUtils::ReorderFrames(psd->mFirstFrame->mFrame,
                                   aLine->GetChildCount(),
                                   lineWM, mContainerSize,
                                   psd->mIStart + mTextIndent + dx);
    if (dx) {
      aLine->IndentBy(dx, ContainerWidth());
    }
  } else if (dx) {
    for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      pfd->mBounds.IStart(lineWM) += dx;
      pfd->mFrame->SetRect(lineWM, pfd->mBounds, ContainerWidthForSpan(psd));
    }
    aLine->IndentBy(dx, ContainerWidth());
  }
}


void
nsLineLayout::ApplyRelativePositioning(PerFrameData* aPFD)
{
  if (!aPFD->mRelativePos) {
    return;
  }

  nsIFrame* frame = aPFD->mFrame;
  WritingMode frameWM = frame->GetWritingMode();
  LogicalPoint origin = frame->GetLogicalPosition(ContainerWidth());
  
  
  nsHTMLReflowState::ApplyRelativePositioning(frame, frameWM,
                                              aPFD->mOffsets, &origin,
                                              ContainerWidth());
  frame->SetPosition(frameWM, origin, ContainerWidth());
}


void
nsLineLayout::RelativePositionAnnotations(PerSpanData* aRubyPSD,
                                          nsOverflowAreas& aOverflowAreas)
{
  MOZ_ASSERT(aRubyPSD->mFrame->mFrame->GetType() == nsGkAtoms::rubyFrame);
  for (PerFrameData* pfd = aRubyPSD->mFirstFrame; pfd; pfd = pfd->mNext) {
    MOZ_ASSERT(pfd->mFrame->GetType() == nsGkAtoms::rubyBaseContainerFrame);
    for (PerFrameData* rtc = pfd->mNextAnnotation;
         rtc; rtc = rtc->mNextAnnotation) {
      nsIFrame* rtcFrame = rtc->mFrame;
      MOZ_ASSERT(rtcFrame->GetType() == nsGkAtoms::rubyTextContainerFrame);
      ApplyRelativePositioning(rtc);
      nsOverflowAreas rtcOverflowAreas;
      RelativePositionFrames(rtc->mSpan, rtcOverflowAreas);
      aOverflowAreas.UnionWith(rtcOverflowAreas + rtcFrame->GetPosition());
    }
  }
}

void
nsLineLayout::RelativePositionFrames(PerSpanData* psd, nsOverflowAreas& aOverflowAreas)
{
  nsOverflowAreas overflowAreas;
  WritingMode wm = psd->mWritingMode;
  if (psd != mRootSpan) {
    
    
    
    
    
    
    
    
    
    
    nsRect adjustedBounds(nsPoint(0, 0), psd->mFrame->mFrame->GetSize());

    overflowAreas.ScrollableOverflow().UnionRect(
      psd->mFrame->mOverflowAreas.ScrollableOverflow(), adjustedBounds);
    overflowAreas.VisualOverflow().UnionRect(
      psd->mFrame->mOverflowAreas.VisualOverflow(), adjustedBounds);
  }
  else {
    LogicalRect rect(wm, psd->mIStart, mBStartEdge,
                     psd->mICoord - psd->mIStart, mFinalLineBSize);
    
    
    
    
    overflowAreas.VisualOverflow() = rect.GetPhysicalRect(wm, ContainerWidth());
    overflowAreas.ScrollableOverflow() = overflowAreas.VisualOverflow();
  }

  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    nsIFrame* frame = pfd->mFrame;

    
    ApplyRelativePositioning(pfd);

    
    
    
    if (frame->HasView())
      nsContainerFrame::SyncFrameViewAfterReflow(mPresContext, frame,
        frame->GetView(), pfd->mOverflowAreas.VisualOverflow(),
        NS_FRAME_NO_SIZE_VIEW);

    
    
    
    
    nsOverflowAreas r;
    if (pfd->mSpan) {
      
      
      RelativePositionFrames(pfd->mSpan, r);
    } else {
      r = pfd->mOverflowAreas;
      if (pfd->mIsTextFrame) {
        
        
        
        
        if (pfd->mRecomputeOverflow ||
            frame->StyleContext()->HasTextDecorationLines()) {
          nsTextFrame* f = static_cast<nsTextFrame*>(frame);
          r = f->RecomputeOverflow(mBlockReflowState->frame);
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

    overflowAreas.UnionWith(r + frame->GetPosition());
  }

  
  if (psd->mFrame->mFrame->GetType() == nsGkAtoms::rubyFrame) {
    RelativePositionAnnotations(psd, overflowAreas);
  }

  
  
  if (psd != mRootSpan) {
    PerFrameData* spanPFD = psd->mFrame;
    nsIFrame* frame = spanPFD->mFrame;
    frame->FinishAndStoreOverflow(overflowAreas, frame->GetSize());
  }
  aOverflowAreas = overflowAreas;
}
