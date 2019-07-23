




















































#include "nsCOMPtr.h"
#include "nsHTMLParts.h"
#include "nsCRT.h"
#include "nsSplittableFrame.h"
#include "nsLineLayout.h"
#include "nsString.h"
#include "nsUnicharUtils.h"
#include "nsPresContext.h"
#include "nsIContent.h"
#include "nsStyleConsts.h"
#include "nsStyleContext.h"
#include "nsCoord.h"
#include "nsIFontMetrics.h"
#include "nsIRenderingContext.h"
#include "nsIPresShell.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsIDOMText.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsCSSPseudoElements.h"
#include "nsCompatibility.h"
#include "nsCSSColorUtils.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsFrame.h"
#include "nsPlaceholderFrame.h"
#include "nsTextFrameUtils.h"
#include "nsTextRunTransformations.h"
#include "nsFrameManager.h"
#include "nsTextFrameTextRunCache.h"
#include "nsExpirationTracker.h"
#include "nsTextFrame.h"
#include "nsICaseConversion.h"
#include "nsIUGenCategory.h"
#include "nsUnicharUtilCIID.h"

#include "nsTextFragment.h"
#include "nsGkAtoms.h"
#include "nsFrameSelection.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsILookAndFeel.h"
#include "nsCSSRendering.h"
#include "nsContentUtils.h"
#include "nsLineBreaker.h"
#include "nsIWordBreaker.h"
#include "nsGenericDOMDataNode.h"

#include "nsILineIterator.h"

#include "nsIServiceManager.h"
#ifdef ACCESSIBILITY
#include "nsIAccessible.h"
#include "nsIAccessibilityService.h"
#endif
#include "nsAutoPtr.h"
#include "nsStyleSet.h"

#include "nsBidiFrames.h"
#include "nsBidiPresUtils.h"
#include "nsBidiUtils.h"

#include "nsIThebesFontMetrics.h"
#include "gfxFont.h"
#include "gfxContext.h"
#include "gfxTextRunWordCache.h"
#include "gfxImageSurface.h"

#ifdef NS_DEBUG
#undef NOISY_BLINK
#undef NOISY_REFLOW
#undef NOISY_TRIM
#else
#undef NOISY_BLINK
#undef NOISY_REFLOW
#undef NOISY_TRIM
#endif





#define TEXT_FIRST_LETTER    0x00100000


#define TEXT_START_OF_LINE   0x00200000


#define TEXT_END_OF_LINE     0x00400000

#define TEXT_HYPHEN_BREAK    0x00800000


#define TEXT_TRIMMED_TRAILING_WHITESPACE 0x01000000



#define TEXT_JUSTIFICATION_ENABLED       0x02000000

#define TEXT_SELECTION_UNDERLINE_OVERFLOWED 0x04000000

#define TEXT_REFLOW_FLAGS    \
  (TEXT_FIRST_LETTER|TEXT_START_OF_LINE|TEXT_END_OF_LINE|TEXT_HYPHEN_BREAK| \
   TEXT_TRIMMED_TRAILING_WHITESPACE|TEXT_JUSTIFICATION_ENABLED| \
   TEXT_HAS_NONCOLLAPSED_CHARACTERS|TEXT_SELECTION_UNDERLINE_OVERFLOWED)



#define TEXT_IS_ONLY_WHITESPACE    0x08000000

#define TEXT_ISNOT_ONLY_WHITESPACE 0x10000000

#define TEXT_WHITESPACE_FLAGS      0x18000000





#define TEXT_IN_TEXTRUN_USER_DATA  0x40000000










































struct TextRunMappedFlow {
  nsTextFrame* mStartFrame;
  PRInt32      mDOMOffsetToBeforeTransformOffset;
  
  PRUint32     mContentLength;
};








struct TextRunUserData {
  TextRunMappedFlow* mMappedFlows;
  PRInt32            mMappedFlowCount;

  PRUint32           mLastFlowIndex;
};






class nsTextPaintStyle {
public:
  nsTextPaintStyle(nsTextFrame* aFrame);

  nscolor GetTextColor();
  



  PRBool GetSelectionColors(nscolor* aForeColor,
                            nscolor* aBackColor);
  void GetHighlightColors(nscolor* aForeColor,
                          nscolor* aBackColor);
  void GetIMESelectionColors(PRInt32  aIndex,
                             nscolor* aForeColor,
                             nscolor* aBackColor);
  
  PRBool GetSelectionUnderlineForPaint(PRInt32  aIndex,
                                       nscolor* aLineColor,
                                       float*   aRelativeSize,
                                       PRUint8* aStyle);

  
  static PRBool GetSelectionUnderline(nsPresContext* aPresContext,
                                      PRInt32 aIndex,
                                      nscolor* aLineColor,
                                      float* aRelativeSize,
                                      PRUint8* aStyle);

  nsPresContext* PresContext() { return mPresContext; }

  enum {
    eIndexRawInput = 0,
    eIndexSelRawText,
    eIndexConvText,
    eIndexSelConvText,
    eIndexSpellChecker
  };

  static PRInt32 GetUnderlineStyleIndexForSelectionType(PRInt32 aSelectionType)
  {
    switch (aSelectionType) {
      case nsISelectionController::SELECTION_IME_RAWINPUT:
        return eIndexRawInput;
      case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
        return eIndexSelRawText;
      case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
        return eIndexConvText;
      case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
        return eIndexSelConvText;
      case nsISelectionController::SELECTION_SPELLCHECK:
        return eIndexSpellChecker;
      default:
        NS_WARNING("non-IME selection type");
        return eIndexRawInput;
    }
  }

protected:
  nsTextFrame*   mFrame;
  nsPresContext* mPresContext;
  PRPackedBool   mInitCommonColors;
  PRPackedBool   mInitSelectionColors;

  

  PRInt16      mSelectionStatus; 
  nscolor      mSelectionTextColor;
  nscolor      mSelectionBGColor;

  

  PRInt32 mSufficientContrast;
  nscolor mFrameBackgroundColor;

  
  
  
  struct nsSelectionStyle {
    PRBool mInit;
    nscolor mTextColor;
    nscolor mBGColor;
    nscolor mUnderlineColor;
    PRUint8 mUnderlineStyle;
    float   mUnderlineRelativeSize;
  };
  nsSelectionStyle mSelectionStyle[5];

  
  void InitCommonColors();
  PRBool InitSelectionColors();

  nsSelectionStyle* GetSelectionStyle(PRInt32 aIndex);
  void InitSelectionStyle(PRInt32 aIndex);

  PRBool EnsureSufficientContrast(nscolor *aForeColor, nscolor *aBackColor);

  nscolor GetResolvedForeColor(nscolor aColor, nscolor aDefaultForeColor,
                               nscolor aBackColor);
};

static void
DestroyUserData(void* aUserData)
{
  TextRunUserData* userData = static_cast<TextRunUserData*>(aUserData);
  if (userData) {
    nsMemory::Free(userData);
  }
}



static void
ClearAllTextRunReferences(nsTextFrame* aFrame, gfxTextRun* aTextRun)
{
  aFrame->RemoveStateBits(TEXT_IN_TEXTRUN_USER_DATA);
  while (aFrame) {
    NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
                 "Bad frame");
    if (aFrame->GetTextRun() != aTextRun)
      break;
    aFrame->SetTextRun(nsnull);
    aFrame = static_cast<nsTextFrame*>(aFrame->GetNextContinuation());
  }
}


static void
UnhookTextRunFromFrames(gfxTextRun* aTextRun)
{
  if (!aTextRun->GetUserData())
    return;

  
  
  if (aTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW) {
    nsIFrame* firstInFlow = static_cast<nsIFrame*>(aTextRun->GetUserData());
    ClearAllTextRunReferences(static_cast<nsTextFrame*>(firstInFlow), aTextRun);
  } else {
    TextRunUserData* userData =
      static_cast<TextRunUserData*>(aTextRun->GetUserData());
    PRInt32 i;
    for (i = 0; i < userData->mMappedFlowCount; ++i) {
      ClearAllTextRunReferences(userData->mMappedFlows[i].mStartFrame, aTextRun);
    }
    DestroyUserData(userData);
  }
  aTextRun->SetUserData(nsnull);  
}

class FrameTextRunCache;

static FrameTextRunCache *gTextRuns = nsnull;




class FrameTextRunCache : public nsExpirationTracker<gfxTextRun,3> {
public:
  enum { TIMEOUT_SECONDS = 10 };
  FrameTextRunCache()
      : nsExpirationTracker<gfxTextRun,3>(TIMEOUT_SECONDS*1000) {}
  ~FrameTextRunCache() {
    AgeAllGenerations();
  }

  void RemoveFromCache(gfxTextRun* aTextRun) {
    if (aTextRun->GetExpirationState()->IsTracked()) {
      RemoveObject(aTextRun);
    }
    if (aTextRun->GetFlags() & gfxTextRunWordCache::TEXT_IN_CACHE) {
      gfxTextRunWordCache::RemoveTextRun(aTextRun);
    }
  }

  
  virtual void NotifyExpired(gfxTextRun* aTextRun) {
    UnhookTextRunFromFrames(aTextRun);
    RemoveFromCache(aTextRun);
    delete aTextRun;
  }
};

static gfxTextRun *
MakeTextRun(const PRUnichar *aText, PRUint32 aLength,
            gfxFontGroup *aFontGroup, const gfxFontGroup::Parameters* aParams,
            PRUint32 aFlags)
{
    nsAutoPtr<gfxTextRun> textRun;
    if (aLength == 0) {
        textRun = aFontGroup->MakeEmptyTextRun(aParams, aFlags);
    } else if (aLength == 1 && aText[0] == ' ') {
        textRun = aFontGroup->MakeSpaceTextRun(aParams, aFlags);
    } else {
        textRun = gfxTextRunWordCache::MakeTextRun(aText, aLength, aFontGroup,
            aParams, aFlags);
    }
    if (!textRun)
        return nsnull;
    nsresult rv = gTextRuns->AddObject(textRun);
    if (NS_FAILED(rv)) {
        gTextRuns->RemoveFromCache(textRun);
        return nsnull;
    }
    return textRun.forget();
}

static gfxTextRun *
MakeTextRun(const PRUint8 *aText, PRUint32 aLength,
            gfxFontGroup *aFontGroup, const gfxFontGroup::Parameters* aParams,
            PRUint32 aFlags)
{
    nsAutoPtr<gfxTextRun> textRun;
    if (aLength == 0) {
        textRun = aFontGroup->MakeEmptyTextRun(aParams, aFlags);
    } else if (aLength == 1 && aText[0] == ' ') {
        textRun = aFontGroup->MakeSpaceTextRun(aParams, aFlags);
    } else {
        textRun = gfxTextRunWordCache::MakeTextRun(aText, aLength, aFontGroup,
            aParams, aFlags);
    }
    if (!textRun)
        return nsnull;
    nsresult rv = gTextRuns->AddObject(textRun);
    if (NS_FAILED(rv)) {
        gTextRuns->RemoveFromCache(textRun);
        return nsnull;
    }
    return textRun.forget();
}

nsresult
nsTextFrameTextRunCache::Init() {
    gTextRuns = new FrameTextRunCache();
    return gTextRuns ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

void
nsTextFrameTextRunCache::Shutdown() {
    delete gTextRuns;
    gTextRuns = nsnull;
}

PRInt32 nsTextFrame::GetContentEnd() const {
  nsTextFrame* next = static_cast<nsTextFrame*>(GetNextContinuation());
  return next ? next->GetContentOffset() : GetFragment()->GetLength();
}

PRInt32 nsTextFrame::GetInFlowContentLength() {
#ifdef IBMBIDI
  nsTextFrame* nextBidi = nsnull;
  PRInt32      start = -1, end;

  if (mState & NS_FRAME_IS_BIDI) {
    nextBidi = static_cast<nsTextFrame*>(GetLastInFlow()->GetNextContinuation());
    if (nextBidi) {
      nextBidi->GetOffsets(start, end);
      return start - mContentOffset;
    }
  }
#endif 
  return GetFragment()->GetLength() - mContentOffset;
}






static PRBool IsSpaceCombiningSequenceTail(const nsTextFragment* aFrag, PRUint32 aPos)
{
  NS_ASSERTION(aPos <= aFrag->GetLength(), "Bad offset");
  if (!aFrag->Is2b())
    return PR_FALSE;
  return nsTextFrameUtils::IsSpaceCombiningSequenceTail(
    aFrag->Get2b() + aPos, aFrag->GetLength() - aPos);
}


static PRBool IsCSSWordSpacingSpace(const nsTextFragment* aFrag,
                                    PRUint32 aPos, const nsStyleText* aStyleText)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");

  PRUnichar ch = aFrag->CharAt(aPos);
  switch (ch) {
  case ' ':
  case CH_NBSP:
    return !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  case '\t': return !aStyleText->WhiteSpaceIsSignificant();
  case '\n': return !aStyleText->NewlineIsSignificant();
  default: return PR_FALSE;
  }
}



static PRBool IsTrimmableSpace(const PRUnichar* aChars, PRUint32 aLength)
{
  NS_ASSERTION(aLength > 0, "No text for IsSpace!");

  PRUnichar ch = *aChars;
  if (ch == ' ')
    return !nsTextFrameUtils::IsSpaceCombiningSequenceTail(aChars + 1, aLength - 1);
  return ch == '\t' || ch == '\f' || ch == '\n';
}



static PRBool IsTrimmableSpace(char aCh)
{
  return aCh == ' ' || aCh == '\t' || aCh == '\f' || aCh == '\n';
}

static PRBool IsTrimmableSpace(const nsTextFragment* aFrag, PRUint32 aPos,
                               const nsStyleText* aStyleText)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");

  switch (aFrag->CharAt(aPos)) {
  case ' ': return !aStyleText->WhiteSpaceIsSignificant() &&
                   !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  case '\n': return !aStyleText->NewlineIsSignificant();
  case '\t':
  case '\f': return !aStyleText->WhiteSpaceIsSignificant();
  default: return PR_FALSE;
  }
}

static PRBool IsSelectionSpace(const nsTextFragment* aFrag, PRUint32 aPos)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");
  PRUnichar ch = aFrag->CharAt(aPos);
  if (ch == ' ' || ch == CH_NBSP)
    return !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  return ch == '\t' || ch == '\n' || ch == '\f';
}






static PRUint32
GetTrimmableWhitespaceCount(const nsTextFragment* aFrag,
                            PRInt32 aStartOffset, PRInt32 aLength,
                            PRInt32 aDirection)
{
  PRInt32 count = 0;
  if (aFrag->Is2b()) {
    const PRUnichar* str = aFrag->Get2b() + aStartOffset;
    PRInt32 fragLen = aFrag->GetLength() - aStartOffset;
    for (; count < aLength; ++count) {
      if (!IsTrimmableSpace(str, fragLen))
        break;
      str += aDirection;
      fragLen -= aDirection;
    }
  } else {
    const char* str = aFrag->Get1b() + aStartOffset;
    for (; count < aLength; ++count) {
      if (!IsTrimmableSpace(*str))
        break;
      str += aDirection;
    }
  }
  return count;
}

static PRBool
IsAllWhitespace(const nsTextFragment* aFrag, PRBool aAllowNewline)
{
  if (aFrag->Is2b())
    return PR_FALSE;
  PRInt32 len = aFrag->GetLength();
  const char* str = aFrag->Get1b();
  for (PRInt32 i = 0; i < len; ++i) {
    char ch = str[i];
    if (ch == ' ' || ch == '\t' || (ch == '\n' && aAllowNewline))
      continue;
    return PR_FALSE;
  }
  return PR_TRUE;
}










class BuildTextRunsScanner {
public:
  BuildTextRunsScanner(nsPresContext* aPresContext, gfxContext* aContext,
      nsIFrame* aLineContainer) :
    mCurrentFramesAllSameTextRun(nsnull),
    mContext(aContext),
    mLineContainer(aLineContainer),
    mBidiEnabled(aPresContext->BidiEnabled()),
    mSkipIncompleteTextRuns(PR_FALSE),
    mNextRunContextInfo(nsTextFrameUtils::INCOMING_NONE),
    mCurrentRunContextInfo(nsTextFrameUtils::INCOMING_NONE) {
    ResetRunInfo();
  }
  ~BuildTextRunsScanner() {
    NS_ASSERTION(mBreakSinks.IsEmpty(), "Should have been cleared");
    NS_ASSERTION(mTextRunsToDelete.IsEmpty(), "Should have been cleared");
    NS_ASSERTION(mLineBreakBeforeFrames.IsEmpty(), "Should have been cleared");
    NS_ASSERTION(mMappedFlows.IsEmpty(), "Should have been cleared");
  }

  void SetAtStartOfLine() {
    mStartOfLine = PR_TRUE;
    mCanStopOnThisLine = PR_FALSE;
  }
  void SetSkipIncompleteTextRuns(PRBool aSkip) {
    mSkipIncompleteTextRuns = aSkip;
  }
  void SetCommonAncestorWithLastFrame(nsIFrame* aFrame) {
    mCommonAncestorWithLastFrame = aFrame;
  }
  PRBool CanStopOnThisLine() {
    return mCanStopOnThisLine;
  }
  nsIFrame* GetCommonAncestorWithLastFrame() {
    return mCommonAncestorWithLastFrame;
  }
  void LiftCommonAncestorWithLastFrameToParent(nsIFrame* aFrame) {
    if (mCommonAncestorWithLastFrame &&
        mCommonAncestorWithLastFrame->GetParent() == aFrame) {
      mCommonAncestorWithLastFrame = aFrame;
    }
  }
  void ScanFrame(nsIFrame* aFrame);
  PRBool IsTextRunValidForMappedFlows(gfxTextRun* aTextRun);
  void FlushFrames(PRBool aFlushLineBreaks, PRBool aSuppressTrailingBreak);
  void FlushLineBreaks(gfxTextRun* aTrailingTextRun);
  void ResetRunInfo() {
    mLastFrame = nsnull;
    mMappedFlows.Clear();
    mLineBreakBeforeFrames.Clear();
    mMaxTextLength = 0;
    mDoubleByteText = PR_FALSE;
  }
  void AccumulateRunInfo(nsTextFrame* aFrame);
  




  gfxTextRun* BuildTextRunForFrames(void* aTextBuffer);
  void AssignTextRun(gfxTextRun* aTextRun);
  nsTextFrame* GetNextBreakBeforeFrame(PRUint32* aIndex);
  void SetupBreakSinksForTextRun(gfxTextRun* aTextRun, PRBool aIsExistingTextRun,
                                 PRBool aSuppressSink);
  struct FindBoundaryState {
    nsIFrame*    mStopAtFrame;
    nsTextFrame* mFirstTextFrame;
    nsTextFrame* mLastTextFrame;
    PRPackedBool mSeenTextRunBoundaryOnLaterLine;
    PRPackedBool mSeenTextRunBoundaryOnThisLine;
    PRPackedBool mSeenSpaceForLineBreakingOnThisLine;
  };
  enum FindBoundaryResult {
    FB_CONTINUE,
    FB_STOPPED_AT_STOP_FRAME,
    FB_FOUND_VALID_TEXTRUN_BOUNDARY
  };
  FindBoundaryResult FindBoundaries(nsIFrame* aFrame, FindBoundaryState* aState);

  PRBool ContinueTextRunAcrossFrames(nsTextFrame* aFrame1, nsTextFrame* aFrame2);

  
  
  
  struct MappedFlow {
    nsTextFrame* mStartFrame;
    nsTextFrame* mEndFrame;
    
    
    
    
    
    nsIFrame*    mAncestorControllingInitialBreak;
    
    PRInt32 GetContentEnd() {
      return mEndFrame ? mEndFrame->GetContentOffset()
          : mStartFrame->GetFragment()->GetLength();
    }
  };

  class BreakSink : public nsILineBreakSink {
  public:
    BreakSink(gfxTextRun* aTextRun, gfxContext* aContext, PRUint32 aOffsetIntoTextRun,
              PRBool aExistingTextRun) :
                mTextRun(aTextRun), mContext(aContext),
                mOffsetIntoTextRun(aOffsetIntoTextRun),
                mChangedBreaks(PR_FALSE), mExistingTextRun(aExistingTextRun) {}

    virtual void SetBreaks(PRUint32 aOffset, PRUint32 aLength,
                           PRPackedBool* aBreakBefore) {
      if (mTextRun->SetPotentialLineBreaks(aOffset + mOffsetIntoTextRun, aLength,
                                           aBreakBefore, mContext)) {
        mChangedBreaks = PR_TRUE;
        
        mTextRun->ClearFlagBits(nsTextFrameUtils::TEXT_NO_BREAKS);
      }
    }
    
    virtual void SetCapitalization(PRUint32 aOffset, PRUint32 aLength,
                                   PRPackedBool* aCapitalize) {
      NS_ASSERTION(mTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_TRANSFORMED,
                   "Text run should be transformed!");
      nsTransformedTextRun* transformedTextRun =
        static_cast<nsTransformedTextRun*>(mTextRun);
      transformedTextRun->SetCapitalization(aOffset + mOffsetIntoTextRun, aLength,
                                            aCapitalize, mContext);
    }

    void Finish() {
      NS_ASSERTION(!(mTextRun->GetFlags() &
                     (gfxTextRunWordCache::TEXT_UNUSED_FLAGS |
                      nsTextFrameUtils::TEXT_UNUSED_FLAG)),
                   "Flag set that should never be set! (memory safety error?)");
      if (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_TRANSFORMED) {
        nsTransformedTextRun* transformedTextRun =
          static_cast<nsTransformedTextRun*>(mTextRun);
        transformedTextRun->FinishSettingProperties(mContext);
      }
    }

    gfxTextRun*  mTextRun;
    gfxContext*  mContext;
    PRUint32     mOffsetIntoTextRun;
    PRPackedBool mChangedBreaks;
    PRPackedBool mExistingTextRun;
  };

private:
  nsAutoTArray<MappedFlow,10>   mMappedFlows;
  nsAutoTArray<nsTextFrame*,50> mLineBreakBeforeFrames;
  nsAutoTArray<nsAutoPtr<BreakSink>,10> mBreakSinks;
  nsAutoTArray<gfxTextRun*,5>   mTextRunsToDelete;
  nsLineBreaker                 mLineBreaker;
  gfxTextRun*                   mCurrentFramesAllSameTextRun;
  gfxContext*                   mContext;
  nsIFrame*                     mLineContainer;
  nsTextFrame*                  mLastFrame;
  
  
  nsIFrame*                     mCommonAncestorWithLastFrame;
  
  PRUint32                      mMaxTextLength;
  PRPackedBool                  mDoubleByteText;
  PRPackedBool                  mBidiEnabled;
  PRPackedBool                  mStartOfLine;
  PRPackedBool                  mSkipIncompleteTextRuns;
  PRPackedBool                  mCanStopOnThisLine;
  PRUint8                       mNextRunContextInfo;
  PRUint8                       mCurrentRunContextInfo;
};

static nsIFrame*
FindLineContainer(nsIFrame* aFrame)
{
  while (aFrame && aFrame->CanContinueTextRun()) {
    aFrame = aFrame->GetParent();
  }
  return aFrame;
}

static PRBool
IsLineBreakingWhiteSpace(PRUnichar aChar)
{
  
  
  
  
  
  
  return nsLineBreaker::IsSpace(aChar) || aChar == 0x0A;
}

static PRBool
TextContainsLineBreakerWhiteSpace(const void* aText, PRUint32 aLength,
                                  PRBool aIsDoubleByte)
{
  PRUint32 i;
  if (aIsDoubleByte) {
    const PRUnichar* chars = static_cast<const PRUnichar*>(aText);
    for (i = 0; i < aLength; ++i) {
      if (IsLineBreakingWhiteSpace(chars[i]))
        return PR_TRUE;
    }
    return PR_FALSE;
  } else {
    const PRUint8* chars = static_cast<const PRUint8*>(aText);
    for (i = 0; i < aLength; ++i) {
      if (IsLineBreakingWhiteSpace(chars[i]))
        return PR_TRUE;
    }
    return PR_FALSE;
  }
}

struct FrameTextTraversal {
  
  
  nsIFrame*    mFrameToScan;
  
  nsIFrame*    mOverflowFrameToScan;
  
  PRPackedBool mScanSiblings;

  
  
  PRPackedBool mLineBreakerCanCrossFrameBoundary;
  PRPackedBool mTextRunCanCrossFrameBoundary;

  nsIFrame* NextFrameToScan() {
    nsIFrame* f;
    if (mFrameToScan) {
      f = mFrameToScan;
      mFrameToScan = mScanSiblings ? f->GetNextSibling() : nsnull;
    } else if (mOverflowFrameToScan) {
      f = mOverflowFrameToScan;
      mOverflowFrameToScan = mScanSiblings ? f->GetNextSibling() : nsnull;
    } else {
      f = nsnull;
    }
    return f;
  }
};

static FrameTextTraversal
CanTextCrossFrameBoundary(nsIFrame* aFrame, nsIAtom* aType)
{
  NS_ASSERTION(aType == aFrame->GetType(), "Wrong type");

  FrameTextTraversal result;

  PRBool continuesTextRun = aFrame->CanContinueTextRun();
  if (aType == nsGkAtoms::placeholderFrame) {
    
    
    result.mLineBreakerCanCrossFrameBoundary = PR_TRUE;
    result.mOverflowFrameToScan = nsnull;
    if (continuesTextRun) {
      
      
      
      
      
      result.mFrameToScan =
        (static_cast<nsPlaceholderFrame*>(aFrame))->GetOutOfFlowFrame();
      result.mScanSiblings = PR_FALSE;
      result.mTextRunCanCrossFrameBoundary = PR_FALSE;
    } else {
      result.mFrameToScan = nsnull;
      result.mTextRunCanCrossFrameBoundary = PR_TRUE;
    }
  } else {
    if (continuesTextRun) {
      result.mFrameToScan = aFrame->GetFirstChild(nsnull);
      result.mOverflowFrameToScan = aFrame->GetFirstChild(nsGkAtoms::overflowList);
      NS_WARN_IF_FALSE(!result.mOverflowFrameToScan,
                       "Scanning overflow inline frames is something we should avoid");
      result.mScanSiblings = PR_TRUE;
      result.mTextRunCanCrossFrameBoundary = PR_TRUE;
      result.mLineBreakerCanCrossFrameBoundary = PR_TRUE;
    } else {
      result.mFrameToScan = nsnull;
      result.mOverflowFrameToScan = nsnull;
      result.mTextRunCanCrossFrameBoundary = PR_FALSE;
      result.mLineBreakerCanCrossFrameBoundary = PR_FALSE;
    }
  }    
  return result;
}

BuildTextRunsScanner::FindBoundaryResult
BuildTextRunsScanner::FindBoundaries(nsIFrame* aFrame, FindBoundaryState* aState)
{
  nsIAtom* frameType = aFrame->GetType();
  nsTextFrame* textFrame = frameType == nsGkAtoms::textFrame
    ? static_cast<nsTextFrame*>(aFrame) : nsnull;
  if (textFrame) {
    if (aState->mLastTextFrame &&
        textFrame != aState->mLastTextFrame->GetNextInFlow() &&
        !ContinueTextRunAcrossFrames(aState->mLastTextFrame, textFrame)) {
      aState->mSeenTextRunBoundaryOnThisLine = PR_TRUE;
      if (aState->mSeenSpaceForLineBreakingOnThisLine)
        return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
    }
    if (!aState->mFirstTextFrame) {
      aState->mFirstTextFrame = textFrame;
    }
    aState->mLastTextFrame = textFrame;
  }
  
  if (aFrame == aState->mStopAtFrame)
    return FB_STOPPED_AT_STOP_FRAME;

  if (textFrame) {
    if (!aState->mSeenSpaceForLineBreakingOnThisLine) {
      const nsTextFragment* frag = textFrame->GetFragment();
      PRUint32 start = textFrame->GetContentOffset();
      const void* text = frag->Is2b()
          ? static_cast<const void*>(frag->Get2b() + start)
          : static_cast<const void*>(frag->Get1b() + start);
      if (TextContainsLineBreakerWhiteSpace(text, textFrame->GetContentLength(),
                                            frag->Is2b())) {
        aState->mSeenSpaceForLineBreakingOnThisLine = PR_TRUE;
        if (aState->mSeenTextRunBoundaryOnLaterLine)
          return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
      }
    }
    return FB_CONTINUE; 
  }

  FrameTextTraversal traversal =
    CanTextCrossFrameBoundary(aFrame, frameType);
  if (!traversal.mTextRunCanCrossFrameBoundary) {
    aState->mSeenTextRunBoundaryOnThisLine = PR_TRUE;
    if (aState->mSeenSpaceForLineBreakingOnThisLine)
      return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
  }
  
  for (nsIFrame* f = traversal.NextFrameToScan(); f;
       f = traversal.NextFrameToScan()) {
    FindBoundaryResult result = FindBoundaries(f, aState);
    if (result != FB_CONTINUE)
      return result;
  }

  if (!traversal.mTextRunCanCrossFrameBoundary) {
    aState->mSeenTextRunBoundaryOnThisLine = PR_TRUE;
    if (aState->mSeenSpaceForLineBreakingOnThisLine)
      return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
  }

  return FB_CONTINUE;
}



