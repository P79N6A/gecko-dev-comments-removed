












































#define PL_ARENA_CONST_ALIGN_MASK (sizeof(void*)-1)
#include "plarena.h"

#include "nsCOMPtr.h"
#include "nsLineLayout.h"
#include "nsBlockFrame.h"
#include "nsInlineFrame.h"
#include "nsStyleConsts.h"
#include "nsHTMLContainerFrame.h"
#include "nsSpaceManager.h"
#include "nsStyleContext.h"
#include "nsPresContext.h"
#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"
#include "nsGkAtoms.h"
#include "nsPlaceholderFrame.h"
#include "nsIDocument.h"
#include "nsIHTMLDocument.h"
#include "nsIContent.h"
#include "nsTextFragment.h"
#include "nsBidiUtils.h"
#include "nsLayoutUtils.h"

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



#define FIX_BUG_50257

#define PLACED_LEFT  0x1
#define PLACED_RIGHT 0x2

nsLineLayout::nsLineLayout(nsPresContext* aPresContext,
                           nsSpaceManager* aSpaceManager,
                           const nsHTMLReflowState* aOuterReflowState,
                           const nsLineList::iterator* aLine)
  : mPresContext(aPresContext),
    mSpaceManager(aSpaceManager),
    mBlockReflowState(aOuterReflowState),
    mLastOptionalBreakContent(nsnull),
    mForceBreakContent(nsnull),
    mLastOptionalBreakContentOffset(-1),
    mForceBreakContentOffset(-1),
    mTrailingTextFrame(nsnull),
    mBlockRS(nsnull),
    mMinLineHeight(0),
    mTextIndent(0)
{
  NS_ASSERTION(aSpaceManager || aOuterReflowState->frame->GetType() ==
                                  nsGkAtoms::letterFrame,
               "space manager should be present");
  MOZ_COUNT_CTOR(nsLineLayout);

  
  mStyleText = aOuterReflowState->frame->GetStyleText();
  mTextAlign = mStyleText->mTextAlign;
  mLineNumber = 0;
  mColumn = 0;
  mFlags = 0; 
  SetFlag(LL_ENDSINWHITESPACE, PR_TRUE);
  mPlacedFloats = 0;
  mTotalPlacedFrames = 0;
  mTopEdge = 0;

  
  
  
  
  PL_INIT_ARENA_POOL(&mArena, "nsLineLayout", 1024);
  mFrameFreeList = nsnull;
  mSpanFreeList = nsnull;

  mCurrentSpan = mRootSpan = nsnull;
  mSpanDepth = 0;

  if (aLine) {
    SetFlag(LL_GOTLINEBOX, PR_TRUE);
    mLineBox = *aLine;
  }

  mCompatMode = mPresContext->CompatibilityMode();
}

nsLineLayout::~nsLineLayout()
{
  MOZ_COUNT_DTOR(nsLineLayout);

  NS_ASSERTION(nsnull == mRootSpan, "bad line-layout user");

  
  
  
  
  
  
  
  
  
  PL_FreeArenaPool(&mArena);
  PL_FinishArenaPool(&mArena);
}



inline PRBool
HasPrevInFlow(nsIFrame *aFrame)
{
  nsIFrame *prevInFlow = aFrame->GetPrevInFlow();
  return prevInFlow != nsnull;
}

void
nsLineLayout::BeginLineReflow(nscoord aX, nscoord aY,
                              nscoord aWidth, nscoord aHeight,
                              PRBool aImpactedByFloats,
                              PRBool aIsTopOfPage)
{
  NS_ASSERTION(nsnull == mRootSpan, "bad linelayout user");
#ifdef DEBUG
  if ((aWidth != NS_UNCONSTRAINEDSIZE) && CRAZY_WIDTH(aWidth)) {
    NS_NOTREACHED("bad width");
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": Init: bad caller: width WAS %d(0x%x)\n",
           aWidth, aWidth);
  }
  if ((aHeight != NS_UNCONSTRAINEDSIZE) && CRAZY_HEIGHT(aHeight)) {
    NS_NOTREACHED("bad height");
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

  mColumn = 0;
  
  SetFlag(LL_ENDSINWHITESPACE, PR_TRUE);
  SetFlag(LL_UNDERSTANDSNWHITESPACE, PR_FALSE);
  SetFlag(LL_FIRSTLETTERSTYLEOK, PR_FALSE);
  SetFlag(LL_ISTOPOFPAGE, aIsTopOfPage);
  SetFlag(LL_UPDATEDBAND, PR_FALSE);
  mPlacedFloats = 0;
  SetFlag(LL_IMPACTEDBYFLOATS, aImpactedByFloats);
  mTotalPlacedFrames = 0;
  SetFlag(LL_CANPLACEFLOAT, PR_TRUE);
  SetFlag(LL_LINEENDSINBR, PR_FALSE);
  mSpanDepth = 0;
  mMaxTopBoxHeight = mMaxBottomBoxHeight = 0;

  PerSpanData* psd;
  NewPerSpanData(&psd);
  mCurrentSpan = mRootSpan = psd;
  psd->mReflowState = mBlockReflowState;
  psd->mLeftEdge = aX;
  psd->mX = aX;
  if (NS_UNCONSTRAINEDSIZE == aWidth) {
    psd->mRightEdge = NS_UNCONSTRAINEDSIZE;
  }
  else {
    psd->mRightEdge = aX + aWidth;
  }

  mTopEdge = aY;

  psd->mNoWrap = !mStyleText->WhiteSpaceCanWrap();
  psd->mDirection = mBlockReflowState->mStyleVisibility->mDirection;
  psd->mChangedFrameDirection = PR_FALSE;

  
  

  if (0 == mLineNumber && !HasPrevInFlow(mBlockReflowState->frame)) {
    nscoord indent = 0;
    nsStyleUnit unit = mStyleText->mTextIndent.GetUnit();
    if (eStyleUnit_Coord == unit) {
      indent = mStyleText->mTextIndent.GetCoordValue();
    }
    else if (eStyleUnit_Percent == unit) {
      nscoord width =
        nsHTMLReflowState::GetContainingBlockContentWidth(mBlockReflowState);
      if ((0 != width) && (NS_UNCONSTRAINEDSIZE != width)) {
        indent = nscoord(mStyleText->mTextIndent.GetPercentValue() * width);
      }
    }

    mTextIndent = indent;

    if (NS_STYLE_DIRECTION_RTL == psd->mDirection) {
      if (NS_UNCONSTRAINEDSIZE != psd->mRightEdge) {
        psd->mRightEdge -= indent;
      }
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
  mCurrentSpan = mRootSpan = nsnull;

  NS_ASSERTION(mSpansAllocated == mSpansFreed, "leak");
  NS_ASSERTION(mFramesAllocated == mFramesFreed, "leak");

#if 0
  static PRInt32 maxSpansAllocated = NS_LINELAYOUT_NUM_SPANS;
  static PRInt32 maxFramesAllocated = NS_LINELAYOUT_NUM_FRAMES;
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
nsLineLayout::UpdateBand(nscoord aX, nscoord aY,
                         nscoord aWidth, nscoord aHeight,
                         PRBool aPlacedLeftFloat,
                         nsIFrame* aFloatFrame)
{
#ifdef REALLY_NOISY_REFLOW
  printf("nsLL::UpdateBand %d, %d, %d, %d, frame=%p placedLeft=%s\n  will set mImpacted to PR_TRUE\n",
         aX, aY, aWidth, aHeight, aFloatFrame, aPlacedLeftFloat?"true":"false");
#endif
  PerSpanData* psd = mRootSpan;
  NS_PRECONDITION(psd->mX == psd->mLeftEdge, "update-band called late");
#ifdef DEBUG
  if ((aWidth != NS_UNCONSTRAINEDSIZE) && CRAZY_WIDTH(aWidth)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": UpdateBand: bad caller: width WAS %d(0x%x)\n",
           aWidth, aWidth);
  }
  if ((aHeight != NS_UNCONSTRAINEDSIZE) && CRAZY_HEIGHT(aHeight)) {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": UpdateBand: bad caller: height WAS %d(0x%x)\n",
           aHeight, aHeight);
  }
#endif

  
  nscoord deltaWidth = 0;
  if (NS_UNCONSTRAINEDSIZE != psd->mRightEdge) {
    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aWidth, "switched constraints");
    nscoord oldWidth = psd->mRightEdge - psd->mLeftEdge;
    deltaWidth = aWidth - oldWidth;
  }
#ifdef NOISY_REFLOW
  nsFrame::ListTag(stdout, mBlockReflowState->frame);
  printf(": UpdateBand: %d,%d,%d,%d deltaWidth=%d %s float\n",
         aX, aY, aWidth, aHeight, deltaWidth,
         aPlacedLeftFloat ? "left" : "right");
#endif

  psd->mLeftEdge = aX;
  psd->mX = aX;
  if (NS_UNCONSTRAINEDSIZE == aWidth) {
    psd->mRightEdge = NS_UNCONSTRAINEDSIZE;
  }
  else {
    psd->mRightEdge = aX + aWidth;
  }
  mTopEdge = aY;
  SetFlag(LL_UPDATEDBAND, PR_TRUE);
  mPlacedFloats |= (aPlacedLeftFloat ? PLACED_LEFT : PLACED_RIGHT);
  SetFlag(LL_IMPACTEDBYFLOATS, PR_TRUE);

  SetFlag(LL_LASTFLOATWASLETTERFRAME,
          nsGkAtoms::letterFrame == aFloatFrame->GetType());

  
  mRootSpan->mContainsFloat = PR_TRUE;              
  psd = mCurrentSpan;
  while (psd != mRootSpan) {
    NS_ASSERTION(nsnull != psd, "null ptr");
    if (nsnull == psd) {
      break;
    }
    NS_ASSERTION(psd->mX == psd->mLeftEdge, "bad float placement");
    if (NS_UNCONSTRAINEDSIZE == aWidth) {
      psd->mRightEdge = NS_UNCONSTRAINEDSIZE;
    }
    else {
      psd->mRightEdge += deltaWidth;
    }
    psd->mContainsFloat = PR_TRUE;
#ifdef NOISY_REFLOW
    printf("  span %p: oldRightEdge=%d newRightEdge=%d\n",
           psd, psd->mRightEdge - deltaWidth, psd->mRightEdge);
#endif
    psd = psd->mParent;
  }
}





