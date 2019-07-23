
















































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
            PRInt32 aLineNumber) {
    mBlockRS = aState;
    mMinLineHeight = aMinLineHeight;
    mLineNumber = aLineNumber;
  }

  PRInt32 GetLineNumber() const {
    return mLineNumber;
  }

  void BeginLineReflow(nscoord aX, nscoord aY,
                       nscoord aWidth, nscoord aHeight,
                       PRBool aImpactedByFloats,
                       PRBool aIsTopOfPage);

  void EndLineReflow();

  







  void UpdateBand(const nsRect& aNewAvailableSpace,
                  nsIFrame* aFloatFrame);

  nsresult BeginSpan(nsIFrame* aFrame,
                     const nsHTMLReflowState* aSpanReflowState,
                     nscoord aLeftEdge,
                     nscoord aRightEdge);

  
  nscoord EndSpan(nsIFrame* aFrame);

  PRInt32 GetCurrentSpanCount() const;

  void SplitLineTo(PRInt32 aNewCount);

  PRBool IsZeroHeight();

  
  
  nsresult ReflowFrame(nsIFrame* aFrame,
                       nsReflowStatus& aReflowStatus,
                       nsHTMLReflowMetrics* aMetrics,
                       PRBool& aPushedFrame);

  nsresult AddBulletFrame(nsIFrame* aFrame,
                          const nsHTMLReflowMetrics& aMetrics);

  void RemoveBulletFrame(nsIFrame* aFrame) {
    PushFrame(aFrame);
  }

  void VerticalAlignLine();

  PRBool TrimTrailingWhiteSpace();

  void HorizontalAlignFrames(nsRect& aLineBounds, PRBool aAllowJustify);

  




  void RelativePositionFrames(nsRect& aCombinedArea);

  

  
protected:
#define LL_FIRSTLETTERSTYLEOK          0x00000008
#define LL_ISTOPOFPAGE                 0x00000010
#define LL_IMPACTEDBYFLOATS            0x00000040
#define LL_LASTFLOATWASLETTERFRAME     0x00000080
#define LL_LINEISEMPTY                 0x00000100
#define LL_LINEENDSINBR                0x00000200
#define LL_NEEDBACKUP                  0x00000400
#define LL_INFIRSTLINE                 0x00000800
#define LL_GOTLINEBOX                  0x00001000
#define LL_INFIRSTLETTER               0x00002000
#define LL_HASBULLET                   0x00004000
#define LL_LASTFLAG                    LL_HASBULLET

  void SetFlag(PRUint32 aFlag, PRBool aValue)
  {
    NS_ASSERTION(aFlag<=LL_LASTFLAG, "bad flag");
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
    NS_ASSERTION(aFlag<=LL_LASTFLAG, "bad flag");
    return !!(mFlags & aFlag);
  }