#define NUM_LINES_TO_BUILD_TEXT_RUNS 200











static void
BuildTextRuns(gfxContext* aContext, nsTextFrame* aForFrame,
              nsIFrame* aLineContainer,
              const nsLineList::iterator* aForFrameLine)
{
  NS_ASSERTION(aForFrame || aLineContainer,
               "One of aForFrame or aLineContainer must be set!");
  NS_ASSERTION(!aForFrameLine || aLineContainer,
               "line but no line container");
  
  if (!aLineContainer) {
    aLineContainer = FindLineContainer(aForFrame);
  } else {
    NS_ASSERTION(!aForFrame ||
                 (aLineContainer == FindLineContainer(aForFrame) ||
                  (aLineContainer->GetType() == nsGkAtoms::letterFrame &&
                   aLineContainer->GetStyleDisplay()->IsFloating())),
                 "Wrong line container hint");
  }

  nsPresContext* presContext = aLineContainer->PresContext();
  BuildTextRunsScanner scanner(presContext, aContext, aLineContainer);

  nsBlockFrame* block = nsLayoutUtils::GetAsBlock(aLineContainer);

  if (!block) {
    NS_ASSERTION(!aLineContainer->GetPrevInFlow() && !aLineContainer->GetNextInFlow(),
                 "Breakable non-block line containers not supported");
    
    
    scanner.SetAtStartOfLine();
    scanner.SetCommonAncestorWithLastFrame(nsnull);
    nsIFrame* child = aLineContainer->GetFirstChild(nsnull);
    while (child) {
      scanner.ScanFrame(child);
      child = child->GetNextSibling();
    }
    
    scanner.SetAtStartOfLine();
    scanner.FlushFrames(PR_TRUE, PR_FALSE);
    return;
  }

  

  PRBool isValid = PR_TRUE;
  nsBlockInFlowLineIterator backIterator(block, &isValid);
  if (aForFrameLine) {
    backIterator = nsBlockInFlowLineIterator(block, *aForFrameLine, PR_FALSE);
  } else {
    backIterator = nsBlockInFlowLineIterator(block, aForFrame, &isValid);
    NS_ASSERTION(isValid, "aForFrame not found in block, someone lied to us");
    NS_ASSERTION(backIterator.GetContainer() == block,
                 "Someone lied to us about the block");
  }
  nsBlockFrame::line_iterator startLine = backIterator.GetLine();

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsBlockInFlowLineIterator forwardIterator = backIterator;
  nsTextFrame* stopAtFrame = aForFrame;
  nsTextFrame* nextLineFirstTextFrame = nsnull;
  PRBool seenTextRunBoundaryOnLaterLine = PR_FALSE;
  PRBool mayBeginInTextRun = PR_TRUE;
  while (PR_TRUE) {
    forwardIterator = backIterator;
    nsBlockFrame::line_iterator line = backIterator.GetLine();
    if (!backIterator.Prev() || backIterator.GetLine()->IsBlock()) {
      mayBeginInTextRun = PR_FALSE;
      break;
    }

    BuildTextRunsScanner::FindBoundaryState state = { stopAtFrame, nsnull, nsnull,
      seenTextRunBoundaryOnLaterLine, PR_FALSE, PR_FALSE };
    nsIFrame* child = line->mFirstChild;
    PRBool foundBoundary = PR_FALSE;
    PRInt32 i;
    for (i = line->GetChildCount() - 1; i >= 0; --i) {
      BuildTextRunsScanner::FindBoundaryResult result =
          scanner.FindBoundaries(child, &state);
      if (result == BuildTextRunsScanner::FB_FOUND_VALID_TEXTRUN_BOUNDARY) {
        foundBoundary = PR_TRUE;
        break;
      } else if (result == BuildTextRunsScanner::FB_STOPPED_AT_STOP_FRAME) {
        break;
      }
      child = child->GetNextSibling();
    }
    if (foundBoundary)
      break;
    if (!stopAtFrame && state.mLastTextFrame && nextLineFirstTextFrame &&
        !scanner.ContinueTextRunAcrossFrames(state.mLastTextFrame, nextLineFirstTextFrame)) {
      
      if (state.mSeenSpaceForLineBreakingOnThisLine)
        break;
      seenTextRunBoundaryOnLaterLine = PR_TRUE;
    } else if (state.mSeenTextRunBoundaryOnThisLine) {
      seenTextRunBoundaryOnLaterLine = PR_TRUE;
    }
    stopAtFrame = nsnull;
    if (state.mFirstTextFrame) {
      nextLineFirstTextFrame = state.mFirstTextFrame;
    }
  }
  scanner.SetSkipIncompleteTextRuns(mayBeginInTextRun);

  
  
  
  
  PRBool seenStartLine = PR_FALSE;
  PRUint32 linesAfterStartLine = 0;
  do {
    nsBlockFrame::line_iterator line = forwardIterator.GetLine();
    if (line->IsBlock())
      break;
    line->SetInvalidateTextRuns(PR_FALSE);
    scanner.SetAtStartOfLine();
    scanner.SetCommonAncestorWithLastFrame(nsnull);
    nsIFrame* child = line->mFirstChild;
    PRInt32 i;
    for (i = line->GetChildCount() - 1; i >= 0; --i) {
      scanner.ScanFrame(child);
      child = child->GetNextSibling();
    }
    if (line.get() == startLine.get()) {
      seenStartLine = PR_TRUE;
    }
    if (seenStartLine) {
      ++linesAfterStartLine;
      if (linesAfterStartLine >= NUM_LINES_TO_BUILD_TEXT_RUNS && scanner.CanStopOnThisLine()) {
        
        
        
        
        
        scanner.FlushLineBreaks(nsnull);
        
        
        scanner.ResetRunInfo();
        return;
      }
    }
  } while (forwardIterator.Next());

  
  scanner.SetAtStartOfLine();
  scanner.FlushFrames(PR_TRUE, PR_FALSE);
}

static PRUnichar*
ExpandBuffer(PRUnichar* aDest, PRUint8* aSrc, PRUint32 aCount)
{
  while (aCount) {
    *aDest = *aSrc;
    ++aDest;
    ++aSrc;
    --aCount;
  }
  return aDest;
}

PRBool BuildTextRunsScanner::IsTextRunValidForMappedFlows(gfxTextRun* aTextRun)
{
  if (aTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW)
    return mMappedFlows.Length() == 1 &&
      mMappedFlows[0].mStartFrame == static_cast<nsTextFrame*>(aTextRun->GetUserData()) &&
      mMappedFlows[0].mEndFrame == nsnull;

  TextRunUserData* userData = static_cast<TextRunUserData*>(aTextRun->GetUserData());
  if (userData->mMappedFlowCount != PRInt32(mMappedFlows.Length()))
    return PR_FALSE;
  PRUint32 i;
  for (i = 0; i < mMappedFlows.Length(); ++i) {
    if (userData->mMappedFlows[i].mStartFrame != mMappedFlows[i].mStartFrame ||
        PRInt32(userData->mMappedFlows[i].mContentLength) !=
            mMappedFlows[i].GetContentEnd() - mMappedFlows[i].mStartFrame->GetContentOffset())
      return PR_FALSE;
  }
  return PR_TRUE;
}





void BuildTextRunsScanner::FlushFrames(PRBool aFlushLineBreaks, PRBool aSuppressTrailingBreak)
{
  if (mMappedFlows.Length() == 0)
    return;

  gfxTextRun* textRun = nsnull;
  if (!mMappedFlows.IsEmpty()) {
    if (!mSkipIncompleteTextRuns && mCurrentFramesAllSameTextRun &&
        ((mCurrentFramesAllSameTextRun->GetFlags() & nsTextFrameUtils::TEXT_INCOMING_WHITESPACE) != 0) ==
        ((mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_WHITESPACE) != 0) &&
        ((mCurrentFramesAllSameTextRun->GetFlags() & gfxTextRunWordCache::TEXT_INCOMING_ARABICCHAR) != 0) ==
        ((mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_ARABICCHAR) != 0) &&
        IsTextRunValidForMappedFlows(mCurrentFramesAllSameTextRun)) {
      
      textRun = mCurrentFramesAllSameTextRun;

      
      
      SetupBreakSinksForTextRun(textRun, PR_TRUE, PR_FALSE);
      mNextRunContextInfo = nsTextFrameUtils::INCOMING_NONE;
      if (textRun->GetFlags() & nsTextFrameUtils::TEXT_TRAILING_WHITESPACE) {
        mNextRunContextInfo |= nsTextFrameUtils::INCOMING_WHITESPACE;
      }
      if (textRun->GetFlags() & gfxTextRunWordCache::TEXT_TRAILING_ARABICCHAR) {
        mNextRunContextInfo |= nsTextFrameUtils::INCOMING_ARABICCHAR;
      }
    } else {
      nsAutoTArray<PRUint8,BIG_TEXT_NODE_SIZE> buffer;
      if (!buffer.AppendElements(mMaxTextLength*(mDoubleByteText ? 2 : 1)))
        return;
      textRun = BuildTextRunForFrames(buffer.Elements());
    }
  }

  if (aFlushLineBreaks) {
    FlushLineBreaks(aSuppressTrailingBreak ? nsnull : textRun);
  }

  mCanStopOnThisLine = PR_TRUE;
  ResetRunInfo();
}

void BuildTextRunsScanner::FlushLineBreaks(gfxTextRun* aTrailingTextRun)
{
  PRBool trailingLineBreak;
  nsresult rv = mLineBreaker.Reset(&trailingLineBreak);
  
  
  
  if (NS_SUCCEEDED(rv) && trailingLineBreak && aTrailingTextRun) {
    aTrailingTextRun->SetFlagBits(nsTextFrameUtils::TEXT_HAS_TRAILING_BREAK);
  }

  PRUint32 i;
  for (i = 0; i < mBreakSinks.Length(); ++i) {
    if (!mBreakSinks[i]->mExistingTextRun || mBreakSinks[i]->mChangedBreaks) {
      
      
    }
    mBreakSinks[i]->Finish();
  }
  mBreakSinks.Clear();

  for (i = 0; i < mTextRunsToDelete.Length(); ++i) {
    gfxTextRun* deleteTextRun = mTextRunsToDelete[i];
    gTextRuns->RemoveFromCache(deleteTextRun);
    delete deleteTextRun;
  }
  mTextRunsToDelete.Clear();
}

void BuildTextRunsScanner::AccumulateRunInfo(nsTextFrame* aFrame)
{
  NS_ASSERTION(mMaxTextLength <= mMaxTextLength + aFrame->GetContentLength(), "integer overflow");
  mMaxTextLength += aFrame->GetContentLength();
  mDoubleByteText |= aFrame->GetFragment()->Is2b();
  mLastFrame = aFrame;
  mCommonAncestorWithLastFrame = aFrame->GetParent();

  MappedFlow* mappedFlow = &mMappedFlows[mMappedFlows.Length() - 1];
  NS_ASSERTION(mappedFlow->mStartFrame == aFrame ||
               mappedFlow->GetContentEnd() == aFrame->GetContentOffset(),
               "Overlapping or discontiguous frames => BAD");
  mappedFlow->mEndFrame = static_cast<nsTextFrame*>(aFrame->GetNextContinuation());
  if (mCurrentFramesAllSameTextRun != aFrame->GetTextRun()) {
    mCurrentFramesAllSameTextRun = nsnull;
  }

  if (mStartOfLine) {
    mLineBreakBeforeFrames.AppendElement(aFrame);
    mStartOfLine = PR_FALSE;
  }
}

static nscoord StyleToCoord(const nsStyleCoord& aCoord)
{
  if (eStyleUnit_Coord == aCoord.GetUnit()) {
    return aCoord.GetCoordValue();
  } else {
    return 0;
  }
}

static PRBool
HasTerminalNewline(const nsTextFrame* aFrame)
{
  if (aFrame->GetContentLength() == 0)
    return PR_FALSE;
  const nsTextFragment* frag = aFrame->GetFragment();
  return frag->CharAt(aFrame->GetContentEnd() - 1) == '\n';
}

PRBool
BuildTextRunsScanner::ContinueTextRunAcrossFrames(nsTextFrame* aFrame1, nsTextFrame* aFrame2)
{
  if (mBidiEnabled &&
      NS_GET_EMBEDDING_LEVEL(aFrame1) != NS_GET_EMBEDDING_LEVEL(aFrame2))
    return PR_FALSE;

  nsStyleContext* sc1 = aFrame1->GetStyleContext();
  const nsStyleText* textStyle1 = sc1->GetStyleText();
  
  
  
  
  
  
  if (textStyle1->NewlineIsSignificant() && HasTerminalNewline(aFrame1))
    return PR_FALSE;

  if (aFrame1->GetContent() == aFrame2->GetContent() &&
      aFrame1->GetNextInFlow() != aFrame2) {
    
    
    
    
    
    
    return PR_FALSE;
  }

  nsStyleContext* sc2 = aFrame2->GetStyleContext();
  if (sc1 == sc2)
    return PR_TRUE;
  const nsStyleFont* fontStyle1 = sc1->GetStyleFont();
  const nsStyleFont* fontStyle2 = sc2->GetStyleFont();
  const nsStyleText* textStyle2 = sc2->GetStyleText();
  return fontStyle1->mFont.BaseEquals(fontStyle2->mFont) &&
    sc1->GetStyleVisibility()->mLangGroup == sc2->GetStyleVisibility()->mLangGroup &&
    nsLayoutUtils::GetTextRunFlagsForStyle(sc1, textStyle1, fontStyle1) ==
      nsLayoutUtils::GetTextRunFlagsForStyle(sc2, textStyle2, fontStyle2);
}

void BuildTextRunsScanner::ScanFrame(nsIFrame* aFrame)
{
  
  if (mMappedFlows.Length() > 0) {
    MappedFlow* mappedFlow = &mMappedFlows[mMappedFlows.Length() - 1];
    if (mappedFlow->mEndFrame == aFrame &&
        (aFrame->GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION)) {
      NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
                   "Flow-sibling of a text frame is not a text frame?");

      
      
      
      if (mLastFrame->GetStyleContext() == aFrame->GetStyleContext() &&
          !HasTerminalNewline(mLastFrame)) {
        AccumulateRunInfo(static_cast<nsTextFrame*>(aFrame));
        return;
      }
    }
  }

  nsIAtom* frameType = aFrame->GetType();
  
  if (frameType == nsGkAtoms::textFrame) {
    nsTextFrame* frame = static_cast<nsTextFrame*>(aFrame);

    if (mLastFrame) {
      if (!ContinueTextRunAcrossFrames(mLastFrame, frame)) {
        FlushFrames(PR_FALSE, PR_FALSE);
      } else {
        if (mLastFrame->GetContent() == frame->GetContent()) {
          AccumulateRunInfo(frame);
          return;
        }
      }
    }

    MappedFlow* mappedFlow = mMappedFlows.AppendElement();
    if (!mappedFlow)
      return;

    mappedFlow->mStartFrame = frame;
    mappedFlow->mAncestorControllingInitialBreak = mCommonAncestorWithLastFrame;

    AccumulateRunInfo(frame);
    if (mMappedFlows.Length() == 1) {
      mCurrentFramesAllSameTextRun = frame->GetTextRun();
      mCurrentRunContextInfo = mNextRunContextInfo;
    }
    return;
  }

  FrameTextTraversal traversal =
    CanTextCrossFrameBoundary(aFrame, frameType);
  PRBool isBR = frameType == nsGkAtoms::brFrame;
  if (!traversal.mLineBreakerCanCrossFrameBoundary) {
    
    
    FlushFrames(PR_TRUE, isBR);
    mCommonAncestorWithLastFrame = aFrame;
    mNextRunContextInfo &= ~nsTextFrameUtils::INCOMING_WHITESPACE;
    mStartOfLine = PR_FALSE;
  } else if (!traversal.mTextRunCanCrossFrameBoundary) {
    FlushFrames(PR_FALSE, PR_FALSE);
  }

  for (nsIFrame* f = traversal.NextFrameToScan(); f;
       f = traversal.NextFrameToScan()) {
    ScanFrame(f);
  }

  if (!traversal.mLineBreakerCanCrossFrameBoundary) {
    
    
    FlushFrames(PR_TRUE, isBR);
    mCommonAncestorWithLastFrame = aFrame;
    mNextRunContextInfo &= ~nsTextFrameUtils::INCOMING_WHITESPACE;
  } else if (!traversal.mTextRunCanCrossFrameBoundary) {
    FlushFrames(PR_FALSE, PR_FALSE);
  }

  LiftCommonAncestorWithLastFrameToParent(aFrame->GetParent());
}

nsTextFrame*
BuildTextRunsScanner::GetNextBreakBeforeFrame(PRUint32* aIndex)
{
  PRUint32 index = *aIndex;
  if (index >= mLineBreakBeforeFrames.Length())
    return nsnull;
  *aIndex = index + 1;
  return static_cast<nsTextFrame*>(mLineBreakBeforeFrames.ElementAt(index));
}

static PRUint32
GetSpacingFlags(nscoord spacing)
{
  return spacing ? gfxTextRunFactory::TEXT_ENABLE_SPACING : 0;
}

static gfxFontGroup*
GetFontGroupForFrame(nsIFrame* aFrame,
                     nsIFontMetrics** aOutFontMetrics = nsnull)
{
  if (aOutFontMetrics)
    *aOutFontMetrics = nsnull;

  nsCOMPtr<nsIFontMetrics> metrics;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(metrics));

  if (!metrics)
    return nsnull;

  nsIFontMetrics* metricsRaw = metrics;
  if (aOutFontMetrics) {
    *aOutFontMetrics = metricsRaw;
    NS_ADDREF(*aOutFontMetrics);
  }
  nsIThebesFontMetrics* fm = static_cast<nsIThebesFontMetrics*>(metricsRaw);
  
  
  
  
  return fm->GetThebesFontGroup();
}

static already_AddRefed<gfxContext>
GetReferenceRenderingContext(nsTextFrame* aTextFrame, nsIRenderingContext* aRC)
{
  nsCOMPtr<nsIRenderingContext> tmp = aRC;
  if (!tmp) {
    nsresult rv = aTextFrame->PresContext()->PresShell()->
      CreateRenderingContext(aTextFrame, getter_AddRefs(tmp));
    if (NS_FAILED(rv))
      return nsnull;
  }

  gfxContext* ctx = tmp->ThebesContext();
  NS_ADDREF(ctx);
  return ctx;
}





static gfxTextRun*
GetHyphenTextRun(gfxTextRun* aTextRun, gfxContext* aContext, nsTextFrame* aTextFrame)
{
  nsRefPtr<gfxContext> ctx = aContext;
  if (!ctx) {
    ctx = GetReferenceRenderingContext(aTextFrame, nsnull);
  }
  if (!ctx)
    return nsnull;

  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();
  PRUint32 flags = gfxFontGroup::TEXT_IS_PERSISTENT;

  
  
  
  static const PRUnichar unicodeHyphen = 0x2010;
  gfxFont *font = fontGroup->GetFontAt(0);
  if (font && font->HasCharacter(unicodeHyphen)) {
    return gfxTextRunCache::MakeTextRun(&unicodeHyphen, 1, fontGroup, ctx,
                                        aTextRun->GetAppUnitsPerDevUnit(), flags);
  }

  static const PRUint8 dash = '-';
  return gfxTextRunCache::MakeTextRun(&dash, 1, fontGroup, ctx,
                                      aTextRun->GetAppUnitsPerDevUnit(),
                                      flags);
}

static gfxFont::Metrics
GetFirstFontMetrics(gfxFontGroup* aFontGroup)
{
  if (!aFontGroup)
    return gfxFont::Metrics();
  gfxFont* font = aFontGroup->GetFontAt(0);
  if (!font)
    return gfxFont::Metrics();
  return font->GetMetrics();
}

PR_STATIC_ASSERT(NS_STYLE_WHITESPACE_NORMAL == 0);
PR_STATIC_ASSERT(NS_STYLE_WHITESPACE_PRE == 1);
PR_STATIC_ASSERT(NS_STYLE_WHITESPACE_NOWRAP == 2);
PR_STATIC_ASSERT(NS_STYLE_WHITESPACE_PRE_WRAP == 3);
PR_STATIC_ASSERT(NS_STYLE_WHITESPACE_PRE_LINE == 4);

static const nsTextFrameUtils::CompressionMode CSSWhitespaceToCompressionMode[] =
{
  nsTextFrameUtils::COMPRESS_WHITESPACE_NEWLINE, 
  nsTextFrameUtils::COMPRESS_NONE,               
  nsTextFrameUtils::COMPRESS_WHITESPACE_NEWLINE, 
  nsTextFrameUtils::COMPRESS_NONE,               
  nsTextFrameUtils::COMPRESS_WHITESPACE          
};

gfxTextRun*
BuildTextRunsScanner::BuildTextRunForFrames(void* aTextBuffer)
{
  gfxSkipCharsBuilder builder;

  const void* textPtr = aTextBuffer;
  PRBool anySmallcapsStyle = PR_FALSE;
  PRBool anyTextTransformStyle = PR_FALSE;
  PRInt32 endOfLastContent = 0;
  PRUint32 textFlags = nsTextFrameUtils::TEXT_NO_BREAKS;

  if (mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_WHITESPACE) {
    textFlags |= nsTextFrameUtils::TEXT_INCOMING_WHITESPACE;
  }
  if (mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_ARABICCHAR) {
    textFlags |= gfxTextRunWordCache::TEXT_INCOMING_ARABICCHAR;
  }

  nsAutoTArray<PRInt32,50> textBreakPoints;
  TextRunUserData dummyData;
  TextRunMappedFlow dummyMappedFlow;

  TextRunUserData* userData;
  TextRunUserData* userDataToDestroy;
  
  
  if (mMappedFlows.Length() == 1 && !mMappedFlows[0].mEndFrame &&
      mMappedFlows[0].mStartFrame->GetContentOffset() == 0) {
    userData = &dummyData;
    userDataToDestroy = nsnull;
    dummyData.mMappedFlows = &dummyMappedFlow;
  } else {
    userData = static_cast<TextRunUserData*>
      (nsMemory::Alloc(sizeof(TextRunUserData) + mMappedFlows.Length()*sizeof(TextRunMappedFlow)));
    userDataToDestroy = userData;
    userData->mMappedFlows = reinterpret_cast<TextRunMappedFlow*>(userData + 1);
  }
  userData->mMappedFlowCount = mMappedFlows.Length();
  userData->mLastFlowIndex = 0;

  PRUint32 currentTransformedTextOffset = 0;

  PRUint32 nextBreakIndex = 0;
  nsTextFrame* nextBreakBeforeFrame = GetNextBreakBeforeFrame(&nextBreakIndex);
  PRBool enabledJustification = mLineContainer &&
    mLineContainer->GetStyleText()->mTextAlign == NS_STYLE_TEXT_ALIGN_JUSTIFY;

  PRUint32 i;
  const nsStyleText* textStyle = nsnull;
  const nsStyleFont* fontStyle = nsnull;
  nsStyleContext* lastStyleContext = nsnull;
  for (i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    nsTextFrame* f = mappedFlow->mStartFrame;

    lastStyleContext = f->GetStyleContext();
    
    textStyle = f->GetStyleText();
    if (NS_STYLE_TEXT_TRANSFORM_NONE != textStyle->mTextTransform) {
      anyTextTransformStyle = PR_TRUE;
    }
    textFlags |= GetSpacingFlags(StyleToCoord(textStyle->mLetterSpacing));
    textFlags |= GetSpacingFlags(textStyle->mWordSpacing);
    nsTextFrameUtils::CompressionMode compression =
      CSSWhitespaceToCompressionMode[textStyle->mWhiteSpace];
    if (enabledJustification && !textStyle->WhiteSpaceIsSignificant()) {
      textFlags |= gfxTextRunFactory::TEXT_ENABLE_SPACING;
    }
    fontStyle = f->GetStyleFont();
    if (NS_STYLE_FONT_VARIANT_SMALL_CAPS == fontStyle->mFont.variant) {
      anySmallcapsStyle = PR_TRUE;
    }

    
    nsIContent* content = f->GetContent();
    const nsTextFragment* frag = f->GetFragment();
    PRInt32 contentStart = mappedFlow->mStartFrame->GetContentOffset();
    PRInt32 contentEnd = mappedFlow->GetContentEnd();
    PRInt32 contentLength = contentEnd - contentStart;

    TextRunMappedFlow* newFlow = &userData->mMappedFlows[i];
    newFlow->mStartFrame = mappedFlow->mStartFrame;
    newFlow->mDOMOffsetToBeforeTransformOffset = builder.GetCharCount() -
      mappedFlow->mStartFrame->GetContentOffset();
    newFlow->mContentLength = contentLength;

    while (nextBreakBeforeFrame && nextBreakBeforeFrame->GetContent() == content) {
      textBreakPoints.AppendElement(
          nextBreakBeforeFrame->GetContentOffset() + newFlow->mDOMOffsetToBeforeTransformOffset);
      nextBreakBeforeFrame = GetNextBreakBeforeFrame(&nextBreakIndex);
    }

    PRUint32 analysisFlags;
    if (frag->Is2b()) {
      NS_ASSERTION(mDoubleByteText, "Wrong buffer char size!");
      PRUnichar* bufStart = static_cast<PRUnichar*>(aTextBuffer);
      PRUnichar* bufEnd = nsTextFrameUtils::TransformText(
          frag->Get2b() + contentStart, contentLength, bufStart,
          compression, &mNextRunContextInfo, &builder, &analysisFlags);
      aTextBuffer = bufEnd;
    } else {
      if (mDoubleByteText) {
        
        
        nsAutoTArray<PRUint8,BIG_TEXT_NODE_SIZE> tempBuf;
        if (!tempBuf.AppendElements(contentLength)) {
          DestroyUserData(userDataToDestroy);
          return nsnull;
        }
        PRUint8* bufStart = tempBuf.Elements();
        PRUint8* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const PRUint8*>(frag->Get1b()) + contentStart, contentLength,
            bufStart, compression, &mNextRunContextInfo, &builder, &analysisFlags);
        aTextBuffer = ExpandBuffer(static_cast<PRUnichar*>(aTextBuffer),
                                   tempBuf.Elements(), end - tempBuf.Elements());
      } else {
        PRUint8* bufStart = static_cast<PRUint8*>(aTextBuffer);
        PRUint8* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const PRUint8*>(frag->Get1b()) + contentStart, contentLength,
            bufStart, compression, &mNextRunContextInfo, &builder, &analysisFlags);
        aTextBuffer = end;
      }
    }
    textFlags |= analysisFlags;

    currentTransformedTextOffset =
      (static_cast<const PRUint8*>(aTextBuffer) - static_cast<const PRUint8*>(textPtr)) >> mDoubleByteText;

    endOfLastContent = contentEnd;
  }

  
  if (!builder.IsOK()) {
    DestroyUserData(userDataToDestroy);
    return nsnull;
  }

  void* finalUserData;
  if (userData == &dummyData) {
    textFlags |= nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW;
    userData = nsnull;
    finalUserData = mMappedFlows[0].mStartFrame;
  } else {
    finalUserData = userData;
  }

  PRUint32 transformedLength = currentTransformedTextOffset;

  
  nsTextFrame* firstFrame = mMappedFlows[0].mStartFrame;
  gfxFontGroup* fontGroup = GetFontGroupForFrame(firstFrame);
  if (!fontGroup) {
    DestroyUserData(userDataToDestroy);
    return nsnull;
  }

  if (textFlags & nsTextFrameUtils::TEXT_HAS_TAB) {
    textFlags |= gfxTextRunFactory::TEXT_ENABLE_SPACING;
  }
  if (textFlags & nsTextFrameUtils::TEXT_HAS_SHY) {
    textFlags |= gfxTextRunFactory::TEXT_ENABLE_HYPHEN_BREAKS;
  }
  if (mBidiEnabled && (NS_GET_EMBEDDING_LEVEL(firstFrame) & 1)) {
    textFlags |= gfxTextRunFactory::TEXT_IS_RTL;
  }
  if (mNextRunContextInfo & nsTextFrameUtils::INCOMING_WHITESPACE) {
    textFlags |= nsTextFrameUtils::TEXT_TRAILING_WHITESPACE;
  }
  if (mNextRunContextInfo & nsTextFrameUtils::INCOMING_ARABICCHAR) {
    textFlags |= gfxTextRunWordCache::TEXT_TRAILING_ARABICCHAR;
  }
  
  
  textFlags |= nsLayoutUtils::GetTextRunFlagsForStyle(lastStyleContext,
      textStyle, fontStyle);
  
  
  if (!(textFlags & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED)) {
    textFlags |= gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX;
  }

  gfxSkipChars skipChars;
  skipChars.TakeFrom(&builder);
  
  NS_ASSERTION(nextBreakIndex == mLineBreakBeforeFrames.Length(),
               "Didn't find all the frames to break-before...");
  gfxSkipCharsIterator iter(skipChars);
  nsAutoTArray<PRUint32,50> textBreakPointsAfterTransform;
  for (i = 0; i < textBreakPoints.Length(); ++i) {
    nsTextFrameUtils::AppendLineBreakOffset(&textBreakPointsAfterTransform, 
            iter.ConvertOriginalToSkipped(textBreakPoints[i]));
  }
  if (mStartOfLine) {
    nsTextFrameUtils::AppendLineBreakOffset(&textBreakPointsAfterTransform,
                                            transformedLength);
  }

  
  nsAutoPtr<nsTransformingTextRunFactory> transformingFactory;
  if (anySmallcapsStyle) {
    transformingFactory = new nsFontVariantTextRunFactory();
  }
  if (anyTextTransformStyle) {
    transformingFactory =
      new nsCaseTransformTextRunFactory(transformingFactory.forget());
  }
  nsTArray<nsStyleContext*> styles;
  if (transformingFactory) {
    iter.SetOriginalOffset(0);
    for (i = 0; i < mMappedFlows.Length(); ++i) {
      MappedFlow* mappedFlow = &mMappedFlows[i];
      nsTextFrame* f;
      for (f = mappedFlow->mStartFrame; f != mappedFlow->mEndFrame;
           f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
        PRUint32 offset = iter.GetSkippedOffset();
        iter.AdvanceOriginal(f->GetContentLength());
        PRUint32 end = iter.GetSkippedOffset();
        nsStyleContext* sc = f->GetStyleContext();
        PRUint32 j;
        for (j = offset; j < end; ++j) {
          styles.AppendElement(sc);
        }
      }
    }
    textFlags |= nsTextFrameUtils::TEXT_IS_TRANSFORMED;
    NS_ASSERTION(iter.GetSkippedOffset() == transformedLength,
                 "We didn't cover all the characters in the text run!");
  }

  gfxTextRun* textRun;
  gfxTextRunFactory::Parameters params =
      { mContext, finalUserData, &skipChars,
        textBreakPointsAfterTransform.Elements(), textBreakPointsAfterTransform.Length(),
        firstFrame->PresContext()->AppUnitsPerDevPixel() };

  if (mDoubleByteText) {
    const PRUnichar* text = static_cast<const PRUnichar*>(textPtr);
    if (transformingFactory) {
      textRun = transformingFactory->MakeTextRun(text, transformedLength, &params,
                                                 fontGroup, textFlags, styles.Elements());
      if (textRun) {
        
        transformingFactory.forget();
      }
    } else {
      textRun = MakeTextRun(text, transformedLength, fontGroup, &params, textFlags);
    }
  } else {
    const PRUint8* text = static_cast<const PRUint8*>(textPtr);
    textFlags |= gfxFontGroup::TEXT_IS_8BIT;
    if (transformingFactory) {
      textRun = transformingFactory->MakeTextRun(text, transformedLength, &params,
                                                 fontGroup, textFlags, styles.Elements());
      if (textRun) {
        
        transformingFactory.forget();
      }
    } else {
      textRun = MakeTextRun(text, transformedLength, fontGroup, &params, textFlags);
    }
  }
  if (!textRun) {
    DestroyUserData(userDataToDestroy);
    return nsnull;
  }

  
  
  
  
  SetupBreakSinksForTextRun(textRun, PR_FALSE, mSkipIncompleteTextRuns);

  if (mSkipIncompleteTextRuns) {
    mSkipIncompleteTextRuns = !TextContainsLineBreakerWhiteSpace(textPtr,
        transformedLength, mDoubleByteText);
    
    
    mTextRunsToDelete.AppendElement(textRun);
    
    
    
    
    
    textRun->SetUserData(nsnull);
    DestroyUserData(userDataToDestroy);
    return nsnull;
  }

  
  
  AssignTextRun(textRun);
  return textRun;
}

