















#ifndef nsLineLayout_h___
#define nsLineLayout_h___

#include "nsFrame.h"
#include "nsDeque.h"
#include "nsLineBox.h"
#include "nsBlockReflowState.h"
#include "plarena.h"
#include "gfxTypes.h"

class nsBlockFrame;

class nsFloatManager;
class nsPlaceholderFrame;
struct nsStyleText;

class nsLineLayout {
public:
  nsLineLayout(nsPresContext* aPresContext,
               nsFloatManager* aFloatManager,
               const nsHTMLReflowState* aOuterReflowState,
               const nsLineList::iterator* aLine);
  ~nsLineLayout();

  void Init(nsBlockReflowState* aState, nscoord aMinLineHeight,
            int32_t aLineNumber) {
    mBlockRS = aState;
    mMinLineHeight = aMinLineHeight;
    mLineNumber = aLineNumber;
  }

  int32_t GetLineNumber() const {
    return mLineNumber;
  }

  void BeginLineReflow(nscoord aX, nscoord aY,
                       nscoord aWidth, nscoord aHeight,
                       bool aImpactedByFloats,
                       bool aIsTopOfPage,
                       uint8_t aDirection);

  void EndLineReflow();

  







  void UpdateBand(const nsRect& aNewAvailableSpace,
                  nsIFrame* aFloatFrame);

  void BeginSpan(nsIFrame* aFrame, const nsHTMLReflowState* aSpanReflowState,
                 nscoord aLeftEdge, nscoord aRightEdge, nscoord* aBaseline);

  
  nscoord EndSpan(nsIFrame* aFrame);

  int32_t GetCurrentSpanCount() const;

  void SplitLineTo(int32_t aNewCount);

  bool IsZeroHeight();

  
  
  nsresult ReflowFrame(nsIFrame* aFrame,
                       nsReflowStatus& aReflowStatus,
                       nsHTMLReflowMetrics* aMetrics,
                       bool& aPushedFrame);

  void AddBulletFrame(nsIFrame* aFrame, const nsHTMLReflowMetrics& aMetrics);

  void RemoveBulletFrame(nsIFrame* aFrame) {
    PushFrame(aFrame);
  }

  void VerticalAlignLine();

  bool TrimTrailingWhiteSpace();

  void HorizontalAlignFrames(nsRect& aLineBounds, bool aIsLastLine);

  




  void RelativePositionFrames(nsOverflowAreas& aOverflowAreas);

  

  void SetTextJustificationWeights(int32_t aNumSpaces, int32_t aNumLetters) {
    mTextJustificationNumSpaces = aNumSpaces;
    mTextJustificationNumLetters = aNumLetters;
  }

  



  bool LineIsEmpty() const
  {
    return mLineIsEmpty;
  }

  




  bool LineAtStart() const
  {
    return mLineAtStart;
  }

  bool LineIsBreakable() const;

  bool GetLineEndsInBR() const 
  { 
    return mLineEndsInBR;
  }

  void SetLineEndsInBR(bool aOn) 
  { 
    mLineEndsInBR = aOn;
  }

  
  
  
  bool AddFloat(nsIFrame* aFloat, nscoord aAvailableWidth)
  {
    return mBlockRS->AddFloat(this, aFloat, aAvailableWidth);
  }

  void SetTrimmableWidth(nscoord aTrimmableWidth) {
    mTrimmableWidth = aTrimmableWidth;
  }

  

  bool GetFirstLetterStyleOK() const {
    return mFirstLetterStyleOK;
  }

  void SetFirstLetterStyleOK(bool aSetting) {
    mFirstLetterStyleOK = aSetting;
  }

  bool GetInFirstLetter() const {
    return mInFirstLetter;
  }

  void SetInFirstLetter(bool aSetting) {
    mInFirstLetter = aSetting;
  }

  bool GetInFirstLine() const {
    return mInFirstLine;
  }

  void SetInFirstLine(bool aSetting) {
    mInFirstLine = aSetting;
  }

  
  
  void SetDirtyNextLine() {
    mDirtyNextLine = true;
  }
  bool GetDirtyNextLine() {
    return mDirtyNextLine;
  }

  

  nsPresContext* mPresContext;

  























  bool NotifyOptionalBreakPosition(nsIContent* aContent, int32_t aOffset,
                                     bool aFits, gfxBreakPriority aPriority) {
    NS_ASSERTION(!aFits || !mNeedBackup,
                  "Shouldn't be updating the break position with a break that fits after we've already flagged an overrun");
    
    
    if ((aFits && aPriority >= mLastOptionalBreakPriority) ||
        !mLastOptionalBreakContent) {
      mLastOptionalBreakContent = aContent;
      mLastOptionalBreakContentOffset = aOffset;
      mLastOptionalBreakPriority = aPriority;
    }
    return aContent && mForceBreakContent == aContent &&
      mForceBreakContentOffset == aOffset;
  }
  




