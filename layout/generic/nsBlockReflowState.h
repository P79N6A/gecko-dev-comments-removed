






#ifndef nsBlockReflowState_h__
#define nsBlockReflowState_h__

#include "nsFloatManager.h"
#include "nsLineBox.h"
#include "nsHTMLReflowState.h"

class nsBlockFrame;
class nsFrameList;
class nsOverflowContinuationTracker;

  
#define BRS_UNCONSTRAINEDBSIZE    0x00000001
#define BRS_ISBSTARTMARGINROOT    0x00000002  // Is this frame a root for block
#define BRS_ISBENDMARGINROOT      0x00000004  //  direction start/end margin collapsing?
#define BRS_APPLYBSTARTMARGIN     0x00000008  // See ShouldApplyTopMargin
#define BRS_ISFIRSTINFLOW         0x00000010

#define BRS_HAVELINEADJACENTTOTOP 0x00000020

#define BRS_FLOAT_MGR             0x00000040


#define BRS_LINE_LAYOUT_EMPTY     0x00000080
#define BRS_ISOVERFLOWCONTAINER   0x00000100

#define BRS_PROPTABLE_FLOATCLIST  0x00000200

#define BRS_FLOAT_FRAGMENTS_INSIDE_COLUMN_ENABLED 0x00000400
#define BRS_LASTFLAG              BRS_FLOAT_FRAGMENTS_INSIDE_COLUMN_ENABLED

class nsBlockReflowState {
public:
  nsBlockReflowState(const nsHTMLReflowState& aReflowState,
                     nsPresContext* aPresContext,
                     nsBlockFrame* aFrame,
                     bool aBStartMarginRoot, bool aBEndMarginRoot,
                     bool aBlockNeedsFloatManager,
                     nscoord aConsumedBSize = NS_INTRINSICSIZE);

  








  nsFlowAreaRect GetFloatAvailableSpace() const
    { return GetFloatAvailableSpace(mBCoord); }
  nsFlowAreaRect GetFloatAvailableSpace(nscoord aBCoord) const
    { return GetFloatAvailableSpaceWithState(aBCoord, nullptr); }
  nsFlowAreaRect
    GetFloatAvailableSpaceWithState(nscoord aBCoord,
                                    nsFloatManager::SavedState *aState) const;
  nsFlowAreaRect
    GetFloatAvailableSpaceForBSize(nscoord aBCoord, nscoord aBSize,
                                   nsFloatManager::SavedState *aState) const;

  






  bool AddFloat(nsLineLayout*       aLineLayout,
                nsIFrame*           aFloat,
                nscoord             aAvailableISize);
private:
  bool CanPlaceFloat(nscoord aFloatISize,
                     const nsFlowAreaRect& aFloatAvailableSpace);
public:
  bool FlowAndPlaceFloat(nsIFrame* aFloat);
private:
  void PushFloatPastBreak(nsIFrame* aFloat);
public:
  void PlaceBelowCurrentLineFloats(nsFloatCacheFreeList& aFloats,
                                   nsLineBox* aLine);

  
  
  
  nscoord ClearFloats(nscoord aBCoord, uint8_t aBreakType,
                      nsIFrame *aReplacedBlock = nullptr,
                      uint32_t aFlags = 0);

  bool IsAdjacentWithTop() const {
    return mBCoord == mBorderPadding.BStart(mReflowState.GetWritingMode());
  }

  


  const mozilla::LogicalMargin& BorderPadding() const {
    return mBorderPadding;
  }

  


  nscoord GetConsumedBSize();

  
  void ReconstructMarginBefore(nsLineList::iterator aLine);

  
  
  void ComputeReplacedBlockOffsetsForFloats(nsIFrame* aFrame,
                          const mozilla::LogicalRect& aFloatAvailableSpace,
                                            nscoord&  aIStartResult,
                                            nscoord&  aIEndResult);

  
  void ComputeBlockAvailSpace(nsIFrame* aFrame,
                              const nsStyleDisplay* aDisplay,
                              const nsFlowAreaRect& aFloatAvailableSpace,
                              bool aBlockAvoidsFloats,
                              mozilla::LogicalRect& aResult);

protected:
  void RecoverFloats(nsLineList::iterator aLine, nscoord aDeltaBCoord);

public:
  void RecoverStateFrom(nsLineList::iterator aLine, nscoord aDeltaBCoord);