static PRBool
HasCompressedLeadingWhitespace(nsTextFrame* aFrame, const nsStyleText* aStyleText,
                               PRInt32 aContentEndOffset,
                               const gfxSkipCharsIterator& aIterator)
{
  if (!aIterator.IsOriginalCharSkipped())
    return PR_FALSE;

  gfxSkipCharsIterator iter = aIterator;
  PRInt32 frameContentOffset = aFrame->GetContentOffset();
  const nsTextFragment* frag = aFrame->GetFragment();
  while (frameContentOffset < aContentEndOffset && iter.IsOriginalCharSkipped()) {
    if (IsTrimmableSpace(frag, frameContentOffset, aStyleText))
      return PR_TRUE;
    ++frameContentOffset;
    iter.AdvanceOriginal(1);
  }
  return PR_FALSE;
}

void
BuildTextRunsScanner::SetupBreakSinksForTextRun(gfxTextRun* aTextRun,
                                                PRBool aIsExistingTextRun,
                                                PRBool aSuppressSink)
{
  
  nsIAtom* lang = mMappedFlows[0].mStartFrame->GetStyleVisibility()->mLangGroup;
  
  
  
  gfxSkipCharsIterator iter(aTextRun->GetSkipChars());

  PRUint32 i;
  for (i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    PRUint32 offset = iter.GetSkippedOffset();
    gfxSkipCharsIterator iterNext = iter;
    iterNext.AdvanceOriginal(mappedFlow->GetContentEnd() -
            mappedFlow->mStartFrame->GetContentOffset());

    nsAutoPtr<BreakSink>* breakSink = mBreakSinks.AppendElement(
      new BreakSink(aTextRun, mContext, offset, aIsExistingTextRun));
    if (!breakSink || !*breakSink)
      return;

    PRUint32 length = iterNext.GetSkippedOffset() - offset;
    PRUint32 flags = 0;
    nsIFrame* initialBreakController = mappedFlow->mAncestorControllingInitialBreak;
    if (!initialBreakController) {
      initialBreakController = mLineContainer;
    }
    if (!initialBreakController->GetStyleText()->WhiteSpaceCanWrap()) {
      flags |= nsLineBreaker::BREAK_SUPPRESS_INITIAL;
    }
    nsTextFrame* startFrame = mappedFlow->mStartFrame;
    const nsStyleText* textStyle = startFrame->GetStyleText();
    if (!textStyle->WhiteSpaceCanWrap()) {
      flags |= nsLineBreaker::BREAK_SUPPRESS_INSIDE;
    }
    if (aTextRun->GetFlags() & nsTextFrameUtils::TEXT_NO_BREAKS) {
      flags |= nsLineBreaker::BREAK_SKIP_SETTING_NO_BREAKS;
    }
    if (textStyle->mTextTransform == NS_STYLE_TEXT_TRANSFORM_CAPITALIZE) {
      flags |= nsLineBreaker::BREAK_NEED_CAPITALIZATION;
    }

    if (HasCompressedLeadingWhitespace(startFrame, textStyle,
                                       mappedFlow->GetContentEnd(), iter)) {
      mLineBreaker.AppendInvisibleWhitespace(flags);
    }

    if (length > 0) {
      BreakSink* sink = aSuppressSink ? nsnull : (*breakSink).get();
      if (aTextRun->GetFlags() & gfxFontGroup::TEXT_IS_8BIT) {
        mLineBreaker.AppendText(lang, aTextRun->GetText8Bit() + offset,
                                length, flags, sink);
      } else {
        mLineBreaker.AppendText(lang, aTextRun->GetTextUnicode() + offset,
                                length, flags, sink);
      }
    }
    
    iter = iterNext;
  }
}

void
BuildTextRunsScanner::AssignTextRun(gfxTextRun* aTextRun)
{
  PRUint32 i;
  for (i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    nsTextFrame* startFrame = mappedFlow->mStartFrame;
    nsTextFrame* endFrame = mappedFlow->mEndFrame;
    nsTextFrame* f;
    for (f = startFrame; f != endFrame;
         f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
#ifdef DEBUG_roc
      if (f->GetTextRun()) {
        gfxTextRun* textRun = f->GetTextRun();
        if (textRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW) {
          if (mMappedFlows[0].mStartFrame != static_cast<nsTextFrame*>(textRun->GetUserData())) {
            NS_WARNING("REASSIGNING SIMPLE FLOW TEXT RUN!");
          }
        } else {
          TextRunUserData* userData =
            static_cast<TextRunUserData*>(textRun->GetUserData());
         
          if (PRUint32(userData->mMappedFlowCount) >= mMappedFlows.Length() ||
              userData->mMappedFlows[userData->mMappedFlowCount - 1].mStartFrame !=
              mMappedFlows[userData->mMappedFlowCount - 1].mStartFrame) {
            NS_WARNING("REASSIGNING MULTIFLOW TEXT RUN (not append)!");
          }
        }
      }
#endif
      f->ClearTextRun();
      f->SetTextRun(aTextRun);
    }
    
    
    startFrame->AddStateBits(TEXT_IN_TEXTRUN_USER_DATA);
  }
}

gfxSkipCharsIterator
nsTextFrame::EnsureTextRun(gfxContext* aReferenceContext, nsIFrame* aLineContainer,
                           const nsLineList::iterator* aLine,
                           PRUint32* aFlowEndInTextRun)
{
  if (mTextRun && (!aLine || !(*aLine)->GetInvalidateTextRuns())) {
    if (mTextRun->GetExpirationState()->IsTracked()) {
      gTextRuns->MarkUsed(mTextRun);
    }
  } else {
    nsRefPtr<gfxContext> ctx = aReferenceContext;
    if (!ctx) {
      ctx = GetReferenceRenderingContext(this, nsnull);
    }
    if (ctx) {
      BuildTextRuns(ctx, this, aLineContainer, aLine);
    }
    if (!mTextRun) {
      
      
      static const gfxSkipChars emptySkipChars;
      return gfxSkipCharsIterator(emptySkipChars, 0);
    }
  }

  if (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW) {
    if (aFlowEndInTextRun) {
      *aFlowEndInTextRun = mTextRun->GetLength();
    }
    return gfxSkipCharsIterator(mTextRun->GetSkipChars(), 0, mContentOffset);
  }

  TextRunUserData* userData = static_cast<TextRunUserData*>(mTextRun->GetUserData());
  
  PRInt32 direction;
  PRInt32 startAt = userData->mLastFlowIndex;
  
  for (direction = 1; direction >= -1; direction -= 2) {
    PRInt32 i;
    for (i = startAt; 0 <= i && i < userData->mMappedFlowCount; i += direction) {
      TextRunMappedFlow* flow = &userData->mMappedFlows[i];
      if (flow->mStartFrame->GetContent() == mContent) {
        
        
        userData->mLastFlowIndex = i;
        gfxSkipCharsIterator iter(mTextRun->GetSkipChars(),
                                  flow->mDOMOffsetToBeforeTransformOffset, mContentOffset);
        if (aFlowEndInTextRun) {
          if (i + 1 < userData->mMappedFlowCount) {
            gfxSkipCharsIterator end(mTextRun->GetSkipChars());
            *aFlowEndInTextRun = end.ConvertOriginalToSkipped(
                flow[1].mStartFrame->GetContentOffset() + flow[1].mDOMOffsetToBeforeTransformOffset);
          } else {
            *aFlowEndInTextRun = mTextRun->GetLength();
          }
        }
        return iter;
      }
      ++flow;
    }
    startAt = userData->mLastFlowIndex - 1;
  }
  NS_ERROR("Can't find flow containing this frame???");
  static const gfxSkipChars emptySkipChars;
  return gfxSkipCharsIterator(emptySkipChars, 0);
}

static PRUint32
GetEndOfTrimmedText(const nsTextFragment* aFrag, const nsStyleText* aStyleText,
                    PRUint32 aStart, PRUint32 aEnd,
                    gfxSkipCharsIterator* aIterator)
{
  aIterator->SetSkippedOffset(aEnd);
  while (aIterator->GetSkippedOffset() > aStart) {
    aIterator->AdvanceSkipped(-1);
    if (!IsTrimmableSpace(aFrag, aIterator->GetOriginalOffset(), aStyleText))
      return aIterator->GetSkippedOffset() + 1;
  }
  return aStart;
}

nsTextFrame::TrimmedOffsets
nsTextFrame::GetTrimmedOffsets(const nsTextFragment* aFrag,
                               PRBool aTrimAfter)
{
  NS_ASSERTION(mTextRun, "Need textrun here");
  
  
  NS_ASSERTION(!(GetStateBits() & NS_FRAME_FIRST_REFLOW),
               "Can only call this on frames that have been reflowed");
  NS_ASSERTION(!(GetStateBits() & NS_FRAME_IN_REFLOW),
               "Can only call this on frames that are not being reflowed");

  TrimmedOffsets offsets = { GetContentOffset(), GetContentLength() };
  const nsStyleText* textStyle = GetStyleText();
  
  
  if (textStyle->WhiteSpaceIsSignificant())
    return offsets;

  if (GetStateBits() & TEXT_START_OF_LINE) {
    PRInt32 whitespaceCount =
      GetTrimmableWhitespaceCount(aFrag,
                                  offsets.mStart, offsets.mLength, 1);
    offsets.mStart += whitespaceCount;
    offsets.mLength -= whitespaceCount;
  }

  if (aTrimAfter && (GetStateBits() & TEXT_END_OF_LINE)) {
    
    
    
    PRInt32 whitespaceCount =
      GetTrimmableWhitespaceCount(aFrag,
                                  offsets.GetEnd() - 1, offsets.mLength, -1);
    offsets.mLength -= whitespaceCount;
  }
  return offsets;
}








static PRBool IsJustifiableCharacter(const nsTextFragment* aFrag, PRInt32 aPos,
                                     PRBool aLangIsCJ)
{
  PRUnichar ch = aFrag->CharAt(aPos);
  if (ch == '\n' || ch == '\t')
    return PR_TRUE;
  if (ch == ' ') {
    
    if (!aFrag->Is2b())
      return PR_TRUE;
    return !nsTextFrameUtils::IsSpaceCombiningSequenceTail(
        aFrag->Get2b() + aPos + 1, aFrag->GetLength() - (aPos + 1));
  }
  if (ch < 0x2150u)
    return PR_FALSE;
  if (aLangIsCJ && (
       (0x2150u <= ch && ch <= 0x22ffu) || 
       (0x2460u <= ch && ch <= 0x24ffu) || 
       (0x2580u <= ch && ch <= 0x27bfu) || 
       (0x27f0u <= ch && ch <= 0x2bffu) || 
                                           
                                           
       (0x2e80u <= ch && ch <= 0x312fu) || 
                                           
                                           
       (0x3190u <= ch && ch <= 0xabffu) || 
                                           
                                           
                                           
       (0xf900u <= ch && ch <= 0xfaffu) || 
       (0xff5eu <= ch && ch <= 0xff9fu)    
     ))
    return PR_TRUE;
  return PR_FALSE;
}

static void ClearMetrics(nsHTMLReflowMetrics& aMetrics)
{
  aMetrics.width = 0;
  aMetrics.height = 0;
  aMetrics.ascent = 0;
}

static PRInt32 FindChar(const nsTextFragment* frag,
                        PRInt32 aOffset, PRInt32 aLength, PRUnichar ch)
{
  PRInt32 i = 0;
  if (frag->Is2b()) {
    const PRUnichar* str = frag->Get2b() + aOffset;
    for (; i < aLength; ++i) {
      if (*str == ch)
        return i + aOffset;
      ++str;
    }
  } else {
    if (PRUint16(ch) <= 0xFF) {
      const char* str = frag->Get1b() + aOffset;
      const void* p = memchr(str, ch, aLength);
      if (p)
        return (static_cast<const char*>(p) - str) + aOffset;
    }
  }
  return -1;
}

static PRBool IsChineseJapaneseLangGroup(nsIFrame* aFrame)
{
  nsIAtom* langGroup = aFrame->GetStyleVisibility()->mLangGroup;
  return langGroup == nsGkAtoms::Japanese
      || langGroup == nsGkAtoms::Chinese
      || langGroup == nsGkAtoms::Taiwanese
      || langGroup == nsGkAtoms::HongKongChinese;
}

#ifdef DEBUG
static PRBool IsInBounds(const gfxSkipCharsIterator& aStart, PRInt32 aContentLength,
                         PRUint32 aOffset, PRUint32 aLength) {
  if (aStart.GetSkippedOffset() > aOffset)
    return PR_FALSE;
  if (aContentLength == PR_INT32_MAX)
    return PR_TRUE;
  gfxSkipCharsIterator iter(aStart);
  iter.AdvanceOriginal(aContentLength);
  return iter.GetSkippedOffset() >= aOffset + aLength;
}
#endif

class NS_STACK_CLASS PropertyProvider : public gfxTextRun::PropertyProvider {
public:
  








  PropertyProvider(gfxTextRun* aTextRun, const nsStyleText* aTextStyle,
                   const nsTextFragment* aFrag, nsTextFrame* aFrame,
                   const gfxSkipCharsIterator& aStart, PRInt32 aLength,
                   nsIFrame* aLineContainer,
                   nscoord aOffsetFromBlockOriginForTabs)
    : mTextRun(aTextRun), mFontGroup(nsnull),
      mTextStyle(aTextStyle), mFrag(aFrag),
      mLineContainer(aLineContainer),
      mFrame(aFrame), mStart(aStart), mTempIterator(aStart),
      mTabWidths(nsnull), mLength(aLength),
      mWordSpacing(mTextStyle->mWordSpacing),
      mLetterSpacing(StyleToCoord(mTextStyle->mLetterSpacing)),
      mJustificationSpacing(0),
      mHyphenWidth(-1),
      mOffsetFromBlockOriginForTabs(aOffsetFromBlockOriginForTabs),
      mReflowing(PR_TRUE)
  {
    NS_ASSERTION(mStart.IsInitialized(), "Start not initialized?");
  }

  




  PropertyProvider(nsTextFrame* aFrame, const gfxSkipCharsIterator& aStart)
    : mTextRun(aFrame->GetTextRun()), mFontGroup(nsnull),
      mTextStyle(aFrame->GetStyleText()),
      mFrag(aFrame->GetFragment()),
      mLineContainer(nsnull),
      mFrame(aFrame), mStart(aStart), mTempIterator(aStart),
      mTabWidths(nsnull),
      mLength(aFrame->GetContentLength()),
      mWordSpacing(mTextStyle->mWordSpacing),
      mLetterSpacing(StyleToCoord(mTextStyle->mLetterSpacing)),
      mJustificationSpacing(0),
      mHyphenWidth(-1),
      mOffsetFromBlockOriginForTabs(0),
      mReflowing(PR_FALSE)
  {
    NS_ASSERTION(mTextRun, "Textrun not initialized!");
  }

  
  void InitializeForDisplay(PRBool aTrimAfter);

  virtual void GetSpacing(PRUint32 aStart, PRUint32 aLength, Spacing* aSpacing);
  virtual gfxFloat GetHyphenWidth();
  virtual void GetHyphenationBreaks(PRUint32 aStart, PRUint32 aLength,
                                    PRPackedBool* aBreakBefore);

  void GetSpacingInternal(PRUint32 aStart, PRUint32 aLength, Spacing* aSpacing,
                          PRBool aIgnoreTabs);

  


  PRUint32 ComputeJustifiableCharacters(PRInt32 aOffset, PRInt32 aLength);
  




  void FindJustificationRange(gfxSkipCharsIterator* aStart,
                              gfxSkipCharsIterator* aEnd);

  const nsStyleText* GetStyleText() { return mTextStyle; }
  nsTextFrame* GetFrame() { return mFrame; }
  
  
  
  const gfxSkipCharsIterator& GetStart() { return mStart; }
  
  PRUint32 GetOriginalLength() {
    NS_ASSERTION(mLength != PR_INT32_MAX, "Length not known");
    return mLength;
  }
  const nsTextFragment* GetFragment() { return mFrag; }

  gfxFontGroup* GetFontGroup() {
    if (!mFontGroup)
      InitFontGroupAndFontMetrics();
    return mFontGroup;
  }

  nsIFontMetrics* GetFontMetrics() {
    if (!mFontMetrics)
      InitFontGroupAndFontMetrics();
    return mFontMetrics;
  }

  gfxFloat* GetTabWidths(PRUint32 aTransformedStart, PRUint32 aTransformedLength);

  const gfxSkipCharsIterator& GetEndHint() { return mTempIterator; }

protected:
  void SetupJustificationSpacing();

  void InitFontGroupAndFontMetrics() {
    mFontGroup = GetFontGroupForFrame(mFrame, getter_AddRefs(mFontMetrics));
  }

  gfxTextRun*           mTextRun;
  gfxFontGroup*         mFontGroup;
  nsCOMPtr<nsIFontMetrics> mFontMetrics;
  const nsStyleText*    mTextStyle;
  const nsTextFragment* mFrag;
  nsIFrame*             mLineContainer;
  nsTextFrame*          mFrame;
  gfxSkipCharsIterator  mStart;  
  gfxSkipCharsIterator  mTempIterator;
  
  
  
  nsTArray<gfxFloat>*   mTabWidths;

  PRInt32               mLength; 
  gfxFloat              mWordSpacing;     
  gfxFloat              mLetterSpacing;   
  gfxFloat              mJustificationSpacing;
  gfxFloat              mHyphenWidth;
  gfxFloat              mOffsetFromBlockOriginForTabs;
  PRPackedBool          mReflowing;
};

PRUint32
PropertyProvider::ComputeJustifiableCharacters(PRInt32 aOffset, PRInt32 aLength)
{
  
  nsSkipCharsRunIterator
    run(mStart, nsSkipCharsRunIterator::LENGTH_INCLUDES_SKIPPED, aLength);
  run.SetOriginalOffset(aOffset);
  PRUint32 justifiableChars = 0;
  PRBool isCJK = IsChineseJapaneseLangGroup(mFrame);
  while (run.NextRun()) {
    PRInt32 i;
    for (i = 0; i < run.GetRunLength(); ++i) {
      justifiableChars +=
        IsJustifiableCharacter(mFrag, run.GetOriginalOffset() + i, isCJK);
    }
  }
  return justifiableChars;
}




static void FindClusterStart(gfxTextRun* aTextRun,
                             gfxSkipCharsIterator* aPos)
{
  while (aPos->GetOriginalOffset() > 0) {
    if (aPos->IsOriginalCharSkipped() ||
        aTextRun->IsClusterStart(aPos->GetSkippedOffset())) {
      break;
    }
    aPos->AdvanceOriginal(-1);
  }
}




static void FindClusterEnd(gfxTextRun* aTextRun, PRInt32 aOriginalEnd,
                           gfxSkipCharsIterator* aPos)
{
  NS_PRECONDITION(aPos->GetOriginalOffset() < aOriginalEnd,
                  "character outside string");
  aPos->AdvanceOriginal(1);
  while (aPos->GetOriginalOffset() < aOriginalEnd) {
    if (aPos->IsOriginalCharSkipped() ||
        aTextRun->IsClusterStart(aPos->GetSkippedOffset())) {
      break;
    }
    aPos->AdvanceOriginal(1);
  }
  aPos->AdvanceOriginal(-1);
}


void
PropertyProvider::GetSpacing(PRUint32 aStart, PRUint32 aLength,
                             Spacing* aSpacing)
{
  GetSpacingInternal(aStart, aLength, aSpacing,
                     (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TAB) == 0);
}

static PRBool
CanAddSpacingAfter(gfxTextRun* aTextRun, PRUint32 aOffset)
{
  if (aOffset + 1 >= aTextRun->GetLength())
    return PR_TRUE;
  return aTextRun->IsClusterStart(aOffset + 1) &&
    aTextRun->IsLigatureGroupStart(aOffset + 1);
}

void
PropertyProvider::GetSpacingInternal(PRUint32 aStart, PRUint32 aLength,
                                     Spacing* aSpacing, PRBool aIgnoreTabs)
{
  NS_PRECONDITION(IsInBounds(mStart, mLength, aStart, aLength), "Range out of bounds");

  PRUint32 index;
  for (index = 0; index < aLength; ++index) {
    aSpacing[index].mBefore = 0.0;
    aSpacing[index].mAfter = 0.0;
  }

  
  gfxSkipCharsIterator start(mStart);
  start.SetSkippedOffset(aStart);

  
  if (mWordSpacing || mLetterSpacing) {
    
    nsSkipCharsRunIterator
      run(start, nsSkipCharsRunIterator::LENGTH_UNSKIPPED_ONLY, aLength);
    while (run.NextRun()) {
      PRUint32 runOffsetInSubstring = run.GetSkippedOffset() - aStart;
      PRInt32 i;
      gfxSkipCharsIterator iter = run.GetPos();
      for (i = 0; i < run.GetRunLength(); ++i) {
        if (CanAddSpacingAfter(mTextRun, run.GetSkippedOffset() + i)) {
          
          aSpacing[runOffsetInSubstring + i].mAfter += mLetterSpacing;
        }
        if (IsCSSWordSpacingSpace(mFrag, i + run.GetOriginalOffset(),
                                  mTextStyle)) {
          
          
          iter.SetSkippedOffset(run.GetSkippedOffset() + i);
          FindClusterEnd(mTextRun, run.GetOriginalOffset() + run.GetRunLength(),
                         &iter);
          aSpacing[iter.GetSkippedOffset() - aStart].mAfter += mWordSpacing;
        }
      }
    }
  }

  
  if (!aIgnoreTabs)
    aIgnoreTabs = mFrame->GetStyleText()->mTabSize == 0;

  
  if (!aIgnoreTabs) {
    gfxFloat* tabs = GetTabWidths(aStart, aLength);
    if (tabs) {
      for (index = 0; index < aLength; ++index) {
        aSpacing[index].mAfter += tabs[index];
      }
    }
  }

  
  if (mJustificationSpacing) {
    gfxFloat halfJustificationSpace = mJustificationSpacing/2;
    
    
    PRBool isCJK = IsChineseJapaneseLangGroup(mFrame);
    gfxSkipCharsIterator justificationStart(mStart), justificationEnd(mStart);
    FindJustificationRange(&justificationStart, &justificationEnd);

    nsSkipCharsRunIterator
      run(start, nsSkipCharsRunIterator::LENGTH_UNSKIPPED_ONLY, aLength);
    while (run.NextRun()) {
      PRInt32 i;
      gfxSkipCharsIterator iter = run.GetPos();
      for (i = 0; i < run.GetRunLength(); ++i) {
        PRInt32 originalOffset = run.GetOriginalOffset() + i;
        if (IsJustifiableCharacter(mFrag, originalOffset, isCJK)) {
          iter.SetOriginalOffset(originalOffset);
          FindClusterStart(mTextRun, &iter);
          PRUint32 clusterFirstChar = iter.GetSkippedOffset();
          FindClusterEnd(mTextRun, run.GetOriginalOffset() + run.GetRunLength(), &iter);
          PRUint32 clusterLastChar = iter.GetSkippedOffset();
          
          if (clusterFirstChar >= justificationStart.GetSkippedOffset() &&
              clusterLastChar < justificationEnd.GetSkippedOffset()) {
            aSpacing[clusterFirstChar - aStart].mBefore += halfJustificationSpace;
            aSpacing[clusterLastChar - aStart].mAfter += halfJustificationSpace;
          }
        }
      }
    }
  }
}

static void TabWidthDestructor(void* aObject, nsIAtom* aProp, void* aValue,
                               void* aData)
{
  delete static_cast<nsTArray<gfxFloat>*>(aValue);
}

static gfxFloat
ComputeTabWidthAppUnits(nsIFrame* aFrame, gfxTextRun* aTextRun)
{
  
  const nsStyleText* textStyle = aFrame->GetStyleText();
  
  
  
  gfxFloat spaceWidthAppUnits =
    NS_roundf(GetFirstFontMetrics(
                GetFontGroupForFrame(aFrame)).spaceWidth *
              aTextRun->GetAppUnitsPerDevUnit());
  return textStyle->mTabSize * spaceWidthAppUnits;
}


static gfxFloat
AdvanceToNextTab(gfxFloat aX, nsIFrame* aFrame,
                 gfxTextRun* aTextRun, gfxFloat* aCachedTabWidth)
{
  if (*aCachedTabWidth < 0) {
    *aCachedTabWidth = ComputeTabWidthAppUnits(aFrame, aTextRun);
  }

  
  
  
  return NS_ceil((aX + 1)/(*aCachedTabWidth))*(*aCachedTabWidth);
}