  void RestoreSavedBreakPosition(nsIContent* aContent, int32_t aOffset,
                                 gfxBreakPriority aPriority) {
    mLastOptionalBreakContent = aContent;
    mLastOptionalBreakContentOffset = aOffset;
    mLastOptionalBreakPriority = aPriority;
  }
  


  void ClearOptionalBreakPosition() {
    mNeedBackup = false;
    mLastOptionalBreakContent = nullptr;
    mLastOptionalBreakContentOffset = -1;
    mLastOptionalBreakPriority = eNoBreak;
  }
  
  
  nsIContent* GetLastOptionalBreakPosition(int32_t* aOffset,
                                           gfxBreakPriority* aPriority) {
    *aOffset = mLastOptionalBreakContentOffset;
    *aPriority = mLastOptionalBreakPriority;
    return mLastOptionalBreakContent;
  }
  
  


  
  bool NeedsBackup() { return mNeedBackup; }
  
  
  
  
  
  

  
  
  
  void ForceBreakAtPosition(nsIContent* aContent, int32_t aOffset) {
    mForceBreakContent = aContent;
    mForceBreakContentOffset = aOffset;
  }
  bool HaveForcedBreakPosition() { return mForceBreakContent != nullptr; }
  int32_t GetForcedBreakPosition(nsIContent* aContent) {
    return mForceBreakContent == aContent ? mForceBreakContentOffset : -1;
  }

  




  nsIFrame* GetLineContainerFrame() const { return mBlockReflowState->frame; }
  const nsHTMLReflowState* GetLineContainerRS() const {
    return mBlockReflowState;
  }
  const nsLineList::iterator* GetLine() const {
    return mGotLineBox ? &mLineBox : nullptr;
  }
  nsLineList::iterator* GetLine() {
    return mGotLineBox ? &mLineBox : nullptr;
  }
  
  









  nscoord GetCurrentFrameXDistanceFromBlock();

protected:
  
  nsFloatManager* mFloatManager;
  const nsStyleText* mStyleText; 
  const nsHTMLReflowState* mBlockReflowState;

  nsIContent* mLastOptionalBreakContent;
  nsIContent* mForceBreakContent;
  
  
  friend class nsInlineFrame;

  nsBlockReflowState* mBlockRS;

  nsLineList::iterator mLineBox;

  
  
  
  

  struct PerSpanData;
  struct PerFrameData;
  friend struct PerSpanData;
  friend struct PerFrameData;
  struct PerFrameData {
    
    PerFrameData* mNext;
    PerFrameData* mPrev;

    
    PerSpanData* mSpan;

    
    nsIFrame* mFrame;

    
    nscoord mAscent;
    nsRect mBounds;
    nsOverflowAreas mOverflowAreas;

    
    nsMargin mMargin;
    nsMargin mBorderPadding;
    nsMargin mOffsets;

    
    int32_t mJustificationNumSpaces;
    int32_t mJustificationNumLetters;
    
    
    uint8_t mVerticalAlign;


#define PFD_RELATIVEPOS                 0x00000001
#define PFD_ISTEXTFRAME                 0x00000002
#define PFD_ISNONEMPTYTEXTFRAME         0x00000004
#define PFD_ISNONWHITESPACETEXTFRAME    0x00000008
#define PFD_ISLETTERFRAME               0x00000010
#define PFD_RECOMPUTEOVERFLOW           0x00000020
#define PFD_ISBULLET                    0x00000040
#define PFD_SKIPWHENTRIMMINGWHITESPACE  0x00000080
#define PFD_LASTFLAG                    PFD_SKIPWHENTRIMMINGWHITESPACE

    uint8_t mFlags;

    void SetFlag(uint32_t aFlag, bool aValue)
    {
      NS_ASSERTION(aFlag<=PFD_LASTFLAG, "bad flag");
      NS_ASSERTION(aFlag<=UINT8_MAX, "bad flag");
      if (aValue) { 
        mFlags |= aFlag;
      }
      else {        
        mFlags &= ~aFlag;
      }
    }

    bool GetFlag(uint32_t aFlag) const
    {
      NS_ASSERTION(aFlag<=PFD_LASTFLAG, "bad flag");
      return !!(mFlags & aFlag);
    }


    PerFrameData* Last() {
      PerFrameData* pfd = this;
      while (pfd->mNext) {
        pfd = pfd->mNext;
      }
      return pfd;
    }
  };
  PerFrameData* mFrameFreeList;