void
nsLineLayout::UpdateFrames()
{
  NS_ASSERTION(nsnull != mRootSpan, "UpdateFrames with no active spans");

  PerSpanData* psd = mRootSpan;
  if (PLACED_LEFT & mPlacedFloats) {
    PerFrameData* pfd = psd->mFirstFrame;
    while (nsnull != pfd) {
      pfd->mBounds.x = psd->mX;
      pfd = pfd->mNext;
    }
  }
}

nsresult
nsLineLayout::NewPerSpanData(PerSpanData** aResult)
{
  PerSpanData* psd = mSpanFreeList;
  if (nsnull == psd) {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &mArena, sizeof(PerSpanData));
    if (nsnull == mem) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    psd = NS_REINTERPRET_CAST(PerSpanData*, mem);
  }
  else {
    mSpanFreeList = psd->mNextFreeSpan;
  }
  psd->mParent = nsnull;
  psd->mFrame = nsnull;
  psd->mFirstFrame = nsnull;
  psd->mLastFrame = nsnull;
  psd->mContainsFloat = PR_FALSE;
  psd->mZeroEffectiveSpanBox = PR_FALSE;

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
                        nscoord aRightEdge)
{
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

    psd->mNoWrap =
      !aSpanReflowState->frame->GetStyleText()->WhiteSpaceCanWrap();
    psd->mDirection = aSpanReflowState->mStyleVisibility->mDirection;
    psd->mChangedFrameDirection = PR_FALSE;

    
    mCurrentSpan = psd;
    mSpanDepth++;
  }
  return rv;
}

void
nsLineLayout::EndSpan(nsIFrame* aFrame, nsSize& aSizeResult)
{
  NS_ASSERTION(mSpanDepth > 0, "end-span without begin-span");
#ifdef NOISY_REFLOW
  nsFrame::IndentBy(stdout, mSpanDepth);
  nsFrame::ListTag(stdout, aFrame);
  printf(": EndSpan width=%d\n", mCurrentSpan->mX - mCurrentSpan->mLeftEdge);
#endif
  PerSpanData* psd = mCurrentSpan;
  nscoord width = 0;
  nscoord maxHeight = 0;
  if (nsnull != psd->mLastFrame) {
    width = psd->mX - psd->mLeftEdge;
    PerFrameData* pfd = psd->mFirstFrame;
    while (nsnull != pfd) {
      




      if (NS_UNCONSTRAINEDSIZE != psd->mRightEdge ||  
          pfd->mNext ||                               
          !pfd->GetFlag(PFD_ISTEXTFRAME) ||           
          pfd->GetFlag(PFD_ISNONWHITESPACETEXTFRAME)  
         ) {
        if (pfd->mBounds.height > maxHeight) maxHeight = pfd->mBounds.height;

        
      }
      pfd = pfd->mNext;
    }
  }
  aSizeResult.width = width;
  aSizeResult.height = maxHeight;

  mSpanDepth--;
  mCurrentSpan->mReflowState = nsnull;  
  mCurrentSpan = mCurrentSpan->mParent;
}

PRInt32
nsLineLayout::GetCurrentSpanCount() const
{
  NS_ASSERTION(mCurrentSpan == mRootSpan, "bad linelayout user");
  PRInt32 count = 0;
  PerFrameData* pfd = mRootSpan->mFirstFrame;
  while (nsnull != pfd) {
    count++;
    pfd = pfd->mNext;
  }
  return count;
}