gfxFloat*
PropertyProvider::GetTabWidths(PRUint32 aStart, PRUint32 aLength)
{
  if (!mTabWidths) {
    if (!mReflowing) {
      mTabWidths = static_cast<nsTArray<gfxFloat>*>
                              (mFrame->GetProperty(nsGkAtoms::tabWidthProperty));
      if (!mTabWidths) {
        NS_WARNING("We need precomputed tab widths, but they're not here...");
        return nsnull;
      }
    } else {
      if (!mLineContainer) {
        
        
        return nsnull;
      }

      nsAutoPtr<nsTArray<gfxFloat> > tabs(new nsTArray<gfxFloat>());
      if (!tabs)
        return nsnull;
      nsresult rv = mFrame->SetProperty(nsGkAtoms::tabWidthProperty, tabs,
                                        TabWidthDestructor, nsnull);
      if (NS_FAILED(rv))
        return nsnull;
      mTabWidths = tabs.forget();
    }
  }

  PRUint32 startOffset = mStart.GetSkippedOffset();
  PRUint32 tabsEnd = startOffset + mTabWidths->Length();
  if (tabsEnd < aStart + aLength) {
    if (!mReflowing) {
      NS_WARNING("We need precomputed tab widths, but we don't have enough...");
      return nsnull;
    }
    
    if (!mTabWidths->AppendElements(aStart + aLength - tabsEnd))
      return nsnull;
    
    gfxFloat tabWidth = -1;
    for (PRUint32 i = tabsEnd; i < aStart + aLength; ++i) {
      Spacing spacing;
      GetSpacingInternal(i, 1, &spacing, PR_TRUE);
      mOffsetFromBlockOriginForTabs += spacing.mBefore;

      if (mTextRun->GetChar(i) != '\t') {
        (*mTabWidths)[i - startOffset] = 0;
        if (mTextRun->IsClusterStart(i)) {
          PRUint32 clusterEnd = i + 1;
          while (clusterEnd < mTextRun->GetLength() &&
                 !mTextRun->IsClusterStart(clusterEnd)) {
            ++clusterEnd;
          }
          mOffsetFromBlockOriginForTabs +=
            mTextRun->GetAdvanceWidth(i, clusterEnd - i, nsnull);
        }
      } else {
        double nextTab = AdvanceToNextTab(mOffsetFromBlockOriginForTabs,
                mFrame, mTextRun, &tabWidth);
        (*mTabWidths)[i - startOffset] = nextTab - mOffsetFromBlockOriginForTabs;
        mOffsetFromBlockOriginForTabs = nextTab;
      }

      mOffsetFromBlockOriginForTabs += spacing.mAfter;
    }
  }

  return mTabWidths->Elements() + aStart - startOffset;
}

gfxFloat
PropertyProvider::GetHyphenWidth()
{
  if (mHyphenWidth < 0) {
    gfxTextRunCache::AutoTextRun hyphenTextRun(GetHyphenTextRun(mTextRun, nsnull, mFrame));
    mHyphenWidth = mLetterSpacing;
    if (hyphenTextRun.get()) {
      mHyphenWidth += hyphenTextRun->GetAdvanceWidth(0, hyphenTextRun->GetLength(), nsnull);
    }
  }
  return mHyphenWidth;
}

void
PropertyProvider::GetHyphenationBreaks(PRUint32 aStart, PRUint32 aLength,
                                       PRPackedBool* aBreakBefore)
{
  NS_PRECONDITION(IsInBounds(mStart, mLength, aStart, aLength), "Range out of bounds");
  NS_PRECONDITION(mLength != PR_INT32_MAX, "Can't call this with undefined length");

  if (!mTextStyle->WhiteSpaceCanWrap()) {
    memset(aBreakBefore, PR_FALSE, aLength);
    return;
  }

  
  nsSkipCharsRunIterator
    run(mStart, nsSkipCharsRunIterator::LENGTH_UNSKIPPED_ONLY, aLength);
  run.SetSkippedOffset(aStart);
  
  run.SetVisitSkipped();

  PRInt32 prevTrailingCharOffset = run.GetPos().GetOriginalOffset() - 1;
  PRBool allowHyphenBreakBeforeNextChar =
    prevTrailingCharOffset >= mStart.GetOriginalOffset() &&
    prevTrailingCharOffset < mStart.GetOriginalOffset() + mLength &&
    mFrag->CharAt(prevTrailingCharOffset) == CH_SHY;

  while (run.NextRun()) {
    NS_ASSERTION(run.GetRunLength() > 0, "Shouldn't return zero-length runs");
    if (run.IsSkipped()) {
      
      
      
      allowHyphenBreakBeforeNextChar =
        mFrag->CharAt(run.GetOriginalOffset() + run.GetRunLength() - 1) == CH_SHY;
    } else {
      PRInt32 runOffsetInSubstring = run.GetSkippedOffset() - aStart;
      memset(aBreakBefore + runOffsetInSubstring, 0, run.GetRunLength());
      
      aBreakBefore[runOffsetInSubstring] = allowHyphenBreakBeforeNextChar &&
          (!(mFrame->GetStateBits() & TEXT_START_OF_LINE) ||
           run.GetSkippedOffset() > mStart.GetSkippedOffset());
      allowHyphenBreakBeforeNextChar = PR_FALSE;
    }
  }
}

void
PropertyProvider::InitializeForDisplay(PRBool aTrimAfter)
{
  nsTextFrame::TrimmedOffsets trimmed =
    mFrame->GetTrimmedOffsets(mFrag, aTrimAfter);
  mStart.SetOriginalOffset(trimmed.mStart);
  mLength = trimmed.mLength;
  SetupJustificationSpacing();
}

static PRUint32 GetSkippedDistance(const gfxSkipCharsIterator& aStart,
                                   const gfxSkipCharsIterator& aEnd)
{
  return aEnd.GetSkippedOffset() - aStart.GetSkippedOffset();
}

void
PropertyProvider::FindJustificationRange(gfxSkipCharsIterator* aStart,
                                         gfxSkipCharsIterator* aEnd)
{
  NS_PRECONDITION(mLength != PR_INT32_MAX, "Can't call this with undefined length");
  NS_ASSERTION(aStart && aEnd, "aStart or/and aEnd is null");

  aStart->SetOriginalOffset(mStart.GetOriginalOffset());
  aEnd->SetOriginalOffset(mStart.GetOriginalOffset() + mLength);

  
  if (mFrame->GetStateBits() & TEXT_START_OF_LINE) {
    while (aStart->GetOriginalOffset() < aEnd->GetOriginalOffset()) {
      aStart->AdvanceOriginal(1);
      if (!aStart->IsOriginalCharSkipped() &&
          mTextRun->IsClusterStart(aStart->GetSkippedOffset()))
        break;
    }
  }

  
  if (mFrame->GetStateBits() & TEXT_END_OF_LINE) {
    while (aEnd->GetOriginalOffset() > aStart->GetOriginalOffset()) {
      aEnd->AdvanceOriginal(-1);
      if (!aEnd->IsOriginalCharSkipped() &&
          mTextRun->IsClusterStart(aEnd->GetSkippedOffset()))
        break;
    }
  }
}

void
PropertyProvider::SetupJustificationSpacing()
{
  NS_PRECONDITION(mLength != PR_INT32_MAX, "Can't call this with undefined length");

  if (!(mFrame->GetStateBits() & TEXT_JUSTIFICATION_ENABLED))
    return;

  gfxSkipCharsIterator start(mStart), end(mStart);
  
  
  
  nsTextFrame::TrimmedOffsets trimmed =
    mFrame->GetTrimmedOffsets(mFrag, PR_TRUE);
  end.AdvanceOriginal(trimmed.mLength);
  gfxSkipCharsIterator realEnd(end);
  FindJustificationRange(&start, &end);

  PRInt32 justifiableCharacters =
    ComputeJustifiableCharacters(start.GetOriginalOffset(),
                                 end.GetOriginalOffset() - start.GetOriginalOffset());
  if (justifiableCharacters == 0) {
    
    
    return;
  }

  gfxFloat naturalWidth =
    mTextRun->GetAdvanceWidth(mStart.GetSkippedOffset(),
                              GetSkippedDistance(mStart, realEnd), this);
  if (mFrame->GetStateBits() & TEXT_HYPHEN_BREAK) {
    gfxTextRunCache::AutoTextRun hyphenTextRun(GetHyphenTextRun(mTextRun, nsnull, mFrame));
    if (hyphenTextRun.get()) {
      naturalWidth +=
        hyphenTextRun->GetAdvanceWidth(0, hyphenTextRun->GetLength(), nsnull);
    }
  }
  gfxFloat totalJustificationSpace = mFrame->GetSize().width - naturalWidth;
  if (totalJustificationSpace <= 0) {
    
    return;
  }
  
  mJustificationSpacing = totalJustificationSpace/justifiableCharacters;
}





class nsBlinkTimer : public nsITimerCallback
{
public:
  nsBlinkTimer();
  virtual ~nsBlinkTimer();

  NS_DECL_ISUPPORTS

  void AddFrame(nsPresContext* aPresContext, nsIFrame* aFrame);

  PRBool RemoveFrame(nsIFrame* aFrame);

  PRInt32 FrameCount();

  void Start();

  void Stop();

  NS_DECL_NSITIMERCALLBACK

  static nsresult AddBlinkFrame(nsPresContext* aPresContext, nsIFrame* aFrame);
  static nsresult RemoveBlinkFrame(nsIFrame* aFrame);
  
  static PRBool   GetBlinkIsOff() { return sState == 3; }
  
protected:

  struct FrameData {
    nsPresContext* mPresContext;  
    nsIFrame*       mFrame;


    FrameData(nsPresContext* aPresContext,
              nsIFrame*       aFrame)
      : mPresContext(aPresContext), mFrame(aFrame) {}
  };

  class FrameDataComparator {
    public:
      PRBool Equals(const FrameData& aTimer, nsIFrame* const& aFrame) const {
        return aTimer.mFrame == aFrame;
      }
  };

  nsCOMPtr<nsITimer> mTimer;
  nsTArray<FrameData> mFrames;
  nsPresContext* mPresContext;

protected:

  static nsBlinkTimer* sTextBlinker;
  static PRUint32      sState; 
  
};

nsBlinkTimer* nsBlinkTimer::sTextBlinker = nsnull;
PRUint32      nsBlinkTimer::sState = 0;

#ifdef NOISY_BLINK
static PRTime gLastTick;
#endif

nsBlinkTimer::nsBlinkTimer()
{
}

nsBlinkTimer::~nsBlinkTimer()
{
  Stop();
  sTextBlinker = nsnull;
}

void nsBlinkTimer::Start()
{
  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_OK == rv) {
    mTimer->InitWithCallback(this, 250, nsITimer::TYPE_REPEATING_PRECISE);
  }
}

void nsBlinkTimer::Stop()
{
  if (nsnull != mTimer) {
    mTimer->Cancel();
    mTimer = nsnull;
  }
}

NS_IMPL_ISUPPORTS1(nsBlinkTimer, nsITimerCallback)

void nsBlinkTimer::AddFrame(nsPresContext* aPresContext, nsIFrame* aFrame) {
  mFrames.AppendElement(FrameData(aPresContext, aFrame));
  if (1 == mFrames.Length()) {
    Start();
  }
}

PRBool nsBlinkTimer::RemoveFrame(nsIFrame* aFrame) {
  mFrames.RemoveElement(aFrame, FrameDataComparator());
  
  if (mFrames.IsEmpty()) {
    Stop();
  }
  return PR_TRUE;
}

PRInt32 nsBlinkTimer::FrameCount() {
  return PRInt32(mFrames.Length());
}

NS_IMETHODIMP nsBlinkTimer::Notify(nsITimer *timer)
{
  
  
  
  sState = (sState + 1) % 4;
  if (sState == 1 || sState == 2)
    
    return NS_OK;

#ifdef NOISY_BLINK
  PRTime now = PR_Now();
  char buf[50];
  PRTime delta;
  LL_SUB(delta, now, gLastTick);
  gLastTick = now;
  PR_snprintf(buf, sizeof(buf), "%lldusec", delta);
  printf("%s\n", buf);
#endif

  PRUint32 i, n = mFrames.Length();
  for (i = 0; i < n; i++) {
    FrameData& frameData = mFrames.ElementAt(i);

    
    
    nsRect bounds(nsPoint(0, 0), frameData.mFrame->GetSize());
    frameData.mFrame->Invalidate(bounds);
  }
  return NS_OK;
}



nsresult nsBlinkTimer::AddBlinkFrame(nsPresContext* aPresContext, nsIFrame* aFrame)
{
  if (!sTextBlinker)
  {
    sTextBlinker = new nsBlinkTimer;
    if (!sTextBlinker) return NS_ERROR_OUT_OF_MEMORY;
  }
  
  NS_ADDREF(sTextBlinker);

  sTextBlinker->AddFrame(aPresContext, aFrame);
  return NS_OK;
}



nsresult nsBlinkTimer::RemoveBlinkFrame(nsIFrame* aFrame)
{
  NS_ASSERTION(sTextBlinker, "Should have blink timer here");
  
  nsBlinkTimer* blinkTimer = sTextBlinker;    
  if (!blinkTimer) return NS_OK;
  
  blinkTimer->RemoveFrame(aFrame);  
  NS_RELEASE(blinkTimer);
  
  return NS_OK;
}



static nscolor
EnsureDifferentColors(nscolor colorA, nscolor colorB)
{
  if (colorA == colorB) {
    nscolor res;
    res = NS_RGB(NS_GET_R(colorA) ^ 0xff,
                 NS_GET_G(colorA) ^ 0xff,
                 NS_GET_B(colorA) ^ 0xff);
    return res;
  }
  return colorA;
}



static nscolor
DarkenColor(nscolor aColor)
{
  PRUint16  hue, sat, value;
  PRUint8 alpha;

  
  NS_RGB2HSV(aColor, hue, sat, value, alpha);

  
  
  
  
  
  
  if (value > sat) {
    value = sat;
    
    NS_HSV2RGB(aColor, hue, sat, value, alpha);
  }
  return aColor;
}




static PRBool
ShouldDarkenColors(nsPresContext* aPresContext)
{
  return !aPresContext->GetBackgroundColorDraw() &&
    !aPresContext->GetBackgroundImageDraw();
}

nsTextPaintStyle::nsTextPaintStyle(nsTextFrame* aFrame)
  : mFrame(aFrame),
    mPresContext(aFrame->PresContext()),
    mInitCommonColors(PR_FALSE),
    mInitSelectionColors(PR_FALSE)
{
  for (PRUint32 i = 0; i < NS_ARRAY_LENGTH(mSelectionStyle); i++)
    mSelectionStyle[i].mInit = PR_FALSE;
}

PRBool
nsTextPaintStyle::EnsureSufficientContrast(nscolor *aForeColor, nscolor *aBackColor)
{
  InitCommonColors();

  
  
  PRInt32 backLuminosityDifference =
            NS_LUMINOSITY_DIFFERENCE(*aBackColor, mFrameBackgroundColor);
  if (backLuminosityDifference >= mSufficientContrast)
    return PR_FALSE;

  
  
  PRInt32 foreLuminosityDifference =
            NS_LUMINOSITY_DIFFERENCE(*aForeColor, mFrameBackgroundColor);
  if (backLuminosityDifference < foreLuminosityDifference) {
    nscolor tmpColor = *aForeColor;
    *aForeColor = *aBackColor;
    *aBackColor = tmpColor;
    return PR_TRUE;
  }
  return PR_FALSE;
}

nscolor
nsTextPaintStyle::GetTextColor()
{
  nscolor color = mFrame->GetStyleColor()->mColor;
  if (ShouldDarkenColors(mPresContext)) {
    color = DarkenColor(color);
  }
  return color;
}

PRBool
nsTextPaintStyle::GetSelectionColors(nscolor* aForeColor,
                                     nscolor* aBackColor)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");

  if (!InitSelectionColors())
    return PR_FALSE;

  *aForeColor = mSelectionTextColor;
  *aBackColor = mSelectionBGColor;
  return PR_TRUE;
}

void
nsTextPaintStyle::GetHighlightColors(nscolor* aForeColor,
                                     nscolor* aBackColor)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");
  
  nsILookAndFeel* look = mPresContext->LookAndFeel();
  nscolor foreColor, backColor;
  look->GetColor(nsILookAndFeel::eColor_TextHighlightBackground,
                 backColor);
  look->GetColor(nsILookAndFeel::eColor_TextHighlightForeground,
                 foreColor);
  EnsureSufficientContrast(&foreColor, &backColor);
  *aForeColor = foreColor;
  *aBackColor = backColor;
}

void
nsTextPaintStyle::GetIMESelectionColors(PRInt32  aIndex,
                                        nscolor* aForeColor,
                                        nscolor* aBackColor)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");
  NS_ASSERTION(aIndex >= 0 && aIndex < 5, "Index out of range");

  nsSelectionStyle* selectionStyle = GetSelectionStyle(aIndex);
  *aForeColor = selectionStyle->mTextColor;
  *aBackColor = selectionStyle->mBGColor;
}

PRBool
nsTextPaintStyle::GetSelectionUnderlineForPaint(PRInt32  aIndex,
                                                nscolor* aLineColor,
                                                float*   aRelativeSize,
                                                PRUint8* aStyle)
{
  NS_ASSERTION(aLineColor, "aLineColor is null");
  NS_ASSERTION(aRelativeSize, "aRelativeSize is null");
  NS_ASSERTION(aIndex >= 0 && aIndex < 5, "Index out of range");

  nsSelectionStyle* selectionStyle = GetSelectionStyle(aIndex);
  if (selectionStyle->mUnderlineStyle == NS_STYLE_BORDER_STYLE_NONE ||
      selectionStyle->mUnderlineColor == NS_TRANSPARENT ||
      selectionStyle->mUnderlineRelativeSize <= 0.0f)
    return PR_FALSE;

  *aLineColor = selectionStyle->mUnderlineColor;
  *aRelativeSize = selectionStyle->mUnderlineRelativeSize;
  *aStyle = selectionStyle->mUnderlineStyle;
  return PR_TRUE;
}

void
nsTextPaintStyle::InitCommonColors()
{
  if (mInitCommonColors)
    return;

  nsStyleContext* sc = mFrame->GetStyleContext();

  nsStyleContext* bgContext =
    nsCSSRendering::FindNonTransparentBackground(sc);
  NS_ASSERTION(bgContext, "Cannot find NonTransparentBackground.");
  const nsStyleBackground* bg = bgContext->GetStyleBackground();

  nscolor defaultBgColor = mPresContext->DefaultBackgroundColor();
  mFrameBackgroundColor = NS_ComposeColors(defaultBgColor,
                                           bg->mBackgroundColor);

  if (bgContext->GetStyleDisplay()->mAppearance) {
    
    mSufficientContrast = 0;
    mInitCommonColors = PR_TRUE;
    return;
  }

  NS_ASSERTION(NS_GET_A(defaultBgColor) == 255,
               "default background color is not opaque");

  nsILookAndFeel* look = mPresContext->LookAndFeel();
  nscolor defaultWindowBackgroundColor, selectionTextColor, selectionBGColor;
  look->GetColor(nsILookAndFeel::eColor_TextSelectBackground,
                 selectionBGColor);
  look->GetColor(nsILookAndFeel::eColor_TextSelectForeground,
                 selectionTextColor);
  look->GetColor(nsILookAndFeel::eColor_WindowBackground,
                 defaultWindowBackgroundColor);

  mSufficientContrast =
    NS_MIN(NS_MIN(NS_SUFFICIENT_LUMINOSITY_DIFFERENCE,
                  NS_LUMINOSITY_DIFFERENCE(selectionTextColor,
                                           selectionBGColor)),
                  NS_LUMINOSITY_DIFFERENCE(defaultWindowBackgroundColor,
                                           selectionBGColor));

  mInitCommonColors = PR_TRUE;
}

static nsIContent*
FindElementAncestor(nsINode* aNode)
{
  while (aNode && !aNode->IsNodeOfType(nsINode::eELEMENT)) {
    aNode = aNode->GetParent();
  }
  return static_cast<nsIContent*>(aNode);
}

PRBool
nsTextPaintStyle::InitSelectionColors()
{
  if (mInitSelectionColors)
    return PR_TRUE;

  PRInt16 selectionFlags;
  PRInt16 selectionStatus = mFrame->GetSelectionStatus(&selectionFlags);
  if (!(selectionFlags & nsISelectionDisplay::DISPLAY_TEXT) ||
      selectionStatus < nsISelectionController::SELECTION_ON) {
    
    
    
    return PR_FALSE;
  }

  mInitSelectionColors = PR_TRUE;

  nsIFrame* nonGeneratedAncestor = nsLayoutUtils::GetNonGeneratedAncestor(mFrame);
  nsIContent* selectionContent = FindElementAncestor(nonGeneratedAncestor->GetContent());

  if (selectionContent &&
      selectionStatus == nsISelectionController::SELECTION_ON) {
    nsRefPtr<nsStyleContext> sc = nsnull;
    sc = mPresContext->StyleSet()->
      ProbePseudoStyleFor(selectionContent, nsCSSPseudoElements::mozSelection,
                          mFrame->GetStyleContext());
    
    if (sc) {
      const nsStyleBackground* bg = sc->GetStyleBackground();
      mSelectionBGColor = bg->mBackgroundColor;
      mSelectionTextColor = sc->GetStyleColor()->mColor;
      return PR_TRUE;
    }
  }

  nsILookAndFeel* look = mPresContext->LookAndFeel();

  nscolor selectionBGColor;
  look->GetColor(nsILookAndFeel::eColor_TextSelectBackground,
                 selectionBGColor);

  if (selectionStatus == nsISelectionController::SELECTION_ATTENTION) {
    look->GetColor(nsILookAndFeel::eColor_TextSelectBackgroundAttention,
                   mSelectionBGColor);
    mSelectionBGColor  = EnsureDifferentColors(mSelectionBGColor,
                                               selectionBGColor);
  } else if (selectionStatus != nsISelectionController::SELECTION_ON) {
    look->GetColor(nsILookAndFeel::eColor_TextSelectBackgroundDisabled,
                   mSelectionBGColor);
    mSelectionBGColor  = EnsureDifferentColors(mSelectionBGColor,
                                               selectionBGColor);
  } else {
    mSelectionBGColor = selectionBGColor;
  }

  look->GetColor(nsILookAndFeel::eColor_TextSelectForeground,
                 mSelectionTextColor);

  
  if (mSelectionTextColor == NS_DONT_CHANGE_COLOR) {
    mSelectionTextColor = EnsureDifferentColors(mFrame->GetStyleColor()->mColor,
                                                mSelectionBGColor);
  } else {
    EnsureSufficientContrast(&mSelectionTextColor, &mSelectionBGColor);
  }
  return PR_TRUE;
}

nsTextPaintStyle::nsSelectionStyle*
nsTextPaintStyle::GetSelectionStyle(PRInt32 aIndex)
{
  InitSelectionStyle(aIndex);
  return &mSelectionStyle[aIndex];
}

struct StyleIDs {
  nsILookAndFeel::nsColorID mForeground, mBackground, mLine;
  nsILookAndFeel::nsMetricID mLineStyle;
  nsILookAndFeel::nsMetricFloatID mLineRelativeSize;
};
static StyleIDs SelectionStyleIDs[] = {
  { nsILookAndFeel::eColor_IMERawInputForeground,
    nsILookAndFeel::eColor_IMERawInputBackground,
    nsILookAndFeel::eColor_IMERawInputUnderline,
    nsILookAndFeel::eMetric_IMERawInputUnderlineStyle,
    nsILookAndFeel::eMetricFloat_IMEUnderlineRelativeSize },
  { nsILookAndFeel::eColor_IMESelectedRawTextForeground,
    nsILookAndFeel::eColor_IMESelectedRawTextBackground,
    nsILookAndFeel::eColor_IMESelectedRawTextUnderline,
    nsILookAndFeel::eMetric_IMESelectedRawTextUnderlineStyle,
    nsILookAndFeel::eMetricFloat_IMEUnderlineRelativeSize },
  { nsILookAndFeel::eColor_IMEConvertedTextForeground,
    nsILookAndFeel::eColor_IMEConvertedTextBackground,
    nsILookAndFeel::eColor_IMEConvertedTextUnderline,
    nsILookAndFeel::eMetric_IMEConvertedTextUnderlineStyle,
    nsILookAndFeel::eMetricFloat_IMEUnderlineRelativeSize },
  { nsILookAndFeel::eColor_IMESelectedConvertedTextForeground,
    nsILookAndFeel::eColor_IMESelectedConvertedTextBackground,
    nsILookAndFeel::eColor_IMESelectedConvertedTextUnderline,
    nsILookAndFeel::eMetric_IMESelectedConvertedTextUnderline,
    nsILookAndFeel::eMetricFloat_IMEUnderlineRelativeSize },
  { nsILookAndFeel::eColor_LAST_COLOR,
    nsILookAndFeel::eColor_LAST_COLOR,
    nsILookAndFeel::eColor_SpellCheckerUnderline,
    nsILookAndFeel::eMetric_SpellCheckerUnderlineStyle,
    nsILookAndFeel::eMetricFloat_SpellCheckerUnderlineRelativeSize }
};

static PRUint8 sUnderlineStyles[] = {
  nsCSSRendering::DECORATION_STYLE_NONE,   
  nsCSSRendering::DECORATION_STYLE_DOTTED, 
  nsCSSRendering::DECORATION_STYLE_DASHED, 
  nsCSSRendering::DECORATION_STYLE_SOLID,  
  nsCSSRendering::DECORATION_STYLE_DOUBLE, 
  nsCSSRendering::DECORATION_STYLE_WAVY    
};

void
nsTextPaintStyle::InitSelectionStyle(PRInt32 aIndex)
{
  NS_ASSERTION(aIndex >= 0 && aIndex < 5, "aIndex is invalid");
  nsSelectionStyle* selectionStyle = &mSelectionStyle[aIndex];
  if (selectionStyle->mInit)
    return;

  StyleIDs* styleIDs = &SelectionStyleIDs[aIndex];

  nsILookAndFeel* look = mPresContext->LookAndFeel();
  nscolor foreColor, backColor;
  if (styleIDs->mForeground == nsILookAndFeel::eColor_LAST_COLOR) {
    foreColor = NS_SAME_AS_FOREGROUND_COLOR;
  } else {
    look->GetColor(styleIDs->mForeground, foreColor);
  }
  if (styleIDs->mBackground == nsILookAndFeel::eColor_LAST_COLOR) {
    backColor = NS_TRANSPARENT;
  } else {
    look->GetColor(styleIDs->mBackground, backColor);
  }

  
  NS_ASSERTION(foreColor != NS_TRANSPARENT,
               "foreColor cannot be NS_TRANSPARENT");
  NS_ASSERTION(backColor != NS_SAME_AS_FOREGROUND_COLOR,
               "backColor cannot be NS_SAME_AS_FOREGROUND_COLOR");
  NS_ASSERTION(backColor != NS_40PERCENT_FOREGROUND_COLOR,
               "backColor cannot be NS_40PERCENT_FOREGROUND_COLOR");

  foreColor = GetResolvedForeColor(foreColor, GetTextColor(), backColor);

  if (NS_GET_A(backColor) > 0)
    EnsureSufficientContrast(&foreColor, &backColor);

  nscolor lineColor;
  float relativeSize;
  PRUint8 lineStyle;
  GetSelectionUnderline(mPresContext, aIndex,
                        &lineColor, &relativeSize, &lineStyle);
  lineColor = GetResolvedForeColor(lineColor, foreColor, backColor);

  selectionStyle->mTextColor       = foreColor;
  selectionStyle->mBGColor         = backColor;
  selectionStyle->mUnderlineColor  = lineColor;
  selectionStyle->mUnderlineStyle  = lineStyle;
  selectionStyle->mUnderlineRelativeSize = relativeSize;
  selectionStyle->mInit            = PR_TRUE;
}

 PRBool
nsTextPaintStyle::GetSelectionUnderline(nsPresContext* aPresContext,
                                        PRInt32 aIndex,
                                        nscolor* aLineColor,
                                        float* aRelativeSize,
                                        PRUint8* aStyle)
{
  NS_ASSERTION(aPresContext, "aPresContext is null");
  NS_ASSERTION(aRelativeSize, "aRelativeSize is null");
  NS_ASSERTION(aStyle, "aStyle is null");
  NS_ASSERTION(aIndex >= 0 && aIndex < 5, "Index out of range");

  nsILookAndFeel* look = aPresContext->LookAndFeel();

  StyleIDs& styleID = SelectionStyleIDs[aIndex];
  nscolor color;
  float size;
  PRInt32 style;

  look->GetColor(styleID.mLine, color);
  look->GetMetric(styleID.mLineStyle, style);
  if (!NS_IS_VALID_UNDERLINE_STYLE(style)) {
    NS_ERROR("Invalid underline style value is specified");
    style = NS_UNDERLINE_STYLE_SOLID;
  }
  look->GetMetric(styleID.mLineRelativeSize, size);

  NS_ASSERTION(size, "selection underline relative size must be larger than 0");

  if (aLineColor) {
    *aLineColor = color;
  }
  *aRelativeSize = size;
  *aStyle = sUnderlineStyles[style];

  return sUnderlineStyles[style] != nsCSSRendering::DECORATION_STYLE_NONE &&
         color != NS_TRANSPARENT &&
         size > 0.0f;
}

inline nscolor Get40PercentColor(nscolor aForeColor, nscolor aBackColor)
{
  nscolor foreColor = NS_RGBA(NS_GET_R(aForeColor),
                              NS_GET_G(aForeColor),
                              NS_GET_B(aForeColor),
                              (PRUint8)(255 * 0.4f));
  
  return NS_ComposeColors(aBackColor, foreColor);
}