  struct PerSpanData {
    union {
      PerSpanData* mParent;
      PerSpanData* mNextFreeSpan;
    };
    PerFrameData* mFrame;
    PerFrameData* mFirstFrame;
    PerFrameData* mLastFrame;

    const nsHTMLReflowState* mReflowState;
    bool mNoWrap;
    uint8_t mDirection;
    bool mChangedFrameDirection;
    bool mZeroEffectiveSpanBox;
    bool mContainsFloat;
    bool mHasNonemptyContent;

    nscoord mLeftEdge;
    nscoord mX;
    nscoord mRightEdge;

    nscoord mTopLeading, mBottomLeading;
    nscoord mLogicalHeight;
    nscoord mMinY, mMaxY;
    nscoord* mBaseline;

    void AppendFrame(PerFrameData* pfd) {
      if (nullptr == mLastFrame) {
        mFirstFrame = pfd;
      }
      else {
        mLastFrame->mNext = pfd;
        pfd->mPrev = mLastFrame;
      }
      mLastFrame = pfd;
    }
  };
  PerSpanData* mSpanFreeList;
  PerSpanData* mRootSpan;
  PerSpanData* mCurrentSpan;

  gfxBreakPriority mLastOptionalBreakPriority;
  int32_t     mLastOptionalBreakContentOffset;
  int32_t     mForceBreakContentOffset;

  nscoord mMinLineHeight;
  
  
  
  nscoord mTextIndent;

  
  
  int32_t mLineNumber;
  int32_t mTextJustificationNumSpaces;
  int32_t mTextJustificationNumLetters;

  int32_t mTotalPlacedFrames;

  nscoord mTopEdge;
  nscoord mMaxTopBoxHeight;
  nscoord mMaxBottomBoxHeight;

  nscoord mInflationMinFontSize;

  
  
  nscoord mFinalLineHeight;
  
  
  nscoord mTrimmableWidth;

  bool mFirstLetterStyleOK      : 1;
  bool mIsTopOfPage             : 1;
  bool mImpactedByFloats        : 1;
  bool mLastFloatWasLetterFrame : 1;
  bool mLineIsEmpty             : 1;
  bool mLineEndsInBR            : 1;
  bool mNeedBackup              : 1;
  bool mInFirstLine             : 1;
  bool mGotLineBox              : 1;
  bool mInFirstLetter           : 1;
  bool mHasBullet               : 1;
  bool mDirtyNextLine           : 1;
  bool mLineAtStart             : 1;

  int32_t mSpanDepth;
#ifdef DEBUG
  int32_t mSpansAllocated, mSpansFreed;
  int32_t mFramesAllocated, mFramesFreed;
#endif
  PLArenaPool mArena; 

  


  PerFrameData* NewPerFrameData();

  


  PerSpanData* NewPerSpanData();

  void FreeSpan(PerSpanData* psd);

  bool InBlockContext() const {
    return mSpanDepth == 0;
  }

  void PushFrame(nsIFrame* aFrame);

  void ApplyStartMargin(PerFrameData* pfd,
                        nsHTMLReflowState& aReflowState);

  bool CanPlaceFrame(PerFrameData* pfd,
                       uint8_t aFrameDirection,
                       bool aNotSafeToBreak,
                       bool aFrameCanContinueTextRun,
                       bool aCanRollBackBeforeFrame,
                       nsHTMLReflowMetrics& aMetrics,
                       nsReflowStatus& aStatus,
                       bool* aOptionalBreakAfterFits);

  void PlaceFrame(PerFrameData* pfd,
                  nsHTMLReflowMetrics& aMetrics);

  void VerticalAlignFrames(PerSpanData* psd);

  void PlaceTopBottomFrames(PerSpanData* psd,
                            nscoord aDistanceFromTop,
                            nscoord aLineHeight);

  void RelativePositionFrames(PerSpanData* psd, nsOverflowAreas& aOverflowAreas);

  bool TrimTrailingWhiteSpaceIn(PerSpanData* psd, nscoord* aDeltaWidth);

  void ComputeJustificationWeights(PerSpanData* psd, int32_t* numSpaces, int32_t* numLetters);

  struct FrameJustificationState {
    int32_t mTotalNumSpaces;
    int32_t mTotalNumLetters;
    nscoord mTotalWidthForSpaces;
    nscoord mTotalWidthForLetters;
    int32_t mNumSpacesProcessed;
    int32_t mNumLettersProcessed;
    nscoord mWidthForSpacesProcessed;
    nscoord mWidthForLettersProcessed;
  };

  
  
  nscoord ApplyFrameJustification(PerSpanData* aPSD,
                                  FrameJustificationState* aState);


#ifdef DEBUG
  void DumpPerSpanData(PerSpanData* psd, int32_t aIndent);
#endif
};

#endif 
