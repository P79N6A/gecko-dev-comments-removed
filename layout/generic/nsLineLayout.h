















#ifndef nsLineLayout_h___
#define nsLineLayout_h___

#include "nsLineBox.h"
#include "nsBlockReflowState.h"
#include "plarena.h"
#include "gfxTypes.h"
#include "WritingModes.h"
#include "JustificationUtils.h"

class nsFloatManager;
struct nsStyleText;

class nsLineLayout {
public:
  



  nsLineLayout(nsPresContext* aPresContext,
               nsFloatManager* aFloatManager,
               const nsHTMLReflowState* aOuterReflowState,
               const nsLineList::iterator* aLine,
               nsLineLayout* aBaseLineLayout);
  ~nsLineLayout();

  void Init(nsBlockReflowState* aState, nscoord aMinLineBSize,
            int32_t aLineNumber) {
    mBlockRS = aState;
    mMinLineBSize = aMinLineBSize;
    mLineNumber = aLineNumber;
  }

  int32_t GetLineNumber() const {
    return mLineNumber;
  }

  void BeginLineReflow(nscoord aICoord, nscoord aBCoord,
                       nscoord aISize, nscoord aBSize,
                       bool aImpactedByFloats,
                       bool aIsTopOfPage,
                       mozilla::WritingMode aWritingMode,
                       nscoord aContainerWidth);

  void EndLineReflow();

  







  void UpdateBand(mozilla::WritingMode aWM,
                  const mozilla::LogicalRect& aNewAvailableSpace,
                  nsIFrame* aFloatFrame);

  void BeginSpan(nsIFrame* aFrame, const nsHTMLReflowState* aSpanReflowState,
                 nscoord aLeftEdge, nscoord aRightEdge, nscoord* aBaseline);

  
  nscoord EndSpan(nsIFrame* aFrame);

  
  
  void AttachLastFrameToBaseLineLayout()
  {
    AttachFrameToBaseLineLayout(LastFrame());
  }

  
  
  void AttachRootFrameToBaseLineLayout()
  {
    AttachFrameToBaseLineLayout(mRootSpan->mFrame);
  }

  int32_t GetCurrentSpanCount() const;

  void SplitLineTo(int32_t aNewCount);

  bool IsZeroBSize();

  
  
  void ReflowFrame(nsIFrame* aFrame,
                   nsReflowStatus& aReflowStatus,
                   nsHTMLReflowMetrics* aMetrics,
                   bool& aPushedFrame);

  void AddBulletFrame(nsIFrame* aFrame, const nsHTMLReflowMetrics& aMetrics);

  void RemoveBulletFrame(nsIFrame* aFrame) {
    PushFrame(aFrame);
  }

  


  void VerticalAlignLine();

  
  
  nscoord GetFinalLineBSize() const
  {
    NS_ASSERTION(mFinalLineBSize != nscoord_MIN,
                 "VerticalAlignLine should have been called before");
    return mFinalLineBSize;
  }

  bool TrimTrailingWhiteSpace();

  


  void TextAlignLine(nsLineBox* aLine, bool aIsLastLine);

  




  void RelativePositionFrames(nsOverflowAreas& aOverflowAreas)
  {
    RelativePositionFrames(mRootSpan, aOverflowAreas);
  }

  