  void AdvanceToNextLine() {
    if (GetFlag(BRS_LINE_LAYOUT_EMPTY)) {
      SetFlag(BRS_LINE_LAYOUT_EMPTY, false);
    } else {
      mLineNumber++;
    }
  }

  

  
  

  
  nsBlockFrame* mBlock;

  nsPresContext* mPresContext;

  const nsHTMLReflowState& mReflowState;

  nsFloatManager* mFloatManager;

  
  
  
  
  
  mozilla::WritingMode mFloatManagerWM;
  mozilla::LogicalPoint mFloatManagerOrigin;

  
  nsReflowStatus mReflowStatus;

  
  
  
  
  nsFloatManager::SavedState mFloatManagerStateBefore;

  nscoord mBEndEdge;

  
  
  
  
  
  
  
  
  
  mozilla::LogicalRect mContentArea;
  nscoord ContentIStart() const {
    return mContentArea.IStart(mReflowState.GetWritingMode());
  }
  nscoord ContentISize() const {
    return mContentArea.ISize(mReflowState.GetWritingMode());
  }
  nscoord ContentIEnd() const {
    return mContentArea.IEnd(mReflowState.GetWritingMode());
  }
  nscoord ContentBStart() const {
    return mContentArea.BStart(mReflowState.GetWritingMode());
  }
  nscoord ContentBSize() const {
    return mContentArea.BSize(mReflowState.GetWritingMode());
  }
  nscoord ContentBEnd() const {
    return mContentArea.BEnd(mReflowState.GetWritingMode());
  }
  mozilla::LogicalSize ContentSize(mozilla::WritingMode aWM) const {
    mozilla::WritingMode wm = mReflowState.GetWritingMode();
    return mContentArea.Size(wm).ConvertTo(aWM, wm);
  }

  
  nsSize mContainerSize;
  nscoord ContainerWidth() const { return mContainerSize.width; }
  nscoord ContainerHeight() const { return mContainerSize.height; }

  
  
  
  nsFrameList *mPushedFloats;
  
  
  void SetupPushedFloatList();
  






  void AppendPushedFloatChain(nsIFrame* aFloatCont);

  
  nsOverflowContinuationTracker* mOverflowTracker;

  

  
  
  

  
  
  nsLineList::iterator mCurrentLine;

  
  
  
  
  nsLineList::iterator mLineAdjacentToTop;

  
  nscoord mBCoord;

  
  mozilla::LogicalMargin mBorderPadding;

  
  nsOverflowAreas mFloatOverflowAreas;

  nsFloatCacheFreeList mFloatCacheFreeList;

  
  
  nsIFrame* mPrevChild;

  
  nsCollapsingMargin mPrevBEndMargin;

  
  
  
  
  nsBlockFrame* mNextInFlow;

  

  
  

  
  
  
  nsFloatCacheFreeList mCurrentLineFloats;

  
  
  
  
  nsFloatCacheFreeList mBelowCurrentLineFloats;

  nscoord mMinLineHeight;

  int32_t mLineNumber;

  int16_t mFlags;
 
  uint8_t mFloatBreakType;

  
  nscoord mConsumedBSize;

  void SetFlag(uint32_t aFlag, bool aValue)
  {
    NS_ASSERTION(aFlag<=BRS_LASTFLAG, "bad flag");
    if (aValue) { 
      mFlags |= aFlag;
    }
    else {        
      mFlags &= ~aFlag;
    }
  }

  bool GetFlag(uint32_t aFlag) const
  {
    NS_ASSERTION(aFlag<=BRS_LASTFLAG, "bad flag");
    return !!(mFlags & aFlag);
  }
};

#endif 
