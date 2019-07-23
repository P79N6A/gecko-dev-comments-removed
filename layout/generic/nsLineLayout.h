
















































#ifndef nsLineLayout_h___
#define nsLineLayout_h___

#include "nsFrame.h"
#include "nsDeque.h"
#include "nsLineBox.h"
#include "nsBlockReflowState.h"
#include "plarena.h"

class nsBlockFrame;

class nsSpaceManager;
class nsPlaceholderFrame;
struct nsStyleText;

class nsLineLayout {
public:
  nsLineLayout(nsPresContext* aPresContext,
               nsSpaceManager* aSpaceManager,
               const nsHTMLReflowState* aOuterReflowState,
               const nsLineList::iterator* aLine);
  ~nsLineLayout();

  void Init(nsBlockReflowState* aState, nscoord aMinLineHeight,
            PRInt32 aLineNumber) {
    mBlockRS = aState;
    mMinLineHeight = aMinLineHeight;
    mLineNumber = aLineNumber;
  }

  PRInt32 GetColumn() {
    return mColumn;
  }

  void SetColumn(PRInt32 aNewColumn) {
    mColumn = aNewColumn;
  }
  
  PRInt32 GetLineNumber() const {
    return mLineNumber;
  }

  void BeginLineReflow(nscoord aX, nscoord aY,
                       nscoord aWidth, nscoord aHeight,
                       PRBool aImpactedByFloats,
                       PRBool aIsTopOfPage);

  void EndLineReflow();

  void UpdateBand(nscoord aX, nscoord aY, nscoord aWidth, nscoord aHeight,
                  PRBool aPlacedLeftFloat,
                  nsIFrame* aFloatFrame);

  nsresult BeginSpan(nsIFrame* aFrame,
                     const nsHTMLReflowState* aSpanReflowState,
                     nscoord aLeftEdge,
                     nscoord aRightEdge);

  void EndSpan(nsIFrame* aFrame, nsSize& aSizeResult);

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
#define LL_ENDSINWHITESPACE            0x00000001
#define LL_UNDERSTANDSNWHITESPACE      0x00000002
#define LL_INWORD                      0x00000004
#define LL_FIRSTLETTERSTYLEOK          0x00000008
#define LL_ISTOPOFPAGE                 0x00000010
#define LL_UPDATEDBAND                 0x00000020
#define LL_IMPACTEDBYFLOATS            0x00000040
#define LL_LASTFLOATWASLETTERFRAME     0x00000080
#define LL_CANPLACEFLOAT               0x00000100
#define LL_LINEENDSINBR                0x00000200





#define LL_LINEENDSINSOFTBR            0x00000400
#define LL_NEEDBACKUP                  0x00000800
#define LL_LASTTEXTFRAME_WRAPPINGENABLED 0x00001000
#define LL_INFIRSTLINE                 0x00002000
#define LL_GOTLINEBOX                  0x00004000
#define LL_LASTFLAG                    LL_GOTLINEBOX

  PRUint16 mFlags;

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
    PRBool result = (mFlags & aFlag);
    if (result) return PR_TRUE;
    return PR_FALSE;
  }