  void SetJustificationInfo(const mozilla::JustificationInfo& aInfo)
  {
    mJustificationInfo = aInfo;
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

  
  
  
  bool AddFloat(nsIFrame* aFloat, nscoord aAvailableISize)
  {
    
    
    
    
    MOZ_ASSERT(mBlockRS,
               "Should not call this method if there is no block reflow state "
               "available");
    return mBlockRS->AddFloat(this, aFloat, aAvailableISize);
  }

  void SetTrimmableISize(nscoord aTrimmableISize) {
    mTrimmableISize = aTrimmableISize;
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

  

























  bool NotifyOptionalBreakPosition(nsIFrame* aFrame, int32_t aOffset,
                                   bool aFits, gfxBreakPriority aPriority) {
    NS_ASSERTION(!aFits || !mNeedBackup,
                  "Shouldn't be updating the break position with a break that fits after we've already flagged an overrun");
    
    
    if ((aFits && aPriority >= mLastOptionalBreakPriority) ||
        !mLastOptionalBreakFrame) {
      mLastOptionalBreakFrame = aFrame;
      mLastOptionalBreakFrameOffset = aOffset;
      mLastOptionalBreakPriority = aPriority;
    }
    return aFrame && mForceBreakFrame == aFrame &&
      mForceBreakFrameOffset == aOffset;
  }
  




  void RestoreSavedBreakPosition(nsIFrame* aFrame, int32_t aOffset,
                                 gfxBreakPriority aPriority) {
    mLastOptionalBreakFrame = aFrame;
    mLastOptionalBreakFrameOffset = aOffset;
    mLastOptionalBreakPriority = aPriority;
  }
  


  void ClearOptionalBreakPosition() {
    mNeedBackup = false;
    mLastOptionalBreakFrame = nullptr;
    mLastOptionalBreakFrameOffset = -1;
    mLastOptionalBreakPriority = gfxBreakPriority::eNoBreak;
  }
  
  
  nsIFrame* GetLastOptionalBreakPosition(int32_t* aOffset,
                                         gfxBreakPriority* aPriority) {
    *aOffset = mLastOptionalBreakFrameOffset;
    *aPriority = mLastOptionalBreakPriority;
    return mLastOptionalBreakFrame;
  }
  
  


  
  bool NeedsBackup() { return mNeedBackup; }
  
  
  
  
  
  

  
  
  
  void ForceBreakAtPosition(nsIFrame* aFrame, int32_t aOffset) {
    mForceBreakFrame = aFrame;
    mForceBreakFrameOffset = aOffset;
  }
  bool HaveForcedBreakPosition() { return mForceBreakFrame != nullptr; }
  int32_t GetForcedBreakPosition(nsIFrame* aFrame) {
    return mForceBreakFrame == aFrame ? mForceBreakFrameOffset : -1;
  }

  




  nsIFrame* LineContainerFrame() const { return mBlockReflowState->frame; }
  const nsHTMLReflowState* LineContainerRS() const { return mBlockReflowState; }
  const nsLineList::iterator* GetLine() const {
    return mGotLineBox ? &mLineBox : nullptr;
  }
  nsLineList::iterator* GetLine() {
    return mGotLineBox ? &mLineBox : nullptr;
  }
  
  









  nscoord GetCurrentFrameInlineDistanceFromBlock();

  



  void AdvanceICoord(nscoord aAmount) { mCurrentSpan->mICoord += aAmount; }
  


  mozilla::WritingMode GetWritingMode() { return mRootSpan->mWritingMode; }
  


  nscoord GetCurrentICoord() { return mCurrentSpan->mICoord; }

protected:
  
  nsFloatManager* mFloatManager;
  const nsStyleText* mStyleText; 
  const nsHTMLReflowState* mBlockReflowState;

  
  
  
  
  
  
  
  nsLineLayout* const mBaseLineLayout;

  nsLineLayout* GetOutermostLineLayout() {
    nsLineLayout* lineLayout = this;
    while (lineLayout->mBaseLineLayout) {
      lineLayout = lineLayout->mBaseLineLayout;
    }
    return lineLayout;
  }

  nsIFrame* mLastOptionalBreakFrame;
  nsIFrame* mForceBreakFrame;
  
  
  friend class nsInlineFrame;

  nsBlockReflowState* mBlockRS;

  
  
  
  
  nsLineList::iterator mLineBox;

  
  
  
  

  struct PerSpanData;
  struct PerFrameData;
  friend struct PerSpanData;
  friend struct PerFrameData;
  struct PerFrameData
  {
    explicit PerFrameData(mozilla::WritingMode aWritingMode)
      : mBounds(aWritingMode)
      , mMargin(aWritingMode)
      , mBorderPadding(aWritingMode)
      , mOffsets(aWritingMode)
    {}

    
    PerFrameData* mNext;
    PerFrameData* mPrev;

    
    
    
    
    
    
    PerFrameData* mNextAnnotation;

    
    PerSpanData* mSpan;

    
    nsIFrame* mFrame;

    
    nscoord mAscent;
    
    
    
    mozilla::LogicalRect mBounds;
    nsOverflowAreas mOverflowAreas;

    
    mozilla::LogicalMargin mMargin;        
    mozilla::LogicalMargin mBorderPadding; 
    mozilla::LogicalMargin mOffsets;       

    
    
    
    
    mozilla::JustificationInfo mJustificationInfo;
    mozilla::JustificationAssignment mJustificationAssignment;
    
    
    bool mRelativePos : 1;
    bool mIsTextFrame : 1;
    bool mIsNonEmptyTextFrame : 1;
    bool mIsNonWhitespaceTextFrame : 1;
    bool mIsLetterFrame : 1;
    bool mRecomputeOverflow : 1;
    bool mIsBullet : 1;
    bool mSkipWhenTrimmingWhitespace : 1;
    bool mIsEmpty : 1;
    bool mIsLinkedToBase : 1;

    
    uint8_t mBlockDirAlign;

    PerFrameData* Last() {
      PerFrameData* pfd = this;
      while (pfd->mNext) {
        pfd = pfd->mNext;
      }
      return pfd;
    }

    bool IsStartJustifiable() const
    {
      return mJustificationInfo.mIsStartJustifiable;
    }

    bool IsEndJustifiable() const
    {
      return mJustificationInfo.mIsEndJustifiable;
    }

    bool ParticipatesInJustification() const;
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
    mozilla::WritingMode mWritingMode;
    bool mZeroEffectiveSpanBox;
    bool mContainsFloat;
    bool mHasNonemptyContent;

    nscoord mIStart;
    nscoord mICoord;
    nscoord mIEnd;

    nscoord mBStartLeading, mBEndLeading;
    nscoord mLogicalBSize;
    nscoord mMinBCoord, mMaxBCoord;
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

  
  
  
  
  nscoord ContainerWidthForSpan(PerSpanData* aPSD) {
    return (aPSD == mRootSpan)
      ? mContainerWidth
      : aPSD->mFrame->mBounds.Width(mRootSpan->mWritingMode);
  }

  gfxBreakPriority mLastOptionalBreakPriority;
  int32_t     mLastOptionalBreakFrameOffset;
  int32_t     mForceBreakFrameOffset;

  nscoord mMinLineBSize;
  
  
  
  nscoord mTextIndent;

  
  
  int32_t mLineNumber;
  mozilla::JustificationInfo mJustificationInfo;

  int32_t mTotalPlacedFrames;

  nscoord mBStartEdge;
  nscoord mMaxStartBoxBSize;
  nscoord mMaxEndBoxBSize;

  nscoord mInflationMinFontSize;

  
  
  nscoord mFinalLineBSize;
  
  
  
  nscoord mTrimmableISize;

  
  nscoord mContainerWidth;

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
  bool mHasRuby                 : 1;

  int32_t mSpanDepth;
#ifdef DEBUG
  int32_t mSpansAllocated, mSpansFreed;
  int32_t mFramesAllocated, mFramesFreed;
#endif
  PLArenaPool mArena; 

  


  PerFrameData* NewPerFrameData(nsIFrame* aFrame);

  


  PerSpanData* NewPerSpanData();

  PerFrameData* LastFrame() const { return mCurrentSpan->mLastFrame; }

  





  void UnlinkFrame(PerFrameData* pfd);

  


  void FreeFrame(PerFrameData* pfd);

  void FreeSpan(PerSpanData* psd);

  bool InBlockContext() const {
    return mSpanDepth == 0;
  }

  void PushFrame(nsIFrame* aFrame);

  void AllowForStartMargin(PerFrameData* pfd,
                           nsHTMLReflowState& aReflowState);

  void SyncAnnotationContainersBounds(PerFrameData* aRubyFrame);

  bool CanPlaceFrame(PerFrameData* pfd,
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
                            nscoord aDistanceFromStart,
                            nscoord aLineBSize);

  void ApplyRelativePositioning(PerFrameData* aPFD);

  void RelativePositionAnnotations(PerSpanData* aRubyPSD,
                                   nsOverflowAreas& aOverflowAreas);

  void RelativePositionFrames(PerSpanData* psd, nsOverflowAreas& aOverflowAreas);

  bool TrimTrailingWhiteSpaceIn(PerSpanData* psd, nscoord* aDeltaISize);

  struct JustificationComputationState;
  int32_t ComputeFrameJustification(PerSpanData* psd,
                                    JustificationComputationState& aState);

  void AdvanceAnnotationInlineBounds(PerFrameData* aPFD,
                                     nscoord aContainerWidth,
                                     nscoord aDeltaICoord,
                                     nscoord aDeltaISize);

  void ApplyLineJustificationToAnnotations(PerFrameData* aPFD,
                                           PerSpanData* aContainingSpan,
                                           nscoord aDeltaICoord,
                                           nscoord aDeltaISize);

  
  
  nscoord ApplyFrameJustification(
      PerSpanData* aPSD, mozilla::JustificationApplicationState& aState);

  void ExpandRubyBox(PerFrameData* aFrame, nscoord aReservedISize,
                     nscoord aContainerWidth);

  void ExpandRubyBoxWithAnnotations(PerFrameData* aFrame,
                                    nscoord aContainerWidth);

  void ExpandInlineRubyBoxes(PerSpanData* aSpan);

  void AttachFrameToBaseLineLayout(PerFrameData* aFrame);

#ifdef DEBUG
  void DumpPerSpanData(PerSpanData* psd, int32_t aIndent);
#endif
};

#endif 
