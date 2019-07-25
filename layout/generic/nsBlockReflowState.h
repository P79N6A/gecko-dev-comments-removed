









































#ifndef nsBlockReflowState_h__
#define nsBlockReflowState_h__

#include "nsFloatManager.h"
#include "nsLineBox.h"
#include "nsFrameList.h"
#include "nsBlockFrame.h"

  
#define BRS_UNCONSTRAINEDHEIGHT   0x00000001
#define BRS_ISTOPMARGINROOT       0x00000002  // Is this frame a root for top/bottom margin collapsing?
#define BRS_ISBOTTOMMARGINROOT    0x00000004
#define BRS_APPLYTOPMARGIN        0x00000008  // See ShouldApplyTopMargin
#define BRS_ISFIRSTINFLOW         0x00000010

#define BRS_HAVELINEADJACENTTOTOP 0x00000020

#define BRS_FLOAT_MGR             0x00000040


#define BRS_LINE_LAYOUT_EMPTY     0x00000080
#define BRS_ISOVERFLOWCONTAINER   0x00000100

#define BRS_PROPTABLE_FLOATCLIST  0x00000200
#define BRS_LASTFLAG              BRS_PROPTABLE_FLOATCLIST

class nsBlockReflowState {
public:
  nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                     nsPresContext* aPresContext,
                     nsBlockFrame* aFrame,
                     const nsHTMLReflowMetrics& aMetrics,
                     bool aTopMarginRoot, bool aBottomMarginRoot,
                     bool aBlockNeedsFloatManager);

  








  nsFlowAreaRect GetFloatAvailableSpace() const
    { return GetFloatAvailableSpace(mY); }
  nsFlowAreaRect GetFloatAvailableSpace(nscoord aY) const
    { return GetFloatAvailableSpaceWithState(aY, nsnull); }
  nsFlowAreaRect
    GetFloatAvailableSpaceWithState(nscoord aY,
                                    nsFloatManager::SavedState *aState) const;
  nsFlowAreaRect
    GetFloatAvailableSpaceForHeight(nscoord aY, nscoord aHeight,
                                    nsFloatManager::SavedState *aState) const;

  






  bool AddFloat(nsLineLayout*       aLineLayout,
                  nsIFrame*           aFloat,
                  nscoord             aAvailableWidth);
private:
  bool CanPlaceFloat(nscoord aFloatWidth,
                       const nsFlowAreaRect& aFloatAvailableSpace);
public:
  bool FlowAndPlaceFloat(nsIFrame* aFloat);
private:
  void PushFloatPastBreak(nsIFrame* aFloat);
public:
  void PlaceBelowCurrentLineFloats(nsFloatCacheFreeList& aFloats,
                                   nsLineBox* aLine);

  
  
  
  nscoord ClearFloats(nscoord aY, PRUint8 aBreakType,
                      nsIFrame *aReplacedBlock = nsnull,
                      PRUint32 aFlags = 0);

  bool IsAdjacentWithTop() const {
    return mY ==
      ((mFlags & BRS_ISFIRSTINFLOW) ? mReflowState.mComputedBorderPadding.top : 0);
  }

  



  nsMargin BorderPadding() const {
    nsMargin result = mReflowState.mComputedBorderPadding;
    if (!(mFlags & BRS_ISFIRSTINFLOW)) {
      result.top = 0;
      if (mFlags & BRS_ISOVERFLOWCONTAINER) {
        result.bottom = 0;
      }
    }
    return result;
  }

  
  const nsMargin& Margin() const {
    return mReflowState.mComputedMargin;
  }

  
  void ReconstructMarginAbove(nsLineList::iterator aLine);

  
  
  
  void ComputeReplacedBlockOffsetsForFloats(nsIFrame* aFrame,
                                            const nsRect& aFloatAvailableSpace,
                                            nscoord& aLeftResult,
                                            nscoord& aRightResult,
                                       nsBlockFrame::ReplacedElementWidthToClear
                                                      *aReplacedWidth = nsnull);

  
  void ComputeBlockAvailSpace(nsIFrame* aFrame,
                              const nsStyleDisplay* aDisplay,
                              const nsFlowAreaRect& aFloatAvailableSpace,
                              bool aBlockAvoidsFloats,
                              nsRect& aResult);

protected:
  void RecoverFloats(nsLineList::iterator aLine, nscoord aDeltaY);

public:
  void RecoverStateFrom(nsLineList::iterator aLine, nscoord aDeltaY);

  void AdvanceToNextLine() {
    if (GetFlag(BRS_LINE_LAYOUT_EMPTY)) {
      SetFlag(BRS_LINE_LAYOUT_EMPTY, PR_FALSE);
    } else {
      mLineNumber++;
    }
  }

  nsLineBox* NewLineBox(nsIFrame* aFrame, PRInt32 aCount, bool aIsBlock);

  void FreeLineBox(nsLineBox* aLine);

  

  
  

  
  nsBlockFrame* mBlock;

  nsPresContext* mPresContext;

  const nsHTMLReflowState& mReflowState;

  nsFloatManager* mFloatManager;

  
  
  
  
  
  nscoord mFloatManagerX, mFloatManagerY;

  
  nsReflowStatus mReflowStatus;

  
  
  
  
  nsFloatManager::SavedState mFloatManagerStateBefore;

  nscoord mBottomEdge;

  
  
  
  
  
  
  
  
  nsRect mContentArea;

  
  
  
  nsFrameList *mPushedFloats;
  
  
  void SetupPushedFloatList();
  
  void AppendPushedFloat(nsIFrame* aFloatCont) {
    SetupPushedFloatList();
    aFloatCont->AddStateBits(NS_FRAME_IS_PUSHED_FLOAT);
    mPushedFloats->AppendFrame(mBlock, aFloatCont);
  }

  
  nsOverflowContinuationTracker* mOverflowTracker;

  

  
  
  

  
  
  nsLineList::iterator mCurrentLine;

  
  
  
  
  nsLineList::iterator mLineAdjacentToTop;

  
  nscoord mY;

  
  nsOverflowAreas mFloatOverflowAreas;

  nsFloatCacheFreeList mFloatCacheFreeList;

  
  
  nsIFrame* mPrevChild;

  
  nsCollapsingMargin mPrevBottomMargin;

  
  
  
  
  nsBlockFrame* mNextInFlow;

  

  
  

  
  
  
  nsFloatCacheFreeList mCurrentLineFloats;

  
  
  
  
  nsFloatCacheFreeList mBelowCurrentLineFloats;

  nscoord mMinLineHeight;

  PRInt32 mLineNumber;

  PRInt16 mFlags;
 
  PRUint8 mFloatBreakType;

  void SetFlag(PRUint32 aFlag, bool aValue)
  {
    NS_ASSERTION(aFlag<=BRS_LASTFLAG, "bad flag");
    NS_ASSERTION(aValue==PR_FALSE || aValue==PR_TRUE, "bad value");
    if (aValue) { 
      mFlags |= aFlag;
    }
    else {        
      mFlags &= ~aFlag;
    }
  }

  bool GetFlag(PRUint32 aFlag) const
  {
    NS_ASSERTION(aFlag<=BRS_LASTFLAG, "bad flag");
    return !!(mFlags & aFlag);
  }
};

#endif 