public:

  
  

  void SetEndsInWhiteSpace(PRBool aState) {
    SetFlag(LL_ENDSINWHITESPACE, aState);
  }

  PRBool GetEndsInWhiteSpace() const {
    return GetFlag(LL_ENDSINWHITESPACE);
  }

  void SetUnderstandsWhiteSpace(PRBool aSetting) {
    SetFlag(LL_UNDERSTANDSNWHITESPACE, aSetting);
  }

  void SetTextJustificationWeights(PRInt32 aNumSpaces, PRInt32 aNumLetters) {
    mTextJustificationNumSpaces = aNumSpaces;
    mTextJustificationNumLetters = aNumLetters;
  }

  PRBool CanPlaceFloatNow() const;

  PRBool LineIsBreakable() const;

  PRBool GetLineEndsInBR() const 
  { 
    return GetFlag(LL_LINEENDSINBR); 
  }

  void SetLineEndsInBR(PRBool aOn) 
  { 
    SetFlag(LL_LINEENDSINBR, aOn); 
  }

  PRBool GetLineEndsInSoftBR() const 
  { 
    return GetFlag(LL_LINEENDSINSOFTBR); 
  }

  void SetLineEndsInSoftBR(PRBool aOn) 
  { 
    SetFlag(LL_LINEENDSINSOFTBR, aOn); 
  }

  PRBool InStrictMode() const
  {
    return mCompatMode != eCompatibility_NavQuirks;
  }

  nsCompatibility GetCompatMode() const
  {
    return mCompatMode;
  }

  
  
  
  PRBool InitFloat(nsPlaceholderFrame* aFrame, nsReflowStatus& aReflowStatus) {
    return mBlockRS->InitFloat(*this, aFrame, aReflowStatus);
  }

  PRBool AddFloat(nsPlaceholderFrame* aFrame, nsReflowStatus& aReflowStatus) {
    return mBlockRS->AddFloat(*this, aFrame, PR_FALSE, aReflowStatus);
  }

  





  PRBool InWord() {
    return GetFlag(LL_INWORD);
  }
  void SetInWord(PRBool aInWord) {
    SetFlag(LL_INWORD, aInWord);
  }
  
  








  nsIFrame* GetTrailingTextFrame(PRBool* aWrappingEnabled) const {
    *aWrappingEnabled = GetFlag(LL_LASTTEXTFRAME_WRAPPINGENABLED);
    return mTrailingTextFrame;
  }
  void SetTrailingTextFrame(nsIFrame* aFrame, PRBool aWrappingEnabled)
  { 
    mTrailingTextFrame = aFrame;
    SetFlag(LL_LASTTEXTFRAME_WRAPPINGENABLED, aWrappingEnabled);
  }

  

  PRBool GetFirstLetterStyleOK() const {
    return GetFlag(LL_FIRSTLETTERSTYLEOK);
  }

  void SetFirstLetterStyleOK(PRBool aSetting) {
    SetFlag(LL_FIRSTLETTERSTYLEOK, aSetting);
  }

  void SetFirstLetterFrame(nsIFrame* aFrame) {
    mFirstLetterFrame = aFrame;
  }

  PRBool GetInFirstLine() const {
    return GetFlag(LL_INFIRSTLINE);
  }

  void SetInFirstLine(PRBool aSetting) {
    SetFlag(LL_INFIRSTLINE, aSetting);
  }

  

  static PRBool TreatFrameAsBlock(nsIFrame* aFrame);

  

  nsPresContext* mPresContext;

  



















  PRBool NotifyOptionalBreakPosition(nsIContent* aContent, PRInt32 aOffset,
                                     PRBool aFits) {
    NS_ASSERTION(!GetFlag(LL_NEEDBACKUP),
                  "Shouldn't be updating the break position after we've already flagged an overrun");
    
    
    if (aFits || !mLastOptionalBreakContent) {
      mLastOptionalBreakContent = aContent;
      mLastOptionalBreakContentOffset = aOffset;
    }
    return aContent && mForceBreakContent == aContent &&
      mForceBreakContentOffset == aOffset;
  }
  




  void RestoreSavedBreakPosition(nsIContent* aContent, PRInt32 aOffset) {
    mLastOptionalBreakContent = aContent;
    mLastOptionalBreakContentOffset = aOffset;
  }
  


  void ClearOptionalBreakPosition() {
    SetFlag(LL_NEEDBACKUP, PR_FALSE);
    mLastOptionalBreakContent = nsnull;
    mLastOptionalBreakContentOffset = -1;
  }
  
  
  nsIContent* GetLastOptionalBreakPosition(PRInt32* aOffset) {
    *aOffset = mLastOptionalBreakContentOffset;
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
  
  nsSpaceManager* mSpaceManager;
  const nsStyleText* mStyleText; 
  const nsHTMLReflowState* mBlockReflowState;

  nsIContent* mLastOptionalBreakContent;
  nsIContent* mForceBreakContent;
  PRInt32     mLastOptionalBreakContentOffset;
  PRInt32     mForceBreakContentOffset;
  
  nsIFrame* mTrailingTextFrame;

  
  friend class nsInlineFrame;

  nsBlockReflowState* mBlockRS;
  nsCompatibility mCompatMode;
  nscoord mMinLineHeight;
  PRUint8 mTextAlign;

  PRUint8 mPlacedFloats;
  
  
  
  nscoord mTextIndent;

  
  
  nsIFrame* mFirstLetterFrame;
  PRInt32 mLineNumber;
  PRInt32 mColumn;
  PRInt32 mTextJustificationNumSpaces;
  PRInt32 mTextJustificationNumLetters;

  nsLineList::iterator mLineBox;

  PRInt32 mTotalPlacedFrames;

  nscoord mTopEdge;
  nscoord mMaxTopBoxHeight;
  nscoord mMaxBottomBoxHeight;

  
  
  nscoord mFinalLineHeight;

  
  
  
  

  struct PerSpanData;
  struct PerFrameData;
  friend struct PerSpanData;
  friend struct PerFrameData;
  struct PerFrameData {
    
    PerFrameData* mNext;
    PerFrameData* mPrev;

    
    PerSpanData* mSpan;

    
    nsIFrame* mFrame;
    nsCSSFrameType mFrameType;

    
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
      PRBool result = (mFlags & aFlag);
      if (result) return PR_TRUE;
      return PR_FALSE;
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
  PRInt32 mSpanDepth;
#ifdef DEBUG
  PRInt32 mSpansAllocated, mSpansFreed;
  PRInt32 mFramesAllocated, mFramesFreed;
#endif
  PLArenaPool mArena; 

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
                       nsReflowStatus& aStatus);

  void PlaceFrame(PerFrameData* pfd,
                  nsHTMLReflowMetrics& aMetrics);

  void UpdateFrames();

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