nscolor
nsTextPaintStyle::GetResolvedForeColor(nscolor aColor,
                                       nscolor aDefaultForeColor,
                                       nscolor aBackColor)
{
  if (aColor == NS_SAME_AS_FOREGROUND_COLOR)
    return aDefaultForeColor;

  if (aColor != NS_40PERCENT_FOREGROUND_COLOR)
    return aColor;

  
  nscolor actualBGColor = aBackColor;
  if (actualBGColor == NS_TRANSPARENT) {
    InitCommonColors();
    actualBGColor = mFrameBackgroundColor;
  }
  return Get40PercentColor(aDefaultForeColor, actualBGColor);
}



#ifdef ACCESSIBILITY
NS_IMETHODIMP nsTextFrame::GetAccessible(nsIAccessible** aAccessible)
{
  if (IsEmpty()) {
    nsAutoString renderedWhitespace;
    GetRenderedText(&renderedWhitespace, nsnull, nsnull, 0, 1);
    if (renderedWhitespace.IsEmpty()) {
      return NS_ERROR_FAILURE;
    }
  }

  nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

  if (accService) {
    return accService->CreateHTMLTextAccessible(static_cast<nsIFrame*>(this), aAccessible);
  }
  return NS_ERROR_FAILURE;
}
#endif



NS_IMETHODIMP
nsTextFrame::Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow)
{
  NS_ASSERTION(!aPrevInFlow, "Can't be a continuation!");
  NS_PRECONDITION(aContent->IsNodeOfType(nsINode::eTEXT),
                  "Bogus content!");

  if (!PresContext()->IsDynamic()) {
    AddStateBits(TEXT_BLINK_ON_OR_PRINTING);
  }

  
  aContent->UnsetFlags(NS_CREATE_FRAME_IF_NON_WHITESPACE);
  
  
  return nsFrame::Init(aContent, aParent, aPrevInFlow);
}

void
nsTextFrame::Destroy()
{
  
  
  
  ClearTextRun();
  if (mNextContinuation) {
    mNextContinuation->SetPrevInFlow(nsnull);
  }
  
  nsFrame::Destroy();
}

class nsContinuingTextFrame : public nsTextFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();

  virtual nsIFrame* GetPrevContinuation() const {
    return mPrevContinuation;
  }
  NS_IMETHOD SetPrevContinuation(nsIFrame* aPrevContinuation) {
    NS_ASSERTION (!aPrevContinuation || GetType() == aPrevContinuation->GetType(),
                  "setting a prev continuation with incorrect type!");
    NS_ASSERTION (!nsSplittableFrame::IsInPrevContinuationChain(aPrevContinuation, this),
                  "creating a loop in continuation chain!");
    mPrevContinuation = aPrevContinuation;
    RemoveStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
    return NS_OK;
  }
  virtual nsIFrame* GetPrevInFlowVirtual() const { return GetPrevInFlow(); }
  nsIFrame* GetPrevInFlow() const {
    return (GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? mPrevContinuation : nsnull;
  }
  NS_IMETHOD SetPrevInFlow(nsIFrame* aPrevInFlow) {
    NS_ASSERTION (!aPrevInFlow || GetType() == aPrevInFlow->GetType(),
                  "setting a prev in flow with incorrect type!");
    NS_ASSERTION (!nsSplittableFrame::IsInPrevContinuationChain(aPrevInFlow, this),
                  "creating a loop in continuation chain!");
    mPrevContinuation = aPrevInFlow;
    AddStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
    return NS_OK;
  }
  virtual nsIFrame* GetFirstInFlow() const;
  virtual nsIFrame* GetFirstContinuation() const;

  virtual void AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  
  virtual nsresult GetRenderedText(nsAString* aString = nsnull,
                                   gfxSkipChars* aSkipChars = nsnull,
                                   gfxSkipCharsIterator* aSkipIter = nsnull,
                                   PRUint32 aSkippedStartOffset = 0,
                                   PRUint32 aSkippedMaxLength = PR_UINT32_MAX)
  { return NS_ERROR_NOT_IMPLEMENTED; } 

protected:
  nsContinuingTextFrame(nsStyleContext* aContext) : nsTextFrame(aContext) {}
  nsIFrame* mPrevContinuation;
};

NS_IMETHODIMP
nsContinuingTextFrame::Init(nsIContent* aContent,
                            nsIFrame*   aParent,
                            nsIFrame*   aPrevInFlow)
{
  NS_ASSERTION(aPrevInFlow, "Must be a continuation!");

  if (!PresContext()->IsDynamic()) {
    AddStateBits(TEXT_BLINK_ON_OR_PRINTING);
  }

  
  nsresult rv = nsFrame::Init(aContent, aParent, aPrevInFlow);

#ifdef IBMBIDI
  nsTextFrame* nextContinuation =
    static_cast<nsTextFrame*>(aPrevInFlow->GetNextContinuation());
#endif 
  
  SetPrevInFlow(aPrevInFlow);
  aPrevInFlow->SetNextInFlow(this);
  nsTextFrame* prev = static_cast<nsTextFrame*>(aPrevInFlow);
  mContentOffset = prev->GetContentOffset() + prev->GetContentLengthHint();
  NS_ASSERTION(mContentOffset < PRInt32(GetFragment()->GetLength()),
               "Creating ContinuingTextFrame, but there is no more content");
  if (prev->GetStyleContext() != GetStyleContext()) {
    
    
    prev->ClearTextRun();
  } else {
    mTextRun = prev->GetTextRun();
  }
#ifdef IBMBIDI
  if (aPrevInFlow->GetStateBits() & NS_FRAME_IS_BIDI) {
    nsPropertyTable *propTable = PresContext()->PropertyTable();
    propTable->SetProperty(this, nsGkAtoms::embeddingLevel,
          propTable->GetProperty(aPrevInFlow, nsGkAtoms::embeddingLevel),
                           nsnull, nsnull);
    propTable->SetProperty(this, nsGkAtoms::baseLevel,
              propTable->GetProperty(aPrevInFlow, nsGkAtoms::baseLevel),
                           nsnull, nsnull);
    propTable->SetProperty(this, nsGkAtoms::charType,
               propTable->GetProperty(aPrevInFlow, nsGkAtoms::charType),
                           nsnull, nsnull);
    if (nextContinuation) {
      SetNextContinuation(nextContinuation);
      nextContinuation->SetPrevContinuation(this);
      
      while (nextContinuation &&
             nextContinuation->GetContentOffset() < mContentOffset) {
        NS_ASSERTION(
          propTable->GetProperty(this, nsGkAtoms::embeddingLevel) ==
          propTable->GetProperty(nextContinuation, nsGkAtoms::embeddingLevel) &&
          propTable->GetProperty(this, nsGkAtoms::baseLevel) ==
          propTable->GetProperty(nextContinuation, nsGkAtoms::baseLevel) &&
          propTable->GetProperty(this, nsGkAtoms::charType) ==
          propTable->GetProperty(nextContinuation, nsGkAtoms::charType),
            "stealing text from different type of BIDI continuation");
        nextContinuation->mContentOffset = mContentOffset;
        nextContinuation = static_cast<nsTextFrame*>(nextContinuation->GetNextContinuation());
      }
    }
    mState |= NS_FRAME_IS_BIDI;
  } 
#endif 

  return rv;
}

void
nsContinuingTextFrame::Destroy()
{
  
  
  
  
  
  
  
  
  
  if ((GetStateBits() & TEXT_IN_TEXTRUN_USER_DATA) ||
      !mPrevContinuation ||
      mPrevContinuation->GetStyleContext() != GetStyleContext()) {
    ClearTextRun();
    
    
    if (mPrevContinuation) {
      (static_cast<nsTextFrame*>(mPrevContinuation))->ClearTextRun();
    }
  }
  nsSplittableFrame::RemoveFromFlow(this);
  
  nsFrame::Destroy();
}

nsIFrame*
nsContinuingTextFrame::GetFirstInFlow() const
{
  
  nsIFrame *firstInFlow,
           *previous = const_cast<nsIFrame*>
                                 (static_cast<const nsIFrame*>(this));
  do {
    firstInFlow = previous;
    previous = firstInFlow->GetPrevInFlow();
  } while (previous);
  return firstInFlow;
}

nsIFrame*
nsContinuingTextFrame::GetFirstContinuation() const
{
  
  nsIFrame *firstContinuation,
  *previous = const_cast<nsIFrame*>
                        (static_cast<const nsIFrame*>(mPrevContinuation));

  NS_ASSERTION(previous, "How can an nsContinuingTextFrame be the first continuation?");

  do {
    firstContinuation = previous;
    previous = firstContinuation->GetPrevContinuation();
  } while (previous);
  return firstContinuation;
}












 nscoord
nsTextFrame::GetMinWidth(nsIRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::MinWidthFromInline(this, aRenderingContext);
}


 nscoord
nsTextFrame::GetPrefWidth(nsIRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::PrefWidthFromInline(this, aRenderingContext);
}

 void
nsContinuingTextFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                         InlineMinWidthData *aData)
{
  
  return;
}

 void
nsContinuingTextFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                          InlinePrefWidthData *aData)
{
  
  return;
}

static void 
DestroySelectionDetails(SelectionDetails* aDetails)
{
  while (aDetails) {
    SelectionDetails* next = aDetails->mNext;
    delete aDetails;
    aDetails = next;
  }
}



#if defined(DEBUG_rbs) || defined(DEBUG_bzbarsky)
static void
VerifyNotDirty(nsFrameState state)
{
  PRBool isZero = state & NS_FRAME_FIRST_REFLOW;
  PRBool isDirty = state & NS_FRAME_IS_DIRTY;
  if (!isZero && isDirty)
    NS_WARNING("internal offsets may be out-of-sync");
}
#define DEBUG_VERIFY_NOT_DIRTY(state) \
VerifyNotDirty(state)
#else
#define DEBUG_VERIFY_NOT_DIRTY(state)
#endif

nsIFrame*
NS_NewTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsTextFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsTextFrame)

nsIFrame*
NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsContinuingTextFrame(aContext);
}

NS_IMPL_FRAMEARENA_HELPERS(nsContinuingTextFrame)

nsTextFrame::~nsTextFrame()
{
  if (0 != (mState & TEXT_BLINK_ON_OR_PRINTING) && PresContext()->IsDynamic())
  {
    nsBlinkTimer::RemoveBlinkFrame(this);
  }
}

NS_IMETHODIMP
nsTextFrame::GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor)
{
  FillCursorInformationFromStyle(GetStyleUserInterface(), aCursor);  
  if (NS_STYLE_CURSOR_AUTO == aCursor.mCursor) {
    aCursor.mCursor = NS_STYLE_CURSOR_TEXT;

    
    nsIFrame *ancestorFrame = this;
    while ((ancestorFrame = ancestorFrame->GetParent()) != nsnull) {
      nsIContent *ancestorContent = ancestorFrame->GetContent();
      if (ancestorContent && ancestorContent->HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
        nsAutoString tabIndexStr;
        ancestorContent->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
        if (!tabIndexStr.IsEmpty()) {
          PRInt32 rv, tabIndexVal = tabIndexStr.ToInteger(&rv);
          if (NS_SUCCEEDED(rv) && tabIndexVal >= 0) {
            aCursor.mCursor = NS_STYLE_CURSOR_DEFAULT;
            break;
          }
        }
      }
    }
  }

  return NS_OK;
}

nsIFrame*
nsTextFrame::GetLastInFlow() const
{
  nsTextFrame* lastInFlow = const_cast<nsTextFrame*>(this);
  while (lastInFlow->GetNextInFlow())  {
    lastInFlow = static_cast<nsTextFrame*>(lastInFlow->GetNextInFlow());
  }
  NS_POSTCONDITION(lastInFlow, "illegal state in flow chain.");
  return lastInFlow;
}
nsIFrame*
nsTextFrame::GetLastContinuation() const
{
  nsTextFrame* lastInFlow = const_cast<nsTextFrame*>(this);
  while (lastInFlow->mNextContinuation)  {
    lastInFlow = static_cast<nsTextFrame*>(lastInFlow->mNextContinuation);
  }
  NS_POSTCONDITION(lastInFlow, "illegal state in continuation chain.");
  return lastInFlow;
}

void
nsTextFrame::ClearTextRun()
{
  
  gfxTextRun* textRun = mTextRun;
  
  if (!textRun)
    return;

  UnhookTextRunFromFrames(textRun);
  











  if (!(textRun->GetFlags() & gfxTextRunWordCache::TEXT_IN_CACHE)) {
    
    gTextRuns->RemoveFromCache(textRun);
    delete textRun;
  }
}

static void
ClearTextRunsInFlowChain(nsTextFrame* aFrame)
{
  nsTextFrame* f;
  for (f = aFrame; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
    f->ClearTextRun();
  }
}

NS_IMETHODIMP
nsTextFrame::CharacterDataChanged(CharacterDataChangeInfo* aInfo)
{
  ClearTextRunsInFlowChain(this);

  nsTextFrame* targetTextFrame;
  PRInt32 nodeLength = mContent->GetText()->GetLength();

  if (aInfo->mAppend) {
    targetTextFrame = static_cast<nsTextFrame*>(GetLastContinuation());
    targetTextFrame->mState &= ~TEXT_WHITESPACE_FLAGS;
  } else {
    
    
    
    
    nsTextFrame* textFrame = this;
    PRInt32 newLength = nodeLength;
    do {
      textFrame->mState &= ~TEXT_WHITESPACE_FLAGS;
      
      if (textFrame->mContentOffset > newLength) {
        textFrame->mContentOffset = newLength;
      }
      textFrame = static_cast<nsTextFrame*>(textFrame->GetNextContinuation());
      if (!textFrame) {
        break;
      }
      textFrame->mState |= NS_FRAME_IS_DIRTY;
    } while (1);
    targetTextFrame = this;
  }

  
  PresContext()->GetPresShell()->FrameNeedsReflow(targetTextFrame,
                                                  nsIPresShell::eStyleChange,
                                                  NS_FRAME_IS_DIRTY);

  return NS_OK;
}

 void
nsTextFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsFrame::DidSetStyleContext(aOldStyleContext);
  ClearTextRun();
} 

class nsDisplayText : public nsDisplayItem {
public:
  nsDisplayText(nsTextFrame* aFrame) : nsDisplayItem(aFrame) {
    MOZ_COUNT_CTOR(nsDisplayText);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayText() {
    MOZ_COUNT_DTOR(nsDisplayText);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder) {
    return mFrame->GetOverflowRect() + aBuilder->ToReferenceFrame(mFrame);
  }
  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt,
                            HitTestState* aState) {
    return nsRect(aBuilder->ToReferenceFrame(mFrame), mFrame->GetSize()).Contains(aPt) ? mFrame : nsnull;
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("Text")
};

void
nsDisplayText::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect) {
  static_cast<nsTextFrame*>(mFrame)->
    PaintText(aCtx, aBuilder->ToReferenceFrame(mFrame), aDirtyRect);
}

NS_IMETHODIMP
nsTextFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return NS_OK;
  
  DO_GLOBAL_REFLOW_COUNT_DSP("nsTextFrame");

  if ((0 != (mState & TEXT_BLINK_ON_OR_PRINTING)) && nsBlinkTimer::GetBlinkIsOff() &&
      PresContext()->IsDynamic() && !aBuilder->IsForEventDelivery())
    return NS_OK;
    
  return aLists.Content()->AppendNewToTop(new (aBuilder) nsDisplayText(this));
}

static nsIFrame*
GetGeneratedContentOwner(nsIFrame* aFrame, PRBool* aIsBefore)
{
  *aIsBefore = PR_FALSE;
  while (aFrame && (aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT)) {
    if (aFrame->GetStyleContext()->GetPseudoType() == nsCSSPseudoElements::before) {
      *aIsBefore = PR_TRUE;
    }
    aFrame = aFrame->GetParent();
  }
  return aFrame;
}

SelectionDetails*
nsTextFrame::GetSelectionDetails()
{
  const nsFrameSelection* frameSelection = GetConstFrameSelection();
  if (!(GetStateBits() & NS_FRAME_GENERATED_CONTENT)) {
    SelectionDetails* details =
      frameSelection->LookUpSelection(mContent, GetContentOffset(),
                                      GetContentLength(), PR_FALSE);
    SelectionDetails* sd;
    for (sd = details; sd; sd = sd->mNext) {
      sd->mStart += mContentOffset;
      sd->mEnd += mContentOffset;
    }
    return details;
  }

  
  
  PRBool isBefore;
  nsIFrame* owner = GetGeneratedContentOwner(this, &isBefore);
  if (!owner || !owner->GetContent())
    return nsnull;

  SelectionDetails* details =
    frameSelection->LookUpSelection(owner->GetContent(),
        isBefore ? 0 : owner->GetContent()->GetChildCount(), 0, PR_FALSE);
  SelectionDetails* sd;
  for (sd = details; sd; sd = sd->mNext) {
    
    sd->mStart = GetContentOffset();
    sd->mEnd = GetContentEnd();
  }
  return details;
}

static void
FillClippedRect(gfxContext* aCtx, nsPresContext* aPresContext,
                nscolor aColor, const gfxRect& aDirtyRect, const gfxRect& aRect)
{
  gfxRect r = aRect.Intersect(aDirtyRect);
  
  PRInt32 app = aPresContext->AppUnitsPerDevPixel();
  aCtx->NewPath();
  
  aCtx->Rectangle(gfxRect(r.X() / app, r.Y() / app,
                          r.Width() / app, r.Height() / app), PR_TRUE);
  aCtx->SetColor(gfxRGBA(aColor));
  aCtx->Fill();
}

nsTextFrame::TextDecorations
nsTextFrame::GetTextDecorations(nsPresContext* aPresContext)
{
  TextDecorations decorations;

  
  
  
  if (eCompatibility_NavQuirks != aPresContext->CompatibilityMode())
    return decorations;

  PRBool useOverride = PR_FALSE;
  nscolor overrideColor;

  
  PRUint8 decorMask = NS_STYLE_TEXT_DECORATION_UNDERLINE | 
                      NS_STYLE_TEXT_DECORATION_OVERLINE |
                      NS_STYLE_TEXT_DECORATION_LINE_THROUGH;

  for (nsStyleContext* context = GetStyleContext();
       decorMask && context && context->HasTextDecorations();
       context = context->GetParent()) {
    const nsStyleTextReset* styleText = context->GetStyleTextReset();
    if (!useOverride && 
        (NS_STYLE_TEXT_DECORATION_OVERRIDE_ALL & styleText->mTextDecoration)) {
      
      
      useOverride = PR_TRUE;
      overrideColor = context->GetStyleColor()->mColor;
    }

    PRUint8 useDecorations = decorMask & styleText->mTextDecoration;
    if (useDecorations) {
      nscolor color = context->GetStyleColor()->mColor;
  
      if (NS_STYLE_TEXT_DECORATION_UNDERLINE & useDecorations) {
        decorations.mUnderColor = useOverride ? overrideColor : color;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
        decorations.mDecorations |= NS_STYLE_TEXT_DECORATION_UNDERLINE;
      }
      if (NS_STYLE_TEXT_DECORATION_OVERLINE & useDecorations) {
        decorations.mOverColor = useOverride ? overrideColor : color;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_OVERLINE;
        decorations.mDecorations |= NS_STYLE_TEXT_DECORATION_OVERLINE;
      }
      if (NS_STYLE_TEXT_DECORATION_LINE_THROUGH & useDecorations) {
        decorations.mStrikeColor = useOverride ? overrideColor : color;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
        decorations.mDecorations |= NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
      }
    }
  }

  return decorations;
}

void
nsTextFrame::UnionTextDecorationOverflow(nsPresContext* aPresContext,
                                         PropertyProvider& aProvider,
                                         nsRect* aOverflowRect)
{
  
  nsRect shadowRect = nsLayoutUtils::GetTextShadowRectsUnion(*aOverflowRect, this);
  aOverflowRect->UnionRect(*aOverflowRect, shadowRect);

  if (IsFloatingFirstLetterChild()) {
    
    
    nscoord fontAscent, fontHeight;
    nsIFontMetrics* fm = aProvider.GetFontMetrics();
    fm->GetMaxAscent(fontAscent);
    fm->GetMaxHeight(fontHeight);
    nsRect fontRect(0, mAscent - fontAscent, GetSize().width, fontHeight);
    aOverflowRect->UnionRect(*aOverflowRect, fontRect);
  }

  
  
  nsRect decorationRect;
  if (!(GetStateBits() & NS_FRAME_SELECTED_CONTENT) ||
      !CombineSelectionUnderlineRect(aPresContext, *aOverflowRect))
    return;
  AddStateBits(TEXT_SELECTION_UNDERLINE_OVERFLOWED);
}

void 
nsTextFrame::PaintTextDecorations(gfxContext* aCtx, const gfxRect& aDirtyRect,
                                  const gfxPoint& aFramePt,
                                  const gfxPoint& aTextBaselinePt,
                                  nsTextPaintStyle& aTextPaintStyle,
                                  PropertyProvider& aProvider,
                                  const nscolor* aOverrideColor)
{
  TextDecorations decorations =
    GetTextDecorations(aTextPaintStyle.PresContext());
  if (!decorations.HasDecorationlines())
    return;

  gfxFont* firstFont = aProvider.GetFontGroup()->GetFontAt(0);
  if (!firstFont)
    return; 
  const gfxFont::Metrics& fontMetrics = firstFont->GetMetrics();
  gfxFloat app = aTextPaintStyle.PresContext()->AppUnitsPerDevPixel();

  
  gfxPoint pt(aFramePt.x / app, (aTextBaselinePt.y - mAscent) / app);
  gfxSize size(GetRect().width / app, 0);
  gfxFloat ascent = gfxFloat(mAscent) / app;

  nscolor lineColor;
  if (decorations.HasOverline()) {
    lineColor = aOverrideColor ? *aOverrideColor : decorations.mOverColor;
    size.height = fontMetrics.underlineSize;
    nsCSSRendering::PaintDecorationLine(
      aCtx, lineColor, pt, size, ascent, fontMetrics.maxAscent,
      NS_STYLE_TEXT_DECORATION_OVERLINE,
      nsCSSRendering::DECORATION_STYLE_SOLID);
  }
  if (decorations.HasUnderline()) {
    lineColor = aOverrideColor ? *aOverrideColor : decorations.mUnderColor;
    size.height = fontMetrics.underlineSize;
    gfxFloat offset = aProvider.GetFontGroup()->GetUnderlineOffset();
    nsCSSRendering::PaintDecorationLine(
      aCtx, lineColor, pt, size, ascent, offset,
      NS_STYLE_TEXT_DECORATION_UNDERLINE,
      nsCSSRendering::DECORATION_STYLE_SOLID);
  }
  if (decorations.HasStrikeout()) {
    lineColor = aOverrideColor ? *aOverrideColor : decorations.mStrikeColor;
    size.height = fontMetrics.strikeoutSize;
    gfxFloat offset = fontMetrics.strikeoutOffset;
    nsCSSRendering::PaintDecorationLine(
      aCtx, lineColor, pt, size, ascent, offset,
      NS_STYLE_TEXT_DECORATION_LINE_THROUGH,
      nsCSSRendering::DECORATION_STYLE_SOLID);
  }
}

static gfxFloat
ComputeDescentLimitForSelectionUnderline(nsPresContext* aPresContext,
                                         nsTextFrame* aFrame,
                                         const gfxFont::Metrics& aFontMetrics)
{
  gfxFloat app = aPresContext->AppUnitsPerDevPixel();
  nscoord lineHeightApp =
    nsHTMLReflowState::CalcLineHeight(aFrame->GetStyleContext(), NS_AUTOHEIGHT);
  gfxFloat lineHeight = gfxFloat(lineHeightApp) / app;
  if (lineHeight <= aFontMetrics.maxHeight) {
    return aFontMetrics.maxDescent;
  }
  return aFontMetrics.maxDescent + (lineHeight - aFontMetrics.maxHeight) / 2;
}



static const SelectionType SelectionTypesWithDecorations =
  nsISelectionController::SELECTION_SPELLCHECK |
  nsISelectionController::SELECTION_IME_RAWINPUT |
  nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT |
  nsISelectionController::SELECTION_IME_CONVERTEDTEXT |
  nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT;

static PRUint8
GetTextDecorationStyle(const nsTextRangeStyle &aRangeStyle)
{
  NS_PRECONDITION(aRangeStyle.IsLineStyleDefined(),
                  "aRangeStyle.mLineStyle have to be defined");
  switch (aRangeStyle.mLineStyle) {
    case nsTextRangeStyle::LINESTYLE_NONE:
      return nsCSSRendering::DECORATION_STYLE_NONE;
    case nsTextRangeStyle::LINESTYLE_SOLID:
      return nsCSSRendering::DECORATION_STYLE_SOLID;
    case nsTextRangeStyle::LINESTYLE_DOTTED:
      return nsCSSRendering::DECORATION_STYLE_DOTTED;
    case nsTextRangeStyle::LINESTYLE_DASHED:
      return nsCSSRendering::DECORATION_STYLE_DASHED;
    case nsTextRangeStyle::LINESTYLE_DOUBLE:
      return nsCSSRendering::DECORATION_STYLE_DOUBLE;
    case nsTextRangeStyle::LINESTYLE_WAVY:
      return nsCSSRendering::DECORATION_STYLE_WAVY;
    default:
      NS_WARNING("Requested underline style is not valid");
      return nsCSSRendering::DECORATION_STYLE_SOLID;
  }
}

static gfxFloat
ComputeSelectionUnderlineHeight(nsPresContext* aPresContext,
                                const gfxFont::Metrics& aFontMetrics,
                                SelectionType aSelectionType)
{
  switch (aSelectionType) {
    case nsISelectionController::SELECTION_IME_RAWINPUT:
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
      return aFontMetrics.underlineSize;
    case nsISelectionController::SELECTION_SPELLCHECK: {
      
      
      
      
      
      
      PRInt32 defaultFontSize =
        aPresContext->AppUnitsToDevPixels(nsStyleFont(aPresContext).mFont.size);
      gfxFloat fontSize = NS_MIN(gfxFloat(defaultFontSize),
                                 aFontMetrics.emHeight);
      fontSize = NS_MAX(fontSize, 1.0);
      return NS_ceil(fontSize / 20);
    }
    default:
      NS_WARNING("Requested underline style is not valid");
      return aFontMetrics.underlineSize;
  }
}





static void DrawSelectionDecorations(gfxContext* aContext, SelectionType aType,
    nsTextFrame* aFrame,
    nsTextPaintStyle& aTextPaintStyle,
    const nsTextRangeStyle &aRangeStyle,
    const gfxPoint& aPt, gfxFloat aWidth,
    gfxFloat aAscent, const gfxFont::Metrics& aFontMetrics)
{
  gfxPoint pt(aPt);
  gfxSize size(aWidth,
               ComputeSelectionUnderlineHeight(aTextPaintStyle.PresContext(),
                                               aFontMetrics, aType));
  gfxFloat descentLimit =
    ComputeDescentLimitForSelectionUnderline(aTextPaintStyle.PresContext(),
                                             aFrame, aFontMetrics);

  float relativeSize;
  PRUint8 style;
  nscolor color;
  PRInt32 index =
    nsTextPaintStyle::GetUnderlineStyleIndexForSelectionType(aType);
  PRBool weDefineSelectionUnderline =
    aTextPaintStyle.GetSelectionUnderlineForPaint(index, &color,
                                                  &relativeSize, &style);

  switch (aType) {
    case nsISelectionController::SELECTION_IME_RAWINPUT:
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT: {
      
      
      
      
      
      
      
      
      
      
      
      
      
      pt.x += 1.0;
      size.width -= 2.0;
      if (aRangeStyle.IsDefined()) {
        
        if (aRangeStyle.IsLineStyleDefined()) {
          if (aRangeStyle.mLineStyle == nsTextRangeStyle::LINESTYLE_NONE) {
            return;
          }
          style = GetTextDecorationStyle(aRangeStyle);
          relativeSize = aRangeStyle.mIsBoldLine ? 2.0f : 1.0f;
        } else if (!weDefineSelectionUnderline) {
          
          return;
        }
        if (aRangeStyle.IsUnderlineColorDefined()) {
          color = aRangeStyle.mUnderlineColor;
        } else if (aRangeStyle.IsForegroundColorDefined()) {
          color = aRangeStyle.mForegroundColor;
        } else {
          NS_ASSERTION(!aRangeStyle.IsBackgroundColorDefined(),
                       "Only the background color is defined");
          color = aTextPaintStyle.GetTextColor();
        }
      } else if (!weDefineSelectionUnderline) {
        
        
        return;
      }
      break;
    }
    case nsISelectionController::SELECTION_SPELLCHECK:
      if (!weDefineSelectionUnderline)
        return;
      break;
    default:
      NS_WARNING("Requested selection decorations when there aren't any");
      return;
  }
  size.height *= relativeSize;
  nsCSSRendering::PaintDecorationLine(
    aContext, color, pt, size, aAscent, aFontMetrics.underlineOffset,
    NS_STYLE_TEXT_DECORATION_UNDERLINE, style, descentLimit);
}