public:

  

  void SetTextJustificationWeights(PRInt32 aNumSpaces, PRInt32 aNumLetters) {
    mTextJustificationNumSpaces = aNumSpaces;
    mTextJustificationNumLetters = aNumLetters;
  }

  



  PRBool LineIsEmpty() const
  {
    return GetFlag(LL_LINEISEMPTY);
  }

  PRBool LineIsBreakable() const;

  PRBool GetLineEndsInBR() const 
  { 
    return GetFlag(LL_LINEENDSINBR); 
  }

  void SetLineEndsInBR(PRBool aOn) 
  { 
    SetFlag(LL_LINEENDSINBR, aOn); 
  }

  
  
  
  PRBool AddFloat(nsPlaceholderFrame* aFrame,
                  nscoord aAvailableWidth,
                  nsReflowStatus& aReflowStatus) {
    return mBlockRS->AddFloat(*this, aFrame, aAvailableWidth, aReflowStatus);
  }

  void SetTrimmableWidth(nscoord aTrimmableWidth) {
    mTrimmableWidth = aTrimmableWidth;
  }

  

  PRBool GetFirstLetterStyleOK() const {
    return GetFlag(LL_FIRSTLETTERSTYLEOK);
  }

  void SetFirstLetterStyleOK(PRBool aSetting) {
    SetFlag(LL_FIRSTLETTERSTYLEOK, aSetting);
  }

  PRBool GetInFirstLetter() const {
    return GetFlag(LL_INFIRSTLETTER);
  }

  void SetInFirstLetter(PRBool aSetting) {
    SetFlag(LL_INFIRSTLETTER, aSetting);
  }

  PRBool GetInFirstLine() const {
    return GetFlag(LL_INFIRSTLINE);
  }

  void SetInFirstLine(PRBool aSetting) {
    SetFlag(LL_INFIRSTLINE, aSetting);
  }

  

  nsPresContext* mPresContext;

  























  PRBool NotifyOptionalBreakPosition(nsIContent* aContent, PRInt32 aOffset,
                                     PRBool aFits, gfxBreakPriority aPriority) {
    NS_ASSERTION(!aFits || !GetFlag(LL_NEEDBACKUP),
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
  




  void RestoreSavedBreakPosition(nsIContent* aContent, PRInt32 aOffset,
                                 gfxBreakPriority aPriority) {
    mLastOptionalBreakContent = aContent;
    mLastOptionalBreakContentOffset = aOffset;
    mLastOptionalBreakPriority = aPriority;
  }
  


  void ClearOptionalBreakPosition() {
    SetFlag(LL_NEEDBACKUP, PR_FALSE);
    mLastOptionalBreakContent = nsnull;
    mLastOptionalBreakContentOffset = -1;
    mLastOptionalBreakPriority = eNoBreak;
  }
  
  
  nsIContent* GetLastOptionalBreakPosition(PRInt32* aOffset,
                                           gfxBreakPriority* aPriority) {
    *aOffset = mLastOptionalBreakContentOffset;
    *aPriority = mLastOptionalBreakPriority;
    return mLastOptionalBreakContent;
  }
  
  


  
  PRBool NeedsBackup() { return GetFlag(LL_NEEDBACKUP); }
  
  
  
  
  
  

  
  
  
  void ForceBreakAtPosition(nsIContent* aContent, PRInt32 aOffset) {
    mForceBreakContent = aContent;
    mForceBreakContentOffset = aOffset;
  }
  PRBool HaveForcedBreakPosition() { return mForceBreakContent != nsnull; }
  PRInt32 GetForcedBreakPosition(nsIContent* aContent) {
    return mForceBreakContent == aContent ? mForceBreakContentOffset : -1;
  }

  




  nsIFrame* GetLineContainerFrame() const { return mBlockReflowState->frame; }
  const nsLineList::iterator* GetLine() const {
    return GetFlag(LL_GOTLINEBOX) ? &mLineBox : nsnull;
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
    nsRect mCombinedArea;

    
    nsMargin mMargin;
    nsMargin mBorderPadding;
    nsMargin mOffsets;

    
    PRInt32 mJustificationNumSpaces;
    PRInt32 mJustificationNumLetters;
    
    
    PRUint8 mVerticalAlign;


#define PFD_RELATIVEPOS                 0x00000001
#define PFD_ISTEXTFRAME                 0x00000002
#define PFD_ISNONEMPTYTEXTFRAME         0x00000004
#define PFD_ISNONWHITESPACETEXTFRAME    0x00000008
#define PFD_ISLETTERFRAME               0x00000010
#define PFD_RECOMPUTEOVERFLOW           0x00000020
#define PFD_ISBULLET                    0x00000040
#define PFD_SKIPWHENTRIMMINGWHITESPACE  0x00000080
#define PFD_LASTFLAG                    PFD_SKIPWHENTRIMMINGWHITESPACE

    PRUint8 mFlags;

    void SetFlag(PRUint32 aFlag, PRBool aValue)
    {
      NS_ASSERTION(aFlag<=PFD_LASTFLAG, "bad flag");
      NS_ASSERTION(aFlag<=PR_UINT8_MAX, "bad flag");
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
    PRPackedBool mNoWrap;
    PRUint8 mDirection;
    PRPackedBool mChangedFrameDirection;
    PRPackedBool mZeroEffectiveSpanBox;
    PRPackedBool mContainsFloat;
    PRPackedBool mHasNonemptyContent;

    nscoord mLeftEdge;
    nscoord mX;
    nscoord mRightEdge;

    nscoord mTopLeading, mBottomLeading;
    nscoord mLogicalHeight;
    nscoord mMinY, mMaxY;

    void AppendFrame(PerFrameData* pfd) {
      if (nsnull == mLastFrame) {
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
  PRInt32     mLastOptionalBreakContentOffset;
  PRInt32     mForceBreakContentOffset;

  nscoord mMinLineHeight;
  
  
  
  nscoord mTextIndent;

  
  
  PRInt32 mLineNumber;
  PRInt32 mTextJustificationNumSpaces;
  PRInt32 mTextJustificationNumLetters;

  PRInt32 mTotalPlacedFrames;

  nscoord mTopEdge;
  nscoord mMaxTopBoxHeight;
  nscoord mMaxBottomBoxHeight;

  
  
  nscoord mFinalLineHeight;
  
  
  nscoord mTrimmableWidth;

  PRInt32 mSpanDepth;
#ifdef DEBUG
  PRInt32 mSpansAllocated, mSpansFreed;
  PRInt32 mFramesAllocated, mFramesFreed;
#endif
  PLArenaPool mArena; 

  PRUint16 mFlags;

  PRUint8 mTextAlign;

  nsresult NewPerFrameData(PerFrameData** aResult);

  nsresult NewPerSpanData(PerSpanData** aResult);

  void FreeSpan(PerSpanData* psd);

  PRBool InBlockContext() const {
    return mSpanDepth == 0;
  }

  void PushFrame(nsIFrame* aFrame);

  void ApplyStartMargin(PerFrameData* pfd,
                        nsHTMLReflowState& aReflowState);

  PRBool CanPlaceFrame(PerFrameData* pfd,
                       const nsHTMLReflowState& aReflowState,
                       PRBool aNotSafeToBreak,
                       PRBool aFrameCanContinueTextRun,
                       PRBool aCanRollBackBeforeFrame,
                       nsHTMLReflowMetrics& aMetrics,
                       nsReflowStatus& aStatus,
                       PRBool* aOptionalBreakAfterFits);

  void PlaceFrame(PerFrameData* pfd,
                  nsHTMLReflowMetrics& aMetrics);

  void VerticalAlignFrames(PerSpanData* psd);

  void PlaceTopBottomFrames(PerSpanData* psd,
                            nscoord aDistanceFromTop,
                            nscoord aLineHeight);

  void RelativePositionFrames(PerSpanData* psd, nsRect& aCombinedArea);

  PRBool TrimTrailingWhiteSpaceIn(PerSpanData* psd, nscoord* aDeltaWidth);

  void ComputeJustificationWeights(PerSpanData* psd, PRInt32* numSpaces, PRInt32* numLetters);

  struct FrameJustificationState {
    PRInt32 mTotalNumSpaces;
    PRInt32 mTotalNumLetters;
    nscoord mTotalWidthForSpaces;
    nscoord mTotalWidthForLetters;
    PRInt32 mNumSpacesProcessed;
    PRInt32 mNumLettersProcessed;
    nscoord mWidthForSpacesProcessed;
    nscoord mWidthForLettersProcessed;
  };

  
  
  nscoord ApplyFrameJustification(PerSpanData* aPSD,
                                  FrameJustificationState* aState);


#ifdef DEBUG
  void DumpPerSpanData(PerSpanData* psd, PRInt32 aIndent);
#endif
};

#endif 
