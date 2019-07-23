









































#ifndef nsBlockReflowState_h__
#define nsBlockReflowState_h__

#include "nsBlockBandData.h"
#include "nsLineBox.h"
#include "nsFrameList.h"
#include "nsContainerFrame.h"

class nsBlockFrame;

  
#define BRS_UNCONSTRAINEDHEIGHT   0x00000001
#define BRS_ISTOPMARGINROOT       0x00000002  // Is this frame a root for top/bottom margin collapsing?
#define BRS_ISBOTTOMMARGINROOT    0x00000004
#define BRS_APPLYTOPMARGIN        0x00000008  // See ShouldApplyTopMargin
#define BRS_ISFIRSTINFLOW         0x00000010

#define BRS_HAVELINEADJACENTTOTOP 0x00000020

#define BRS_SPACE_MGR             0x00000040
#define BRS_ISOVERFLOWCONTAINER   0x00000100
#define BRS_LASTFLAG              BRS_ISOVERFLOWCONTAINER

class nsBlockReflowState {
public:
  nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                     nsPresContext* aPresContext,
                     nsBlockFrame* aFrame,
                     const nsHTMLReflowMetrics& aMetrics,
                     PRBool aTopMarginRoot, PRBool aBottomMarginRoot,
                     PRBool aBlockNeedsSpaceManager);

  ~nsBlockReflowState();

  
  
  
  
  
  void SetupOverflowPlaceholdersProperty();

  




  void GetAvailableSpace() { GetAvailableSpace(mY, PR_FALSE); }
  void GetAvailableSpace(nscoord aY, PRBool aRelaxHeightConstraint);

  




  PRBool InitFloat(nsLineLayout&       aLineLayout,
                   nsPlaceholderFrame* aPlaceholderFrame,
                   nsReflowStatus&     aReflowStatus);
  PRBool AddFloat(nsLineLayout&       aLineLayout,
                  nsPlaceholderFrame* aPlaceholderFrame,
                  PRBool              aInitialReflow,
                  nsReflowStatus&     aReflowStatus);
  PRBool CanPlaceFloat(const nsSize& aFloatSize, PRUint8 aFloats, PRBool aForceFit);
  PRBool FlowAndPlaceFloat(nsFloatCache*   aFloatCache,
                           PRBool*         aIsLeftFloat,
                           nsReflowStatus& aReflowStatus,
                           PRBool          aForceFit);
  PRBool PlaceBelowCurrentLineFloats(nsFloatCacheFreeList& aFloats, PRBool aForceFit);

  
  
  nscoord ClearFloats(nscoord aY, PRUint8 aBreakType);

  PRBool IsAdjacentWithTop() const {
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

  void ComputeBlockAvailSpace(nsIFrame* aFrame,
                              const nsStyleDisplay* aDisplay,
                              nsRect& aResult);

protected:
  void RecoverFloats(nsLineList::iterator aLine, nscoord aDeltaY);

public:
  void RecoverStateFrom(nsLineList::iterator aLine, nscoord aDeltaY);

  void AdvanceToNextLine() {
    mLineNumber++;
  }

  PRBool IsImpactedByFloat() const;

  nsLineBox* NewLineBox(nsIFrame* aFrame, PRInt32 aCount, PRBool aIsBlock);

  void FreeLineBox(nsLineBox* aLine);

  

  
  

  
  nsBlockFrame* mBlock;

  nsPresContext* mPresContext;

  const nsHTMLReflowState& mReflowState;

  nsSpaceManager* mSpaceManager;

  
  
  
  
  
  nscoord mSpaceManagerX, mSpaceManagerY;

  
  nsReflowStatus mReflowStatus;

  nscoord mBottomEdge;

  
  
  
  
  
  nsSize mContentArea;

  
  
  
  
  
  
  nsFrameList mOverflowPlaceholders;

  
  nsOverflowContinuationTracker mOverflowTracker;

  

  
  
  

  
  
  nsLineList::iterator mCurrentLine;

  
  
  
  
  nsLineList::iterator mLineAdjacentToTop;

  
  nscoord mY;

  
  nsRect mAvailSpaceRect;

  
  nsRect mFloatCombinedArea;

  nsFloatCacheFreeList mFloatCacheFreeList;

  
  
  nsIFrame* mPrevChild;

  
  nsCollapsingMargin mPrevBottomMargin;

  
  
  
  
  nsBlockFrame* mNextInFlow;

  
  nsBlockBandData mBand;

  

  
  

  
  
  
  nsFloatCacheFreeList mCurrentLineFloats;

  
  
  
  
  nsFloatCacheFreeList mBelowCurrentLineFloats;

  nscoord mMinLineHeight;

  PRInt32 mLineNumber;

  PRInt16 mFlags;
 
  PRUint8 mFloatBreakType;

  void SetFlag(PRUint32 aFlag, PRBool aValue)
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

  PRBool GetFlag(PRUint32 aFlag) const
  {
    NS_ASSERTION(aFlag<=BRS_LASTFLAG, "bad flag");
    PRBool result = (mFlags & aFlag);
    if (result) return PR_TRUE;
    return PR_FALSE;
  }
};

#endif 