static PRBool GetSelectionTextColors(SelectionType aType,
                                     nsTextPaintStyle& aTextPaintStyle,
                                     const nsTextRangeStyle &aRangeStyle,
                                     nscolor* aForeground, nscolor* aBackground)
{
  switch (aType) {
    case nsISelectionController::SELECTION_NORMAL:
      return aTextPaintStyle.GetSelectionColors(aForeground, aBackground);
    case nsISelectionController::SELECTION_FIND:
      aTextPaintStyle.GetHighlightColors(aForeground, aBackground);
      return PR_TRUE;
    case nsISelectionController::SELECTION_IME_RAWINPUT:
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
      if (aRangeStyle.IsDefined()) {
        *aForeground = aTextPaintStyle.GetTextColor();
        *aBackground = NS_RGBA(0,0,0,0);
        if (!aRangeStyle.IsForegroundColorDefined() &&
            !aRangeStyle.IsBackgroundColorDefined()) {
          return PR_FALSE;
        }
        if (aRangeStyle.IsForegroundColorDefined()) {
          *aForeground = aRangeStyle.mForegroundColor;
        }
        if (aRangeStyle.IsBackgroundColorDefined()) {
          *aBackground = aRangeStyle.mBackgroundColor;
        }
        return PR_TRUE;
      }
      aTextPaintStyle.GetIMESelectionColors(
        nsTextPaintStyle::GetUnderlineStyleIndexForSelectionType(aType),
        aForeground, aBackground);
      return PR_TRUE;
    default:
      *aForeground = aTextPaintStyle.GetTextColor();
      *aBackground = NS_RGBA(0,0,0,0);
      return PR_FALSE;
  }
}








class SelectionIterator {
public:
  



  SelectionIterator(SelectionDetails** aSelectionDetails,
                    PRInt32 aStart, PRInt32 aLength,
                    PropertyProvider& aProvider, gfxTextRun* aTextRun);

  












  PRBool GetNextSegment(gfxFloat* aXOffset, PRUint32* aOffset, PRUint32* aLength,
                        gfxFloat* aHyphenWidth, SelectionType* aType,
                        nsTextRangeStyle* aStyle);
  void UpdateWithAdvance(gfxFloat aAdvance) {
    mXOffset += aAdvance*mTextRun->GetDirection();
  }

private:
  SelectionDetails**      mSelectionDetails;
  PropertyProvider&       mProvider;
  gfxTextRun*             mTextRun;
  gfxSkipCharsIterator    mIterator;
  PRInt32                 mOriginalStart;
  PRInt32                 mOriginalEnd;
  gfxFloat                mXOffset;
};

SelectionIterator::SelectionIterator(SelectionDetails** aSelectionDetails,
    PRInt32 aStart, PRInt32 aLength, PropertyProvider& aProvider,
    gfxTextRun* aTextRun)
  : mSelectionDetails(aSelectionDetails), mProvider(aProvider),
    mTextRun(aTextRun), mIterator(aProvider.GetStart()),
    mOriginalStart(aStart), mOriginalEnd(aStart + aLength),
    mXOffset(mTextRun->IsRightToLeft() ? aProvider.GetFrame()->GetSize().width : 0)
{
  mIterator.SetOriginalOffset(aStart);
}

PRBool SelectionIterator::GetNextSegment(gfxFloat* aXOffset,
    PRUint32* aOffset, PRUint32* aLength, gfxFloat* aHyphenWidth,
    SelectionType* aType, nsTextRangeStyle* aStyle)
{
  if (mIterator.GetOriginalOffset() >= mOriginalEnd)
    return PR_FALSE;
  
  
  PRUint32 runOffset = mIterator.GetSkippedOffset();
  
  PRInt32 index = mIterator.GetOriginalOffset() - mOriginalStart;
  SelectionDetails* sdptr = mSelectionDetails[index];
  SelectionType type =
    sdptr ? sdptr->mType : nsISelectionController::SELECTION_NONE;
  nsTextRangeStyle style;
  if (sdptr) {
    style = sdptr->mTextRangeStyle;
  }
  for (++index; mOriginalStart + index < mOriginalEnd; ++index) {
    if (sdptr != mSelectionDetails[index])
      break;
  }
  mIterator.SetOriginalOffset(index + mOriginalStart);

  
  while (mIterator.GetOriginalOffset() < mOriginalEnd &&
         !mIterator.IsOriginalCharSkipped() &&
         !mTextRun->IsClusterStart(mIterator.GetSkippedOffset())) {
    mIterator.AdvanceOriginal(1);
  }

  PRBool haveHyphenBreak =
    (mProvider.GetFrame()->GetStateBits() & TEXT_HYPHEN_BREAK) != 0;
  *aOffset = runOffset;
  *aLength = mIterator.GetSkippedOffset() - runOffset;
  *aXOffset = mXOffset;
  *aHyphenWidth = 0;
  if (mIterator.GetOriginalOffset() == mOriginalEnd && haveHyphenBreak) {
    *aHyphenWidth = mProvider.GetHyphenWidth();
  }
  *aType = type;
  *aStyle = style;
  return PR_TRUE;
}

static void
AddHyphenToMetrics(nsTextFrame* aTextFrame, gfxTextRun* aBaseTextRun,
                   gfxTextRun::Metrics* aMetrics,
                   gfxFont::BoundingBoxType aBoundingBoxType,
                   gfxContext* aContext)
{
  
  gfxTextRunCache::AutoTextRun hyphenTextRun(
    GetHyphenTextRun(aBaseTextRun, aContext, aTextFrame));
  if (!hyphenTextRun.get())
    return;

  gfxTextRun::Metrics hyphenMetrics =
    hyphenTextRun->MeasureText(0, hyphenTextRun->GetLength(),
                               aBoundingBoxType, aContext, nsnull);
  aMetrics->CombineWith(hyphenMetrics, aBaseTextRun->IsRightToLeft());
}

void
nsTextFrame::PaintOneShadow(PRUint32 aOffset, PRUint32 aLength,
                            nsCSSShadowItem* aShadowDetails,
                            PropertyProvider* aProvider, const nsRect& aDirtyRect,
                            const gfxPoint& aFramePt, const gfxPoint& aTextBaselinePt,
                            gfxContext* aCtx, const nscolor& aForegroundColor)
{
  gfxPoint shadowOffset(aShadowDetails->mXOffset, aShadowDetails->mYOffset);
  nscoord blurRadius = NS_MAX(aShadowDetails->mRadius, 0);

  gfxTextRun::Metrics shadowMetrics =
    mTextRun->MeasureText(aOffset, aLength, gfxFont::LOOSE_INK_EXTENTS,
                          nsnull, aProvider);
  if (GetStateBits() & TEXT_HYPHEN_BREAK) {
    AddHyphenToMetrics(this, mTextRun, &shadowMetrics, gfxFont::LOOSE_INK_EXTENTS, aCtx);
  }

  
  
  
  gfxRect shadowGfxRect = shadowMetrics.mBoundingBox +
     gfxPoint(aFramePt.x, aTextBaselinePt.y) + shadowOffset;
  nsRect shadowRect(shadowGfxRect.X(), shadowGfxRect.Y(),
                    shadowGfxRect.Width(), shadowGfxRect.Height());;

  nsContextBoxBlur contextBoxBlur;
  gfxContext* shadowContext = contextBoxBlur.Init(shadowRect, blurRadius,
                                                  PresContext()->AppUnitsPerDevPixel(),
                                                  aCtx, aDirtyRect);
  if (!shadowContext)
    return;

  nscolor shadowColor;
  if (aShadowDetails->mHasColor)
    shadowColor = aShadowDetails->mColor;
  else
    shadowColor = aForegroundColor;

  aCtx->Save();
  aCtx->NewPath();
  aCtx->SetColor(gfxRGBA(shadowColor));

  
  
  
  gfxRect dirtyGfxRect(aDirtyRect.x, aDirtyRect.y, aDirtyRect.width, aDirtyRect.height);
  gfxFloat advanceWidth;
  DrawText(shadowContext,
           aTextBaselinePt + shadowOffset,
           aOffset, aLength, &dirtyGfxRect, aProvider, advanceWidth,
           (GetStateBits() & TEXT_HYPHEN_BREAK) != 0);

  
  
  
  nsTextPaintStyle textPaintStyle(this);
  PaintTextDecorations(shadowContext, dirtyGfxRect, aFramePt + shadowOffset,
                       aTextBaselinePt + shadowOffset,
                       textPaintStyle, *aProvider, &shadowColor);

  contextBoxBlur.DoPaint();
  aCtx->Restore();
}



void
nsTextFrame::PaintTextWithSelectionColors(gfxContext* aCtx,
    const gfxPoint& aFramePt,
    const gfxPoint& aTextBaselinePt, const gfxRect& aDirtyRect,
    PropertyProvider& aProvider, nsTextPaintStyle& aTextPaintStyle,
    SelectionDetails* aDetails, SelectionType* aAllTypes)
{
  PRInt32 contentOffset = aProvider.GetStart().GetOriginalOffset();
  PRInt32 contentLength = aProvider.GetOriginalLength();

  
  nsAutoTArray<SelectionDetails*,BIG_TEXT_NODE_SIZE> prevailingSelectionsBuffer;
  if (!prevailingSelectionsBuffer.AppendElements(contentLength))
    return;
  SelectionDetails** prevailingSelections = prevailingSelectionsBuffer.Elements();

  PRInt32 i;
  SelectionType allTypes = 0;
  for (i = 0; i < contentLength; ++i) {
    prevailingSelections[i] = nsnull;
  }

  SelectionDetails *sdptr = aDetails;
  PRBool anyBackgrounds = PR_FALSE;
  while (sdptr) {
    PRInt32 start = NS_MAX(0, sdptr->mStart - contentOffset);
    PRInt32 end = NS_MIN(contentLength, sdptr->mEnd - contentOffset);
    SelectionType type = sdptr->mType;
    if (start < end) {
      allTypes |= type;
      
      nscolor foreground, background;
      if (GetSelectionTextColors(type, aTextPaintStyle, sdptr->mTextRangeStyle,
                                 &foreground, &background)) {
        if (NS_GET_A(background) > 0) {
          anyBackgrounds = PR_TRUE;
        }
        for (i = start; i < end; ++i) {
          
          if (!prevailingSelections[i] ||
              type < prevailingSelections[i]->mType) {
            prevailingSelections[i] = sdptr;
          }
        }
      }
    }
    sdptr = sdptr->mNext;
  }
  *aAllTypes = allTypes;

  gfxFloat xOffset, hyphenWidth;
  PRUint32 offset, length; 
  SelectionType type;
  nsTextRangeStyle rangeStyle;
  
  if (anyBackgrounds) {
    SelectionIterator iterator(prevailingSelections, contentOffset, contentLength,
                               aProvider, mTextRun);
    while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth,
                                   &type, &rangeStyle)) {
      nscolor foreground, background;
      GetSelectionTextColors(type, aTextPaintStyle, rangeStyle,
                             &foreground, &background);
      
      gfxFloat advance = hyphenWidth +
        mTextRun->GetAdvanceWidth(offset, length, &aProvider);
      if (NS_GET_A(background) > 0) {
        gfxFloat x = xOffset - (mTextRun->IsRightToLeft() ? advance : 0);
        FillClippedRect(aCtx, aTextPaintStyle.PresContext(),
                        background, aDirtyRect,
                        gfxRect(aFramePt.x + x, aFramePt.y, advance, GetSize().height));
      }
      iterator.UpdateWithAdvance(advance);
    }
  }
  
  
  SelectionIterator iterator(prevailingSelections, contentOffset, contentLength,
                             aProvider, mTextRun);
  while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth,
                                 &type, &rangeStyle)) {
    nscolor foreground, background;
    GetSelectionTextColors(type, aTextPaintStyle, rangeStyle,
                           &foreground, &background);
    
    aCtx->SetColor(gfxRGBA(foreground));
    gfxFloat advance;

    DrawText(aCtx, gfxPoint(aFramePt.x + xOffset, aTextBaselinePt.y),
             offset, length, &aDirtyRect, &aProvider,
             advance, hyphenWidth > 0);
    if (hyphenWidth) {
      advance += hyphenWidth;
    }
    iterator.UpdateWithAdvance(advance);
  }
}

void
nsTextFrame::PaintTextSelectionDecorations(gfxContext* aCtx,
    const gfxPoint& aFramePt,
    const gfxPoint& aTextBaselinePt, const gfxRect& aDirtyRect,
    PropertyProvider& aProvider, nsTextPaintStyle& aTextPaintStyle,
    SelectionDetails* aDetails, SelectionType aSelectionType)
{
  PRInt32 contentOffset = aProvider.GetStart().GetOriginalOffset();
  PRInt32 contentLength = aProvider.GetOriginalLength();

  
  nsAutoTArray<SelectionDetails*, BIG_TEXT_NODE_SIZE> selectedCharsBuffer;
  if (!selectedCharsBuffer.AppendElements(contentLength))
    return;
  SelectionDetails** selectedChars = selectedCharsBuffer.Elements();
  PRInt32 i;
  for (i = 0; i < contentLength; ++i) {
    selectedChars[i] = nsnull;
  }

  SelectionDetails *sdptr = aDetails;
  while (sdptr) {
    if (sdptr->mType == aSelectionType) {
      PRInt32 start = NS_MAX(0, sdptr->mStart - contentOffset);
      PRInt32 end = NS_MIN(contentLength, sdptr->mEnd - contentOffset);
      for (i = start; i < end; ++i) {
        selectedChars[i] = sdptr;
      }
    }
    sdptr = sdptr->mNext;
  }

  gfxFont* firstFont = aProvider.GetFontGroup()->GetFontAt(0);
  if (!firstFont)
    return; 
  gfxFont::Metrics decorationMetrics(firstFont->GetMetrics());
  decorationMetrics.underlineOffset =
    aProvider.GetFontGroup()->GetUnderlineOffset();

  SelectionIterator iterator(selectedChars, contentOffset, contentLength,
                             aProvider, mTextRun);
  gfxFloat xOffset, hyphenWidth;
  PRUint32 offset, length;
  PRInt32 app = aTextPaintStyle.PresContext()->AppUnitsPerDevPixel();
  
  gfxPoint pt(0.0, (aTextBaselinePt.y - mAscent) / app);
  SelectionType type;
  nsTextRangeStyle selectedStyle;
  while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth,
                                 &type, &selectedStyle)) {
    gfxFloat advance = hyphenWidth +
      mTextRun->GetAdvanceWidth(offset, length, &aProvider);
    if (type == aSelectionType) {
      pt.x = (aFramePt.x + xOffset -
             (mTextRun->IsRightToLeft() ? advance : 0)) / app;
      gfxFloat width = PR_ABS(advance) / app;
      DrawSelectionDecorations(aCtx, aSelectionType, this, aTextPaintStyle,
                               selectedStyle,
                               pt, width, mAscent / app, decorationMetrics);
    }
    iterator.UpdateWithAdvance(advance);
  }
}

PRBool
nsTextFrame::PaintTextWithSelection(gfxContext* aCtx,
    const gfxPoint& aFramePt,
    const gfxPoint& aTextBaselinePt, const gfxRect& aDirtyRect,
    PropertyProvider& aProvider, nsTextPaintStyle& aTextPaintStyle)
{
  SelectionDetails* details = GetSelectionDetails();
  if (!details)
    return PR_FALSE;

  SelectionType allTypes;
  PaintTextWithSelectionColors(aCtx, aFramePt, aTextBaselinePt, aDirtyRect,
                               aProvider, aTextPaintStyle, details, &allTypes);
  PaintTextDecorations(aCtx, aDirtyRect, aFramePt, aTextBaselinePt,
                       aTextPaintStyle, aProvider);
  PRInt32 i;
  
  
  
  
  allTypes &= SelectionTypesWithDecorations;
  for (i = nsISelectionController::NUM_SELECTIONTYPES - 1; i >= 1; --i) {
    SelectionType type = 1 << (i - 1);
    if (allTypes & type) {
      
      
      
      PaintTextSelectionDecorations(aCtx, aFramePt, aTextBaselinePt, aDirtyRect,
                                    aProvider, aTextPaintStyle, details, type);
    }
  }

  DestroySelectionDetails(details);
  return PR_TRUE;
}

static PRUint32
ComputeTransformedLength(PropertyProvider& aProvider)
{
  gfxSkipCharsIterator iter(aProvider.GetStart());
  PRUint32 start = iter.GetSkippedOffset();
  iter.AdvanceOriginal(aProvider.GetOriginalLength());
  return iter.GetSkippedOffset() - start;
}

gfxFloat
nsTextFrame::GetSnappedBaselineY(gfxContext* aContext, gfxFloat aY)
{
  gfxFloat appUnitsPerDevUnit = mTextRun->GetAppUnitsPerDevUnit();
  gfxFloat baseline = aY + mAscent;
  gfxRect putativeRect(0, baseline/appUnitsPerDevUnit, 1, 1);
  if (!aContext->UserToDevicePixelSnapped(putativeRect))
    return baseline;
  return aContext->DeviceToUser(putativeRect.pos).y*appUnitsPerDevUnit;
}

void
nsTextFrame::PaintText(nsIRenderingContext* aRenderingContext, nsPoint aPt,
                       const nsRect& aDirtyRect)
{
  
  
  
  gfxSkipCharsIterator iter = EnsureTextRun();
  if (!mTextRun)
    return;

  nsTextPaintStyle textPaintStyle(this);
  PropertyProvider provider(this, iter);
  
  provider.InitializeForDisplay(PR_TRUE);

  gfxContext* ctx = aRenderingContext->ThebesContext();

  gfxPoint framePt(aPt.x, aPt.y);
  gfxPoint textBaselinePt(
      mTextRun->IsRightToLeft() ? gfxFloat(aPt.x + GetSize().width) : framePt.x,
      GetSnappedBaselineY(ctx, aPt.y));

  gfxRect dirtyRect(aDirtyRect.x, aDirtyRect.y,
                    aDirtyRect.width, aDirtyRect.height);

  gfxFloat advanceWidth;
  gfxRGBA foregroundColor = gfxRGBA(textPaintStyle.GetTextColor());

  
  const nsStyleText* textStyle = GetStyleText();
  if (textStyle->mTextShadow) {
    
    
    for (PRUint32 i = textStyle->mTextShadow->Length(); i > 0; --i) {
      PaintOneShadow(provider.GetStart().GetSkippedOffset(),
                     ComputeTransformedLength(provider),
                     textStyle->mTextShadow->ShadowAt(i - 1), &provider,
                     aDirtyRect, framePt, textBaselinePt, ctx,
                     textPaintStyle.GetTextColor());
    }
  }

  
  if (nsLayoutUtils::GetNonGeneratedAncestor(this)->GetStateBits() & NS_FRAME_SELECTED_CONTENT) {
    if (PaintTextWithSelection(ctx, framePt, textBaselinePt,
                               dirtyRect, provider, textPaintStyle))
      return;
  }

  ctx->SetColor(foregroundColor);

  DrawText(ctx, textBaselinePt, provider.GetStart().GetSkippedOffset(),
           ComputeTransformedLength(provider), &dirtyRect,
           &provider, advanceWidth,
           (GetStateBits() & TEXT_HYPHEN_BREAK) != 0);
  PaintTextDecorations(ctx, dirtyRect, framePt, textBaselinePt,
                       textPaintStyle, provider);
}

void
nsTextFrame::DrawText(gfxContext* aCtx, const gfxPoint& aTextBaselinePt,
                      PRUint32 aOffset, PRUint32 aLength,
                      const gfxRect* aDirtyRect, PropertyProvider* aProvider,
                      gfxFloat& aAdvanceWidth, PRBool aDrawSoftHyphen)
{
  
  mTextRun->Draw(aCtx, aTextBaselinePt, aOffset, aLength,
                 aDirtyRect, aProvider, &aAdvanceWidth);

  if (aDrawSoftHyphen) {
    
    
    gfxTextRunCache::AutoTextRun hyphenTextRun(GetHyphenTextRun(mTextRun, nsnull, this));
    if (hyphenTextRun.get()) {
      
      
      gfxFloat hyphenBaselineX = aTextBaselinePt.x + mTextRun->GetDirection() * aAdvanceWidth -
        (mTextRun->IsRightToLeft() ? hyphenTextRun->GetAdvanceWidth(0, hyphenTextRun->GetLength(), nsnull) : 0);
      hyphenTextRun->Draw(aCtx, gfxPoint(hyphenBaselineX, aTextBaselinePt.y),
                          0, hyphenTextRun->GetLength(), aDirtyRect, nsnull, nsnull);
    }
  }
}

PRInt16
nsTextFrame::GetSelectionStatus(PRInt16* aSelectionFlags)
{
  
  nsCOMPtr<nsISelectionController> selectionController;
  nsresult rv = GetSelectionController(PresContext(),
                                       getter_AddRefs(selectionController));
  if (NS_FAILED(rv) || !selectionController)
    return nsISelectionController::SELECTION_OFF;

  selectionController->GetSelectionFlags(aSelectionFlags);

  PRInt16 selectionValue;
  selectionController->GetDisplaySelection(&selectionValue);

  return selectionValue;
}

PRBool
nsTextFrame::IsVisibleInSelection(nsISelection* aSelection)
{
  
  PRBool isSelected = (mState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
  if (!isSelected)
    return PR_FALSE;
    
  SelectionDetails* details = GetSelectionDetails();
  PRBool found = PR_FALSE;
    
  
  SelectionDetails *sdptr = details;
  while (sdptr) {
    if (sdptr->mEnd > GetContentOffset() &&
        sdptr->mStart < GetContentEnd() &&
        sdptr->mType == nsISelectionController::SELECTION_NORMAL) {
      found = PR_TRUE;
      break;
    }
    sdptr = sdptr->mNext;
  }
  DestroySelectionDetails(details);

  return found;
}





static PRUint32
CountCharsFit(gfxTextRun* aTextRun, PRUint32 aStart, PRUint32 aLength,
              gfxFloat aWidth, PropertyProvider* aProvider,
              gfxFloat* aFitWidth)
{
  PRUint32 last = 0;
  gfxFloat width = 0;
  PRUint32 i;
  for (i = 1; i <= aLength; ++i) {
    if (i == aLength || aTextRun->IsClusterStart(aStart + i)) {
      gfxFloat nextWidth = width +
          aTextRun->GetAdvanceWidth(aStart + last, i - last, aProvider);
      if (nextWidth > aWidth)
        break;
      last = i;
      width = nextWidth;
    }
  }
  *aFitWidth = width;
  return last;
}

nsIFrame::ContentOffsets
nsTextFrame::CalcContentOffsetsFromFramePoint(nsPoint aPoint)
{
  return GetCharacterOffsetAtFramePointInternal(aPoint, PR_TRUE);
}

nsIFrame::ContentOffsets
nsTextFrame::GetCharacterOffsetAtFramePoint(const nsPoint &aPoint)
{
  return GetCharacterOffsetAtFramePointInternal(aPoint, PR_FALSE);
}

nsIFrame::ContentOffsets
nsTextFrame::GetCharacterOffsetAtFramePointInternal(const nsPoint &aPoint,
                                                    PRBool aForInsertionPoint)
{
  ContentOffsets offsets;
  
  gfxSkipCharsIterator iter = EnsureTextRun();
  if (!mTextRun)
    return offsets;
  
  PropertyProvider provider(this, iter);
  
  provider.InitializeForDisplay(PR_FALSE);
  gfxFloat width = mTextRun->IsRightToLeft() ? mRect.width - aPoint.x : aPoint.x;
  gfxFloat fitWidth;
  PRUint32 skippedLength = ComputeTransformedLength(provider);

  PRUint32 charsFit = CountCharsFit(mTextRun,
      provider.GetStart().GetSkippedOffset(), skippedLength, width, &provider, &fitWidth);

  PRInt32 selectedOffset;
  if (charsFit < skippedLength) {
    
    
    
    gfxSkipCharsIterator extraCluster(provider.GetStart());
    extraCluster.AdvanceSkipped(charsFit);
    gfxSkipCharsIterator extraClusterLastChar(extraCluster);
    FindClusterEnd(mTextRun,
                   provider.GetStart().GetOriginalOffset() + provider.GetOriginalLength(),
                   &extraClusterLastChar);
    gfxFloat charWidth =
        mTextRun->GetAdvanceWidth(extraCluster.GetSkippedOffset(),
                                  GetSkippedDistance(extraCluster, extraClusterLastChar) + 1,
                                  &provider);
    selectedOffset = !aForInsertionPoint || width <= fitWidth + charWidth/2
        ? extraCluster.GetOriginalOffset()
        : extraClusterLastChar.GetOriginalOffset() + 1;
  } else {
    
    
    
    
    
    selectedOffset =
        provider.GetStart().GetOriginalOffset() + provider.GetOriginalLength();
  }

  offsets.content = GetContent();
  offsets.offset = offsets.secondaryOffset = selectedOffset;
  offsets.associateWithNext = mContentOffset == offsets.offset;
  return offsets;
}

PRBool
nsTextFrame::CombineSelectionUnderlineRect(nsPresContext* aPresContext,
                                           nsRect& aRect)
{
  if (aRect.IsEmpty())
    return PR_FALSE;

  nsRect givenRect = aRect;

  nsCOMPtr<nsIFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm));
  nsIThebesFontMetrics* tfm = static_cast<nsIThebesFontMetrics*>(fm.get());
  gfxFontGroup* fontGroup = tfm->GetThebesFontGroup();
  gfxFont* firstFont = fontGroup->GetFontAt(0);
  if (!firstFont)
    return PR_FALSE; 
  const gfxFont::Metrics& metrics = firstFont->GetMetrics();
  gfxFloat underlineOffset = fontGroup->GetUnderlineOffset();
  gfxFloat ascent = aPresContext->AppUnitsToGfxUnits(mAscent);
  gfxFloat descentLimit =
    ComputeDescentLimitForSelectionUnderline(aPresContext, this, metrics);

  SelectionDetails *details = GetSelectionDetails();
  for (SelectionDetails *sd = details; sd; sd = sd->mNext) {
    if (sd->mStart == sd->mEnd || !(sd->mType & SelectionTypesWithDecorations))
      continue;

    PRUint8 style;
    float relativeSize;
    PRInt32 index =
      nsTextPaintStyle::GetUnderlineStyleIndexForSelectionType(sd->mType);
    if (sd->mType == nsISelectionController::SELECTION_SPELLCHECK) {
      if (!nsTextPaintStyle::GetSelectionUnderline(aPresContext, index, nsnull,
                                                   &relativeSize, &style)) {
        continue;
      }
    } else {
      
      nsTextRangeStyle& rangeStyle = sd->mTextRangeStyle;
      if (rangeStyle.IsDefined()) {
        if (!rangeStyle.IsLineStyleDefined() ||
            rangeStyle.mLineStyle == nsTextRangeStyle::LINESTYLE_NONE) {
          continue;
        }
        style = GetTextDecorationStyle(rangeStyle);
        relativeSize = rangeStyle.mIsBoldLine ? 2.0f : 1.0f;
      } else if (!nsTextPaintStyle::GetSelectionUnderline(aPresContext, index,
                                                          nsnull, &relativeSize,
                                                          &style)) {
        continue;
      }
    }
    nsRect decorationArea;
    gfxSize size(aPresContext->AppUnitsToGfxUnits(aRect.width),
                 ComputeSelectionUnderlineHeight(aPresContext,
                                                 metrics, sd->mType));
    relativeSize = NS_MAX(relativeSize, 1.0f);
    size.height *= relativeSize;
    decorationArea =
      nsCSSRendering::GetTextDecorationRect(aPresContext, size,
                                            ascent, underlineOffset,
                                            NS_STYLE_TEXT_DECORATION_UNDERLINE,
                                            style, descentLimit);
    aRect.UnionRect(aRect, decorationArea);
  }
  DestroySelectionDetails(details);

  return !aRect.IsEmpty() && !givenRect.Contains(aRect);
}

void
nsTextFrame::SetSelected(PRBool        aSelected,
                         SelectionType aType)
{
  SetSelectedRange(0, mContent->GetText()->GetLength(), aSelected, aType);
}