void
nsLineLayout::SplitLineTo(PRInt32 aNewCount)
{
  NS_ASSERTION(mCurrentSpan == mRootSpan, "bad linelayout user");

#ifdef REALLY_NOISY_PUSHING
  printf("SplitLineTo %d (current count=%d); before:\n", aNewCount,
         GetCurrentSpanCount());
  DumpPerSpanData(mRootSpan, 1);
#endif
  PerSpanData* psd = mRootSpan;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nsnull != pfd) {
    if (--aNewCount == 0) {
      
      PerFrameData* next = pfd->mNext;
      pfd->mNext = nsnull;
      psd->mLastFrame = pfd;

      
      pfd = next;
      while (nsnull != pfd) {
        next = pfd->mNext;
        pfd->mNext = mFrameFreeList;
        mFrameFreeList = pfd;
#ifdef DEBUG
        mFramesFreed++;
#endif
        if (nsnull != pfd->mSpan) {
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
    
    psd->mFirstFrame = nsnull;
    psd->mLastFrame = nsnull;
  }
  else {
    PerFrameData* prevFrame = pfd->mPrev;
    prevFrame->mNext = nsnull;
    psd->mLastFrame = prevFrame;
  }

  
  pfd->mNext = mFrameFreeList;
  mFrameFreeList = pfd;
#ifdef DEBUG
  mFramesFreed++;
#endif
  if (nsnull != pfd->mSpan) {
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
  while (nsnull != pfd) {
    if (nsnull != pfd->mSpan) {
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

PRBool
nsLineLayout::IsZeroHeight()
{
  PerSpanData* psd = mCurrentSpan;
  PerFrameData* pfd = psd->mFirstFrame;
  while (nsnull != pfd) {
    if (0 != pfd->mBounds.height) {
      return PR_FALSE;
    }
    pfd = pfd->mNext;
  }
  return PR_TRUE;
}

nsresult
nsLineLayout::NewPerFrameData(PerFrameData** aResult)
{
  PerFrameData* pfd = mFrameFreeList;
  if (nsnull == pfd) {
    void *mem;
    PL_ARENA_ALLOCATE(mem, &mArena, sizeof(PerFrameData));
    if (nsnull == mem) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    pfd = NS_REINTERPRET_CAST(PerFrameData*, mem);
  }
  else {
    mFrameFreeList = pfd->mNext;
  }
  pfd->mSpan = nsnull;
  pfd->mNext = nsnull;
  pfd->mPrev = nsnull;
  pfd->mFrame = nsnull;
  pfd->mFlags = 0;  

#ifdef DEBUG
  pfd->mVerticalAlign = 0xFF;
  mFramesAllocated++;
#endif
  *aResult = pfd;
  return NS_OK;
}

PRBool
nsLineLayout::CanPlaceFloatNow() const
{
  return GetFlag(LL_CANPLACEFLOAT);
}

PRBool
nsLineLayout::LineIsBreakable() const
{
  if ((0 != mTotalPlacedFrames) || GetFlag(LL_IMPACTEDBYFLOATS)) {
    return PR_TRUE;
  }
  return PR_FALSE;
}





static PRBool
HasPercentageUnitSide(const nsStyleSides& aSides)
{
  NS_FOR_CSS_SIDES(side) {
    if (eStyleUnit_Percent == aSides.GetUnit(side))
      return PR_TRUE;
  }
  return PR_FALSE;
}

inline PRBool
WidthDependsOnContainer(const nsStyleCoord& aCoord)
{
  return aCoord.GetUnit() == eStyleUnit_Percent ||
         (aCoord.GetUnit() == eStyleUnit_Enumerated &&
          (aCoord.GetIntValue() == NS_STYLE_WIDTH_FILL ||
           aCoord.GetIntValue() == NS_STYLE_WIDTH_SHRINK_WRAP));

}

static PRBool
IsPercentageAware(const nsIFrame* aFrame)
{
  NS_ASSERTION(aFrame, "null frame is not allowed");

  nsIAtom *fType = aFrame->GetType();
  if (fType == nsGkAtoms::textFrame) {
    
    return PR_FALSE;
  }

  
  
  
  
  

  const nsStyleMargin* margin = aFrame->GetStyleMargin();
  if (HasPercentageUnitSide(margin->mMargin)) {
    return PR_TRUE;
  }

  const nsStylePadding* padding = aFrame->GetStylePadding();
  if (HasPercentageUnitSide(padding->mPadding)) {
    return PR_TRUE;
  }

  

  const nsStylePosition* pos = aFrame->GetStylePosition();

  if (WidthDependsOnContainer(pos->mWidth) ||
      WidthDependsOnContainer(pos->mMaxWidth) ||
      WidthDependsOnContainer(pos->mMinWidth) ||
      eStyleUnit_Percent == pos->mOffset.GetRightUnit() ||
      eStyleUnit_Percent == pos->mOffset.GetLeftUnit()) {
    return PR_TRUE;
  }

  if (eStyleUnit_Auto == pos->mWidth.GetUnit()) {
    
    
    const nsStyleDisplay* disp = aFrame->GetStyleDisplay();
    if (disp->mDisplay == NS_STYLE_DISPLAY_INLINE_BLOCK ||
        disp->mDisplay == NS_STYLE_DISPLAY_INLINE_TABLE ||
        fType == nsGkAtoms::HTMLButtonControlFrame ||
        fType == nsGkAtoms::gfxButtonControlFrame ||
        fType == nsGkAtoms::fieldSetFrame ||
        fType == nsGkAtoms::comboboxDisplayFrame) {
      return PR_TRUE;
    }
  }

  return PR_FALSE;
}

nsresult
nsLineLayout::ReflowFrame(nsIFrame* aFrame,
                          nsReflowStatus& aReflowStatus,
                          nsHTMLReflowMetrics* aMetrics,
                          PRBool& aPushedFrame)
{
  
  aPushedFrame = PR_FALSE;

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

  
  
  
  
  if (GetFlag(LL_GOTLINEBOX) && IsPercentageAware(aFrame)) {
    mLineBox->DisableResizeReflowOptimization();
  }

  
  
  
  nsSize availSize(mBlockReflowState->ComputedWidth(), NS_UNCONSTRAINEDSIZE);

  
  nsHTMLReflowState reflowState(mPresContext, *psd->mReflowState,
                                aFrame, availSize);
  reflowState.mLineLayout = this;
  reflowState.mFlags.mIsTopOfPage = GetFlag(LL_ISTOPOFPAGE);
  SetFlag(LL_UNDERSTANDSNWHITESPACE, PR_FALSE);
  mTextJustificationNumSpaces = 0;
  mTextJustificationNumLetters = 0;

  
  
  
  NS_ASSERTION(psd->mRightEdge != NS_UNCONSTRAINEDSIZE,
               "shouldn't have unconstrained widths anymore");
  if (reflowState.ComputedWidth() == NS_UNCONSTRAINEDSIZE)
    reflowState.availableWidth = psd->mRightEdge - psd->mX;

  
  
  pfd->mFrame = aFrame;
  pfd->mMargin = reflowState.mComputedMargin;
  pfd->mBorderPadding = reflowState.mComputedBorderPadding;
  pfd->mFrameType = reflowState.mFrameType;
  pfd->SetFlag(PFD_RELATIVEPOS,
               (reflowState.mStyleDisplay->mPosition == NS_STYLE_POSITION_RELATIVE));
  if (pfd->GetFlag(PFD_RELATIVEPOS)) {
    pfd->mOffsets = reflowState.mComputedOffsets;
  }

  
  
  
  
  pfd->mBounds.x = psd->mX;
  pfd->mBounds.y = mTopEdge;

  
  
  
  
  
  
  
  
  
  
  PRBool notSafeToBreak = CanPlaceFloatNow() && !GetFlag(LL_IMPACTEDBYFLOATS);
  
  
  
  ApplyStartMargin(pfd, reflowState);

  
  
  
  nscoord x = pfd->mBounds.x;
  nscoord y = pfd->mBounds.y;

  aFrame->WillReflow(mPresContext);

  
  
  
  
  
  
  
  nsHTMLReflowMetrics metrics;
#ifdef DEBUG
  metrics.width = nscoord(0xdeadbeef);
  metrics.height = nscoord(0xdeadbeef);
#endif
  nscoord tx = x - psd->mReflowState->mComputedBorderPadding.left;
  nscoord ty = y - psd->mReflowState->mComputedBorderPadding.top;
  mSpaceManager->Translate(tx, ty);

#ifdef IBMBIDI
  PRInt32 start, end;

  if (mPresContext->BidiEnabled()) {
    if (aFrame->GetStateBits() & NS_FRAME_IS_BIDI) {
      aFrame->GetOffsets(start, end);
    }
  }
#endif 

  nsIAtom* frameType = aFrame->GetType();
  PRInt32 savedOptionalBreakOffset;
  nsIContent* savedOptionalBreakContent =
    GetLastOptionalBreakPosition(&savedOptionalBreakOffset);

  rv = aFrame->Reflow(mPresContext, metrics, reflowState, aReflowStatus);
  if (NS_FAILED(rv)) {
    NS_WARNING( "Reflow of frame failed in nsLineLayout" );
    return rv;
  }
  
  pfd->mJustificationNumSpaces = mTextJustificationNumSpaces;
  pfd->mJustificationNumLetters = mTextJustificationNumLetters;

  
  
  PRBool placedFloat = PR_FALSE;
  if (frameType) {
    if (nsGkAtoms::placeholderFrame == frameType) {
      pfd->SetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE, PR_TRUE);
      nsIFrame* outOfFlowFrame = nsLayoutUtils::GetFloatFromPlaceholder(aFrame);
      if (outOfFlowFrame) {
        nsPlaceholderFrame* placeholder = NS_STATIC_CAST(nsPlaceholderFrame*, aFrame);
        
        if (!NS_SUBTREE_DIRTY(aFrame)) {
          
          placedFloat = InitFloat(placeholder, aReflowStatus);
        }
        else {
          placedFloat = AddFloat(placeholder, aReflowStatus);
        }
        if (!placedFloat) {
          aReflowStatus = NS_INLINE_LINE_BREAK_BEFORE();
        }
        if (outOfFlowFrame->GetType() == nsGkAtoms::letterFrame) {
          SetFlag(LL_FIRSTLETTERSTYLEOK, PR_FALSE);
        }
      }
    }
    else if (nsGkAtoms::textFrame == frameType) {
      
      pfd->SetFlag(PFD_ISTEXTFRAME, PR_TRUE);
      
      
      if (metrics.width) {
        pfd->SetFlag(PFD_ISNONEMPTYTEXTFRAME, PR_TRUE);
        nsIContent* content = pfd->mFrame->GetContent();

        const nsTextFragment* frag = content->GetText();
        if (frag) {
          pfd->SetFlag(PFD_ISNONWHITESPACETEXTFRAME,
                       !content->TextIsOnlyWhitespace());

#ifdef IBMBIDI
          if (mPresContext->BidiEnabled()) {
            if (frag->Is2b()) {
              
              
              PRUnichar ch = 
 *frag->Get2b();
              if (IS_BIDI_DIACRITIC(ch)) {
                mPresContext->PropertyTable()->SetProperty(aFrame,
                           nsGkAtoms::endsInDiacritic, NS_INT32_TO_PTR(ch),
                                                           nsnull, nsnull);
              }
            }
          }
#endif 
        }
      }
    }
    else if (nsGkAtoms::letterFrame==frameType) {
      pfd->SetFlag(PFD_ISLETTERFRAME, PR_TRUE);
    }
    else if (nsGkAtoms::brFrame == frameType) {
      pfd->SetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE, PR_TRUE);
    }
  }

  mSpaceManager->Translate(-tx, -ty);

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

  
  
  
  
  
  pfd->mCombinedArea = metrics.mOverflowArea;

  pfd->mBounds.width = metrics.width;
  pfd->mBounds.height = metrics.height;

  
  aFrame->SetSize(nsSize(metrics.width, metrics.height));

  
  aFrame->DidReflow(mPresContext, &reflowState, NS_FRAME_REFLOW_FINISHED);

  if (aMetrics) {
    *aMetrics = metrics;
  }

  if (!NS_INLINE_IS_BREAK_BEFORE(aReflowStatus)) {
    
    
    
    
    if (NS_FRAME_IS_COMPLETE(aReflowStatus)) {
      nsIFrame* kidNextInFlow = aFrame->GetNextInFlow();
      if (nsnull != kidNextInFlow) {
        
        
        
        nsHTMLContainerFrame* parent = NS_STATIC_CAST(nsHTMLContainerFrame*,
                                                      kidNextInFlow->GetParent());
        parent->DeleteNextInFlowChild(mPresContext, kidNextInFlow);
      }
    }

    
    
    PRBool continuingTextRun = aFrame->CanContinueTextRun();
    
    
    
    if (CanPlaceFrame(pfd, reflowState, notSafeToBreak, continuingTextRun,
                      metrics, aReflowStatus)) {
      
      
      
      PlaceFrame(pfd, metrics);
      PerSpanData* span = pfd->mSpan;
      if (span) {
        
        
        
        VerticalAlignFrames(span);
      }
      
      if (!continuingTextRun) {
        SetFlag(LL_INWORD, PR_FALSE);
        mTrailingTextFrame = nsnull;
        if (!psd->mNoWrap && (!CanPlaceFloatNow() || placedFloat)) {
          
          
          
          if (NotifyOptionalBreakPosition(aFrame->GetContent(), PR_INT32_MAX, PR_TRUE)) {
            
            aReflowStatus = NS_INLINE_LINE_BREAK_AFTER(aReflowStatus);
          }
        }
      }
    }
    else {
      PushFrame(aFrame);
      aPushedFrame = PR_TRUE;
      
      
      RestoreSavedBreakPosition(savedOptionalBreakContent,
                                savedOptionalBreakOffset);
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

  if (aFrame->GetStateBits() & NS_FRAME_IS_BIDI) {
    
    if (NS_INLINE_IS_BREAK_BEFORE(aReflowStatus) ) {
      aFrame->AdjustOffsetsForBidi(start, end);
    }
    else if (!NS_FRAME_IS_COMPLETE(aReflowStatus) ) {
      PRInt32 newEnd;
      aFrame->GetOffsets(start, newEnd);
      if (newEnd != end) {
        nsIFrame* nextInFlow = aFrame->GetNextInFlow();
        if (nextInFlow) {
          nextInFlow->GetOffsets(start, end);
          nextInFlow->AdjustOffsetsForBidi(newEnd, end);
        } 
      } 
    } 
  } 

  return rv;
}

void
nsLineLayout::ApplyStartMargin(PerFrameData* pfd,
                               nsHTMLReflowState& aReflowState)
{
  NS_ASSERTION(aReflowState.mStyleDisplay->mFloats == NS_STYLE_FLOAT_NONE,
               "How'd we get a floated inline frame? "
               "The frame ctor should've dealt with this.");

  
  PRBool ltr = (NS_STYLE_DIRECTION_LTR == aReflowState.mStyleVisibility->mDirection);

  
  if (pfd->mFrame->GetPrevContinuation()) {
    
    
    if (ltr)
      pfd->mMargin.left = 0;
    else
      pfd->mMargin.right = 0;
  }
  else {
    pfd->mBounds.x += ltr ? pfd->mMargin.left : pfd->mMargin.right;

    NS_ASSERTION(NS_UNCONSTRAINEDSIZE != aReflowState.availableWidth,
                 "shouldn't have unconstrained widths anymore");
    if (NS_UNCONSTRAINEDSIZE == aReflowState.ComputedWidth()) {
      
      
      
      
      aReflowState.availableWidth -= ltr ? pfd->mMargin.left : pfd->mMargin.right;
    }
  }
}











PRBool
nsLineLayout::CanPlaceFrame(PerFrameData* pfd,
                            const nsHTMLReflowState& aReflowState,
                            PRBool aNotSafeToBreak,
                            PRBool aFrameCanContinueTextRun,
                            nsHTMLReflowMetrics& aMetrics,
                            nsReflowStatus& aStatus)
{
  NS_PRECONDITION(pfd && pfd->mFrame, "bad args, null pointers for frame data");
  
  if (0 != pfd->mBounds.width) {
    NS_ASSERTION(aReflowState.mStyleDisplay->mFloats == NS_STYLE_FLOAT_NONE,
                 "How'd we get a floated inline frame? "
                 "The frame ctor should've dealt with this.");

    
    PRBool ltr = (NS_STYLE_DIRECTION_LTR == aReflowState.mStyleVisibility->mDirection);

    if ((NS_FRAME_IS_NOT_COMPLETE(aStatus) || (pfd->mFrame->GetNextContinuation() && !pfd->mFrame->GetNextInFlow())) 
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
    
    return PR_TRUE;
  }

  PRBool ltr = NS_STYLE_DIRECTION_LTR == aReflowState.mStyleVisibility->mDirection;
  nscoord endMargin = ltr ? pfd->mMargin.right : pfd->mMargin.left;

#ifdef NOISY_CAN_PLACE_FRAME
  if (nsnull != psd->mFrame) {
    nsFrame::ListTag(stdout, psd->mFrame->mFrame);
  }
  else {
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
  } 
  printf(": aNotSafeToBreak=%s frame=", aNotSafeToBreak ? "true" : "false");
  nsFrame::ListTag(stdout, pfd->mFrame);
  printf(" frameWidth=%d\n", pfd->mBounds.XMost() + endMargin - psd->mX);
#endif

  
  
  PRBool outside = pfd->mBounds.XMost() + endMargin > psd->mRightEdge;
  if (!outside) {
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> inside\n");
#endif
    return PR_TRUE;
  }

  
  
  if (0 == pfd->mMargin.left + pfd->mBounds.width + pfd->mMargin.right) {
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> empty frame fits\n");
#endif
    return PR_TRUE;
  }

#ifdef FIX_BUG_50257
  
  if (nsGkAtoms::brFrame == pfd->mFrame->GetType()) {
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> BR frame fits\n");
#endif
    return PR_TRUE;
  }
#endif

  if (aNotSafeToBreak) {
    
    
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> not-safe and not-impacted fits: ");
    while (nsnull != psd) {
      printf("<psd=%p x=%d left=%d> ", psd, psd->mX, psd->mLeftEdge);
      psd = psd->mParent;
    }
    printf("\n");
#endif
    return PR_TRUE;
  }
 
  
  if (pfd->mSpan && pfd->mSpan->mContainsFloat) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    return PR_TRUE;
 }

  if (aFrameCanContinueTextRun) {
    
    
    
    
#ifdef NOISY_CAN_PLACE_FRAME
    printf("   ==> placing overflowing textrun, requesting backup\n");
#endif
    if (!mLastOptionalBreakContent) {
      
      return PR_TRUE;
    }

    
    
    SetFlag(LL_NEEDBACKUP, PR_TRUE);
  }

#ifdef NOISY_CAN_PLACE_FRAME
  printf("   ==> didn't fit\n");
#endif
  aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  return PR_FALSE;
}




void
nsLineLayout::PlaceFrame(PerFrameData* pfd, nsHTMLReflowMetrics& aMetrics)
{
  
  PerSpanData* psd = mCurrentSpan;
  PRBool emptyFrame = PR_FALSE;
  if ((0 == pfd->mBounds.width) && (0 == pfd->mBounds.height)) {
    pfd->mBounds.x = psd->mX;
    pfd->mBounds.y = mTopEdge;
    emptyFrame = PR_TRUE;
  }

  
  if (aMetrics.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE)
    pfd->mAscent = pfd->mFrame->GetBaseline();
  else
    pfd->mAscent = aMetrics.ascent;

  
  
  if (GetFlag(LL_UPDATEDBAND) && InBlockContext()) {
    UpdateFrames();
    SetFlag(LL_UPDATEDBAND, PR_FALSE);
  }

  PRBool ltr = (NS_STYLE_DIRECTION_LTR == pfd->mFrame->GetStyleVisibility()->mDirection);
  
  psd->mX = pfd->mBounds.XMost() + (ltr ? pfd->mMargin.right : pfd->mMargin.left);

  
  
  
  if ((!GetFlag(LL_UNDERSTANDSNWHITESPACE)) && pfd->mBounds.width) {
    SetFlag(LL_ENDSINWHITESPACE, PR_FALSE);
  }

  
  if (!emptyFrame) {
    mTotalPlacedFrames++;
  }
  if (psd->mX != psd->mLeftEdge || pfd->mBounds.x != psd->mLeftEdge) {
    
    
    SetFlag(LL_CANPLACEFLOAT, PR_FALSE);
  }
}

nsresult
nsLineLayout::AddBulletFrame(nsIFrame* aFrame,
                             const nsHTMLReflowMetrics& aMetrics)
{
  NS_ASSERTION(mCurrentSpan == mRootSpan, "bad linelayout user");

  PerFrameData* pfd;
  nsresult rv = NewPerFrameData(&pfd);
  if (NS_SUCCEEDED(rv)) {
    mRootSpan->AppendFrame(pfd);
    pfd->mFrame = aFrame;
    pfd->mMargin.SizeTo(0, 0, 0, 0);
    pfd->mBorderPadding.SizeTo(0, 0, 0, 0);
    pfd->mFrameType = NS_FRAME_REPLACED(NS_CSS_FRAME_TYPE_INLINE);
    pfd->mFlags = 0;  
    pfd->SetFlag(PFD_ISBULLET, PR_TRUE);
    if (aMetrics.ascent == nsHTMLReflowMetrics::ASK_FOR_BASELINE)
      pfd->mAscent = aFrame->GetBaseline();
    else
      pfd->mAscent = aMetrics.ascent;

    
    pfd->mBounds = aFrame->GetRect();
    pfd->mCombinedArea = aMetrics.mOverflowArea;
  }
  return rv;
}

#ifdef DEBUG
void
nsLineLayout::DumpPerSpanData(PerSpanData* psd, PRInt32 aIndent)
{
  nsFrame::IndentBy(stdout, aIndent);
  printf("%p: left=%d x=%d right=%d\n", NS_STATIC_CAST(void*, psd),
         psd->mLeftEdge, psd->mX, psd->mRightEdge);
  PerFrameData* pfd = psd->mFirstFrame;
  while (nsnull != pfd) {
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
  rootPFD.mFrameType = mBlockReflowState->mFrameType;
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
    PerSpanData* span = pfd->mSpan;
#ifdef DEBUG
    NS_ASSERTION(0xFF != pfd->mVerticalAlign, "umr");
#endif
    switch (pfd->mVerticalAlign) {
      case VALIGN_TOP:
        if (span) {
          pfd->mBounds.y = mTopEdge - pfd->mBorderPadding.top +
            span->mTopLeading;
        }
        else {
          pfd->mBounds.y = mTopEdge + pfd->mMargin.top;
        }
        break;
      case VALIGN_BOTTOM:
        if (span) {
          
          pfd->mBounds.y = mTopEdge + lineHeight -
            pfd->mBounds.height + pfd->mBorderPadding.bottom -
            span->mBottomLeading;
        }
        else {
          pfd->mBounds.y = mTopEdge + lineHeight - pfd->mMargin.bottom -
            pfd->mBounds.height;
        }
        break;
      case VALIGN_OTHER:
        pfd->mBounds.y += baselineY;
        break;
    }
    pfd->mFrame->SetRect(pfd->mBounds);
#ifdef NOISY_VERTICAL_ALIGN
    printf("  [child of line]");
    nsFrame::ListTag(stdout, pfd->mFrame);
    printf(": y=%d\n", pfd->mBounds.y);
#endif
    if (span) {
      nscoord distanceFromTop = pfd->mBounds.y - mTopEdge;
      PlaceTopBottomFrames(span, distanceFromTop, lineHeight);
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

  
  mRootSpan->mFrame = nsnull;
}

void
nsLineLayout::PlaceTopBottomFrames(PerSpanData* psd,
                                   nscoord aDistanceFromTop,
                                   nscoord aLineHeight)
{
  PerFrameData* pfd = psd->mFirstFrame;
  while (nsnull != pfd) {
    PerSpanData* span = pfd->mSpan;
#ifdef DEBUG
    NS_ASSERTION(0xFF != pfd->mVerticalAlign, "umr");
#endif
    switch (pfd->mVerticalAlign) {
      case VALIGN_TOP:
        if (span) {
          pfd->mBounds.y = -aDistanceFromTop - pfd->mBorderPadding.top +
            span->mTopLeading;
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
          
          pfd->mBounds.y = -aDistanceFromTop + aLineHeight -
            pfd->mBounds.height + pfd->mBorderPadding.bottom -
            span->mBottomLeading;
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
    pfd = pfd->mNext;
  }
}

#define VERTICAL_ALIGN_FRAMES_NO_MINIMUM 32767
#define VERTICAL_ALIGN_FRAMES_NO_MAXIMUM -32768





void
nsLineLayout::VerticalAlignFrames(PerSpanData* psd)
{
  
  PerFrameData* spanFramePFD = psd->mFrame;
  nsIFrame* spanFrame = spanFramePFD->mFrame;

  
  nsStyleContext* styleContext = spanFrame->GetStyleContext();
  nsIRenderingContext* rc = mBlockReflowState->rendContext;
  nsLayoutUtils::SetFontFromStyle(mBlockReflowState->rendContext, styleContext);
  nsCOMPtr<nsIFontMetrics> fm;
  rc->GetFontMetrics(*getter_AddRefs(fm));

  PRBool preMode = (mStyleText->mWhiteSpace == NS_STYLE_WHITESPACE_PRE) ||
    (mStyleText->mWhiteSpace == NS_STYLE_WHITESPACE_MOZ_PRE_WRAP);

  
  
  
  
  nsIFrame* spanNextInFlow = spanFrame->GetNextInFlow();
  nsIFrame* spanPrevInFlow = spanFrame->GetPrevInFlow();
  PRBool emptyContinuation = spanPrevInFlow && !spanNextInFlow &&
    (0 == spanFramePFD->mBounds.width) && (0 == spanFramePFD->mBounds.height);

#ifdef NOISY_VERTICAL_ALIGN
  printf("[%sSpan]", (psd == mRootSpan)?"Root":"");
  nsFrame::ListTag(stdout, spanFrame);
  printf(": preMode=%s strictMode=%s w/h=%d,%d emptyContinuation=%s",
         preMode ? "yes" : "no",
         InStrictMode() ? "yes" : "no",
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

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  PRBool zeroEffectiveSpanBox = PR_FALSE;
  
  
  
  
  if ((emptyContinuation || mCompatMode != eCompatibility_FullStandards) &&
      ((psd == mRootSpan) ||
       ((0 == spanFramePFD->mBorderPadding.top) &&
        (0 == spanFramePFD->mBorderPadding.right) &&
        (0 == spanFramePFD->mBorderPadding.bottom) &&
        (0 == spanFramePFD->mBorderPadding.left) &&
        (0 == spanFramePFD->mMargin.top) &&
        (0 == spanFramePFD->mMargin.right) &&
        (0 == spanFramePFD->mMargin.bottom) &&
        (0 == spanFramePFD->mMargin.left)))) {
    
    
    
    
    
    
    
    
    

    
    
    
    
    
    zeroEffectiveSpanBox = PR_TRUE;
    for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
      if (pfd->GetFlag(PFD_ISTEXTFRAME) &&
          (pfd->GetFlag(PFD_ISNONWHITESPACETEXTFRAME) || preMode ||
           pfd->mBounds.width != 0)) {
        zeroEffectiveSpanBox = PR_FALSE;
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
    
    
    
    nscoord logicalHeight = nsHTMLReflowState::CalcLineHeight(rc, spanFrame);
    nscoord contentHeight = spanFramePFD->mBounds.height -
      spanFramePFD->mBorderPadding.top - spanFramePFD->mBorderPadding.bottom;

    
    
    if (spanFramePFD->GetFlag(PFD_ISLETTERFRAME) && !spanPrevInFlow &&
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

    
    
    
    baselineY = spanFramePFD->mAscent;


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
  while (nsnull != pfd) {
    nsIFrame* frame = pfd->mFrame;

    
    NS_ASSERTION(frame, "null frame in PerFrameData - something is very very bad");
    if (!frame) {
      return;
    }

    
    nscoord logicalHeight;
    nscoord topLeading;
    PerSpanData* frameSpan = pfd->mSpan;
    if (frameSpan) {
      
      
      logicalHeight = frameSpan->mLogicalHeight;
      topLeading = frameSpan->mTopLeading;
    }
    else {
      
      
      logicalHeight = pfd->mBounds.height + pfd->mMargin.top +
        pfd->mMargin.bottom;
      topLeading = 0;
    }

    
    const nsStyleTextReset* textStyle = frame->GetStyleTextReset();
    nsStyleUnit verticalAlignUnit = textStyle->mVerticalAlign.GetUnit();
#ifdef NOISY_VERTICAL_ALIGN
    printf("  [frame]");
    nsFrame::ListTag(stdout, frame);
    printf(": verticalAlignUnit=%d (enum == %d)\n",
           verticalAlignUnit,
           ((eStyleUnit_Enumerated == verticalAlignUnit)
            ? textStyle->mVerticalAlign.GetIntValue()
            : -1));
#endif

    PRUint8 verticalAlignEnum;
    nscoord parentAscent, parentDescent, parentXHeight;
    nscoord parentSuperscript, parentSubscript;
    nscoord coordOffset, percentOffset, elementLineHeight;
    nscoord revisedBaselineY;
    switch (verticalAlignUnit) {
      case eStyleUnit_Enumerated:
      default:
        if (eStyleUnit_Enumerated == verticalAlignUnit) {
          verticalAlignEnum = textStyle->mVerticalAlign.GetIntValue();
        }
        else {
          verticalAlignEnum = NS_STYLE_VERTICAL_ALIGN_BASELINE;
        }
        switch (verticalAlignEnum) {
          default:
          case NS_STYLE_VERTICAL_ALIGN_BASELINE:
            
            
            pfd->mBounds.y = baselineY - pfd->mAscent;
            pfd->mVerticalAlign = VALIGN_OTHER;
            break;

          case NS_STYLE_VERTICAL_ALIGN_SUB:
            
            
            
            
            fm->GetSubscriptOffset(parentSubscript);
            revisedBaselineY = baselineY + parentSubscript;
            pfd->mBounds.y = revisedBaselineY - pfd->mAscent;
            pfd->mVerticalAlign = VALIGN_OTHER;
            break;

          case NS_STYLE_VERTICAL_ALIGN_SUPER:
            
            
            
            
            fm->GetSuperscriptOffset(parentSuperscript);
            revisedBaselineY = baselineY - parentSuperscript;
            pfd->mBounds.y = revisedBaselineY - pfd->mAscent;
            pfd->mVerticalAlign = VALIGN_OTHER;
            break;

          case NS_STYLE_VERTICAL_ALIGN_TOP:
            pfd->mVerticalAlign = VALIGN_TOP;
            if (logicalHeight > maxTopBoxHeight) {
              maxTopBoxHeight = logicalHeight;
            }
            break;

          case NS_STYLE_VERTICAL_ALIGN_BOTTOM:
            pfd->mVerticalAlign = VALIGN_BOTTOM;
            if (logicalHeight > maxBottomBoxHeight) {
              maxBottomBoxHeight = logicalHeight;
            }
            break;

          case NS_STYLE_VERTICAL_ALIGN_MIDDLE:
            
            
            fm->GetXHeight(parentXHeight);
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

          case NS_STYLE_VERTICAL_ALIGN_TEXT_TOP:
            
            
            fm->GetMaxAscent(parentAscent);
            if (frameSpan) {
              pfd->mBounds.y = baselineY - parentAscent -
                pfd->mBorderPadding.top + frameSpan->mTopLeading;
            }
            else {
              pfd->mBounds.y = baselineY - parentAscent + pfd->mMargin.top;
            }
            pfd->mVerticalAlign = VALIGN_OTHER;
            break;

          case NS_STYLE_VERTICAL_ALIGN_TEXT_BOTTOM:
            
            
            fm->GetMaxDescent(parentDescent);
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

          case NS_STYLE_VERTICAL_ALIGN_MIDDLE_WITH_BASELINE:
            
            if (frameSpan) {
              pfd->mBounds.y = baselineY - pfd->mBounds.height/2;
            }
            else {
              pfd->mBounds.y = baselineY - logicalHeight/2 + pfd->mMargin.top;
            }
            pfd->mVerticalAlign = VALIGN_OTHER;
            break; 	    
        }
        break;

      case eStyleUnit_Coord:
        
        
        
        
        
        coordOffset = textStyle->mVerticalAlign.GetCoordValue();
        revisedBaselineY = baselineY - coordOffset;
        pfd->mBounds.y = revisedBaselineY - pfd->mAscent;
        pfd->mVerticalAlign = VALIGN_OTHER;
        break;

      case eStyleUnit_Percent:
        
        
        elementLineHeight = nsHTMLReflowState::CalcLineHeight(rc, frame);
        percentOffset = nscoord(
          textStyle->mVerticalAlign.GetPercentValue() * elementLineHeight
          );
        revisedBaselineY = baselineY - percentOffset;
        pfd->mBounds.y = revisedBaselineY - pfd->mAscent;
        pfd->mVerticalAlign = VALIGN_OTHER;
        break;
    }

    
    
    if (pfd->mVerticalAlign == VALIGN_OTHER) {
      
      
      
      
      
      
      
      
      
      
      
      
#if 0
      if (!pfd->GetFlag(PFD_ISTEXTFRAME)) {
#else

      PRBool canUpdate = !pfd->GetFlag(PFD_ISTEXTFRAME);
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
            GetCompatMode() != eCompatibility_FullStandards &&
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
               pfd->mSpan ? topLeading : 0,
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
    
    
    
    
    
    
    
    
    
    
    
    PRBool applyMinLH = !(psd->mZeroEffectiveSpanBox); 
    PRBool isFirstLine = !mLineNumber; 
    PRBool isLastLine = (!mLineBox->IsLineWrapped() && !GetFlag(LL_LINEENDSINBR));
    PRBool foundLI = PR_FALSE;  
    
    
    
    
    if (!applyMinLH && (isFirstLine || isLastLine)) {
      nsIContent* blockContent = mRootSpan->mFrame->mFrame->GetContent();
      if (blockContent) {
        nsIAtom *blockTagAtom = blockContent->Tag();
        
        if (isFirstLine && blockTagAtom == nsGkAtoms::li) {
          
          
          if (!IsZeroHeight()) {
            applyMinLH = PR_TRUE;
            foundLI = PR_TRUE;
          }
        }
        
        else if (!applyMinLH && isLastLine &&
                 ((blockTagAtom == nsGkAtoms::li) ||
                  (blockTagAtom == nsGkAtoms::dt) ||
                  (blockTagAtom == nsGkAtoms::dd))) {
          applyMinLH = PR_TRUE;
        }
      }
    }
    if (applyMinLH) {
      if ((psd->mX != psd->mLeftEdge) || preMode || foundLI) {
#ifdef NOISY_VERTICAL_ALIGN
        printf("  [span]==> adjusting min/maxY: currentValues: %d,%d", minY, maxY);
#endif
        nscoord minimumLineHeight = mMinLineHeight;
        nscoord fontAscent, fontHeight;
        fm->GetMaxAscent(fontAscent);
        fm->GetHeight(fontHeight);

        nscoord leading = minimumLineHeight - fontHeight;
        nscoord yTop = -fontAscent - leading/2;
        nscoord yBottom = yTop + minimumLineHeight;
        if (yTop < minY) minY = yTop;
        if (yBottom > maxY) maxY = yBottom;

#ifdef NOISY_VERTICAL_ALIGN
        printf(" new values: %d,%d\n", minY, maxY);
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

      pfd = psd->mFirstFrame;
      while (nsnull != pfd) {
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

PRBool
nsLineLayout::TrimTrailingWhiteSpaceIn(PerSpanData* psd,
                                       nscoord* aDeltaWidth)
{
#ifndef IBMBIDI

  if (NS_STYLE_DIRECTION_RTL == psd->mDirection) {
    *aDeltaWidth = 0;
    return PR_TRUE;
  }
#endif

  PerFrameData* pfd = psd->mFirstFrame;
  if (!pfd) {
    *aDeltaWidth = 0;
    return PR_FALSE;
  }
  pfd = pfd->Last();
  while (nsnull != pfd) {
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
        return PR_TRUE;
      }
    }
    else if (!pfd->GetFlag(PFD_ISTEXTFRAME) &&
             !pfd->GetFlag(PFD_SKIPWHENTRIMMINGWHITESPACE)) {
      
      
      *aDeltaWidth = 0;
      return PR_TRUE;
    }
    else if (pfd->GetFlag(PFD_ISNONEMPTYTEXTFRAME)) {
      nscoord deltaWidth = 0;
      PRBool lastCharIsJustifiable = PR_FALSE;
      pfd->mFrame->TrimTrailingWhiteSpace(mPresContext,
                                          *mBlockReflowState->rendContext,
                                          deltaWidth,
                                          lastCharIsJustifiable);
#ifdef NOISY_TRIM
      nsFrame::ListTag(stdout, (psd == mRootSpan
                                ? mBlockReflowState->frame
                                : psd->mFrame->mFrame));
      printf(": trim of ");
      nsFrame::ListTag(stdout, pfd->mFrame);
      printf(" returned %d\n", deltaWidth);
#endif
      if (lastCharIsJustifiable && pfd->mJustificationNumSpaces > 0) {
        pfd->mJustificationNumSpaces--;
      }

      if (deltaWidth) {
        pfd->mBounds.width -= deltaWidth;

        
        if (psd != mRootSpan) {
          
          
          pfd->mFrame->SetRect(pfd->mBounds);
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

      
      *aDeltaWidth = deltaWidth;
      return PR_TRUE;
    }
    pfd = pfd->mPrev;
  }

  *aDeltaWidth = 0;
  return PR_FALSE;
}

PRBool
nsLineLayout::TrimTrailingWhiteSpace()
{
  PerSpanData* psd = mRootSpan;
  nscoord deltaWidth;
  TrimTrailingWhiteSpaceIn(psd, &deltaWidth);
  return 0 != deltaWidth;
}

void
nsLineLayout::ComputeJustificationWeights(PerSpanData* aPSD,
                                          PRInt32* aNumSpaces,
                                          PRInt32* aNumLetters)
{
  NS_ASSERTION(aPSD, "null arg");
  NS_ASSERTION(aNumSpaces, "null arg");
  NS_ASSERTION(aNumLetters, "null arg");
  PRInt32 numSpaces = 0;
  PRInt32 numLetters = 0;

  for (PerFrameData* pfd = aPSD->mFirstFrame; pfd != nsnull; pfd = pfd->mNext) {

    if (PR_TRUE == pfd->GetFlag(PFD_ISTEXTFRAME)) {
      numSpaces += pfd->mJustificationNumSpaces;
      numLetters += pfd->mJustificationNumLetters;
    }
    else if (pfd->mSpan != nsnull) {
      PRInt32 spanSpaces;
      PRInt32 spanLetters;

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
  for (PerFrameData* pfd = aPSD->mFirstFrame; pfd != nsnull; pfd = pfd->mNext) {
    
    if (!pfd->GetFlag(PFD_ISBULLET)) {
      nscoord dw = 0;
      
      pfd->mBounds.x += deltaX;
      
      if (PR_TRUE == pfd->GetFlag(PFD_ISTEXTFRAME)) {
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
      }
      else {
        if (nsnull != pfd->mSpan) {
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
                                    PRBool aAllowJustify)
{
  PerSpanData* psd = mRootSpan;
  nscoord availWidth = psd->mRightEdge;
  if (NS_UNCONSTRAINEDSIZE == availWidth) {
    NS_NOTREACHED("This shouldn't be called anymore");
    
#ifdef NOISY_HORIZONTAL_ALIGN
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": skipping horizontal alignment in pass1 table reflow\n");
#endif
    return;
  }
  availWidth -= psd->mLeftEdge;
  nscoord remainingWidth = availWidth - aLineBounds.width;
#ifdef NOISY_HORIZONTAL_ALIGN
    nsFrame::ListTag(stdout, mBlockReflowState->frame);
    printf(": availWidth=%d lineWidth=%d delta=%d\n",
           availWidth, aLineBounds.width, remainingWidth);
#endif
#ifdef IBMBIDI
  nscoord dx = 0;
#endif

  if (remainingWidth > 0)
  {
#ifndef IBMBIDI
    nscoord dx = 0;
#endif
    switch (mTextAlign) {
      case NS_STYLE_TEXT_ALIGN_JUSTIFY:
        
        
        if (aAllowJustify) {
          PRInt32 numSpaces;
          PRInt32 numLetters;
            
          ComputeJustificationWeights(psd, &numSpaces, &numLetters);

          if (numSpaces > 0) {
            FrameJustificationState state =
              { numSpaces, numLetters, remainingWidth, 0, 0, 0, 0, 0 };

            
            
            aLineBounds.width += ApplyFrameJustification(psd, &state);
            break;
          }
        }
        
        

      case NS_STYLE_TEXT_ALIGN_DEFAULT:
        if (NS_STYLE_DIRECTION_LTR == psd->mDirection) {
          
          break;
        }
        
        

      case NS_STYLE_TEXT_ALIGN_RIGHT:
      case NS_STYLE_TEXT_ALIGN_MOZ_RIGHT:
        dx = remainingWidth;
        break;

      case NS_STYLE_TEXT_ALIGN_LEFT:
      case NS_STYLE_TEXT_ALIGN_MOZ_LEFT:
        break;

      case NS_STYLE_TEXT_ALIGN_CENTER:
      case NS_STYLE_TEXT_ALIGN_MOZ_CENTER:
        dx = remainingWidth / 2;
        break;
    }
#ifdef IBMBIDI
  }
  else if (remainingWidth < 0) {
    if (NS_STYLE_DIRECTION_RTL == psd->mDirection) {
      dx = remainingWidth;
      psd->mX += dx;
      psd->mLeftEdge += dx;
    }
  }
  PRBool isRTL = ( (NS_STYLE_DIRECTION_RTL == psd->mDirection)
                && (!psd->mChangedFrameDirection) );
  if (dx || isRTL) {
    PerFrameData* bulletPfd = nsnull;
    nscoord maxX = aLineBounds.XMost() + dx;
    PRBool isVisualRTL = PR_FALSE;

    if (isRTL) {
      if (psd->mLastFrame->GetFlag(PFD_ISBULLET) )
        bulletPfd = psd->mLastFrame;
  
      psd->mChangedFrameDirection = PR_TRUE;

      isVisualRTL = mPresContext->IsVisualMode();
    }
    if (dx || isVisualRTL)
#else
    if (0 != dx)
#endif
    {
      for (PerFrameData* pfd = psd->mFirstFrame; pfd
#ifdef IBMBIDI
           && bulletPfd != pfd
#endif
           ; pfd = pfd->mNext) {
#ifdef IBMBIDI
        if (isVisualRTL) {
          
          maxX = pfd->mBounds.x = maxX - (pfd->mMargin.left + pfd->mBounds.width + pfd->mMargin.right);
        }
        else
#endif 
          pfd->mBounds.x += dx;
        pfd->mFrame->SetRect(pfd->mBounds);
      }
      aLineBounds.x += dx;
    }
#ifndef IBMBIDI
    if ((NS_STYLE_DIRECTION_RTL == psd->mDirection) &&
        !psd->mChangedFrameDirection) {
      psd->mChangedFrameDirection = PR_TRUE;
  
      PerFrameData* pfd = psd->mFirstFrame;
      PRUint32 maxX = psd->mRightEdge;
      while (nsnull != pfd) {
        pfd->mBounds.x = maxX - (pfd->mMargin.left + pfd->mBounds.width + pfd->mMargin.right);
        pfd->mFrame->SetRect(pfd->mBounds);
        maxX = pfd->mBounds.x;
        pfd = pfd->mNext;
      }
    }
#endif 
  }
}

void
nsLineLayout::RelativePositionFrames(nsRect& aCombinedArea)
{
  RelativePositionFrames(mRootSpan, aCombinedArea);
}

void
nsLineLayout::RelativePositionFrames(PerSpanData* psd, nsRect& aCombinedArea)
{
  nsRect combinedAreaResult;
  if (nsnull != psd->mFrame) {
    
    
    
    
    
    
    
    nsRect adjustedBounds(0, 0, psd->mFrame->mBounds.width,
                          psd->mFrame->mBounds.height);
    combinedAreaResult.UnionRect(psd->mFrame->mCombinedArea, adjustedBounds);
  }
  else {
    
    
    
    
    combinedAreaResult.x = psd->mLeftEdge;
    
    
    combinedAreaResult.width = psd->mX - combinedAreaResult.x;
    combinedAreaResult.y = mTopEdge;
    combinedAreaResult.height = mFinalLineHeight;
  }

  for (PerFrameData* pfd = psd->mFirstFrame; pfd; pfd = pfd->mNext) {
    nsPoint origin = nsPoint(pfd->mBounds.x, pfd->mBounds.y);
    nsIFrame* frame = pfd->mFrame;

    
    if (pfd->GetFlag(PFD_RELATIVEPOS)) {
      
      
      nsPoint change(pfd->mOffsets.left, pfd->mOffsets.top);
      frame->SetPosition(frame->GetPosition() + change);
      origin += change;
    }

    
    
    
    if (frame->HasView())
      nsContainerFrame::SyncFrameViewAfterReflow(mPresContext, frame,
                                                 frame->GetView(),
                                                 &pfd->mCombinedArea, 
                                                 NS_FRAME_NO_SIZE_VIEW);

    
    
    
    
    nsRect r;
    if (pfd->mSpan) {
      
      
      RelativePositionFrames(pfd->mSpan, r);
    } else {
      
      
      
      nsRect adjustedBounds(0, 0, pfd->mBounds.width, pfd->mBounds.height);
      r.UnionRect(pfd->mCombinedArea, adjustedBounds);

      
      
      
      
      
      
      
      nsContainerFrame::PositionChildViews(frame);
    }

    
    
    
    if (frame->HasView())
      nsContainerFrame::SyncFrameViewAfterReflow(mPresContext, frame,
                                                 frame->GetView(), &r,
                                                 NS_FRAME_NO_MOVE_VIEW);

    combinedAreaResult.UnionRect(combinedAreaResult, r + origin);
  }

  
  
  if (psd->mFrame) {
    PerFrameData* spanPFD = psd->mFrame;
    nsIFrame* frame = spanPFD->mFrame;
    frame->FinishAndStoreOverflow(&combinedAreaResult, frame->GetSize());
  }
  aCombinedArea = combinedAreaResult;
}

PRBool
nsLineLayout::TreatFrameAsBlock(nsIFrame* aFrame)
{
  const nsStyleDisplay* display = aFrame->GetStyleDisplay();
  if (NS_STYLE_POSITION_ABSOLUTE == display->mPosition) {
    return PR_FALSE;
  }
  if (NS_STYLE_FLOAT_NONE != display->mFloats) {
    return PR_FALSE;
  }
  switch (display->mDisplay) {
  case NS_STYLE_DISPLAY_BLOCK:
  case NS_STYLE_DISPLAY_LIST_ITEM:
  case NS_STYLE_DISPLAY_RUN_IN:
  case NS_STYLE_DISPLAY_COMPACT:
  case NS_STYLE_DISPLAY_TABLE:
    return PR_TRUE;
  }
  return PR_FALSE;
}