void
nsTextFrame::SetSelectedRange(PRUint32 aStart,
                              PRUint32 aEnd,
                              PRBool aSelected,
                              SelectionType aType)
{
  NS_ASSERTION(!GetPrevContinuation(), "Should only be called for primary frame");
  DEBUG_VERIFY_NOT_DIRTY(mState);

  
  if (aStart == aEnd)
    return;

  if (aType == nsISelectionController::SELECTION_NORMAL) {
    
    PRBool selectable;
    IsSelectable(&selectable, nsnull);
    if (!selectable)
      return;
  }

  PRBool anySelected = PR_FALSE;

  nsTextFrame* f = this;
  while (f && f->GetContentEnd() <= aStart) {
    if (f->GetStateBits() & NS_FRAME_SELECTED_CONTENT) {
      anySelected = PR_TRUE;
    }
    f = static_cast<nsTextFrame*>(f->GetNextContinuation());
  }

  nsPresContext* presContext = PresContext();
  while (f && f->GetContentOffset() < aEnd) {
    if (aSelected) {
      f->AddStateBits(NS_FRAME_SELECTED_CONTENT);
      anySelected = PR_TRUE;
    } else { 
      SelectionDetails *details = f->GetSelectionDetails();
      if (details) {
        anySelected = PR_TRUE;
        DestroySelectionDetails(details);
      } else {
        f->RemoveStateBits(NS_FRAME_SELECTED_CONTENT);
      }
    }

    
    
    
    PRBool didHaveOverflowingSelection =
      (f->GetStateBits() & TEXT_SELECTION_UNDERLINE_OVERFLOWED) != 0;
    nsRect r(nsPoint(0, 0), GetSize());
    PRBool willHaveOverflowingSelection =
      aSelected && f->CombineSelectionUnderlineRect(presContext, r);
    if (didHaveOverflowingSelection || willHaveOverflowingSelection) {
      presContext->PresShell()->FrameNeedsReflow(f,
                                                 nsIPresShell::eStyleChange,
                                                 NS_FRAME_IS_DIRTY);
    }
    
    f->InvalidateOverflowRect();

    f = static_cast<nsTextFrame*>(f->GetNextContinuation());
  }

  
  while (f && !anySelected) {
    if (f->GetStateBits() & NS_FRAME_SELECTED_CONTENT) {
      anySelected = PR_TRUE;
    }
    f = static_cast<nsTextFrame*>(f->GetNextContinuation());
  }

  if (anySelected) {
    mContent->SetFlags(NS_TEXT_IN_SELECTION);
  } else {
    
    
    mContent->UnsetFlags(NS_TEXT_IN_SELECTION);
  }
}

NS_IMETHODIMP
nsTextFrame::GetPointFromOffset(PRInt32 inOffset,
                                nsPoint* outPoint)
{
  if (!outPoint)
    return NS_ERROR_NULL_POINTER;

  outPoint->x = 0;
  outPoint->y = 0;

  DEBUG_VERIFY_NOT_DIRTY(mState);
  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;

  if (GetContentLength() <= 0) {
    return NS_OK;
  }

  gfxSkipCharsIterator iter = EnsureTextRun();
  if (!mTextRun)
    return NS_ERROR_FAILURE;

  PropertyProvider properties(this, iter);
  
  
  properties.InitializeForDisplay(PR_FALSE);  

  if (inOffset < GetContentOffset()){
    NS_WARNING("offset before this frame's content");
    inOffset = GetContentOffset();
  } else if (inOffset > GetContentEnd()) {
    NS_WARNING("offset after this frame's content");
    inOffset = GetContentEnd();
  }
  PRInt32 trimmedOffset = properties.GetStart().GetOriginalOffset();
  PRInt32 trimmedEnd = trimmedOffset + properties.GetOriginalLength();
  inOffset = NS_MAX(inOffset, trimmedOffset);
  inOffset = NS_MIN(inOffset, trimmedEnd);

  iter.SetOriginalOffset(inOffset);

  if (inOffset < trimmedEnd &&
      !iter.IsOriginalCharSkipped() &&
      !mTextRun->IsClusterStart(iter.GetSkippedOffset())) {
    NS_WARNING("GetPointFromOffset called for non-cluster boundary");
    FindClusterStart(mTextRun, &iter);
  }

  gfxFloat advanceWidth =
    mTextRun->GetAdvanceWidth(properties.GetStart().GetSkippedOffset(),
                              GetSkippedDistance(properties.GetStart(), iter),
                              &properties);
  nscoord width = NSToCoordCeilClamped(advanceWidth);

  if (mTextRun->IsRightToLeft()) {
    outPoint->x = mRect.width - width;
  } else {
    outPoint->x = width;
  }
  outPoint->y = 0;

  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::GetChildFrameContainingOffset(PRInt32   aContentOffset,
                                           PRBool    aHint,
                                           PRInt32*  aOutOffset,
                                           nsIFrame**aOutFrame)
{
  DEBUG_VERIFY_NOT_DIRTY(mState);
#if 0 
  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;
#endif

  NS_ASSERTION(aOutOffset && aOutFrame, "Bad out parameters");
  NS_ASSERTION(aContentOffset >= 0, "Negative content offset, existing code was very broken!");

  nsTextFrame* f = this;
  if (aContentOffset >= mContentOffset) {
    while (PR_TRUE) {
      nsTextFrame* next = static_cast<nsTextFrame*>(f->GetNextContinuation());
      if (!next || aContentOffset < next->GetContentOffset())
        break;
      if (aContentOffset == next->GetContentOffset()) {
        if (aHint) {
          f = next;
        }
        break;
      }
      f = next;
    }
  } else {
    while (PR_TRUE) {
      nsTextFrame* prev = static_cast<nsTextFrame*>(f->GetPrevContinuation());
      if (!prev || aContentOffset > f->GetContentOffset())
        break;
      if (aContentOffset == f->GetContentOffset()) {
        if (!aHint) {
          f = prev;
        }
        break;
      }
      f = prev;
    }
  }
  
  *aOutOffset = aContentOffset - f->GetContentOffset();
  *aOutFrame = f;
  return NS_OK;
}

PRBool
nsTextFrame::PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION(aOffset && *aOffset <= GetContentLength(), "aOffset out of range");

  gfxSkipCharsIterator iter = EnsureTextRun();
  if (!mTextRun)
    return PR_FALSE;

  TrimmedOffsets trimmed = GetTrimmedOffsets(GetFragment(), PR_TRUE);
  
  return iter.ConvertOriginalToSkipped(trimmed.GetEnd()) >
         iter.ConvertOriginalToSkipped(trimmed.mStart);
}









class NS_STACK_CLASS ClusterIterator {
public:
  ClusterIterator(nsTextFrame* aTextFrame, PRInt32 aPosition, PRInt32 aDirection,
                  nsString& aContext);

  PRBool NextCluster();
  PRBool IsWhitespace();
  PRBool IsPunctuation();
  PRBool HaveWordBreakBefore() { return mHaveWordBreak; }
  PRInt32 GetAfterOffset();
  PRInt32 GetBeforeOffset();

private:
  nsCOMPtr<nsIUGenCategory>   mCategories;
  gfxSkipCharsIterator        mIterator;
  const nsTextFragment*       mFrag;
  nsTextFrame*                mTextFrame;
  PRInt32                     mDirection;
  PRInt32                     mCharIndex;
  nsTextFrame::TrimmedOffsets mTrimmed;
  nsTArray<PRPackedBool>      mWordBreaks;
  PRPackedBool                mHaveWordBreak;
};

static PRBool
IsAcceptableCaretPosition(const gfxSkipCharsIterator& aIter, gfxTextRun* aTextRun,
                          nsIFrame* aFrame)
{
  if (aIter.IsOriginalCharSkipped())
    return PR_FALSE;
  PRUint32 index = aIter.GetSkippedOffset();
  if (!aTextRun->IsClusterStart(index))
    return PR_FALSE;
  return !(aFrame->GetStyleText()->NewlineIsSignificant() &&
           aTextRun->GetChar(index) == '\n');
}

PRBool
nsTextFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset)
{
  PRInt32 contentLength = GetContentLength();
  NS_ASSERTION(aOffset && *aOffset <= contentLength, "aOffset out of range");

  PRBool selectable;
  PRUint8 selectStyle;  
  IsSelectable(&selectable, &selectStyle);
  if (selectStyle == NS_STYLE_USER_SELECT_ALL)
    return PR_FALSE;

  gfxSkipCharsIterator iter = EnsureTextRun();
  if (!mTextRun)
    return PR_FALSE;

  TrimmedOffsets trimmed = GetTrimmedOffsets(GetFragment(), PR_FALSE);

  
  PRInt32 startOffset = GetContentOffset() + (*aOffset < 0 ? contentLength : *aOffset);

  if (!aForward) {
    PRInt32 i;
    for (i = NS_MIN(trimmed.GetEnd(), startOffset) - 1;
         i >= trimmed.mStart; --i) {
      iter.SetOriginalOffset(i);
      if (IsAcceptableCaretPosition(iter, mTextRun, this)) {
        *aOffset = i - mContentOffset;
        return PR_TRUE;
      }
    }
    *aOffset = 0;
  } else {
    PRInt32 i;
    for (i = startOffset + 1; i <= trimmed.GetEnd(); ++i) {
      iter.SetOriginalOffset(i);
      
      
      
      if (i == trimmed.GetEnd() ||
          IsAcceptableCaretPosition(iter, mTextRun, this)) {
        *aOffset = i - mContentOffset;
        return PR_TRUE;
      }
    }
    *aOffset = contentLength;
  }
  
  return PR_FALSE;
}

PRBool
ClusterIterator::IsWhitespace()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  return IsSelectionSpace(mFrag, mCharIndex);
}

PRBool
ClusterIterator::IsPunctuation()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  if (!mCategories)
    return PR_FALSE;
  nsIUGenCategory::nsUGenCategory c = mCategories->Get(mFrag->CharAt(mCharIndex));
  return c == nsIUGenCategory::kPunctuation || c == nsIUGenCategory::kSymbol;
}

PRInt32
ClusterIterator::GetBeforeOffset()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  return mCharIndex + (mDirection > 0 ? 0 : 1);
}

PRInt32
ClusterIterator::GetAfterOffset()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  return mCharIndex + (mDirection > 0 ? 1 : 0);
}

PRBool
ClusterIterator::NextCluster()
{
  if (!mDirection)
    return PR_FALSE;
  gfxTextRun* textRun = mTextFrame->GetTextRun();

  mHaveWordBreak = PR_FALSE;
  while (PR_TRUE) {
    PRBool keepGoing = PR_FALSE;
    if (mDirection > 0) {
      if (mIterator.GetOriginalOffset() >= mTrimmed.GetEnd())
        return PR_FALSE;
      keepGoing = mIterator.IsOriginalCharSkipped() ||
          mIterator.GetOriginalOffset() < mTrimmed.mStart ||
          !textRun->IsClusterStart(mIterator.GetSkippedOffset());
      mCharIndex = mIterator.GetOriginalOffset();
      mIterator.AdvanceOriginal(1);
    } else {
      if (mIterator.GetOriginalOffset() <= mTrimmed.mStart)
        return PR_FALSE;
      mIterator.AdvanceOriginal(-1);
      keepGoing = mIterator.IsOriginalCharSkipped() ||
          mIterator.GetOriginalOffset() >= mTrimmed.GetEnd() ||
          !textRun->IsClusterStart(mIterator.GetSkippedOffset());
      mCharIndex = mIterator.GetOriginalOffset();
    }

    if (mWordBreaks[GetBeforeOffset() - mTextFrame->GetContentOffset()]) {
      mHaveWordBreak = PR_TRUE;
    }
    if (!keepGoing)
      return PR_TRUE;
  }
}

ClusterIterator::ClusterIterator(nsTextFrame* aTextFrame, PRInt32 aPosition,
                                 PRInt32 aDirection, nsString& aContext)
  : mTextFrame(aTextFrame), mDirection(aDirection), mCharIndex(-1)
{
  mIterator = aTextFrame->EnsureTextRun();
  if (!aTextFrame->GetTextRun()) {
    mDirection = 0; 
    return;
  }
  mIterator.SetOriginalOffset(aPosition);

  mCategories = do_GetService(NS_UNICHARCATEGORY_CONTRACTID);
  
  mFrag = aTextFrame->GetFragment();
  mTrimmed = aTextFrame->GetTrimmedOffsets(mFrag, PR_TRUE);

  PRInt32 textOffset = aTextFrame->GetContentOffset();
  PRInt32 textLen = aTextFrame->GetContentLength();
  if (!mWordBreaks.AppendElements(textLen + 1)) {
    mDirection = 0; 
    return;
  }
  memset(mWordBreaks.Elements(), PR_FALSE, textLen + 1);
  PRInt32 textStart;
  if (aDirection > 0) {
    if (aContext.IsEmpty()) {
      
      mWordBreaks[0] = PR_TRUE;
    }
    textStart = aContext.Length();
    mFrag->AppendTo(aContext, textOffset, textLen);
  } else {
    if (aContext.IsEmpty()) {
      
      mWordBreaks[textLen] = PR_TRUE;
    }
    textStart = 0;
    nsAutoString str;
    mFrag->AppendTo(str, textOffset, textLen);
    aContext.Insert(str, 0);
  }
  nsIWordBreaker* wordBreaker = nsContentUtils::WordBreaker();
  PRInt32 i;
  for (i = 0; i <= textLen; ++i) {
    PRInt32 indexInText = i + textStart;
    mWordBreaks[i] |=
      wordBreaker->BreakInBetween(aContext.get(), indexInText,
                                  aContext.get() + indexInText,
                                  aContext.Length() - indexInText);
  }
}

PRBool
nsTextFrame::PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                            PRInt32* aOffset, PeekWordState* aState)
{
  PRInt32 contentLength = GetContentLength();
  NS_ASSERTION (aOffset && *aOffset <= contentLength, "aOffset out of range");

  PRBool selectable;
  PRUint8 selectStyle;
  IsSelectable(&selectable, &selectStyle);
  if (selectStyle == NS_STYLE_USER_SELECT_ALL)
    return PR_FALSE;

  PRInt32 offset = GetContentOffset() + (*aOffset < 0 ? contentLength : *aOffset);
  ClusterIterator cIter(this, offset, aForward ? 1 : -1, aState->mContext);

  if (!cIter.NextCluster())
    return PR_FALSE;

  do {
    PRBool isPunctuation = cIter.IsPunctuation();
    PRBool isWhitespace = cIter.IsWhitespace();
    PRBool isWordBreakBefore = cIter.HaveWordBreakBefore();
    if (aWordSelectEatSpace == isWhitespace && !aState->mSawBeforeType) {
      aState->SetSawBeforeType();
      aState->Update(isPunctuation, isWhitespace);
      continue;
    }
    
    if (!aState->mAtStart) {
      PRBool canBreak;
      if (isPunctuation != aState->mLastCharWasPunctuation) {
        canBreak = BreakWordBetweenPunctuation(aState, aForward,
                     isPunctuation, isWhitespace, aIsKeyboardSelect);
      } else if (!aState->mLastCharWasWhitespace &&
                 !isWhitespace && !isPunctuation && isWordBreakBefore) {
        
        
        
        
        
        canBreak = PR_TRUE;
      } else {
        canBreak = isWordBreakBefore && aState->mSawBeforeType;
      }
      if (canBreak) {
        *aOffset = cIter.GetBeforeOffset() - mContentOffset;
        return PR_TRUE;
      }
    }
    aState->Update(isPunctuation, isWhitespace);
  } while (cIter.NextCluster());

  *aOffset = cIter.GetAfterOffset() - mContentOffset;
  return PR_FALSE;
}

 

NS_IMETHODIMP
nsTextFrame::CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex,
    PRInt32 aEndIndex, PRBool aRecurse, PRBool *aFinished, PRBool *aRetval)
{
  if (!aRetval)
    return NS_ERROR_NULL_POINTER;

  
  
  
  for (nsTextFrame* f = this; f;
       f = static_cast<nsTextFrame*>(GetNextContinuation())) {
    if (f->PeekOffsetNoAmount(PR_TRUE, nsnull)) {
      *aRetval = PR_TRUE;
      return NS_OK;
    }
  }

  *aRetval = PR_FALSE;
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::GetOffsets(PRInt32 &start, PRInt32 &end) const
{
  start = GetContentOffset();
  end = GetContentEnd();
  return NS_OK;
}

static PRInt32
FindEndOfPunctuationRun(const nsTextFragment* aFrag,
                        gfxTextRun* aTextRun,
                        gfxSkipCharsIterator* aIter,
                        PRInt32 aOffset,
                        PRInt32 aStart,
                        PRInt32 aEnd)
{
  PRInt32 i;

  for (i = aStart; i < aEnd - aOffset; ++i) {
    if (nsContentUtils::IsPunctuationMarkAt(aFrag, aOffset + i)) {
      aIter->SetOriginalOffset(aOffset + i);
      FindClusterEnd(aTextRun, aEnd, aIter);
      i = aIter->GetOriginalOffset() - aOffset;
    } else {
      break;
    }
  }
  return i;
}














static PRBool
FindFirstLetterRange(const nsTextFragment* aFrag,
                     gfxTextRun* aTextRun,
                     PRInt32 aOffset, const gfxSkipCharsIterator& aIter,
                     PRInt32* aLength)
{
  PRInt32 i;
  PRInt32 length = *aLength;
  PRInt32 endOffset = aOffset + length;
  gfxSkipCharsIterator iter(aIter);

  
  i = FindEndOfPunctuationRun(aFrag, aTextRun, &iter, aOffset, 
                              GetTrimmableWhitespaceCount(aFrag, aOffset, length, 1),
                              endOffset);
  if (i == length)
    return PR_FALSE;

  
  
  if (!nsContentUtils::IsAlphanumericAt(aFrag, aOffset + i)) {
    *aLength = 0;
    return PR_TRUE;
  }

  
  iter.SetOriginalOffset(aOffset + i);
  FindClusterEnd(aTextRun, endOffset, &iter);
  i = iter.GetOriginalOffset() - aOffset;
  if (i + 1 == length)
    return PR_TRUE;

  
  i = FindEndOfPunctuationRun(aFrag, aTextRun, &iter, aOffset, i + 1, endOffset);
  if (i < length)
    *aLength = i;
  return PR_TRUE;
}

static PRUint32
FindStartAfterSkippingWhitespace(PropertyProvider* aProvider,
                                 nsIFrame::InlineIntrinsicWidthData* aData,
                                 const nsStyleText* aTextStyle,
                                 gfxSkipCharsIterator* aIterator,
                                 PRUint32 aFlowEndInTextRun)
{
  if (aData->skipWhitespace) {
    while (aIterator->GetSkippedOffset() < aFlowEndInTextRun &&
           IsTrimmableSpace(aProvider->GetFragment(), aIterator->GetOriginalOffset(), aTextStyle)) {
      aIterator->AdvanceOriginal(1);
    }
  }
  return aIterator->GetSkippedOffset();
}

 
void nsTextFrame::MarkIntrinsicWidthsDirty()
{
  ClearTextRun();
  nsFrame::MarkIntrinsicWidthsDirty();
}



void
nsTextFrame::AddInlineMinWidthForFlow(nsIRenderingContext *aRenderingContext,
                                      nsIFrame::InlineMinWidthData *aData)
{
  PRUint32 flowEndInTextRun;
  gfxContext* ctx = aRenderingContext->ThebesContext();
  gfxSkipCharsIterator iter =
    EnsureTextRun(ctx, aData->lineContainer, aData->line, &flowEndInTextRun);
  if (!mTextRun)
    return;

  
  
  const nsStyleText* textStyle = GetStyleText();
  const nsTextFragment* frag = GetFragment();
  PropertyProvider provider(mTextRun, textStyle, frag, this,
                            iter, PR_INT32_MAX, nsnull, 0);

  PRBool collapseWhitespace = !textStyle->WhiteSpaceIsSignificant();
  PRBool preformatNewlines = textStyle->NewlineIsSignificant();
  PRBool preformatTabs = textStyle->WhiteSpaceIsSignificant();
  gfxFloat tabWidth = -1;
  PRUint32 start =
    FindStartAfterSkippingWhitespace(&provider, aData, textStyle, &iter, flowEndInTextRun);

  
  for (PRUint32 i = start, wordStart = start; i <= flowEndInTextRun; ++i) {
    PRBool preformattedNewline = PR_FALSE;
    PRBool preformattedTab = PR_FALSE;
    if (i < flowEndInTextRun) {
      
      
      
      preformattedNewline = preformatNewlines && mTextRun->GetChar(i) == '\n';
      preformattedTab = preformatTabs && mTextRun->GetChar(i) == '\t';
      if (!mTextRun->CanBreakLineBefore(i) && !preformattedNewline &&
          !preformattedTab) {
        
        continue;
      }
    }

    if (i > wordStart) {
      nscoord width =
        NSToCoordCeilClamped(mTextRun->GetAdvanceWidth(wordStart, i - wordStart, &provider));
      aData->currentLine = NSCoordSaturatingAdd(aData->currentLine, width);
      aData->atStartOfLine = PR_FALSE;

      if (collapseWhitespace) {
        PRUint32 trimStart = GetEndOfTrimmedText(frag, textStyle, wordStart, i, &iter);
        if (trimStart == start) {
          
          
          aData->trailingWhitespace += width;
        } else {
          
          aData->trailingWhitespace =
            NSToCoordCeilClamped(mTextRun->GetAdvanceWidth(trimStart, i - trimStart, &provider));
        }
      } else {
        aData->trailingWhitespace = 0;
      }
    }

    if (preformattedTab) {
      PropertyProvider::Spacing spacing;
      provider.GetSpacing(i, 1, &spacing);
      aData->currentLine += nscoord(spacing.mBefore);
      gfxFloat afterTab =
        AdvanceToNextTab(aData->currentLine, this,
                         mTextRun, &tabWidth);
      aData->currentLine = nscoord(afterTab + spacing.mAfter);
      wordStart = i + 1;
    } else if (i < flowEndInTextRun ||
        (i == mTextRun->GetLength() &&
         (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TRAILING_BREAK))) {
      if (preformattedNewline) {
        aData->ForceBreak(aRenderingContext);
      } else {
        aData->OptionallyBreak(aRenderingContext);
      }
      wordStart = i;
    }
  }

  if (start < flowEndInTextRun) {
    
    aData->skipWhitespace =
      IsTrimmableSpace(provider.GetFragment(),
                       iter.ConvertSkippedToOriginal(flowEndInTextRun - 1),
                       textStyle);
  }
}



 void
nsTextFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinWidthData *aData)
{
  nsTextFrame* f;
  gfxTextRun* lastTextRun = nsnull;
  
  
  for (f = this; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
    
    
    
    if (f == this || f->mTextRun != lastTextRun) {
      nsIFrame* lc;
      if (aData->lineContainer &&
          aData->lineContainer != (lc = FindLineContainer(f))) {
        NS_ASSERTION(f != this, "wrong InlineMinWidthData container"
                                " for first continuation");
        aData->line = nsnull;
        aData->lineContainer = lc;
      }

      
      f->AddInlineMinWidthForFlow(aRenderingContext, aData);
      lastTextRun = f->mTextRun;
    }
  }
}



void
nsTextFrame::AddInlinePrefWidthForFlow(nsIRenderingContext *aRenderingContext,
                                       nsIFrame::InlinePrefWidthData *aData)
{
  PRUint32 flowEndInTextRun;
  gfxContext* ctx = aRenderingContext->ThebesContext();
  gfxSkipCharsIterator iter =
    EnsureTextRun(ctx, aData->lineContainer, aData->line, &flowEndInTextRun);
  if (!mTextRun)
    return;

  
  
  
  const nsStyleText* textStyle = GetStyleText();
  const nsTextFragment* frag = GetFragment();
  PropertyProvider provider(mTextRun, textStyle, frag, this,
                            iter, PR_INT32_MAX, nsnull, 0);

  PRBool collapseWhitespace = !textStyle->WhiteSpaceIsSignificant();
  PRBool preformatNewlines = textStyle->NewlineIsSignificant();
  PRBool preformatTabs = textStyle->WhiteSpaceIsSignificant();
  gfxFloat tabWidth = -1;
  PRUint32 start =
    FindStartAfterSkippingWhitespace(&provider, aData, textStyle, &iter, flowEndInTextRun);

  
  
  
  PRUint32 loopStart = (preformatNewlines || preformatTabs) ? start : flowEndInTextRun;
  for (PRUint32 i = loopStart, lineStart = start; i <= flowEndInTextRun; ++i) {
    PRBool preformattedNewline = PR_FALSE;
    PRBool preformattedTab = PR_FALSE;
    if (i < flowEndInTextRun) {
      
      
      
      NS_ASSERTION(preformatNewlines, "We can't be here unless newlines are hard breaks");
      preformattedNewline = preformatNewlines && mTextRun->GetChar(i) == '\n';
      preformattedTab = preformatTabs && mTextRun->GetChar(i) == '\t';
      if (!preformattedNewline && !preformattedTab) {
        
        continue;
      }
    }

    if (i > lineStart) {
      nscoord width =
        NSToCoordCeilClamped(mTextRun->GetAdvanceWidth(lineStart, i - lineStart, &provider));
      aData->currentLine = NSCoordSaturatingAdd(aData->currentLine, width);

      if (collapseWhitespace) {
        PRUint32 trimStart = GetEndOfTrimmedText(frag, textStyle, lineStart, i, &iter);
        if (trimStart == start) {
          
          
          aData->trailingWhitespace += width;
        } else {
          
          aData->trailingWhitespace =
            NSToCoordCeilClamped(mTextRun->GetAdvanceWidth(trimStart, i - trimStart, &provider));
        }
      } else {
        aData->trailingWhitespace = 0;
      }
    }

    if (preformattedTab) {
      PropertyProvider::Spacing spacing;
      provider.GetSpacing(i, 1, &spacing);
      aData->currentLine += nscoord(spacing.mBefore);
      gfxFloat afterTab =
        AdvanceToNextTab(aData->currentLine, this,
                         mTextRun, &tabWidth);
      aData->currentLine = nscoord(afterTab + spacing.mAfter);
      lineStart = i + 1;
    } else if (preformattedNewline) {
      aData->ForceBreak(aRenderingContext);
      lineStart = i;
    }
  }

  
  if (start < flowEndInTextRun) {
    aData->skipWhitespace =
      IsTrimmableSpace(provider.GetFragment(),
                       iter.ConvertSkippedToOriginal(flowEndInTextRun - 1),
                       textStyle);
  }
}



 void
nsTextFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefWidthData *aData)
{
  nsTextFrame* f;
  gfxTextRun* lastTextRun = nsnull;
  
  
  for (f = this; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
    
    
    
    if (f == this || f->mTextRun != lastTextRun) {
      nsIFrame* lc;
      if (aData->lineContainer &&
          aData->lineContainer != (lc = FindLineContainer(f))) {
        NS_ASSERTION(f != this, "wrong InlinePrefWidthData container"
                                " for first continuation");
        aData->line = nsnull;
        aData->lineContainer = lc;
      }

      
      f->AddInlinePrefWidthForFlow(aRenderingContext, aData);
      lastTextRun = f->mTextRun;
    }
  }
}

 nsSize
nsTextFrame::ComputeSize(nsIRenderingContext *aRenderingContext,
                         nsSize aCBSize, nscoord aAvailableWidth,
                         nsSize aMargin, nsSize aBorder, nsSize aPadding,
                         PRBool aShrinkWrap)
{
  
  return nsSize(NS_UNCONSTRAINEDSIZE, NS_UNCONSTRAINEDSIZE);
}

static nsRect
RoundOut(const gfxRect& aRect)
{
  nsRect r;
  r.x = NSToCoordFloor(aRect.X());
  r.y = NSToCoordFloor(aRect.Y());
  r.width = NSToCoordCeil(aRect.XMost()) - r.x;
  r.height = NSToCoordCeil(aRect.YMost()) - r.y;
  return r;
}

nsRect
nsTextFrame::ComputeTightBounds(gfxContext* aContext) const
{
  if ((GetStyleContext()->HasTextDecorations() &&
       eCompatibility_NavQuirks == PresContext()->CompatibilityMode()) ||
      (GetStateBits() & TEXT_HYPHEN_BREAK)) {
    
    return GetOverflowRect();
  }

  gfxSkipCharsIterator iter = const_cast<nsTextFrame*>(this)->EnsureTextRun();
  if (!mTextRun)
    return nsRect(0, 0, 0, 0);

  PropertyProvider provider(const_cast<nsTextFrame*>(this), iter);
  
  provider.InitializeForDisplay(PR_TRUE);

  gfxTextRun::Metrics metrics =
        mTextRun->MeasureText(provider.GetStart().GetSkippedOffset(),
                              ComputeTransformedLength(provider),
                              gfxFont::TIGHT_HINTED_OUTLINE_EXTENTS,
                              aContext, &provider);
  
  
  return RoundOut(metrics.mBoundingBox) + nsPoint(0, mAscent);
}

static PRBool
HasSoftHyphenBefore(const nsTextFragment* aFrag, gfxTextRun* aTextRun,
                    PRInt32 aStartOffset, const gfxSkipCharsIterator& aIter)
{
  if (!(aTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_SHY))
    return PR_FALSE;
  gfxSkipCharsIterator iter = aIter;
  while (iter.GetOriginalOffset() > aStartOffset) {
    iter.AdvanceOriginal(-1);
    if (!iter.IsOriginalCharSkipped())
      break;
    if (aFrag->CharAt(iter.GetOriginalOffset()) == CH_SHY)
      return PR_TRUE;
  }
  return PR_FALSE;
}

void
nsTextFrame::SetLength(PRInt32 aLength)
{
  mContentLengthHint = aLength;
  PRInt32 end = GetContentOffset() + aLength;
  nsTextFrame* f = static_cast<nsTextFrame*>(GetNextInFlow());
  if (!f)
    return;
  if (end < f->mContentOffset) {
    
    f->mContentOffset = end;
    if (f->GetTextRun() != mTextRun) {
      ClearTextRun();
      f->ClearTextRun();
    }
    return;
  }
  while (f && f->mContentOffset < end) {
    
    f->mContentOffset = end;
    if (f->GetTextRun() != mTextRun) {
      ClearTextRun();
      f->ClearTextRun();
    }
    f = static_cast<nsTextFrame*>(f->GetNextInFlow());
  }
#ifdef DEBUG
  f = this;
  PRInt32 iterations = 0;
  while (f && iterations < 10) {
    f->GetContentLength(); 
    f = static_cast<nsTextFrame*>(f->GetNextContinuation());
    ++iterations;
  }
  f = this;
  iterations = 0;
  while (f && iterations < 10) {
    f->GetContentLength(); 
    f = static_cast<nsTextFrame*>(f->GetPrevContinuation());
    ++iterations;
  }
#endif
}

PRBool
nsTextFrame::IsFloatingFirstLetterChild()
{
  if (!(GetStateBits() & TEXT_FIRST_LETTER))
    return PR_FALSE;
  nsIFrame* frame = GetParent();
  if (!frame || frame->GetType() != nsGkAtoms::letterFrame)
    return PR_FALSE;
  return frame->GetStyleDisplay()->IsFloating();
}

NS_IMETHODIMP
nsTextFrame::Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTextFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);
#ifdef NOISY_REFLOW
  ListTag(stdout);
  printf(": BeginReflow: availableSize=%d,%d\n",
         aReflowState.availableWidth, aReflowState.availableHeight);
#endif

  
  
  

  
  
  
  RemoveStateBits(TEXT_REFLOW_FLAGS | TEXT_WHITESPACE_FLAGS);

  
  
  
  
  PRInt32 maxContentLength = GetInFlowContentLength();

  
  
  
  
  if (!aReflowState.mLineLayout || !maxContentLength) {
    ClearMetrics(aMetrics);
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  nsLineLayout& lineLayout = *aReflowState.mLineLayout;

  if (aReflowState.mFlags.mBlinks) {
    if (0 == (mState & TEXT_BLINK_ON_OR_PRINTING) && PresContext()->IsDynamic()) {
      mState |= TEXT_BLINK_ON_OR_PRINTING;
      nsBlinkTimer::AddBlinkFrame(aPresContext, this);
    }
  }
  else {
    if (0 != (mState & TEXT_BLINK_ON_OR_PRINTING) && PresContext()->IsDynamic()) {
      mState &= ~TEXT_BLINK_ON_OR_PRINTING;
      nsBlinkTimer::RemoveBlinkFrame(this);
    }
  }

  const nsStyleText* textStyle = GetStyleText();

  PRBool atStartOfLine = lineLayout.LineIsEmpty();
  if (atStartOfLine) {
    AddStateBits(TEXT_START_OF_LINE);
  }

  PRUint32 flowEndInTextRun;
  nsIFrame* lineContainer = lineLayout.GetLineContainerFrame();
  gfxContext* ctx = aReflowState.rendContext->ThebesContext();
  const nsTextFragment* frag = GetFragment();

  
  
  
  PRInt32 length = maxContentLength;
  PRInt32 offset = GetContentOffset();

  
  PRInt32 newLineOffset = -1; 
  if (textStyle->NewlineIsSignificant()) {
    newLineOffset = FindChar(frag, offset, length, '\n');
    if (newLineOffset >= 0) {
      length = newLineOffset + 1 - offset;
    }
  }
  if (atStartOfLine && !textStyle->WhiteSpaceIsSignificant()) {
    
    
    PRInt32 skipLength = newLineOffset >= 0 ? length - 1 : length;
    PRInt32 whitespaceCount =
      GetTrimmableWhitespaceCount(frag, offset, skipLength, 1);
    offset += whitespaceCount;
    length -= whitespaceCount;
  }

  PRBool completedFirstLetter = PR_FALSE;
  
  
  if (lineLayout.GetInFirstLetter() || lineLayout.GetInFirstLine()) {
    SetLength(maxContentLength);

    if (lineLayout.GetInFirstLetter()) {
      
      
      
      ClearTextRun();
      
      gfxSkipCharsIterator iter =
        EnsureTextRun(ctx, lineContainer, lineLayout.GetLine(), &flowEndInTextRun);

      if (mTextRun) {
        PRInt32 firstLetterLength = length;
        if (lineLayout.GetFirstLetterStyleOK()) {
          completedFirstLetter =
            FindFirstLetterRange(frag, mTextRun, offset, iter, &firstLetterLength);
          if (newLineOffset >= 0) {
            
            firstLetterLength = NS_MIN(firstLetterLength, length - 1);
            if (length == 1) {
              
              
              
              completedFirstLetter = PR_TRUE;
            }
          }
        } else {
          
          
          
          
          
          firstLetterLength = 0;
          completedFirstLetter = PR_TRUE;
        }
        length = firstLetterLength;
        if (length) {
          AddStateBits(TEXT_FIRST_LETTER);
        }
        
        
        
        SetLength(offset + length - GetContentOffset());
        
        ClearTextRun();
      }
    } 
  }

  gfxSkipCharsIterator iter =
    EnsureTextRun(ctx, lineContainer, lineLayout.GetLine(), &flowEndInTextRun);

  if (mTextRun && iter.GetOriginalEnd() < offset + length) {
    
    
    
    
    ClearTextRun();
    iter = EnsureTextRun(ctx, lineContainer,
                         lineLayout.GetLine(), &flowEndInTextRun);
  }

  if (!mTextRun) {
    ClearMetrics(aMetrics);
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  NS_ASSERTION(gfxSkipCharsIterator(iter).ConvertOriginalToSkipped(offset + length)
                    <= mTextRun->GetLength(),
               "Text run does not map enough text for our reflow");

  
  
  
  
  iter.SetOriginalOffset(offset);
  nscoord xOffsetForTabs = (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TAB) ?
    (lineLayout.GetCurrentFrameXDistanceFromBlock() -
       lineContainer->GetUsedBorderAndPadding().left)
    : -1;
  PropertyProvider provider(mTextRun, textStyle, frag, this, iter, length,
      lineContainer, xOffsetForTabs);

  PRUint32 transformedOffset = provider.GetStart().GetSkippedOffset();

  
  gfxTextRun::Metrics textMetrics;
  gfxFont::BoundingBoxType boundingBoxType = IsFloatingFirstLetterChild() ?
                                               gfxFont::TIGHT_HINTED_OUTLINE_EXTENTS :
                                               gfxFont::LOOSE_INK_EXTENTS;
#ifdef MOZ_MATHML
  NS_ASSERTION(!(NS_REFLOW_CALC_BOUNDING_METRICS & aMetrics.mFlags),
               "We shouldn't be passed NS_REFLOW_CALC_BOUNDING_METRICS anymore");
#endif

  PRInt32 limitLength = length;
  PRInt32 forceBreak = lineLayout.GetForcedBreakPosition(mContent);
  PRBool forceBreakAfter = PR_FALSE;
  if (forceBreak >= offset + length) {
    forceBreakAfter = forceBreak == offset + length;
    
    forceBreak = -1;
  }
  if (forceBreak >= 0) {
    limitLength = forceBreak - offset;
    NS_ASSERTION(limitLength >= 0, "Weird break found!");
  }
  
  
  PRUint32 transformedLength;
  if (offset + limitLength >= PRInt32(frag->GetLength())) {
    NS_ASSERTION(offset + limitLength == PRInt32(frag->GetLength()),
                 "Content offset/length out of bounds");
    NS_ASSERTION(flowEndInTextRun >= transformedOffset,
                 "Negative flow length?");
    transformedLength = flowEndInTextRun - transformedOffset;
  } else {
    
    
    gfxSkipCharsIterator iter(provider.GetStart());
    iter.SetOriginalOffset(offset + limitLength);
    transformedLength = iter.GetSkippedOffset() - transformedOffset;
  }
  PRUint32 transformedLastBreak = 0;
  PRBool usedHyphenation;
  gfxFloat trimmedWidth = 0;
  gfxFloat availWidth = aReflowState.availableWidth;
  PRBool canTrimTrailingWhitespace = !textStyle->WhiteSpaceIsSignificant();
  PRInt32 unusedOffset;  
  gfxBreakPriority breakPriority;
  lineLayout.GetLastOptionalBreakPosition(&unusedOffset, &breakPriority);
  PRUint32 transformedCharsFit =
    mTextRun->BreakAndMeasureText(transformedOffset, transformedLength,
                                  (GetStateBits() & TEXT_START_OF_LINE) != 0,
                                  availWidth,
                                  &provider, !lineLayout.LineIsBreakable(),
                                  canTrimTrailingWhitespace ? &trimmedWidth : nsnull,
                                  &textMetrics, boundingBoxType, ctx,
                                  &usedHyphenation, &transformedLastBreak,
                                  textStyle->WordCanWrap(), &breakPriority);
  
  
  
  gfxSkipCharsIterator end(provider.GetEndHint());
  end.SetSkippedOffset(transformedOffset + transformedCharsFit);
  PRInt32 charsFit = end.GetOriginalOffset() - offset;
  if (offset + charsFit == newLineOffset) {
    
    
    
    
    ++charsFit;
  }
  
  
  
  PRInt32 lastBreak = -1;
  if (charsFit >= limitLength) {
    charsFit = limitLength;
    if (transformedLastBreak != PR_UINT32_MAX) {
      
      
      lastBreak = end.ConvertSkippedToOriginal(transformedOffset + transformedLastBreak);
    }
    end.SetOriginalOffset(offset + charsFit);
    
    
    if ((forceBreak >= 0 || forceBreakAfter) &&
        HasSoftHyphenBefore(frag, mTextRun, offset, end)) {
      usedHyphenation = PR_TRUE;
    }
  }
  if (usedHyphenation) {
    
    AddHyphenToMetrics(this, mTextRun, &textMetrics, boundingBoxType, ctx);
    AddStateBits(TEXT_HYPHEN_BREAK | TEXT_HAS_NONCOLLAPSED_CHARACTERS);
  }

  gfxFloat trimmableWidth = 0;
  PRBool brokeText = forceBreak >= 0 || transformedCharsFit < transformedLength;
  if (canTrimTrailingWhitespace) {
    
    
    
    
    
    
    if (brokeText) {
      
      
      AddStateBits(TEXT_TRIMMED_TRAILING_WHITESPACE);
    } else {
      
      
      
      
      textMetrics.mAdvanceWidth += trimmedWidth;
      trimmableWidth = trimmedWidth;
      if (mTextRun->IsRightToLeft()) {
        
        
        textMetrics.mBoundingBox.MoveBy(gfxPoint(trimmedWidth, 0));
      }
    }
  }

  if (!brokeText && lastBreak >= 0) {
    
    
    NS_ASSERTION(textMetrics.mAdvanceWidth - trimmableWidth <= aReflowState.availableWidth,
                 "If the text doesn't fit, and we have a break opportunity, why didn't MeasureText use it?");
    lineLayout.NotifyOptionalBreakPosition(mContent, lastBreak, PR_TRUE, breakPriority);
  }

  PRInt32 contentLength = offset + charsFit - GetContentOffset();

  
  
  

  
  
  if (GetStateBits() & TEXT_FIRST_LETTER) {
    textMetrics.mAscent = NS_MAX(gfxFloat(0.0), -textMetrics.mBoundingBox.Y());
    textMetrics.mDescent = NS_MAX(gfxFloat(0.0), textMetrics.mBoundingBox.YMost());
  }

  
  
  aMetrics.width = NSToCoordCeil(NS_MAX(gfxFloat(0.0), textMetrics.mAdvanceWidth));

  if (transformedCharsFit == 0 && !usedHyphenation) {
    aMetrics.ascent = 0;
    aMetrics.height = 0;
  } else if (boundingBoxType != gfxFont::LOOSE_INK_EXTENTS) {
    
    aMetrics.ascent = NSToCoordCeil(textMetrics.mAscent);
    aMetrics.height = aMetrics.ascent + NSToCoordCeil(textMetrics.mDescent);
  } else {
    
    
    
    nscoord fontAscent, fontDescent;
    nsIFontMetrics* fm = provider.GetFontMetrics();
    fm->GetMaxAscent(fontAscent);
    fm->GetMaxDescent(fontDescent);
    aMetrics.ascent = NS_MAX(NSToCoordCeil(textMetrics.mAscent), fontAscent);
    nscoord descent = NS_MAX(NSToCoordCeil(textMetrics.mDescent), fontDescent);
    aMetrics.height = aMetrics.ascent + descent;
  }

  NS_ASSERTION(aMetrics.ascent >= 0, "Negative ascent???");
  NS_ASSERTION(aMetrics.height - aMetrics.ascent >= 0, "Negative descent???");

  mAscent = aMetrics.ascent;

  
  nsRect boundingBox = RoundOut(textMetrics.mBoundingBox) + nsPoint(0, mAscent);
  aMetrics.mOverflowArea.UnionRect(boundingBox,
                                   nsRect(0, 0, aMetrics.width, aMetrics.height));

  UnionTextDecorationOverflow(aPresContext, provider, &aMetrics.mOverflowArea);

  
  
  

  
  
  
  
  
  if (transformedCharsFit > 0) {
    lineLayout.SetTrimmableWidth(NSToCoordFloor(trimmableWidth));
    AddStateBits(TEXT_HAS_NONCOLLAPSED_CHARACTERS);
  }
  if (charsFit > 0 && charsFit == length &&
      HasSoftHyphenBefore(frag, mTextRun, offset, end)) {
    
    lineLayout.NotifyOptionalBreakPosition(mContent, offset + length,
        textMetrics.mAdvanceWidth + provider.GetHyphenWidth() <= availWidth,
                                           eNormalBreak);
  }
  PRBool breakAfter = forceBreakAfter;
  
  PRBool emptyTextAtStartOfLine = atStartOfLine && length == 0;
  if (!breakAfter && charsFit == length && !emptyTextAtStartOfLine &&
      transformedOffset + transformedLength == mTextRun->GetLength() &&
      (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TRAILING_BREAK)) {
    
    
    

    
    
    
    
    if (textMetrics.mAdvanceWidth - trimmableWidth > availWidth) {
      breakAfter = PR_TRUE;
    } else {
      lineLayout.NotifyOptionalBreakPosition(mContent, offset + length, PR_TRUE,
                                             eNormalBreak);
    }
  }

  
  aStatus = contentLength == maxContentLength
    ? NS_FRAME_COMPLETE : NS_FRAME_NOT_COMPLETE;

  if (charsFit == 0 && length > 0) {
    
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  } else if (contentLength > 0 && mContentOffset + contentLength - 1 == newLineOffset) {
    
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    lineLayout.SetLineEndsInBR(PR_TRUE);
  } else if (breakAfter) {
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
  }
  if (completedFirstLetter) {
    lineLayout.SetFirstLetterStyleOK(PR_FALSE);
    aStatus |= NS_INLINE_BREAK_FIRST_LETTER_COMPLETE;
  }

  
  if (!textStyle->WhiteSpaceIsSignificant() &&
      lineContainer->GetStyleText()->mTextAlign == NS_STYLE_TEXT_ALIGN_JUSTIFY) {
    AddStateBits(TEXT_JUSTIFICATION_ENABLED);    
    
    PRInt32 numJustifiableCharacters =
      provider.ComputeJustifiableCharacters(offset, charsFit);

    NS_ASSERTION(numJustifiableCharacters <= charsFit,
                 "Bad justifiable character count");
    lineLayout.SetTextJustificationWeights(numJustifiableCharacters,
        charsFit - numJustifiableCharacters);
  }

  SetLength(contentLength);

  if (mContent->HasFlag(NS_TEXT_IN_SELECTION)) {
    
    SelectionDetails* details = GetSelectionDetails();
    if (details) {
      AddStateBits(NS_FRAME_SELECTED_CONTENT);
      DestroySelectionDetails(details);
    } else {
      RemoveStateBits(NS_FRAME_SELECTED_CONTENT);
    }
  }

  Invalidate(aMetrics.mOverflowArea);

#ifdef NOISY_REFLOW
  ListTag(stdout);
  printf(": desiredSize=%d,%d(b=%d) status=%x\n",
         aMetrics.width, aMetrics.height, aMetrics.ascent,
         aStatus);
#endif
  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}

 PRBool
nsTextFrame::CanContinueTextRun() const
{
  
  return PR_TRUE;
}

nsTextFrame::TrimOutput
nsTextFrame::TrimTrailingWhiteSpace(nsIRenderingContext* aRC)
{
  TrimOutput result;
  result.mChanged = PR_FALSE;
  result.mLastCharIsJustifiable = PR_FALSE;
  result.mDeltaWidth = 0;

  AddStateBits(TEXT_END_OF_LINE);

  PRInt32 contentLength = GetContentLength();
  if (!contentLength)
    return result;

  gfxContext* ctx = aRC->ThebesContext();
  gfxSkipCharsIterator start = EnsureTextRun(ctx);
  NS_ENSURE_TRUE(mTextRun, result);

  PRUint32 trimmedStart = start.GetSkippedOffset();

  const nsTextFragment* frag = GetFragment();
  TrimmedOffsets trimmed = GetTrimmedOffsets(frag, PR_TRUE);
  gfxSkipCharsIterator trimmedEndIter = start;
  const nsStyleText* textStyle = GetStyleText();
  gfxFloat delta = 0;
  PRUint32 trimmedEnd = trimmedEndIter.ConvertOriginalToSkipped(trimmed.GetEnd());
  
  if (GetStateBits() & TEXT_TRIMMED_TRAILING_WHITESPACE) {
    
    result.mLastCharIsJustifiable = PR_TRUE;
  } else if (trimmed.GetEnd() < GetContentEnd()) {
    gfxSkipCharsIterator end = trimmedEndIter;
    PRUint32 endOffset = end.ConvertOriginalToSkipped(GetContentOffset() + contentLength);
    if (trimmedEnd < endOffset) {
      
      
      PropertyProvider provider(mTextRun, textStyle, frag, this, start, contentLength,
                                nsnull, 0);
      delta = mTextRun->GetAdvanceWidth(trimmedEnd, endOffset - trimmedEnd, &provider);
      
      
      
      result.mLastCharIsJustifiable = PR_TRUE;
      result.mChanged = PR_TRUE;
    }
  }

  if (!result.mLastCharIsJustifiable &&
      (GetStateBits() & TEXT_JUSTIFICATION_ENABLED)) {
    
    PropertyProvider provider(mTextRun, textStyle, frag, this, start, contentLength,
                              nsnull, 0);
    PRBool isCJK = IsChineseJapaneseLangGroup(this);
    gfxSkipCharsIterator justificationStart(start), justificationEnd(trimmedEndIter);
    provider.FindJustificationRange(&justificationStart, &justificationEnd);

    PRInt32 i;
    for (i = justificationEnd.GetOriginalOffset(); i < trimmed.GetEnd(); ++i) {
      if (IsJustifiableCharacter(frag, i, isCJK)) {
        result.mLastCharIsJustifiable = PR_TRUE;
      }
    }
  }

  gfxFloat advanceDelta;
  mTextRun->SetLineBreaks(trimmedStart, trimmedEnd - trimmedStart,
                          (GetStateBits() & TEXT_START_OF_LINE) != 0, PR_TRUE,
                          &advanceDelta, ctx);
  if (advanceDelta != 0) {
    result.mChanged = PR_TRUE;
  }

  
  
  
  result.mDeltaWidth = NSToCoordFloor(delta - advanceDelta);
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  NS_WARN_IF_FALSE(result.mDeltaWidth >= 0,
                   "Negative deltawidth, something odd is happening");

#ifdef NOISY_TRIM
  ListTag(stdout);
  printf(": trim => %d\n", result.mDeltaWidth);
#endif
  return result;
}

nsRect
nsTextFrame::RecomputeOverflowRect()
{
  gfxSkipCharsIterator iter = EnsureTextRun();
  if (!mTextRun)
    return nsRect(nsPoint(0,0), GetSize());

  PropertyProvider provider(this, iter);
  provider.InitializeForDisplay(PR_TRUE);

  gfxTextRun::Metrics textMetrics =
    mTextRun->MeasureText(provider.GetStart().GetSkippedOffset(),
                          ComputeTransformedLength(provider),
                          gfxFont::LOOSE_INK_EXTENTS, nsnull,
                          &provider);

  nsRect boundingBox = RoundOut(textMetrics.mBoundingBox) + nsPoint(0, mAscent);
  boundingBox.UnionRect(boundingBox,
                        nsRect(nsPoint(0,0), GetSize()));

  UnionTextDecorationOverflow(PresContext(), provider, &boundingBox);

  return boundingBox;
}

static PRUnichar TransformChar(const nsStyleText* aStyle, gfxTextRun* aTextRun,
                               PRUint32 aSkippedOffset, PRUnichar aChar)
{
  if (aChar == '\n') {
    return aStyle->NewlineIsSignificant() ? aChar : ' ';
  }
  switch (aStyle->mTextTransform) {
  case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
    nsContentUtils::GetCaseConv()->ToLower(aChar, &aChar);
    break;
  case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
    nsContentUtils::GetCaseConv()->ToUpper(aChar, &aChar);
    break;
  case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
    if (aTextRun->CanBreakLineBefore(aSkippedOffset)) {
      nsContentUtils::GetCaseConv()->ToTitle(aChar, &aChar);
    }
    break;
  }

  return aChar;
}

nsresult nsTextFrame::GetRenderedText(nsAString* aAppendToString,
                                      gfxSkipChars* aSkipChars,
                                      gfxSkipCharsIterator* aSkipIter,
                                      PRUint32 aSkippedStartOffset,
                                      PRUint32 aSkippedMaxLength)
{
  
  gfxSkipCharsBuilder skipCharsBuilder;
  nsTextFrame* textFrame;
  const nsTextFragment* textFrag = GetFragment();
  PRUint32 keptCharsLength = 0;
  PRUint32 validCharsLength = 0;

  
  for (textFrame = this; textFrame;
       textFrame = static_cast<nsTextFrame*>(textFrame->GetNextContinuation())) {
    

    
    gfxSkipCharsIterator iter = textFrame->EnsureTextRun();
    if (!textFrame->mTextRun)
      return NS_ERROR_FAILURE;

    
    
    
    
    TrimmedOffsets trimmedContentOffsets = textFrame->GetTrimmedOffsets(textFrag, PR_FALSE);
    PRInt32 startOfLineSkipChars = trimmedContentOffsets.mStart - textFrame->mContentOffset;
    if (startOfLineSkipChars > 0) {
      skipCharsBuilder.SkipChars(startOfLineSkipChars);
      iter.SetOriginalOffset(trimmedContentOffsets.mStart);
    }

    
    const nsStyleText* textStyle = textFrame->GetStyleText();
    while (iter.GetOriginalOffset() < trimmedContentOffsets.GetEnd() &&
           keptCharsLength < aSkippedMaxLength) {
      
      if (iter.IsOriginalCharSkipped() || ++validCharsLength <= aSkippedStartOffset) {
        skipCharsBuilder.SkipChar();
      } else {
        ++keptCharsLength;
        skipCharsBuilder.KeepChar();
        if (aAppendToString) {
          aAppendToString->Append(
              TransformChar(textStyle, textFrame->mTextRun, iter.GetSkippedOffset(),
                            textFrag->CharAt(iter.GetOriginalOffset())));
        }
      }
      iter.AdvanceOriginal(1);
    }
    if (keptCharsLength >= aSkippedMaxLength) {
      break; 
    }
  }
  
  if (aSkipChars) {
    aSkipChars->TakeFrom(&skipCharsBuilder); 
    if (aSkipIter) {
      
      
      *aSkipIter = gfxSkipCharsIterator(*aSkipChars, GetContentLength());
    }
  }

  return NS_OK;
}

#ifdef DEBUG

void
nsTextFrame::ToCString(nsCString& aBuf, PRInt32* aTotalContentLength) const
{
  
  const nsTextFragment* frag = GetFragment();
  if (!frag) {
    return;
  }

  
  *aTotalContentLength = frag->GetLength();

  PRInt32 contentLength = GetContentLength();
  
  if (0 == contentLength) {
    return;
  }
  PRInt32 fragOffset = GetContentOffset();
  PRInt32 n = fragOffset + contentLength;
  while (fragOffset < n) {
    PRUnichar ch = frag->CharAt(fragOffset++);
    if (ch == '\r') {
      aBuf.AppendLiteral("\\r");
    } else if (ch == '\n') {
      aBuf.AppendLiteral("\\n");
    } else if (ch == '\t') {
      aBuf.AppendLiteral("\\t");
    } else if ((ch < ' ') || (ch >= 127)) {
      aBuf.Append(nsPrintfCString("\\u%04x", ch));
    } else {
      aBuf.Append(ch);
    }
  }
}
#endif

nsIAtom*
nsTextFrame::GetType() const
{
  return nsGkAtoms::textFrame;
}

 PRBool
nsTextFrame::IsEmpty()
{
  NS_ASSERTION(!(mState & TEXT_IS_ONLY_WHITESPACE) ||
               !(mState & TEXT_ISNOT_ONLY_WHITESPACE),
               "Invalid state");
  
  
  const nsStyleText* textStyle = GetStyleText();
  if (textStyle->WhiteSpaceIsSignificant()) {
    
    return PR_FALSE;
  }

  if (mState & TEXT_ISNOT_ONLY_WHITESPACE) {
    return PR_FALSE;
  }

  if (mState & TEXT_IS_ONLY_WHITESPACE) {
    return PR_TRUE;
  }
  
  PRBool isEmpty = IsAllWhitespace(GetFragment(),
          textStyle->mWhiteSpace != NS_STYLE_WHITESPACE_PRE_LINE);
  mState |= (isEmpty ? TEXT_IS_ONLY_WHITESPACE : TEXT_ISNOT_ONLY_WHITESPACE);
  return isEmpty;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTextFrame::GetFrameName(nsAString& aResult) const
{
  return MakeFrameName(NS_LITERAL_STRING("Text"), aResult);
}

NS_IMETHODIMP_(nsFrameState)
nsTextFrame::GetDebugStateBits() const
{
  
  return nsFrame::GetDebugStateBits() &
    ~(TEXT_WHITESPACE_FLAGS | TEXT_REFLOW_FLAGS);
}

NS_IMETHODIMP
nsTextFrame::List(FILE* out, PRInt32 aIndent) const
{
  
  IndentBy(out, aIndent);
  ListTag(out);
#ifdef DEBUG_waterson
  fprintf(out, " [parent=%p]", mParent);
#endif
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }

  PRInt32 totalContentLength;
  nsCAutoString tmp;
  ToCString(tmp, &totalContentLength);

  
  PRBool isComplete = GetContentEnd() == totalContentLength;
  fprintf(out, "[%d,%d,%c] ", 
          GetContentOffset(), GetContentLength(),
          isComplete ? 'T':'F');
  
  if (GetNextSibling()) {
    fprintf(out, " next=%p", static_cast<void*>(GetNextSibling()));
  }
  nsIFrame* prevContinuation = GetPrevContinuation();
  if (nsnull != prevContinuation) {
    fprintf(out, " prev-continuation=%p", static_cast<void*>(prevContinuation));
  }
  if (nsnull != mNextContinuation) {
    fprintf(out, " next-continuation=%p", static_cast<void*>(mNextContinuation));
  }

  
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    if (mState & NS_FRAME_SELECTED_CONTENT) {
      fprintf(out, " [state=%08x] SELECTED", mState);
    } else {
      fprintf(out, " [state=%08x]", mState);
    }
  }
  fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  if (HasOverflowRect()) {
    nsRect overflowArea = GetOverflowRect();
    fprintf(out, " [overflow=%d,%d,%d,%d]", overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  fprintf(out, " sc=%p", static_cast<void*>(mStyleContext));
  nsIAtom* pseudoTag = mStyleContext->GetPseudoType();
  if (pseudoTag) {
    nsAutoString atomString;
    pseudoTag->ToString(atomString);
    fprintf(out, " pst=%s",
            NS_LossyConvertUTF16toASCII(atomString).get());
  }
  fputs("<\n", out);

  
  aIndent++;

  IndentBy(out, aIndent);
  fputs("\"", out);
  fputs(tmp.get(), out);
  fputs("\"\n", out);

  aIndent--;
  IndentBy(out, aIndent);
  fputs(">\n", out);

  return NS_OK;
}
#endif

void
nsTextFrame::AdjustOffsetsForBidi(PRInt32 aStart, PRInt32 aEnd)
{
  AddStateBits(NS_FRAME_IS_BIDI);

  




  ClearTextRun();

  nsTextFrame* prev = static_cast<nsTextFrame*>(GetPrevContinuation());
  if (prev) {
    
    
    PRInt32 prevOffset = prev->GetContentOffset();
    aStart = NS_MAX(aStart, prevOffset);
    aEnd = NS_MAX(aEnd, prevOffset);
    prev->ClearTextRun();
  }

  mContentOffset = aStart;
  SetLength(aEnd - aStart);
}





PRBool
nsTextFrame::HasTerminalNewline() const
{
  return ::HasTerminalNewline(this);
}

PRBool
nsTextFrame::IsAtEndOfLine() const
{
  return (GetStateBits() & TEXT_END_OF_LINE) != 0;
}

const nsTextFragment*
nsTextFrame::GetFragmentInternal() const
{
  return PresContext()->IsDynamic() ? mContent->GetText() :
    nsLayoutUtils::GetTextFragmentForPrinting(this);
}
