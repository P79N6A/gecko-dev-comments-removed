


















































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
#include "nsVoidArray.h"
#include "nsIDOMText.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsICaret.h"
#include "nsCSSPseudoElements.h"
#include "nsCompatibility.h"
#include "nsCSSColorUtils.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsFrame.h"
#include "nsTextFrameUtils.h"
#include "nsTextRunTransformations.h"
#include "nsFrameManager.h"
#include "nsTextFrameTextRunCache.h"
#include "nsExpirationTracker.h"
#include "nsICaseConversion.h"

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

#define TEXT_REFLOW_FLAGS    \
  (TEXT_FIRST_LETTER|TEXT_START_OF_LINE|TEXT_END_OF_LINE|TEXT_HYPHEN_BREAK| \
   TEXT_TRIMMED_TRAILING_WHITESPACE)



#define TEXT_IS_ONLY_WHITESPACE    0x08000000

#define TEXT_ISNOT_ONLY_WHITESPACE 0x10000000

#define TEXT_WHITESPACE_FLAGS      0x18000000



#define TEXT_IS_RUN_OWNER          0x20000000


#define TEXT_BLINK_ON              0x80000000


























class nsTextFrame;
class PropertyProvider;
















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
  void GetIMESelectionColors(PRInt32  aIndex,
                             nscolor* aForeColor,
                             nscolor* aBackColor);
  
  PRBool GetIMEUnderline(PRInt32  aIndex,
                         nscolor* aLineColor,
                         float*   aRelativeSize,
                         PRUint8* aStyle);

  nsPresContext* PresContext() { return mPresContext; }

  enum {
    eIndexRawInput = 0,
    eIndexSelRawText,
    eIndexConvText,
    eIndexSelConvText
  };

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

  
  struct nsIMEStyle {
    PRBool mInit;
    nscolor mTextColor;
    nscolor mBGColor;
    nscolor mUnderlineColor;
    PRUint8 mUnderlineStyle;
  };
  nsIMEStyle mIMEStyle[4];
  
  float mIMEUnderlineRelativeSize;

  
  void InitCommonColors();
  PRBool InitSelectionColors();

  nsIMEStyle* GetIMEStyle(PRInt32 aIndex);
  void InitIMEStyle(PRInt32 aIndex);

  PRBool EnsureSufficientContrast(nscolor *aForeColor, nscolor *aBackColor);

  nscolor GetResolvedForeColor(nscolor aColor, nscolor aDefaultForeColor,
                               nscolor aBackColor);
};

class nsTextFrame : public nsFrame {
public:
  nsTextFrame(nsStyleContext* aContext) : nsFrame(aContext)
  {
    NS_ASSERTION(mContentOffset == 0, "Bogus content offset");
  }
  
  
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();
  
  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);
  
  NS_IMETHOD CharacterDataChanged(nsPresContext* aPresContext,
                                  nsIContent*     aChild,
                                  PRBool          aAppend);
                                  
  NS_IMETHOD DidSetStyleContext();
  
  virtual nsIFrame* GetNextContinuation() const {
    return mNextContinuation;
  }
  NS_IMETHOD SetNextContinuation(nsIFrame* aNextContinuation) {
    NS_ASSERTION (!aNextContinuation || GetType() == aNextContinuation->GetType(),
                  "setting a next continuation with incorrect type!");
    NS_ASSERTION (!nsSplittableFrame::IsInNextContinuationChain(aNextContinuation, this),
                  "creating a loop in continuation chain!");
    mNextContinuation = aNextContinuation;
    if (aNextContinuation)
      aNextContinuation->RemoveStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
    return NS_OK;
  }
  virtual nsIFrame* GetNextInFlowVirtual() const { return GetNextInFlow(); }
  nsIFrame* GetNextInFlow() const {
    return mNextContinuation && (mNextContinuation->GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? 
      mNextContinuation : nsnull;
  }
  NS_IMETHOD SetNextInFlow(nsIFrame* aNextInFlow) {
    NS_ASSERTION (!aNextInFlow || GetType() == aNextInFlow->GetType(),
                  "setting a next in flow with incorrect type!");
    NS_ASSERTION (!nsSplittableFrame::IsInNextContinuationChain(aNextInFlow, this),
                  "creating a loop in continuation chain!");
    mNextContinuation = aNextInFlow;
    if (aNextInFlow)
      aNextInFlow->AddStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
    return NS_OK;
  }
  virtual nsIFrame* GetLastInFlow() const;
  virtual nsIFrame* GetLastContinuation() const;
  
  virtual nsSplittableType GetSplittableType() const {
    return NS_FRAME_SPLITTABLE;
  }
  
  




  virtual nsIAtom* GetType() const;
  
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    
    
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced |
                                             nsIFrame::eLineParticipant));
  }

#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  NS_IMETHOD_(nsFrameState) GetDebugStateBits() const ;
#endif
  
  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint);
   
  NS_IMETHOD SetSelected(nsPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread);
  
  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                                PRInt32* aOffset, PRBool* aSawBeforeType);

  NS_IMETHOD CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aRecurse, PRBool *aFinished, PRBool *_retval);
  
  
  void SetLength(PRInt32 aLength);
  
  NS_IMETHOD GetOffsets(PRInt32 &start, PRInt32 &end)const;
  
  virtual void AdjustOffsetsForBidi(PRInt32 start, PRInt32 end);
  
  NS_IMETHOD GetPointFromOffset(PRInt32                 inOffset,
                                nsPoint*                outPoint);
  
  NS_IMETHOD  GetChildFrameContainingOffset(PRInt32     inContentOffset,
                                            PRBool                  inHint,
                                            PRInt32*                outFrameContentOffset,
                                            nsIFrame*               *outChildFrame);
  
  virtual PRBool IsVisibleInSelection(nsISelection* aSelection);
  
  virtual PRBool IsEmpty();
  virtual PRBool IsSelfEmpty() { return IsEmpty(); }
  
  



  virtual PRBool HasTerminalNewline() const;
  
#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif
  
  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual void AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  virtual PRBool CanContinueTextRun() const;
  NS_IMETHOD TrimTrailingWhiteSpace(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRC,
                                    nscoord& aDeltaWidth,
                                    PRBool& aLastCharIsJustifiable);
  virtual nsresult GetRenderedText(nsAString* aString = nsnull,
                                   gfxSkipChars* aSkipChars = nsnull,
                                   gfxSkipCharsIterator* aSkipIter = nsnull,
                                   PRUint32 aSkippedStartOffset = 0,
                                   PRUint32 aSkippedMaxLength = PR_UINT32_MAX);

  void AddInlineMinWidthForFlow(nsIRenderingContext *aRenderingContext,
                                nsIFrame::InlineMinWidthData *aData);
  void AddInlinePrefWidthForFlow(nsIRenderingContext *aRenderingContext,
                                 InlinePrefWidthData *aData);

  gfxFloat GetSnappedBaselineY(gfxContext* aContext, gfxFloat aY);

  
  void PaintText(nsIRenderingContext* aRenderingContext, nsPoint aPt,
                 const nsRect& aDirtyRect);
  
  void PaintTextDecorations(gfxContext* aCtx, const gfxRect& aDirtyRect,
                            const gfxPoint& aFramePt, nsTextPaintStyle& aTextStyle,
                            PropertyProvider& aProvider);
  
  
  
  PRBool PaintTextWithSelection(gfxContext* aCtx,
                                const gfxPoint& aFramePt,
                                const gfxPoint& aTextBaselinePt,
                                const gfxRect& aDirtyRect,
                                PropertyProvider& aProvider,
                                nsTextPaintStyle& aTextPaintStyle);
  
  
  
  void PaintTextWithSelectionColors(gfxContext* aCtx,
                                    const gfxPoint& aFramePt,
                                    const gfxPoint& aTextBaselinePt,
                                    const gfxRect& aDirtyRect,
                                    PropertyProvider& aProvider,
                                    nsTextPaintStyle& aTextPaintStyle,
                                    SelectionDetails* aDetails,
                                    SelectionType* aAllTypes);
  
  void PaintTextSelectionDecorations(gfxContext* aCtx,
                                     const gfxPoint& aFramePt,
                                     const gfxPoint& aTextBaselinePt,
                                     const gfxRect& aDirtyRect,
                                     PropertyProvider& aProvider,
                                     nsTextPaintStyle& aTextPaintStyle,
                                     SelectionDetails* aDetails,
                                     SelectionType aSelectionType);

  PRInt16 GetSelectionStatus(PRInt16* aSelectionFlags);

#ifdef DEBUG
  void ToCString(nsString& aBuf, PRInt32* aTotalContentLength) const;
#endif

  PRInt32 GetContentOffset() const { return mContentOffset; }
  PRInt32 GetContentLength() const { return GetContentEnd() - mContentOffset; }
  PRInt32 GetContentEnd() const;
  
  
  
  PRInt32 GetContentLengthHint() const { return mContentLengthHint; }

  
  
  
  
  PRInt32 GetInFlowContentLength();

  
  
  void ClearTextRun();
  











  gfxSkipCharsIterator EnsureTextRun(nsIRenderingContext* aRC = nsnull,
                                     nsIFrame* aLineContainer = nsnull,
                                     const nsLineList::iterator* aLine = nsnull,
                                     PRUint32* aFlowEndInTextRun = nsnull);

  gfxTextRun* GetTextRun() { return mTextRun; }
  void SetTextRun(gfxTextRun* aTextRun) { mTextRun = aTextRun; }

  
  
  
  struct TrimmedOffsets {
    PRInt32 mStart;
    PRInt32 mLength;
    PRInt32 GetEnd() { return mStart + mLength; }
  };
  TrimmedOffsets GetTrimmedOffsets(const nsTextFragment* aFrag,
                                   PRBool aTrimAfter);

protected:
  virtual ~nsTextFrame();
  
  nsIFrame*   mNextContinuation;
  
  
  
  
  
  
  
  
  PRInt32     mContentOffset;
  
  
  
  
  PRInt32     mContentLengthHint;
  nscoord     mAscent;
  gfxTextRun* mTextRun;

  SelectionDetails* GetSelectionDetails();
  
  void AdjustSelectionPointsForBidi(SelectionDetails *sdptr,
                                    PRInt32 textLength,
                                    PRBool isRTLChars,
                                    PRBool isOddLevel,
                                    PRBool isBidiSystem);
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
  NS_ASSERTION(aFrame->GetStateBits() & TEXT_IS_RUN_OWNER,
               "aFrame should be marked as a textrun owner");
  aFrame->RemoveStateBits(TEXT_IS_RUN_OWNER);
  while (aFrame) {
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
  return next ? next->GetContentOffset() : mContent->GetText()->GetLength();
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
  return mContent->TextLength() - mContentOffset;
}






static PRBool IsSpaceCombiningSequenceTail(const nsTextFragment* aFrag, PRUint32 aPos)
{
  NS_ASSERTION(aPos <= aFrag->GetLength(), "Bad offset");
  if (!aFrag->Is2b())
    return PR_FALSE;
  return nsTextFrameUtils::IsSpaceCombiningSequenceTail(
    aFrag->Get2b() + aPos, aFrag->GetLength() - aPos);
}


static PRBool IsCSSWordSpacingSpace(const nsTextFragment* aFrag, PRUint32 aPos)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");
  PRUnichar ch = aFrag->CharAt(aPos);
  if (ch == ' ' || ch == CH_CJKSP)
    return !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  return ch == '\t' || ch == '\n' || ch == '\f';
}



static PRBool IsTrimmableSpace(const PRUnichar* aChars, PRUint32 aLength)
{
  NS_ASSERTION(aLength > 0, "No text for IsSpace!");
  PRUnichar ch = *aChars;
  if (ch == ' ')
    return !nsTextFrameUtils::IsSpaceCombiningSequenceTail(aChars + 1, aLength - 1);
  return ch == '\t' || ch == '\n' || ch == '\f';
}


static PRBool IsTrimmableSpace(char aCh)
{
  return aCh == ' ' || aCh == '\t' || aCh == '\n' || aCh == '\f';
}

static PRBool IsTrimmableSpace(const nsTextFragment* aFrag, PRUint32 aPos)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");
  PRUnichar ch = aFrag->CharAt(aPos);
  if (ch == ' ')
    return !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  return ch == '\t' || ch == '\n' || ch == '\f';
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
GetTrimmableWhitespaceCount(const nsTextFragment* aFrag, PRInt32 aStartOffset,
                            PRInt32 aLength, PRInt32 aDirection)
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










class BuildTextRunsScanner {
public:
  BuildTextRunsScanner(nsPresContext* aPresContext, gfxContext* aContext,
      nsIFrame* aLineContainer) :
    mCurrentFramesAllSameTextRun(nsnull),
    mContext(aContext),
    mLineContainer(aLineContainer),
    mBidiEnabled(aPresContext->BidiEnabled()),    
    mTrimNextRunLeadingWhitespace(PR_FALSE), mSkipIncompleteTextRuns(PR_FALSE) {
    ResetRunInfo();
  }

  void SetAtStartOfLine() {
    mStartOfLine = PR_TRUE;
  }
  void SetSkipIncompleteTextRuns(PRBool aSkip) {
    mSkipIncompleteTextRuns = aSkip;
  }
  void SetCommonAncestorWithLastFrame(nsIFrame* aFrame) {
    mCommonAncestorWithLastFrame = aFrame;
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
  void FlushFrames(PRBool aFlushLineBreaks);
  void ResetRunInfo() {
    mLastFrame = nsnull;
    mMappedFlows.Clear();
    mLineBreakBeforeFrames.Clear();
    mMaxTextLength = 0;
    mDoubleByteText = PR_FALSE;
  }
  void AccumulateRunInfo(nsTextFrame* aFrame);
  void BuildTextRunForFrames(void* aTextBuffer);
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
    PRInt32      mContentOffset;
    PRInt32      mContentEndOffset;
    PRUint32     mTransformedTextOffset; 
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
  PRPackedBool                  mTrimNextRunLeadingWhitespace;
  PRPackedBool                  mCurrentRunTrimLeadingWhitespace;
  PRPackedBool                  mSkipIncompleteTextRuns;
};

static nsIFrame*
FindLineContainer(nsIFrame* aFrame)
{
  while (aFrame && aFrame->IsFrameOfType(nsIFrame::eLineParticipant)) {
    aFrame = aFrame->GetParent();
  }
  return aFrame;
}

static PRBool
TextContainsLineBreakerWhiteSpace(const void* aText, PRUint32 aLength,
                                  PRBool aIsDoubleByte)
{
  PRUint32 i;
  if (aIsDoubleByte) {
    const PRUnichar* chars = static_cast<const PRUnichar*>(aText);
    for (i = 0; i < aLength; ++i) {
      if (nsLineBreaker::IsSpace(chars[i]))
        return PR_TRUE;
    }
    return PR_FALSE;
  } else {
    const PRUint8* chars = static_cast<const PRUint8*>(aText);
    for (i = 0; i < aLength; ++i) {
      if (nsLineBreaker::IsSpace(chars[i]))
        return PR_TRUE;
    }
    return PR_FALSE;
  }
}

static PRBool
CanTextRunCrossFrameBoundary(nsIFrame* aFrame)
{
  
  
  
  return aFrame->CanContinueTextRun() ||
    aFrame->GetType() == nsGkAtoms::placeholderFrame;
}

BuildTextRunsScanner::FindBoundaryResult
BuildTextRunsScanner::FindBoundaries(nsIFrame* aFrame, FindBoundaryState* aState)
{
  nsTextFrame* textFrame = aFrame->GetType() == nsGkAtoms::textFrame
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
      const nsTextFragment* frag = textFrame->GetContent()->GetText();
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

  PRBool continueTextRun = CanTextRunCrossFrameBoundary(aFrame);
  PRBool descendInto = PR_TRUE;
  if (!continueTextRun) {
    
    
    descendInto = !aFrame->IsFloatContainingBlock();
    aState->mSeenTextRunBoundaryOnThisLine = PR_TRUE;
    if (aState->mSeenSpaceForLineBreakingOnThisLine)
      return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
  }
  
  if (descendInto) {
    nsIFrame* child = aFrame->GetFirstChild(nsnull);
    while (child) {
      FindBoundaryResult result = FindBoundaries(child, aState);
      if (result != FB_CONTINUE)
        return result;
      child = child->GetNextSibling();
    }
  }

  if (!continueTextRun) {
    aState->mSeenTextRunBoundaryOnThisLine = PR_TRUE;
    if (aState->mSeenSpaceForLineBreakingOnThisLine)
      return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
  }

  return FB_CONTINUE;
}










static void
BuildTextRuns(nsIRenderingContext* aRC, nsTextFrame* aForFrame,
              nsIFrame* aLineContainer, const nsLineList::iterator* aForFrameLine)
{
  if (!aLineContainer) {
    aLineContainer = FindLineContainer(aForFrame);
  } else {
    NS_ASSERTION(!aForFrame || aLineContainer == FindLineContainer(aForFrame), "Wrong line container hint");
  }

  nsPresContext* presContext = aLineContainer->PresContext();
  gfxContext* ctx = static_cast<gfxContext*>
                               (aRC->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT));
  BuildTextRunsScanner scanner(presContext, ctx, aLineContainer);

  nsBlockFrame* block = nsnull;
  aLineContainer->QueryInterface(kBlockFrameCID, (void**)&block);

  if (!block) {
    
    
    scanner.SetAtStartOfLine();
    scanner.SetCommonAncestorWithLastFrame(nsnull);
    nsIFrame* child = aLineContainer->GetFirstChild(nsnull);
    while (child) {
      scanner.ScanFrame(child);
      child = child->GetNextSibling();
    }
    
    scanner.SetAtStartOfLine();
    scanner.FlushFrames(PR_TRUE);
    return;
  }

  
  nsBlockFrame::line_iterator line;
  if (aForFrameLine) {
    line = *aForFrameLine;
  } else {
    NS_ASSERTION(aForFrame, "One of aForFrame or aForFrameLine must be set!");
    nsIFrame* immediateChild =
      nsLayoutUtils::FindChildContainingDescendant(block, aForFrame);
    
    if (immediateChild->GetStateBits() & NS_FRAME_OUT_OF_FLOW) {
      immediateChild =
        nsLayoutUtils::FindChildContainingDescendant(block,
          presContext->FrameManager()->GetPlaceholderFrameFor(immediateChild));
    }
    line = block->FindLineFor(immediateChild);
    NS_ASSERTION(line != block->end_lines(),
                 "Frame is not in the block!!!");
  }

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsBlockFrame::line_iterator firstLine = block->begin_lines();
  nsTextFrame* stopAtFrame = aForFrame;
  nsTextFrame* nextLineFirstTextFrame = nsnull;
  PRBool seenTextRunBoundaryOnLaterLine = PR_FALSE;
  PRBool mayBeginInTextRun = PR_TRUE;
  while (PR_TRUE) {
    if (line == firstLine) {
      mayBeginInTextRun = PR_FALSE;
      break;
    }
    --line;
    PRBool prevLineIsBlock = line->IsBlock();
    ++line;
    if (prevLineIsBlock) {
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
    --line;
  }
  scanner.SetSkipIncompleteTextRuns(mayBeginInTextRun);

  
  
  
  
  nsBlockFrame::line_iterator endLines = block->end_lines();
  NS_ASSERTION(line != endLines && !line->IsBlock(), "Where is this frame anyway??");
  nsIFrame* child = line->mFirstChild;
  do {
    scanner.SetAtStartOfLine();
    scanner.SetCommonAncestorWithLastFrame(nsnull);
    PRInt32 i;
    for (i = line->GetChildCount() - 1; i >= 0; --i) {
      scanner.ScanFrame(child);
      child = child->GetNextSibling();
    }
    ++line;
  } while (line != endLines && !line->IsBlock());

  
  scanner.SetAtStartOfLine();
  scanner.FlushFrames(PR_TRUE);
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





void BuildTextRunsScanner::FlushFrames(PRBool aFlushLineBreaks)
{
  if (mMappedFlows.Length() == 0)
    return;

  if (!mSkipIncompleteTextRuns && mCurrentFramesAllSameTextRun &&
      ((mCurrentFramesAllSameTextRun->GetFlags() & nsTextFrameUtils::TEXT_INCOMING_WHITESPACE) != 0) ==
      mCurrentRunTrimLeadingWhitespace) {
    
    
    
    

    
    
    SetupBreakSinksForTextRun(mCurrentFramesAllSameTextRun, PR_TRUE, PR_FALSE);
    mTrimNextRunLeadingWhitespace =
      (mCurrentFramesAllSameTextRun->GetFlags() & nsTextFrameUtils::TEXT_TRAILING_WHITESPACE) != 0;
  } else {
    nsAutoTArray<PRUint8,BIG_TEXT_NODE_SIZE> buffer;
    if (!buffer.AppendElements(mMaxTextLength*(mDoubleByteText ? 2 : 1)))
      return;
    BuildTextRunForFrames(buffer.Elements());
  }

  if (aFlushLineBreaks) {
    mLineBreaker.Reset();
    PRUint32 i;
    for (i = 0; i < mBreakSinks.Length(); ++i) {
      if (!mBreakSinks[i]->mExistingTextRun || mBreakSinks[i]->mChangedBreaks) {
        
        
      }
    }
    mBreakSinks.Clear();
  }

  ResetRunInfo();
}

void BuildTextRunsScanner::AccumulateRunInfo(nsTextFrame* aFrame)
{
  mMaxTextLength += aFrame->GetContentLength();
  mDoubleByteText |= aFrame->GetContent()->GetText()->Is2b();
  mLastFrame = aFrame;
  mCommonAncestorWithLastFrame = aFrame;

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
  const nsTextFragment* frag = aFrame->GetContent()->GetText();
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
  
  
  
  
  
  
  if (textStyle1->WhiteSpaceIsSignificant() && HasTerminalNewline(aFrame1))
    return PR_FALSE;

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
    if (mappedFlow->mEndFrame == aFrame) {
      NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
                   "Flow-sibling of a text frame is not a text frame?");

      
      
      
      if (mLastFrame->GetStyleContext() == aFrame->GetStyleContext() &&
          !HasTerminalNewline(mLastFrame)) {
        nsTextFrame* frame = static_cast<nsTextFrame*>(aFrame);
        mappedFlow->mEndFrame = static_cast<nsTextFrame*>(frame->GetNextInFlow());
        NS_ASSERTION(mappedFlow->mContentEndOffset == frame->GetContentOffset(),
                     "Overlapping or discontiguous frames => BAD");
        mappedFlow->mContentEndOffset = frame->GetContentEnd();
        if (mCurrentFramesAllSameTextRun != frame->GetTextRun()) {
          mCurrentFramesAllSameTextRun = nsnull;
        }
        AccumulateRunInfo(frame);
        return;
      }
    }
  }

  
  if (aFrame->GetType() == nsGkAtoms::textFrame) {
    nsTextFrame* frame = static_cast<nsTextFrame*>(aFrame);

    if (mLastFrame && !ContinueTextRunAcrossFrames(mLastFrame, frame)) {
      FlushFrames(PR_FALSE);
    }

    MappedFlow* mappedFlow = mMappedFlows.AppendElement();
    if (!mappedFlow)
      return;

    mappedFlow->mStartFrame = frame;
    mappedFlow->mEndFrame = static_cast<nsTextFrame*>(frame->GetNextInFlow());
    mappedFlow->mAncestorControllingInitialBreak = mCommonAncestorWithLastFrame;
    mappedFlow->mContentOffset = frame->GetContentOffset();
    mappedFlow->mContentEndOffset = frame->GetContentEnd();
    
    mappedFlow->mTransformedTextOffset = 0;
    mLastFrame = frame;

    AccumulateRunInfo(frame);
    if (mMappedFlows.Length() == 1) {
      mCurrentFramesAllSameTextRun = frame->GetTextRun();
      mCurrentRunTrimLeadingWhitespace = mTrimNextRunLeadingWhitespace;
    } else {
      if (mCurrentFramesAllSameTextRun != frame->GetTextRun()) {
        mCurrentFramesAllSameTextRun = nsnull;
      }
    }
    return;
  }

  PRBool continueTextRun = CanTextRunCrossFrameBoundary(aFrame);
  PRBool descendInto = PR_TRUE;
  if (!continueTextRun) {
    FlushFrames(PR_TRUE);
    mCommonAncestorWithLastFrame = nsnull;
    
    
    descendInto = !aFrame->IsFloatContainingBlock();
    mStartOfLine = PR_FALSE;
    mTrimNextRunLeadingWhitespace = PR_FALSE;
  }

  if (descendInto) {
    nsIFrame* f;
    for (f = aFrame->GetFirstChild(nsnull); f; f = f->GetNextSibling()) {
      ScanFrame(f);
    }
  }

  if (!continueTextRun) {
    FlushFrames(PR_TRUE);
    mCommonAncestorWithLastFrame = nsnull;
    mTrimNextRunLeadingWhitespace = PR_FALSE;
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
GetSpacingFlags(const nsStyleCoord& aStyleCoord)
{
  nscoord spacing = StyleToCoord(aStyleCoord);
  if (!spacing)
    return 0;
  if (spacing > 0)
    return gfxTextRunFactory::TEXT_ENABLE_SPACING;
  return gfxTextRunFactory::TEXT_ENABLE_SPACING |
         gfxTextRunFactory::TEXT_ENABLE_NEGATIVE_SPACING;
}

static gfxFontGroup*
GetFontGroupForFrame(nsIFrame* aFrame)
{
  nsCOMPtr<nsIFontMetrics> metrics;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(metrics));

  if (!metrics)
    return nsnull;

  nsIFontMetrics* metricsRaw = metrics;
  nsIThebesFontMetrics* fm = static_cast<nsIThebesFontMetrics*>(metricsRaw);
  return fm->GetThebesFontGroup();
}

static gfxTextRun*
GetHyphenTextRun(gfxTextRun* aTextRun, nsIRenderingContext* aRefContext)
{
  if (NS_UNLIKELY(!aRefContext)) {
    return nsnull;
  }
  gfxContext* ctx = static_cast<gfxContext*>
                               (aRefContext->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT));
  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();
  PRUint32 flags = gfxFontGroup::TEXT_IS_PERSISTENT;

  static const PRUnichar unicodeHyphen = 0x2010;
  gfxTextRun* textRun =
    gfxTextRunCache::MakeTextRun(&unicodeHyphen, 1, fontGroup, ctx,
                                 aTextRun->GetAppUnitsPerDevUnit(), flags);
  if (textRun && textRun->CountMissingGlyphs() == 0)
    return textRun;

  static const PRUint8 dash = '-';
  return gfxTextRunCache::MakeTextRun(&dash, 1, fontGroup, ctx,
                                      aTextRun->GetAppUnitsPerDevUnit(),
                                      flags);
}

static gfxFont::Metrics
GetFontMetrics(gfxFontGroup* aFontGroup)
{
  if (!aFontGroup)
    return gfxFont::Metrics();
  gfxFont* font = aFontGroup->GetFontAt(0);
  if (!font)
    return gfxFont::Metrics();
  return font->GetMetrics();
}

void
BuildTextRunsScanner::BuildTextRunForFrames(void* aTextBuffer)
{
  gfxSkipCharsBuilder builder;

  const void* textPtr = aTextBuffer;
  PRBool anySmallcapsStyle = PR_FALSE;
  PRBool anyTextTransformStyle = PR_FALSE;
  nsIContent* lastContent = nsnull;
  PRInt32 endOfLastContent = 0;
  PRUint32 textFlags = gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX |
    nsTextFrameUtils::TEXT_NO_BREAKS;

  if (mCurrentRunTrimLeadingWhitespace) {
    textFlags |= nsTextFrameUtils::TEXT_INCOMING_WHITESPACE;
  }

  nsAutoTArray<PRUint32,50> textBreakPoints;
  
  if (!textBreakPoints.AppendElements(mLineBreakBeforeFrames.Length() + 1))
    return;

  TextRunUserData dummyData;
  TextRunMappedFlow dummyMappedFlow;

  TextRunUserData* userData;
  
  
  if (mMappedFlows.Length() == 1 && !mMappedFlows[0].mEndFrame &&
      !mMappedFlows[0].mContentOffset) {
    userData = &dummyData;
    dummyData.mMappedFlows = &dummyMappedFlow;
  } else {
    userData = static_cast<TextRunUserData*>
                          (nsMemory::Alloc(sizeof(TextRunUserData) + mMappedFlows.Length()*sizeof(TextRunMappedFlow)));
    userData->mMappedFlows = reinterpret_cast<TextRunMappedFlow*>(userData + 1);
  }
  userData->mLastFlowIndex = 0;

  PRUint32 finalMappedFlowCount = 0;
  PRUint32 currentTransformedTextOffset = 0;

  PRUint32 nextBreakIndex = 0;
  nsTextFrame* nextBreakBeforeFrame = GetNextBreakBeforeFrame(&nextBreakIndex);

  PRUint32 i;
  const nsStyleText* textStyle = nsnull;
  const nsStyleFont* fontStyle = nsnull;
  nsStyleContext* lastStyleContext = nsnull;
  for (i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    nsTextFrame* f = mappedFlow->mStartFrame;

    mappedFlow->mTransformedTextOffset = currentTransformedTextOffset;

    lastStyleContext = f->GetStyleContext();
    
    textStyle = f->GetStyleText();
    if (NS_STYLE_TEXT_TRANSFORM_NONE != textStyle->mTextTransform) {
      anyTextTransformStyle = PR_TRUE;
    }
    textFlags |= GetSpacingFlags(textStyle->mLetterSpacing);
    textFlags |= GetSpacingFlags(textStyle->mWordSpacing);
    PRBool compressWhitespace = !textStyle->WhiteSpaceIsSignificant();
    if (NS_STYLE_TEXT_ALIGN_JUSTIFY == textStyle->mTextAlign && compressWhitespace) {
      textFlags |= gfxTextRunFactory::TEXT_ENABLE_SPACING;
    }
    fontStyle = f->GetStyleFont();
    if (NS_STYLE_FONT_VARIANT_SMALL_CAPS == fontStyle->mFont.variant) {
      anySmallcapsStyle = PR_TRUE;
    }

    
    nsIContent* content = f->GetContent();
    const nsTextFragment* frag = content->GetText();
    PRInt32 contentStart = mappedFlow->mContentOffset;
    PRInt32 contentEnd = mappedFlow->mContentEndOffset;
    PRInt32 contentLength = contentEnd - contentStart;

    if (content == lastContent) {
      NS_ASSERTION(endOfLastContent == contentStart,
                   "Gap or overlap in textframes mapping content?!");
      if (contentStart >= contentEnd)
        continue;
      userData->mMappedFlows[finalMappedFlowCount - 1].mContentLength += contentLength;
    } else {
      TextRunMappedFlow* newFlow = &userData->mMappedFlows[finalMappedFlowCount];

      newFlow->mStartFrame = mappedFlow->mStartFrame;
      newFlow->mDOMOffsetToBeforeTransformOffset = builder.GetCharCount() - mappedFlow->mContentOffset;
      newFlow->mContentLength = contentLength;
      ++finalMappedFlowCount;

      while (nextBreakBeforeFrame && nextBreakBeforeFrame->GetContent() == content) {
        textBreakPoints[nextBreakIndex - 1] =
          nextBreakBeforeFrame->GetContentOffset() + newFlow->mDOMOffsetToBeforeTransformOffset;
        nextBreakBeforeFrame = GetNextBreakBeforeFrame(&nextBreakIndex);
      }
    }

    PRUint32 analysisFlags;
    if (frag->Is2b()) {
      NS_ASSERTION(mDoubleByteText, "Wrong buffer char size!");
      PRUnichar* bufStart = static_cast<PRUnichar*>(aTextBuffer);
      PRUnichar* bufEnd = nsTextFrameUtils::TransformText(
          frag->Get2b() + contentStart, contentLength, bufStart,
          compressWhitespace, &mTrimNextRunLeadingWhitespace, &builder, &analysisFlags);
      aTextBuffer = bufEnd;
    } else {
      if (mDoubleByteText) {
        
        
        nsAutoTArray<PRUint8,BIG_TEXT_NODE_SIZE> tempBuf;
        if (!tempBuf.AppendElements(contentLength)) {
          DestroyUserData(userData);
          return;
        }
        PRUint8* bufStart = tempBuf.Elements();
        PRUint8* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const PRUint8*>(frag->Get1b()) + contentStart, contentLength,
            bufStart, compressWhitespace, &mTrimNextRunLeadingWhitespace,
            &builder, &analysisFlags);
        aTextBuffer = ExpandBuffer(static_cast<PRUnichar*>(aTextBuffer),
                                   tempBuf.Elements(), end - tempBuf.Elements());
      } else {
        PRUint8* bufStart = static_cast<PRUint8*>(aTextBuffer);
        PRUint8* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const PRUint8*>(frag->Get1b()) + contentStart, contentLength,
            bufStart,
            compressWhitespace, &mTrimNextRunLeadingWhitespace, &builder, &analysisFlags);
        aTextBuffer = end;
      }
    }
    
    
    if (!compressWhitespace) {
      mTrimNextRunLeadingWhitespace = PR_FALSE;
    }
    textFlags |= analysisFlags;

    currentTransformedTextOffset =
      (static_cast<const PRUint8*>(aTextBuffer) - static_cast<const PRUint8*>(textPtr)) >> mDoubleByteText;

    lastContent = content;
    endOfLastContent = contentEnd;
  }

  
  if (!builder.IsOK()) {
    DestroyUserData(userData);
    return;
  }

  void* finalUserData;
  if (userData == &dummyData) {
    textFlags |= nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW;
    userData = nsnull;
    finalUserData = mMappedFlows[0].mStartFrame;
  } else {
    userData = static_cast<TextRunUserData*>
                          (nsMemory::Realloc(userData, sizeof(TextRunUserData) + finalMappedFlowCount*sizeof(TextRunMappedFlow)));
    if (!userData)
      return;
    userData->mMappedFlows = reinterpret_cast<TextRunMappedFlow*>(userData + 1);
    userData->mMappedFlowCount = finalMappedFlowCount;
    finalUserData = userData;
  }

  PRUint32 transformedLength = currentTransformedTextOffset;

















  
  nsTextFrame* firstFrame = mMappedFlows[0].mStartFrame;
  gfxFontGroup* fontGroup = GetFontGroupForFrame(firstFrame);
  if (!fontGroup) {
    DestroyUserData(userData);
    return;
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
    for (i = 0; i < mMappedFlows.Length(); ++i) {
      MappedFlow* mappedFlow = &mMappedFlows[i];
      PRUint32 end = i == mMappedFlows.Length() - 1 ? transformedLength :
          mMappedFlows[i + 1].mTransformedTextOffset;
      nsStyleContext* sc = mappedFlow->mStartFrame->GetStyleContext();
      PRUint32 j;
      for (j = mappedFlow->mTransformedTextOffset; j < end; ++j) {
        styles.AppendElement(sc);
      }
    }
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
  if (mTrimNextRunLeadingWhitespace) {
    textFlags |= nsTextFrameUtils::TEXT_TRAILING_WHITESPACE;
  }
  
  
  textFlags |= nsLayoutUtils::GetTextRunFlagsForStyle(lastStyleContext,
      textStyle, fontStyle);

  gfxSkipChars skipChars;
  skipChars.TakeFrom(&builder);
  
  NS_ASSERTION(nextBreakIndex == mLineBreakBeforeFrames.Length(),
               "Didn't find all the frames to break-before...");
  gfxSkipCharsIterator iter(skipChars);
  for (i = 0; i < nextBreakIndex; ++i) {
    PRUint32* breakPoint = &textBreakPoints[i];
    *breakPoint = iter.ConvertOriginalToSkipped(*breakPoint);
  }
  if (mStartOfLine) {
    textBreakPoints[nextBreakIndex] = transformedLength;
    ++nextBreakIndex;
  }

  gfxTextRun* textRun;
  gfxTextRunFactory::Parameters params =
      { mContext, finalUserData, &skipChars,
        textBreakPoints.Elements(), nextBreakIndex,
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
    DestroyUserData(userData);
    return;
  }

  
  
  
  
  SetupBreakSinksForTextRun(textRun, PR_FALSE, mSkipIncompleteTextRuns);

  if (mSkipIncompleteTextRuns) {
    mSkipIncompleteTextRuns = !TextContainsLineBreakerWhiteSpace(textPtr,
        transformedLength, mDoubleByteText);
    
    
    gTextRuns->RemoveFromCache(textRun);
    delete textRun;
    DestroyUserData(userData);
    return;
  }

  
  
  AssignTextRun(textRun);
}

static PRBool
HasCompressedLeadingWhitespace(nsTextFrame* aFrame, PRInt32 aContentEndOffset,
                               const gfxSkipCharsIterator& aIterator)
{
  if (!aIterator.IsOriginalCharSkipped())
    return PR_FALSE;

  gfxSkipCharsIterator iter = aIterator;
  PRInt32 frameContentOffset = aFrame->GetContentOffset();
  const nsTextFragment* frag = aFrame->GetContent()->GetText();
  while (frameContentOffset < aContentEndOffset && iter.IsOriginalCharSkipped()) {
    if (IsTrimmableSpace(frag, frameContentOffset))
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
    nsAutoPtr<BreakSink>* breakSink = mBreakSinks.AppendElement(
      new BreakSink(aTextRun, mContext,
                    mappedFlow->mTransformedTextOffset, aIsExistingTextRun));
    if (!breakSink || !*breakSink)
      return;
    PRUint32 offset = mappedFlow->mTransformedTextOffset;

    PRUint32 length =
      (i == mMappedFlows.Length() - 1 ? aTextRun->GetLength()
       : mMappedFlows[i + 1].mTransformedTextOffset)
      - offset;

    nsTextFrame* startFrame = mappedFlow->mStartFrame;
    if (HasCompressedLeadingWhitespace(startFrame, mappedFlow->mContentEndOffset, iter)) {
      mLineBreaker.AppendInvisibleWhitespace();
    }

    if (length > 0) {
      PRUint32 flags = 0;
      nsIFrame* initialBreakController = mappedFlow->mAncestorControllingInitialBreak;
      if (!initialBreakController) {
        initialBreakController = mLineContainer;
      }
      if (initialBreakController->GetStyleText()->WhiteSpaceCanWrap()) {
        flags |= nsLineBreaker::BREAK_ALLOW_INITIAL;
      }
      const nsStyleText* textStyle = startFrame->GetStyleText();
      if (textStyle->WhiteSpaceCanWrap()) {
        
        
        
        flags |= nsLineBreaker::BREAK_ALLOW_INSIDE;
      }
      
      BreakSink* sink = *breakSink;
      if (aSuppressSink) {
        sink = nsnull;
      } else if (flags) {
        aTextRun->ClearFlagBits(nsTextFrameUtils::TEXT_NO_BREAKS);
      } else if (aTextRun->GetFlags() & nsTextFrameUtils::TEXT_NO_BREAKS) {
        
        
        sink = nsnull;
      }
      if (aTextRun->GetFlags() & gfxFontGroup::TEXT_IS_8BIT) {
        mLineBreaker.AppendText(lang, aTextRun->GetText8Bit() + offset,
                                length, flags, sink);
      } else {
        mLineBreaker.AppendText(lang, aTextRun->GetTextUnicode() + offset,
                                length, flags, sink);
      }
    }
    
    iter.AdvanceOriginal(mappedFlow->mContentEndOffset - mappedFlow->mContentOffset);
  }
}

void
BuildTextRunsScanner::AssignTextRun(gfxTextRun* aTextRun)
{
  nsIContent* lastContent = nsnull;
  PRUint32 i;
  for (i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    nsTextFrame* startFrame = mappedFlow->mStartFrame;
    nsTextFrame* endFrame = mappedFlow->mEndFrame;
    nsTextFrame* f;
    for (f = startFrame; f != endFrame;
         f = static_cast<nsTextFrame*>(f->GetNextInFlow())) {
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
    nsIContent* content = startFrame->GetContent();
    
    
    if (content != lastContent) {
      startFrame->AddStateBits(TEXT_IS_RUN_OWNER);
      lastContent = content;
    }    
  }
}

static already_AddRefed<nsIRenderingContext>
GetReferenceRenderingContext(nsTextFrame* aTextFrame, nsIRenderingContext* aRC)
{
  if (aRC) {
    NS_ADDREF(aRC);
    return aRC;
  }

  nsIRenderingContext* result;      
  nsresult rv = aTextFrame->PresContext()->PresShell()->
    CreateRenderingContext(aTextFrame, &result);
  if (NS_FAILED(rv))
    return nsnull;
  return result;      
}

gfxSkipCharsIterator
nsTextFrame::EnsureTextRun(nsIRenderingContext* aRC, nsIFrame* aLineContainer,
                           const nsLineList::iterator* aLine,
                           PRUint32* aFlowEndInTextRun)
{
  if (mTextRun) {
    if (mTextRun->GetExpirationState()->IsTracked()) {
      gTextRuns->MarkUsed(mTextRun);
    }
  } else {
    nsCOMPtr<nsIRenderingContext> rendContext =
      GetReferenceRenderingContext(this, aRC);
    if (rendContext) {
      BuildTextRuns(rendContext, this, aLineContainer, aLine);
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
GetEndOfTrimmedText(const nsTextFragment* aFrag,
                    PRUint32 aStart, PRUint32 aEnd,
                    gfxSkipCharsIterator* aIterator)
{
  aIterator->SetSkippedOffset(aEnd);
  while (aIterator->GetSkippedOffset() > aStart) {
    aIterator->AdvanceSkipped(-1);
    if (!IsTrimmableSpace(aFrag, aIterator->GetOriginalOffset()))
      return aIterator->GetSkippedOffset() + 1;
  }
  return aStart;
}

nsTextFrame::TrimmedOffsets
nsTextFrame::GetTrimmedOffsets(const nsTextFragment* aFrag,
                               PRBool aTrimAfter)
{
  NS_ASSERTION(mTextRun, "Need textrun here");

  TrimmedOffsets offsets = { GetContentOffset(), GetContentLength() };
  const nsStyleText* textStyle = GetStyleText();
  if (textStyle->WhiteSpaceIsSignificant())
    return offsets;

  if (GetStateBits() & TEXT_START_OF_LINE) {
    PRInt32 whitespaceCount =
      GetTrimmableWhitespaceCount(aFrag, offsets.mStart, offsets.mLength, 1);
    offsets.mStart += whitespaceCount;
    offsets.mLength -= whitespaceCount;
  }

  if (aTrimAfter && (GetStateBits() & TEXT_END_OF_LINE) &&
      textStyle->WhiteSpaceCanWrap()) {
    PRInt32 whitespaceCount =
      GetTrimmableWhitespaceCount(aFrag, offsets.GetEnd() - 1,
                                  offsets.mLength, -1);
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
#ifdef MOZ_MATHML
  aMetrics.mBoundingMetrics.Clear();
#endif
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
  gfxSkipCharsIterator iter(aStart);
  iter.AdvanceOriginal(aContentLength);
  return iter.GetSkippedOffset() >= aOffset + aLength;
}
#endif

class PropertyProvider : public gfxTextRun::PropertyProvider {
public:
  



  PropertyProvider(gfxTextRun* aTextRun, const nsStyleText* aTextStyle,
                   const nsTextFragment* aFrag, nsTextFrame* aFrame,
                   const gfxSkipCharsIterator& aStart, PRInt32 aLength,
                   nsIFrame* aLineContainer,
                   nscoord aOffsetFromBlockOriginForTabs)
    : mTextRun(aTextRun), mFontGroup(nsnull), mTextStyle(aTextStyle), mFrag(aFrag),
      mLineContainer(aLineContainer),
      mFrame(aFrame), mStart(aStart), mTempIterator(aStart),
      mTabWidths(nsnull), mLength(aLength),
      mWordSpacing(StyleToCoord(mTextStyle->mWordSpacing)),
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
      mFrag(aFrame->GetContent()->GetText()),
      mLineContainer(nsnull),
      mFrame(aFrame), mStart(aStart), mTempIterator(aStart),
      mTabWidths(nsnull),
      mLength(aFrame->GetContentLength()),
      mWordSpacing(StyleToCoord(mTextStyle->mWordSpacing)),
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
  void FindEndOfJustificationRange(gfxSkipCharsIterator* aIter);

  const nsStyleText* GetStyleText() { return mTextStyle; }
  nsTextFrame* GetFrame() { return mFrame; }
  
  
  
  const gfxSkipCharsIterator& GetStart() { return mStart; }
  PRUint32 GetOriginalLength() { return mLength; }
  const nsTextFragment* GetFragment() { return mFrag; }

  gfxFontGroup* GetFontGroup() {
    if (!mFontGroup) {
      mFontGroup = GetFontGroupForFrame(mFrame);
    }
    return mFontGroup;
  }

  gfxFloat* GetTabWidths(PRUint32 aTransformedStart, PRUint32 aTransformedLength);

  const gfxSkipCharsIterator& GetEndHint() { return mTempIterator; }

protected:
  void SetupJustificationSpacing();
  
  gfxTextRun*           mTextRun;
  gfxFontGroup*         mFontGroup;
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
    !aTextRun->IsLigatureContinuation(aOffset + 1);
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
        if (IsCSSWordSpacingSpace(mFrag, i + run.GetOriginalOffset())) {
          
          
          iter.SetSkippedOffset(run.GetSkippedOffset() + i);
          FindClusterEnd(mTextRun, run.GetOriginalOffset() + run.GetRunLength(),
                         &iter);
          aSpacing[iter.GetSkippedOffset() - aStart].mAfter += mWordSpacing;
        }
      }
    }
  }

  
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
    gfxSkipCharsIterator justificationEnd(mStart);
    FindEndOfJustificationRange(&justificationEnd);

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
          
          if (clusterLastChar < justificationEnd.GetSkippedOffset()) {
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
    
    PRUint32 i;
    if (!mLineContainer) {
      NS_WARNING("Tabs encountered in a situation where we don't support tabbing");
      for (i = tabsEnd; i < aStart + aLength; ++i) {
        (*mTabWidths)[i - startOffset] = 0;
      }
    } else {
      gfxFloat tabWidth = NS_round(8*mTextRun->GetAppUnitsPerDevUnit()*
        GetFontMetrics(GetFontGroupForFrame(mLineContainer)).spaceWidth);
      
      for (i = tabsEnd; i < aStart + aLength; ++i) {
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
          
          
          
          static const double EPSILON = 0.000001;
          double nextTab = NS_ceil(mOffsetFromBlockOriginForTabs/tabWidth)*tabWidth;
          if (nextTab < mOffsetFromBlockOriginForTabs + EPSILON) {
            nextTab += tabWidth;
          }
          (*mTabWidths)[i - startOffset] = nextTab - mOffsetFromBlockOriginForTabs;
          mOffsetFromBlockOriginForTabs = nextTab;
        }
  
        mOffsetFromBlockOriginForTabs += spacing.mAfter;
      }
    }
  }

  return mTabWidths->Elements() + aStart - startOffset;
}

gfxFloat
PropertyProvider::GetHyphenWidth()
{
  if (mHyphenWidth < 0) {
    nsCOMPtr<nsIRenderingContext> rc = GetReferenceRenderingContext(mFrame, nsnull);
    gfxTextRun* hyphenTextRun = GetHyphenTextRun(mTextRun, rc);
    mHyphenWidth = mLetterSpacing;
    if (hyphenTextRun) {
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

  if (!mTextStyle->WhiteSpaceCanWrap()) {
    memset(aBreakBefore, PR_FALSE, aLength);
    return;
  }

  
  nsSkipCharsRunIterator
    run(mStart, nsSkipCharsRunIterator::LENGTH_UNSKIPPED_ONLY, aLength);
  run.SetSkippedOffset(aStart);
  
  run.SetVisitSkipped();

  PRBool allowHyphenBreakBeforeNextChar =
    run.GetPos().GetOriginalOffset() > mStart.GetOriginalOffset() &&
    mFrag->CharAt(run.GetPos().GetOriginalOffset() - 1) == CH_SHY;

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
PropertyProvider::FindEndOfJustificationRange(gfxSkipCharsIterator* aIter)
{
  aIter->SetOriginalOffset(mStart.GetOriginalOffset() + mLength);

  
  if (!(mFrame->GetStateBits() & TEXT_END_OF_LINE))
    return;
  while (aIter->GetOriginalOffset() > mStart.GetOriginalOffset()) {
    aIter->AdvanceOriginal(-1);
    if (!aIter->IsOriginalCharSkipped() &&
        mTextRun->IsClusterStart(aIter->GetSkippedOffset()))
      break;
  }
}

void
PropertyProvider::SetupJustificationSpacing()
{
  if (NS_STYLE_TEXT_ALIGN_JUSTIFY != mTextStyle->mTextAlign ||
      mTextStyle->WhiteSpaceIsSignificant())
    return;

  gfxSkipCharsIterator end(mStart);
  end.AdvanceOriginal(mLength);
  gfxSkipCharsIterator realEnd(end);
  FindEndOfJustificationRange(&end);

  PRInt32 justifiableCharacters =
    ComputeJustifiableCharacters(mStart.GetOriginalOffset(),
                                 end.GetOriginalOffset() - mStart.GetOriginalOffset());
  if (justifiableCharacters == 0) {
    
    
    return;
  }

  gfxFloat naturalWidth =
    mTextRun->GetAdvanceWidth(mStart.GetSkippedOffset(),
                              GetSkippedDistance(mStart, realEnd), this);
  if (mFrame->GetStateBits() & TEXT_HYPHEN_BREAK) {
    nsCOMPtr<nsIRenderingContext> rc = GetReferenceRenderingContext(mFrame, nsnull);
    gfxTextRun* hyphenTextRun = GetHyphenTextRun(mTextRun, rc);
    if (hyphenTextRun) {
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

  nsCOMPtr<nsITimer> mTimer;
  nsVoidArray     mFrames;
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
  }
}

NS_IMPL_ISUPPORTS1(nsBlinkTimer, nsITimerCallback)

void nsBlinkTimer::AddFrame(nsPresContext* aPresContext, nsIFrame* aFrame) {
  FrameData* frameData = new FrameData(aPresContext, aFrame);
  mFrames.AppendElement(frameData);
  if (1 == mFrames.Count()) {
    Start();
  }
}

PRBool nsBlinkTimer::RemoveFrame(nsIFrame* aFrame) {
  PRInt32 i, n = mFrames.Count();
  PRBool rv = PR_FALSE;
  for (i = 0; i < n; i++) {
    FrameData* frameData = (FrameData*) mFrames.ElementAt(i);

    if (frameData->mFrame == aFrame) {
      rv = mFrames.RemoveElementAt(i);
      delete frameData;
      break;
    }
  }
  
  if (0 == mFrames.Count()) {
    Stop();
  }
  return rv;
}

PRInt32 nsBlinkTimer::FrameCount() {
  return mFrames.Count();
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

  PRInt32 i, n = mFrames.Count();
  for (i = 0; i < n; i++) {
    FrameData* frameData = (FrameData*) mFrames.ElementAt(i);

    
    
    nsRect bounds(nsPoint(0, 0), frameData->mFrame->GetSize());
    frameData->mFrame->Invalidate(bounds, PR_FALSE);
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
  PRUint16  hue,sat,value;

  
  NS_RGB2HSV(aColor,hue,sat,value);

  
  
  
  
  
  
  if (value > sat) {
    value = sat;
    
    NS_HSV2RGB(aColor,hue,sat,value);
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
  for (int i = 0; i < 4; i++)
    mIMEStyle[i].mInit = PR_FALSE;
  mIMEUnderlineRelativeSize = -1.0f;
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
nsTextPaintStyle::GetIMESelectionColors(PRInt32  aIndex,
                                        nscolor* aForeColor,
                                        nscolor* aBackColor)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");
  NS_ASSERTION(aIndex >= 0 && aIndex < 4, "Index out of range");

  nsIMEStyle* IMEStyle = GetIMEStyle(aIndex);
  *aForeColor = IMEStyle->mTextColor;
  *aBackColor = IMEStyle->mBGColor;
}

PRBool
nsTextPaintStyle::GetIMEUnderline(PRInt32  aIndex,
                                  nscolor* aLineColor,
                                  float*   aRelativeSize,
                                  PRUint8* aStyle)
{
  NS_ASSERTION(aLineColor, "aLineColor is null");
  NS_ASSERTION(aRelativeSize, "aRelativeSize is null");
  NS_ASSERTION(aIndex >= 0 && aIndex < 4, "Index out of range");

  nsIMEStyle* IMEStyle = GetIMEStyle(aIndex);
  if (IMEStyle->mUnderlineStyle == NS_STYLE_BORDER_STYLE_NONE ||
      IMEStyle->mUnderlineColor == NS_TRANSPARENT ||
      mIMEUnderlineRelativeSize <= 0.0f)
    return PR_FALSE;

  *aLineColor = IMEStyle->mUnderlineColor;
  *aRelativeSize = mIMEUnderlineRelativeSize;
  *aStyle = IMEStyle->mUnderlineStyle;
  return PR_TRUE;
}

void
nsTextPaintStyle::InitCommonColors()
{
  if (mInitCommonColors)
    return;

  nsStyleContext* sc = mFrame->GetStyleContext();

  const nsStyleBackground* bg =
    nsCSSRendering::FindNonTransparentBackground(sc);
  NS_ASSERTION(bg, "Cannot find NonTransparentBackground.");
  mFrameBackgroundColor = bg->mBackgroundColor;

  nsILookAndFeel* look = mPresContext->LookAndFeel();
  nscolor defaultWindowBackgroundColor, selectionTextColor, selectionBGColor;
  look->GetColor(nsILookAndFeel::eColor_TextSelectBackground,
                 selectionBGColor);
  look->GetColor(nsILookAndFeel::eColor_TextSelectForeground,
                 selectionTextColor);
  look->GetColor(nsILookAndFeel::eColor_WindowBackground,
                 defaultWindowBackgroundColor);

  mSufficientContrast =
    PR_MIN(PR_MIN(NS_SUFFICIENT_LUMINOSITY_DIFFERENCE,
                  NS_LUMINOSITY_DIFFERENCE(selectionTextColor,
                                           selectionBGColor)),
                  NS_LUMINOSITY_DIFFERENCE(defaultWindowBackgroundColor,
                                           selectionBGColor));

  mInitCommonColors = PR_TRUE;
}

static nsIFrame* GetNonGeneratedAncestor(nsIFrame* f) {
  while (f->GetStateBits() & NS_FRAME_GENERATED_CONTENT) {
    f = f->GetParent();
  }
  return f;
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

  nsIFrame* nonGeneratedAncestor = GetNonGeneratedAncestor(mFrame);
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
      if (bg->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT) {
        mSelectionBGColor = NS_RGBA(0,0,0,0);
      }
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

nsTextPaintStyle::nsIMEStyle*
nsTextPaintStyle::GetIMEStyle(PRInt32 aIndex)
{
  InitIMEStyle(aIndex);
  return &mIMEStyle[aIndex];
}

struct StyleIDs {
  nsILookAndFeel::nsColorID mForeground, mBackground, mLine;
  nsILookAndFeel::nsMetricID mLineStyle;
};
static StyleIDs IMEStyleIDs[] = {
  { nsILookAndFeel::eColor_IMERawInputForeground,
    nsILookAndFeel::eColor_IMERawInputBackground,
    nsILookAndFeel::eColor_IMERawInputUnderline,
    nsILookAndFeel::eMetric_IMERawInputUnderlineStyle },
  { nsILookAndFeel::eColor_IMESelectedRawTextForeground,
    nsILookAndFeel::eColor_IMESelectedRawTextBackground,
    nsILookAndFeel::eColor_IMESelectedRawTextUnderline,
    nsILookAndFeel::eMetric_IMESelectedRawTextUnderlineStyle },
  { nsILookAndFeel::eColor_IMEConvertedTextForeground,
    nsILookAndFeel::eColor_IMEConvertedTextBackground,
    nsILookAndFeel::eColor_IMEConvertedTextUnderline,
    nsILookAndFeel::eMetric_IMEConvertedTextUnderlineStyle },
  { nsILookAndFeel::eColor_IMESelectedConvertedTextForeground,
    nsILookAndFeel::eColor_IMESelectedConvertedTextBackground,
    nsILookAndFeel::eColor_IMESelectedConvertedTextUnderline,
    nsILookAndFeel::eMetric_IMESelectedConvertedTextUnderline }
};

static PRUint8 sUnderlineStyles[] = {
  NS_STYLE_BORDER_STYLE_NONE,   
  NS_STYLE_BORDER_STYLE_DOTTED, 
  NS_STYLE_BORDER_STYLE_DASHED, 
  NS_STYLE_BORDER_STYLE_SOLID,  
  NS_STYLE_BORDER_STYLE_DOUBLE  
};

void
nsTextPaintStyle::InitIMEStyle(PRInt32 aIndex)
{
  nsIMEStyle* IMEStyle = &mIMEStyle[aIndex];
  if (IMEStyle->mInit)
    return;

  StyleIDs* styleIDs = &IMEStyleIDs[aIndex];

  nsILookAndFeel* look = mPresContext->LookAndFeel();
  nscolor foreColor, backColor, lineColor;
  PRInt32 lineStyle;
  look->GetColor(styleIDs->mForeground, foreColor);
  look->GetColor(styleIDs->mBackground, backColor);
  look->GetColor(styleIDs->mLine, lineColor);
  look->GetMetric(styleIDs->mLineStyle, lineStyle);

  
  NS_ASSERTION(foreColor != NS_TRANSPARENT,
               "foreColor cannot be NS_TRANSPARENT");
  NS_ASSERTION(backColor != NS_SAME_AS_FOREGROUND_COLOR,
               "backColor cannot be NS_SAME_AS_FOREGROUND_COLOR");
  NS_ASSERTION(backColor != NS_40PERCENT_FOREGROUND_COLOR,
               "backColor cannot be NS_40PERCENT_FOREGROUND_COLOR");

  foreColor = GetResolvedForeColor(foreColor, GetTextColor(), backColor);

  if (NS_GET_A(backColor) > 0)
    EnsureSufficientContrast(&foreColor, &backColor);

  lineColor = GetResolvedForeColor(lineColor, foreColor, backColor);

  if (!NS_IS_VALID_UNDERLINE_STYLE(lineStyle))
    lineStyle = NS_UNDERLINE_STYLE_SOLID;

  IMEStyle->mTextColor       = foreColor;
  IMEStyle->mBGColor         = backColor;
  IMEStyle->mUnderlineColor  = lineColor;
  IMEStyle->mUnderlineStyle  = sUnderlineStyles[lineStyle];
  IMEStyle->mInit            = PR_TRUE;

  if (mIMEUnderlineRelativeSize == -1.0f) {
    look->GetMetric(nsILookAndFeel::eMetricFloat_IMEUnderlineRelativeSize,
                    mIMEUnderlineRelativeSize);
    NS_ASSERTION(mIMEUnderlineRelativeSize >= 0.0f,
                 "underline size must be larger than 0");
  }
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
  if (!IsEmpty() || GetNextInFlow()) {

    nsCOMPtr<nsIAccessibilityService> accService = do_GetService("@mozilla.org/accessibilityService;1");

    if (accService) {
      return accService->CreateHTMLTextAccessible(static_cast<nsIFrame*>(this), aAccessible);
    }
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
  
  
  return nsFrame::Init(aContent, aParent, aPrevInFlow);
}

void
nsTextFrame::Destroy()
{
  if (mNextContinuation) {
    mNextContinuation->SetPrevInFlow(nsnull);
  }
  ClearTextRun();
  
  nsFrame::Destroy();
}

class nsContinuingTextFrame : public nsTextFrame {
public:
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
  
  nsresult rv = nsFrame::Init(aContent, aParent, aPrevInFlow);

  nsIFrame* nextContinuation = aPrevInFlow->GetNextContinuation();
  
  SetPrevInFlow(aPrevInFlow);
  aPrevInFlow->SetNextInFlow(this);
  nsTextFrame* prev = static_cast<nsTextFrame*>(aPrevInFlow);
  mContentOffset = prev->GetContentOffset() + prev->GetContentLengthHint();
  if (prev->GetStyleContext() != GetStyleContext()) {
    
    
    prev->ClearTextRun();
  } else {
    mTextRun = prev->GetTextRun();
  }
#ifdef IBMBIDI
  if (aPrevInFlow->GetStateBits() & NS_FRAME_IS_BIDI) {
    PRInt32 start, end;
    aPrevInFlow->GetOffsets(start, mContentOffset);

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
      nextContinuation->GetOffsets(start, end);
    }
    mState |= NS_FRAME_IS_BIDI;
  } 
#endif 

  return rv;
}

void
nsContinuingTextFrame::Destroy()
{
  if (mPrevContinuation || mNextContinuation) {
    nsSplittableFrame::RemoveFromFlow(this);
  }
  ClearTextRun();
  
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

nsIFrame*
NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext)
{
  return new (aPresShell) nsContinuingTextFrame(aContext);
}

nsTextFrame::~nsTextFrame()
{
  if (0 != (mState & TEXT_BLINK_ON))
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
  
  if (!textRun || !(GetStateBits() & TEXT_IS_RUN_OWNER))
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
  for (f = aFrame; f; f = static_cast<nsTextFrame*>(f->GetNextInFlow())) {
    f->ClearTextRun();
  }
}

NS_IMETHODIMP
nsTextFrame::CharacterDataChanged(nsPresContext* aPresContext,
                                  nsIContent*     aChild,
                                  PRBool          aAppend)
{
  ClearTextRunsInFlowChain(this);

  nsTextFrame* targetTextFrame;
  PRInt32 nodeLength = mContent->GetText()->GetLength();

  if (aAppend) {
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

  
  aPresContext->GetPresShell()->FrameNeedsReflow(targetTextFrame,
                                                 nsIPresShell::eStyleChange,
                                                 NS_FRAME_IS_DIRTY);

  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::DidSetStyleContext()
{
  ClearTextRun();
  return NS_OK;
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
  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt) { return mFrame; }
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

  if ((0 != (mState & TEXT_BLINK_ON)) && nsBlinkTimer::GetBlinkIsOff())
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
  if (!(GetStateBits() & NS_FRAME_GENERATED_CONTENT)) {
    SelectionDetails* details =
      GetFrameSelection()->LookUpSelection(mContent, GetContentOffset(), 
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
    GetFrameSelection()->LookUpSelection(owner->GetContent(),
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
  
  float t2p = 1.0f / aPresContext->AppUnitsPerDevPixel();
  aCtx->NewPath();
  
  aCtx->Rectangle(gfxRect(r.X()*t2p, r.Y()*t2p, r.Width()*t2p, r.Height()*t2p), PR_TRUE);
  aCtx->SetColor(gfxRGBA(aColor));
  aCtx->Fill();
}

void 
nsTextFrame::PaintTextDecorations(gfxContext* aCtx, const gfxRect& aDirtyRect,
                                  const gfxPoint& aFramePt,
                                  nsTextPaintStyle& aTextPaintStyle,
                                  PropertyProvider& aProvider)
{
  
  
  
  if (eCompatibility_NavQuirks != aTextPaintStyle.PresContext()->CompatibilityMode())
    return;

  PRBool useOverride = PR_FALSE;
  nscolor overrideColor;

  PRUint8 decorations = NS_STYLE_TEXT_DECORATION_NONE;
  
  PRUint8 decorMask = NS_STYLE_TEXT_DECORATION_UNDERLINE | 
                      NS_STYLE_TEXT_DECORATION_OVERLINE |
                      NS_STYLE_TEXT_DECORATION_LINE_THROUGH;    
  nscolor overColor, underColor, strikeColor;
  nsStyleContext* context = GetStyleContext();
  PRBool hasDecorations = context->HasTextDecorations();

  while (hasDecorations) {
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
        underColor = useOverride ? overrideColor : color;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_UNDERLINE;
        decorations |= NS_STYLE_TEXT_DECORATION_UNDERLINE;
      }
      if (NS_STYLE_TEXT_DECORATION_OVERLINE & useDecorations) {
        overColor = useOverride ? overrideColor : color;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_OVERLINE;
        decorations |= NS_STYLE_TEXT_DECORATION_OVERLINE;
      }
      if (NS_STYLE_TEXT_DECORATION_LINE_THROUGH & useDecorations) {
        strikeColor = useOverride ? overrideColor : color;
        decorMask &= ~NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
        decorations |= NS_STYLE_TEXT_DECORATION_LINE_THROUGH;
      }
    }
    if (0 == decorMask)
      break;
    context = context->GetParent();
    if (!context)
      break;
    hasDecorations = context->HasTextDecorations();
  }

  if (!decorations)
    return;

  gfxFont::Metrics fontMetrics = GetFontMetrics(aProvider.GetFontGroup());
  gfxFloat a2p = 1.0 / aTextPaintStyle.PresContext()->AppUnitsPerDevPixel();

  
  gfxPoint pt(aFramePt.x * a2p, aFramePt.y * a2p);
  gfxSize size(GetRect().width * a2p, 0);
  gfxFloat ascent = mAscent * a2p;

  if (decorations & NS_FONT_DECORATION_OVERLINE) {
    size.height = fontMetrics.underlineSize;
    nsCSSRendering::PaintDecorationLine(
      aCtx, overColor, pt, size, ascent, ascent, size.height,
      NS_STYLE_TEXT_DECORATION_OVERLINE, NS_STYLE_BORDER_STYLE_SOLID,
      mTextRun->IsRightToLeft());
  }
  if (decorations & NS_FONT_DECORATION_UNDERLINE) {
    size.height = fontMetrics.underlineSize;
    gfxFloat offset = fontMetrics.underlineOffset;
    nsCSSRendering::PaintDecorationLine(
      aCtx, underColor, pt, size, ascent, offset, size.height,
      NS_STYLE_TEXT_DECORATION_UNDERLINE, NS_STYLE_BORDER_STYLE_SOLID,
      mTextRun->IsRightToLeft());
  }
  if (decorations & NS_FONT_DECORATION_LINE_THROUGH) {
    size.height = fontMetrics.strikeoutSize;
    gfxFloat offset = fontMetrics.strikeoutOffset;
    nsCSSRendering::PaintDecorationLine(
      aCtx, strikeColor, pt, size, ascent, offset, size.height,
      NS_STYLE_TEXT_DECORATION_UNDERLINE, NS_STYLE_BORDER_STYLE_SOLID,
      mTextRun->IsRightToLeft());
  }
}


static const SelectionType SelectionTypesWithDecorations =
  nsISelectionController::SELECTION_SPELLCHECK |
  nsISelectionController::SELECTION_IME_RAWINPUT |
  nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT |
  nsISelectionController::SELECTION_IME_CONVERTEDTEXT |
  nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT;

static void DrawIMEUnderline(gfxContext* aContext, PRInt32 aIndex,
    nsTextPaintStyle& aTextPaintStyle, const gfxPoint& aPt, gfxFloat aWidth,
    gfxFloat aAscent, gfxFloat aSize, gfxFloat aOffset, PRBool aIsRTL)
{
  nscolor color;
  float relativeSize;
  PRUint8 style;
  if (!aTextPaintStyle.GetIMEUnderline(aIndex, &color, &relativeSize, &style))
    return;

  gfxFloat actualSize = relativeSize * aSize;
  gfxFloat width = PR_MAX(0, aWidth - 2.0 * aSize);
  gfxPoint pt(aPt.x + 1.0, aPt.y);
  nsCSSRendering::PaintDecorationLine(
    aContext, color, pt, gfxSize(width, actualSize), aAscent, aOffset, aSize,
    NS_STYLE_TEXT_DECORATION_UNDERLINE, style, aIsRTL);
}





static void DrawSelectionDecorations(gfxContext* aContext, SelectionType aType,
    nsTextPaintStyle& aTextPaintStyle, const gfxPoint& aPt, gfxFloat aWidth,
    gfxFloat aAscent, const gfxFont::Metrics& aFontMetrics, PRBool aIsRTL)
{
  gfxSize size(aWidth, aFontMetrics.underlineSize);
  gfxFloat offset = aFontMetrics.underlineOffset;

  switch (aType) {
    case nsISelectionController::SELECTION_SPELLCHECK: {
      nsCSSRendering::PaintDecorationLine(
        aContext, NS_RGB(255,0,0),
        aPt, size, aAscent, aFontMetrics.underlineOffset, size.height,
        NS_STYLE_TEXT_DECORATION_UNDERLINE, NS_STYLE_BORDER_STYLE_DOTTED,
        aIsRTL);
      break;
    }

    case nsISelectionController::SELECTION_IME_RAWINPUT:
      DrawIMEUnderline(aContext, nsTextPaintStyle::eIndexRawInput,
                       aTextPaintStyle, aPt, aWidth, aAscent, size.height,
                       aFontMetrics.underlineOffset, aIsRTL);
      break;
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
      DrawIMEUnderline(aContext, nsTextPaintStyle::eIndexSelRawText,
                       aTextPaintStyle, aPt, aWidth, aAscent, size.height,
                       aFontMetrics.underlineOffset, aIsRTL);
      break;
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
      DrawIMEUnderline(aContext, nsTextPaintStyle::eIndexConvText,
                       aTextPaintStyle, aPt, aWidth, aAscent, size.height,
                       aFontMetrics.underlineOffset, aIsRTL);
      break;
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
      DrawIMEUnderline(aContext, nsTextPaintStyle::eIndexSelConvText,
                       aTextPaintStyle, aPt, aWidth, aAscent, size.height,
                       aFontMetrics.underlineOffset, aIsRTL);
      break;

    default:
      NS_WARNING("Requested selection decorations when there aren't any");
      break;
  }
}









static PRBool GetSelectionTextColors(SelectionType aType, nsTextPaintStyle& aTextPaintStyle,
                                     nscolor* aForeground, nscolor* aBackground)
{
  switch (aType) {
    case nsISelectionController::SELECTION_NORMAL:
      return aTextPaintStyle.GetSelectionColors(aForeground, aBackground);

    case nsISelectionController::SELECTION_IME_RAWINPUT:
      aTextPaintStyle.GetIMESelectionColors(nsTextPaintStyle::eIndexRawInput,
                                            aForeground, aBackground);
      return PR_TRUE;
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
      aTextPaintStyle.GetIMESelectionColors(nsTextPaintStyle::eIndexSelRawText,
                                            aForeground, aBackground);
      return PR_TRUE;
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
      aTextPaintStyle.GetIMESelectionColors(nsTextPaintStyle::eIndexConvText,
                                            aForeground, aBackground);
      return PR_TRUE;
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
      aTextPaintStyle.GetIMESelectionColors(nsTextPaintStyle::eIndexSelConvText,
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
  



  SelectionIterator(SelectionType* aSelectionBuffer, PRInt32 aStart,
                    PRInt32 aLength, PropertyProvider& aProvider,
                    gfxTextRun* aTextRun);
  
  











  PRBool GetNextSegment(gfxFloat* aXOffset, PRUint32* aOffset, PRUint32* aLength,
                        gfxFloat* aHyphenWidth, SelectionType* aType);
  void UpdateWithAdvance(gfxFloat aAdvance) {
    mXOffset += aAdvance*mTextRun->GetDirection();
  }

private:
  SelectionType*          mSelectionBuffer;
  PropertyProvider&       mProvider;
  gfxTextRun*             mTextRun;
  gfxSkipCharsIterator    mIterator;
  PRInt32                 mOriginalStart;
  PRInt32                 mOriginalEnd;
  gfxFloat                mXOffset;
};

SelectionIterator::SelectionIterator(SelectionType* aSelectionBuffer,
    PRInt32 aStart, PRInt32 aLength, PropertyProvider& aProvider,
    gfxTextRun* aTextRun)
  : mSelectionBuffer(aSelectionBuffer), mProvider(aProvider),
    mTextRun(aTextRun), mIterator(aProvider.GetStart()),
    mOriginalStart(aStart), mOriginalEnd(aStart + aLength),
    mXOffset(mTextRun->IsRightToLeft() ? aProvider.GetFrame()->GetSize().width : 0)
{
  mIterator.SetOriginalOffset(aStart);
}

PRBool SelectionIterator::GetNextSegment(gfxFloat* aXOffset,
    PRUint32* aOffset, PRUint32* aLength, gfxFloat* aHyphenWidth, SelectionType* aType)
{
  if (mIterator.GetOriginalOffset() >= mOriginalEnd)
    return PR_FALSE;
  
  
  PRUint32 runOffset = mIterator.GetSkippedOffset();
  
  PRInt32 index = mIterator.GetOriginalOffset() - mOriginalStart;
  SelectionType type = mSelectionBuffer[index];
  for (++index; mOriginalStart + index < mOriginalEnd; ++index) {
    if (mSelectionBuffer[index] != type)
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
  return PR_TRUE;
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

  
  nsAutoTArray<SelectionType,BIG_TEXT_NODE_SIZE> prevailingSelectionsBuffer;
  if (!prevailingSelectionsBuffer.AppendElements(contentLength))
    return;
  SelectionType* prevailingSelections = prevailingSelectionsBuffer.Elements();
  PRInt32 i;
  SelectionType allTypes = 0;
  for (i = 0; i < contentLength; ++i) {
    prevailingSelections[i] = nsISelectionController::SELECTION_NONE;
  }

  SelectionDetails *sdptr = aDetails;
  PRBool anyBackgrounds = PR_FALSE;
  while (sdptr) {
    PRInt32 start = PR_MAX(0, sdptr->mStart - contentOffset);
    PRInt32 end = PR_MIN(contentLength, sdptr->mEnd - contentOffset);
    SelectionType type = sdptr->mType;
    if (start < end) {
      allTypes |= type;
      
      nscolor foreground, background;
      if (GetSelectionTextColors(type, aTextPaintStyle, &foreground, &background)) {
        if (NS_GET_A(background) > 0) {
          anyBackgrounds = PR_TRUE;
        }
        for (i = start; i < end; ++i) {
          PRInt16 currentPrevailingSelection = prevailingSelections[i];
          
          if (currentPrevailingSelection == nsISelectionController::SELECTION_NONE ||
              type < currentPrevailingSelection) {
            prevailingSelections[i] = type;
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
  
  if (anyBackgrounds) {
    SelectionIterator iterator(prevailingSelections, contentOffset, contentLength,
                               aProvider, mTextRun);
    while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth, &type)) {
      nscolor foreground, background;
      GetSelectionTextColors(type, aTextPaintStyle, &foreground, &background);
      
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
  while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth, &type)) {
    nscolor foreground, background;
    GetSelectionTextColors(type, aTextPaintStyle, &foreground, &background);
    
    aCtx->SetColor(gfxRGBA(foreground));
    gfxFloat advance;
    mTextRun->Draw(aCtx, gfxPoint(aFramePt.x + xOffset, aTextBaselinePt.y), offset, length,
                   &aDirtyRect, &aProvider, &advance);
    if (hyphenWidth) {
      
      gfxFloat hyphenBaselineX = aFramePt.x + xOffset + mTextRun->GetDirection()*advance;
      
      
      nsCOMPtr<nsIRenderingContext> rc = GetReferenceRenderingContext(this, nsnull);
      gfxTextRun* hyphenTextRun = GetHyphenTextRun(mTextRun, rc);
      if (hyphenTextRun) {
        hyphenTextRun->Draw(aCtx, gfxPoint(hyphenBaselineX, aTextBaselinePt.y),
                            0, hyphenTextRun->GetLength(), &aDirtyRect, nsnull, nsnull);
      }
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

  
  
  nsAutoTArray<SelectionType,BIG_TEXT_NODE_SIZE> selectedCharsBuffer;
  if (!selectedCharsBuffer.AppendElements(contentLength))
    return;
  SelectionType* selectedChars = selectedCharsBuffer.Elements();
  PRInt32 i;
  for (i = 0; i < contentLength; ++i) {
    selectedChars[i] = nsISelectionController::SELECTION_NONE;
  }

  SelectionDetails *sdptr = aDetails;
  while (sdptr) {
    if (sdptr->mType == aSelectionType) {
      PRInt32 start = PR_MAX(0, sdptr->mStart - contentOffset);
      PRInt32 end = PR_MIN(contentLength, sdptr->mEnd - contentOffset);
      for (i = start; i < end; ++i) {
        selectedChars[i] = aSelectionType;
      }
    }
    sdptr = sdptr->mNext;
  }

  gfxFont::Metrics decorationMetrics = GetFontMetrics(aProvider.GetFontGroup());

  SelectionIterator iterator(selectedChars, contentOffset, contentLength,
                             aProvider, mTextRun);
  gfxFloat xOffset, hyphenWidth;
  PRUint32 offset, length;
  SelectionType type;
  while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth, &type)) {
    gfxFloat advance = hyphenWidth +
      mTextRun->GetAdvanceWidth(offset, length, &aProvider);
    if (type == aSelectionType) {
      gfxFloat a2p = 1.0 / aTextPaintStyle.PresContext()->AppUnitsPerDevPixel();
      
      gfxPoint pt((aTextBaselinePt.x + xOffset) * a2p,
                  (aTextBaselinePt.y - mAscent) * a2p);
      gfxFloat width = PR_ABS(advance) * a2p;
      DrawSelectionDecorations(aCtx, aSelectionType, aTextPaintStyle,
                               pt, width, mAscent * a2p, decorationMetrics,
                               mTextRun->IsRightToLeft());
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
  PaintTextDecorations(aCtx, aDirtyRect, aFramePt, aTextPaintStyle, aProvider);
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
  
  gfxSkipCharsIterator iter = EnsureTextRun(aRenderingContext);
  if (!mTextRun)
    return;

  nsTextPaintStyle textPaintStyle(this);
  PropertyProvider provider(this, iter);
  
  provider.InitializeForDisplay(PR_TRUE);

  gfxContext* ctx = static_cast<gfxContext*>
                               (aRenderingContext->GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT));

  gfxPoint framePt(aPt.x, aPt.y);
  gfxPoint textBaselinePt(
      mTextRun->IsRightToLeft() ? gfxFloat(aPt.x + GetSize().width) : framePt.x,
      GetSnappedBaselineY(ctx, aPt.y));

  gfxRect dirtyRect(aDirtyRect.x, aDirtyRect.y,
                    aDirtyRect.width, aDirtyRect.height);

  
  if (GetNonGeneratedAncestor(this)->GetStateBits() & NS_FRAME_SELECTED_CONTENT) {
    if (PaintTextWithSelection(ctx, framePt, textBaselinePt,
                               dirtyRect, provider, textPaintStyle))
      return;
  }

  gfxFloat advanceWidth;
  gfxFloat* needAdvanceWidth =
    (GetStateBits() & TEXT_HYPHEN_BREAK) ? &advanceWidth : nsnull;
  ctx->SetColor(gfxRGBA(textPaintStyle.GetTextColor()));
  
  mTextRun->Draw(ctx, textBaselinePt,
                 provider.GetStart().GetSkippedOffset(),
                 ComputeTransformedLength(provider),
                 &dirtyRect, &provider, needAdvanceWidth);
  if (GetStateBits() & TEXT_HYPHEN_BREAK) {
    gfxFloat hyphenBaselineX = textBaselinePt.x + mTextRun->GetDirection()*advanceWidth;
    nsCOMPtr<nsIRenderingContext> rc = GetReferenceRenderingContext(this, nsnull);
    gfxTextRun* hyphenTextRun = GetHyphenTextRun(mTextRun, rc);
    if (hyphenTextRun) {
      hyphenTextRun->Draw(ctx, gfxPoint(hyphenBaselineX, textBaselinePt.y),
                          0, hyphenTextRun->GetLength(), &dirtyRect, nsnull, nsnull);
    }
  }
  PaintTextDecorations(ctx, dirtyRect, framePt, textPaintStyle, provider);
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
nsTextFrame::CalcContentOffsetsFromFramePoint(nsPoint aPoint) {
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
    selectedOffset = width <= fitWidth + charWidth/2
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


NS_IMETHODIMP
nsTextFrame::SetSelected(nsPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread)
{
  DEBUG_VERIFY_NOT_DIRTY(mState);
#if 0 
  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;
#endif

  if (aSelected && ParentDisablesSelection())
    return NS_OK;

  
  PRBool selectable;
  IsSelectable(&selectable, nsnull);
  if (!selectable)
    return NS_OK;

  PRBool found = PR_FALSE;
  if (aRange) {
    
    nsCOMPtr<nsIDOMNode> endNode;
    PRInt32 endOffset;
    nsCOMPtr<nsIDOMNode> startNode;
    PRInt32 startOffset;
    aRange->GetEndContainer(getter_AddRefs(endNode));
    aRange->GetEndOffset(&endOffset);
    aRange->GetStartContainer(getter_AddRefs(startNode));
    aRange->GetStartOffset(&startOffset);
    nsCOMPtr<nsIDOMNode> thisNode = do_QueryInterface(GetContent());

    if (thisNode == startNode)
    {
      if (GetContentEnd() >= startOffset)
      {
        found = PR_TRUE;
        if (thisNode == endNode)
        { 
          if (endOffset == startOffset) 
            found = PR_FALSE;

          if (mContentOffset > endOffset)
            found = PR_FALSE;
        }
      }
    }
    else if (thisNode == endNode)
    {
      if (mContentOffset < endOffset)
        found = PR_TRUE;
      else
      {
        found = PR_FALSE;
      }
    }
    else
    {
      found = PR_TRUE;
    }
  }
  else {
    
    found = PR_TRUE;
  }

  if ( aSelected )
    AddStateBits(NS_FRAME_SELECTED_CONTENT);
  else
  { 
    SelectionDetails *details = GetSelectionDetails();
    if (!details) {
      RemoveStateBits(NS_FRAME_SELECTED_CONTENT);
    } else {
      DestroySelectionDetails(details);
    }
  }
  if (found) {
    
    Invalidate(GetOverflowRect(), PR_FALSE);
  }
  if (aSpread == eSpreadDown)
  {
    nsIFrame* frame = GetPrevContinuation();
    while(frame){
      frame->SetSelected(aPresContext, aRange,aSelected,eSpreadNone);
      frame = frame->GetPrevContinuation();
    }
    frame = GetNextContinuation();
    while (frame){
      frame->SetSelected(aPresContext, aRange,aSelected,eSpreadNone);
      frame = frame->GetNextContinuation();
    }
  }
  return NS_OK;
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
  inOffset = PR_MAX(inOffset, trimmedOffset);
  inOffset = PR_MIN(inOffset, trimmedEnd);

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
  nscoord width = NSToCoordCeil(advanceWidth);

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

  TrimmedOffsets trimmed = GetTrimmedOffsets(mContent->GetText(), PR_TRUE);
  
  return iter.ConvertOriginalToSkipped(trimmed.GetEnd()) >
         iter.ConvertOriginalToSkipped(trimmed.mStart);
}









class ClusterIterator {
public:
  ClusterIterator(nsTextFrame* aTextFrame, PRInt32 aPosition, PRInt32 aDirection);

  PRBool NextCluster();
  PRBool IsWhitespace();
  PRBool IsPunctuation();
  PRBool HaveWordBreakBefore();
  PRInt32 GetAfterOffset();
  PRInt32 GetBeforeOffset();

private:
  gfxSkipCharsIterator        mIterator;
  const nsTextFragment*       mFrag;
  nsTextFrame*                mTextFrame;
  PRInt32                     mDirection;
  PRInt32                     mCharIndex;
  nsTextFrame::TrimmedOffsets mTrimmed;
  nsTArray<PRPackedBool>      mWordBreaks;
};

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

  TrimmedOffsets trimmed = GetTrimmedOffsets(mContent->GetText(), PR_TRUE);

  
  PRInt32 startOffset = GetContentOffset() + (*aOffset < 0 ? contentLength : *aOffset);

  if (!aForward) {
    PRInt32 i;
    for (i = PR_MIN(trimmed.GetEnd(), startOffset) - 1;
         i >= trimmed.mStart; --i) {
      iter.SetOriginalOffset(i);
      if (!iter.IsOriginalCharSkipped() &&
          mTextRun->IsClusterStart(iter.GetSkippedOffset())) {
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
          (!iter.IsOriginalCharSkipped() &&
           mTextRun->IsClusterStart(iter.GetSkippedOffset()))) {
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
  return nsTextFrameUtils::IsPunctuationMark(mFrag->CharAt(mCharIndex));
}

PRBool
ClusterIterator::HaveWordBreakBefore()
{
  return mWordBreaks[GetBeforeOffset() - mTextFrame->GetContentOffset()];
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

  while (PR_TRUE) {
    if (mDirection > 0) {
      if (mIterator.GetOriginalOffset() >= mTrimmed.GetEnd())
        return PR_FALSE;
      if (mIterator.IsOriginalCharSkipped() ||
          mIterator.GetOriginalOffset() < mTrimmed.mStart ||
          !textRun->IsClusterStart(mIterator.GetSkippedOffset())) {
        mIterator.AdvanceOriginal(1);
        continue;
      }
      mCharIndex = mIterator.GetOriginalOffset();
      mIterator.AdvanceOriginal(1);
    } else {
      if (mIterator.GetOriginalOffset() <= mTrimmed.mStart)
        return PR_FALSE;
      mIterator.AdvanceOriginal(-1);
      if (mIterator.IsOriginalCharSkipped() ||
          mIterator.GetOriginalOffset() >= mTrimmed.GetEnd() ||
          !textRun->IsClusterStart(mIterator.GetSkippedOffset()))
        continue;
      mCharIndex = mIterator.GetOriginalOffset();
    }

    return PR_TRUE;
  }
}

ClusterIterator::ClusterIterator(nsTextFrame* aTextFrame, PRInt32 aPosition,
                                 PRInt32 aDirection)
  : mTextFrame(aTextFrame), mDirection(aDirection), mCharIndex(-1)
{
  mIterator = aTextFrame->EnsureTextRun();
  if (!aTextFrame->GetTextRun()) {
    mDirection = 0; 
    return;
  }
  mIterator.SetOriginalOffset(aPosition);

  mFrag = aTextFrame->GetContent()->GetText();
  mTrimmed = aTextFrame->GetTrimmedOffsets(mFrag, PR_TRUE);

  PRInt32 textLen = aTextFrame->GetContentLength();
  if (!mWordBreaks.AppendElements(textLen)) {
    mDirection = 0; 
    return;
  }
  memset(mWordBreaks.Elements(), PR_FALSE, textLen);
  nsAutoString text;
  mFrag->AppendTo(text, aTextFrame->GetContentOffset(), textLen);
  nsIWordBreaker* wordBreaker = nsContentUtils::WordBreaker();
  PRInt32 i = 0;
  while ((i = wordBreaker->NextWord(text.get(), textLen, i)) >= 0)
    mWordBreaks[i] = PR_TRUE;
}

PRBool
nsTextFrame::PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                            PRInt32* aOffset, PRBool* aSawBeforeType)
{
  PRInt32 contentLength = GetContentLength();
  NS_ASSERTION (aOffset && *aOffset <= contentLength, "aOffset out of range");

  PRBool selectable;
  PRUint8 selectStyle;
  IsSelectable(&selectable, &selectStyle);
  if (selectStyle == NS_STYLE_USER_SELECT_ALL)
    return PR_FALSE;

  PRInt32 offset = GetContentOffset() + (*aOffset < 0 ? contentLength : *aOffset);
  ClusterIterator cIter(this, offset, aForward ? 1 : -1);

  if (!cIter.NextCluster())
    return PR_FALSE;
  
  PRBool stopAfterPunctuation =
	  nsContentUtils::GetBoolPref("layout.word_select.stop_at_punctuation");
  PRBool stopBeforePunctuation = stopAfterPunctuation && !aIsKeyboardSelect;
  do {
    if (aWordSelectEatSpace == cIter.IsWhitespace() && !*aSawBeforeType) {
      *aSawBeforeType = PR_TRUE;
      continue;
    }
    if (cIter.GetBeforeOffset() != offset &&
        (cIter.IsPunctuation() ? stopBeforePunctuation
                               : cIter.HaveWordBreakBefore() && *aSawBeforeType)) {
      *aOffset = cIter.GetBeforeOffset() - mContentOffset;
      return PR_TRUE;
    }
    if (stopAfterPunctuation && cIter.IsPunctuation()) {
      *aOffset = cIter.GetAfterOffset() - mContentOffset;
      return PR_TRUE;
    }
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














static PRBool
FindFirstLetterRange(const nsTextFragment* aFrag,
                     gfxTextRun* aTextRun,
                     PRInt32 aOffset, const gfxSkipCharsIterator& aIter,
                     PRInt32* aLength)
{
  
  PRInt32 i;
  PRInt32 length = *aLength;
  for (i = 0; i < length; ++i) {
    if (!IsTrimmableSpace(aFrag, aOffset + i) &&
        !nsTextFrameUtils::IsPunctuationMark(aFrag->CharAt(aOffset + i)))
      break;
  }

  if (i == length)
    return PR_FALSE;

  
  gfxSkipCharsIterator iter(aIter);
  PRInt32 nextClusterStart;
  for (nextClusterStart = i + 1; nextClusterStart < length; ++nextClusterStart) {
    iter.SetOriginalOffset(nextClusterStart);
    if (iter.IsOriginalCharSkipped() ||
        aTextRun->IsClusterStart(iter.GetSkippedOffset()))
      break;
  }
  *aLength = nextClusterStart;
  return PR_TRUE;
}

static nsRect ConvertGfxRectOutward(const gfxRect& aRect)
{
  nsRect r;
  r.x = NSToCoordFloor(aRect.X());
  r.y = NSToCoordFloor(aRect.Y());
  r.width = NSToCoordCeil(aRect.XMost()) - r.x;
  r.height = NSToCoordCeil(aRect.YMost()) - r.y;
  return r;
}

static PRUint32
FindStartAfterSkippingWhitespace(PropertyProvider* aProvider,
                                 nsIFrame::InlineIntrinsicWidthData* aData,
                                 PRBool aCollapseWhitespace,
                                 gfxSkipCharsIterator* aIterator,
                                 PRUint32 aFlowEndInTextRun)
{
  if (aData->skipWhitespace && aCollapseWhitespace) {
    while (aIterator->GetSkippedOffset() < aFlowEndInTextRun &&
           IsTrimmableSpace(aProvider->GetFragment(), aIterator->GetOriginalOffset())) {
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
  gfxSkipCharsIterator iter =
    EnsureTextRun(aRenderingContext, nsnull, nsnull, &flowEndInTextRun);
  if (!mTextRun)
    return;

  
  
  const nsTextFragment* frag = mContent->GetText();
  PropertyProvider provider(mTextRun, GetStyleText(), frag, this,
                            iter, GetInFlowContentLength(), nsnull, 0);

  PRBool collapseWhitespace = !provider.GetStyleText()->WhiteSpaceIsSignificant();
  PRUint32 start =
    FindStartAfterSkippingWhitespace(&provider, aData, collapseWhitespace,
                                     &iter, flowEndInTextRun);
  if (start >= flowEndInTextRun)
    return;

  
  for (PRUint32 i = start, wordStart = start; i <= flowEndInTextRun; ++i) {
    PRBool preformattedNewline = PR_FALSE;
    if (i < flowEndInTextRun) {
      
      
      
      preformattedNewline = !collapseWhitespace && mTextRun->GetChar(i) == '\n';
      if (!mTextRun->CanBreakLineBefore(i) && !preformattedNewline) {
        
        continue;
      }
    }

    if (i > wordStart) {
      nscoord width =
        NSToCoordCeil(mTextRun->GetAdvanceWidth(wordStart, i - wordStart, &provider));
      aData->currentLine += width;
      aData->atStartOfLine = PR_FALSE;

      if (collapseWhitespace) {
        nscoord trailingWhitespaceWidth;
        PRUint32 trimStart = GetEndOfTrimmedText(frag, wordStart, i, &iter);
        if (trimStart == start) {
          trailingWhitespaceWidth = width;
        } else {
          trailingWhitespaceWidth =
            NSToCoordCeil(mTextRun->GetAdvanceWidth(trimStart, i - trimStart, &provider));
        }
        aData->trailingWhitespace += trailingWhitespaceWidth;
      } else {
        aData->trailingWhitespace = 0;
      }
    }

    if (i < flowEndInTextRun) {
      if (preformattedNewline) {
        aData->ForceBreak(aRenderingContext);
      } else {
        aData->OptionallyBreak(aRenderingContext);
      }
      wordStart = i;
    }
  }

  
  aData->skipWhitespace =
    IsTrimmableSpace(provider.GetFragment(),
                     iter.ConvertSkippedToOriginal(flowEndInTextRun - 1));
}



 void
nsTextFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinWidthData *aData)
{
  nsTextFrame* f;
  gfxTextRun* lastTextRun = nsnull;
  
  
  for (f = this; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
    
    
    
    if (f == this || f->mTextRun != lastTextRun) {
      
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
  gfxSkipCharsIterator iter =
    EnsureTextRun(aRenderingContext, nsnull, nsnull, &flowEndInTextRun);
  if (!mTextRun)
    return;

  
  
  PropertyProvider provider(mTextRun, GetStyleText(), mContent->GetText(), this,
                            iter, GetInFlowContentLength(), nsnull, 0);

  PRBool collapseWhitespace = !provider.GetStyleText()->WhiteSpaceIsSignificant();
  PRUint32 start =
    FindStartAfterSkippingWhitespace(&provider, aData, collapseWhitespace,
                                     &iter, flowEndInTextRun);
  if (start >= flowEndInTextRun)
    return;

  if (collapseWhitespace) {
    
    
    aData->currentLine +=
      NSToCoordCeil(mTextRun->GetAdvanceWidth(start, flowEndInTextRun - start, &provider));

    PRUint32 trimStart = GetEndOfTrimmedText(provider.GetFragment(), start,
                                             flowEndInTextRun, &iter);
    nscoord trimWidth =
      NSToCoordCeil(mTextRun->GetAdvanceWidth(trimStart, flowEndInTextRun - trimStart, &provider));
    if (trimStart == start) {
      
      
      aData->trailingWhitespace += trimWidth;
    } else {
      
      aData->trailingWhitespace = trimWidth;
    }
  } else {
    
    aData->trailingWhitespace = 0;
    PRUint32 i;
    PRUint32 startRun = start;
    for (i = start; i <= flowEndInTextRun; ++i) {
      if (i < flowEndInTextRun && mTextRun->GetChar(i) != '\n')
        continue;
        
      aData->currentLine +=
        NSToCoordCeil(mTextRun->GetAdvanceWidth(startRun, i - startRun, &provider));
      if (i < flowEndInTextRun) {
        aData->ForceBreak(aRenderingContext);
        startRun = i;
      }
    }
  }

  
  aData->skipWhitespace =
    IsTrimmableSpace(provider.GetFragment(),
                     iter.ConvertSkippedToOriginal(flowEndInTextRun - 1));
}



 void
nsTextFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefWidthData *aData)
{
  nsTextFrame* f;
  gfxTextRun* lastTextRun = nsnull;
  
  
  for (f = this; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
    
    
    
    if (f == this || f->mTextRun != lastTextRun) {
      
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

static void
AddCharToMetrics(gfxTextRun* aCharTextRun, gfxTextRun* aBaseTextRun,
                 gfxTextRun::Metrics* aMetrics, PRBool aTightBoundingBox)
{
  gfxRect charRect;
  
  gfxFloat width = aCharTextRun->GetAdvanceWidth(0, aCharTextRun->GetLength(), nsnull);
  if (aTightBoundingBox) {
    gfxTextRun::Metrics charMetrics =
        aCharTextRun->MeasureText(0, aCharTextRun->GetLength(), PR_TRUE, nsnull);
    charRect = charMetrics.mBoundingBox;
  } else {
    charRect = gfxRect(0, -aMetrics->mAscent, width,
                       aMetrics->mAscent + aMetrics->mDescent);
  }
  if (aBaseTextRun->IsRightToLeft()) {
    
    
    aMetrics->mBoundingBox.MoveBy(gfxPoint(width, 0));
  } else {
    
    charRect.MoveBy(gfxPoint(width, 0));
  }
  aMetrics->mBoundingBox = aMetrics->mBoundingBox.Union(charRect);

  aMetrics->mAdvanceWidth += width;
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

  if (aPresContext->BidiEnabled()) {
    
    
    aPresContext->SetIsBidiSystem(PR_TRUE);
  }

  if (aReflowState.mFlags.mBlinks) {
    if (0 == (mState & TEXT_BLINK_ON)) {
      mState |= TEXT_BLINK_ON;
      nsBlinkTimer::AddBlinkFrame(aPresContext, this);
    }
  }
  else {
    if (0 != (mState & TEXT_BLINK_ON)) {
      mState &= ~TEXT_BLINK_ON;
      nsBlinkTimer::RemoveBlinkFrame(this);
    }
  }

  const nsStyleText* textStyle = GetStyleText();

  PRBool atStartOfLine = lineLayout.CanPlaceFloatNow();
  if (atStartOfLine) {
    AddStateBits(TEXT_START_OF_LINE);
  }

  
  
  PRBool layoutDependentTextRun =
    lineLayout.GetFirstLetterStyleOK() || lineLayout.GetInFirstLine();
  if (layoutDependentTextRun) {
    SetLength(maxContentLength);
  }

  PRUint32 flowEndInTextRun;
  nsIFrame* lineContainer = lineLayout.GetLineContainerFrame();
  gfxSkipCharsIterator iter =
    EnsureTextRun(aReflowState.rendContext, lineContainer,
                  lineLayout.GetLine(), &flowEndInTextRun);

  if (!mTextRun) {
    ClearMetrics(aMetrics);
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  const nsTextFragment* frag = mContent->GetText();
  
  
  
  PRInt32 length = maxContentLength;
  PRInt32 offset = GetContentOffset();

  
  PRInt32 newLineOffset = -1;
  if (textStyle->WhiteSpaceIsSignificant()) {
    newLineOffset = FindChar(frag, offset, length, '\n');
    if (newLineOffset >= 0) {
      length = newLineOffset + 1 - offset;
      newLineOffset -= mContentOffset;
    }
  } else {
    if (atStartOfLine) {
      
      PRInt32 whitespaceCount = GetTrimmableWhitespaceCount(frag, offset, length, 1);
      offset += whitespaceCount;
      length -= whitespaceCount;
    }
  }

  
  PRBool completedFirstLetter = PR_FALSE;
  if (lineLayout.GetFirstLetterStyleOK()) {
    AddStateBits(TEXT_FIRST_LETTER);
    completedFirstLetter = FindFirstLetterRange(frag, mTextRun, offset, iter, &length);
  }

  
  
  
  
  iter.SetOriginalOffset(offset);
  nscoord xOffsetForTabs = (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TAB) ?
         lineLayout.GetCurrentFrameXDistanceFromBlock() : -1;
  PropertyProvider provider(mTextRun, textStyle, frag, this, iter, length,
      lineContainer, xOffsetForTabs);

  PRUint32 transformedOffset = provider.GetStart().GetSkippedOffset();

  
  gfxTextRun::Metrics textMetrics;
  PRBool needTightBoundingBox = (GetStateBits() & TEXT_FIRST_LETTER) != 0;
#ifdef MOZ_MATHML
  if (NS_REFLOW_CALC_BOUNDING_METRICS & aMetrics.mFlags) {
    needTightBoundingBox = PR_TRUE;
  }
#endif
  PRBool suppressInitialBreak = PR_FALSE;
  if (!lineLayout.LineIsBreakable()) {
    suppressInitialBreak = PR_TRUE;
  } else {
    PRBool trailingTextFrameCanWrap;
    nsIFrame* lastTextFrame = lineLayout.GetTrailingTextFrame(&trailingTextFrameCanWrap);
    if (!lastTextFrame) {
      suppressInitialBreak = PR_TRUE;
    }
  }

  PRInt32 limitLength = length;
  PRInt32 forceBreak = lineLayout.GetForcedBreakPosition(mContent);
  if (forceBreak >= offset + length) {
    
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
  PRBool canTrimTrailingWhitespace = !textStyle->WhiteSpaceIsSignificant() &&
    textStyle->WhiteSpaceCanWrap();
  PRUint32 transformedCharsFit =
    mTextRun->BreakAndMeasureText(transformedOffset, transformedLength,
                                  (GetStateBits() & TEXT_START_OF_LINE) != 0,
                                  availWidth,
                                  &provider, suppressInitialBreak,
                                  canTrimTrailingWhitespace ? &trimmedWidth : nsnull,
                                  &textMetrics, needTightBoundingBox,
                                  &usedHyphenation, &transformedLastBreak);
  
  
  
  gfxSkipCharsIterator end(provider.GetEndHint());
  end.SetSkippedOffset(transformedOffset + transformedCharsFit);
  PRInt32 charsFit = end.GetOriginalOffset() - offset;
  
  
  
  PRInt32 lastBreak = -1;
  if (charsFit >= limitLength) {
    charsFit = limitLength;
    if (transformedLastBreak != PR_UINT32_MAX) {
      
      
      lastBreak = end.ConvertSkippedToOriginal(transformedOffset + transformedLastBreak);
    }
    end.SetOriginalOffset(offset + charsFit);
    
    
    if (forceBreak >= 0 && HasSoftHyphenBefore(frag, mTextRun, offset, end)) {
      usedHyphenation = PR_TRUE;
    }
  }
  if (usedHyphenation) {
    
    gfxTextRun* hyphenTextRun = GetHyphenTextRun(mTextRun, aReflowState.rendContext);
    if (hyphenTextRun) {
      AddCharToMetrics(hyphenTextRun,
                       mTextRun, &textMetrics, needTightBoundingBox);
    }
    AddStateBits(TEXT_HYPHEN_BREAK);
  }

  
  
  
  if (forceBreak < 0 && textMetrics.mAdvanceWidth + trimmedWidth <= availWidth) {
    textMetrics.mAdvanceWidth += trimmedWidth;
    if (mTextRun->IsRightToLeft()) {
      
      
      textMetrics.mBoundingBox.MoveBy(gfxPoint(trimmedWidth, 0));
    }
    
    if (lastBreak >= 0) {
      lineLayout.NotifyOptionalBreakPosition(mContent, lastBreak,
          textMetrics.mAdvanceWidth <= aReflowState.availableWidth);
    }
  } else {
    
    
    
    AddStateBits(TEXT_TRIMMED_TRAILING_WHITESPACE);
  }
  PRInt32 contentLength = offset + charsFit - GetContentOffset();
  
  
  
  

  
  
  if (GetStateBits() & TEXT_FIRST_LETTER) {
    textMetrics.mAscent = PR_MAX(0, -textMetrics.mBoundingBox.Y());
    textMetrics.mDescent = PR_MAX(0, textMetrics.mBoundingBox.YMost());
    textMetrics.mAdvanceWidth = textMetrics.mBoundingBox.XMost();
  }
  
  
  
  aMetrics.width = NSToCoordCeil(PR_MAX(0, textMetrics.mAdvanceWidth));
  aMetrics.ascent = NSToCoordCeil(textMetrics.mAscent);
  aMetrics.height = aMetrics.ascent + NSToCoordCeil(textMetrics.mDescent);
  NS_ASSERTION(aMetrics.ascent >= 0, "Negative ascent???");
  NS_ASSERTION(aMetrics.height - aMetrics.ascent >= 0, "Negative descent???");

  mAscent = aMetrics.ascent;

  
  nsRect boundingBox =
    ConvertGfxRectOutward(textMetrics.mBoundingBox + gfxPoint(0, textMetrics.mAscent));
  aMetrics.mOverflowArea.UnionRect(boundingBox,
                                   nsRect(0, 0, aMetrics.width, aMetrics.height));

#ifdef MOZ_MATHML
  
  if (needTightBoundingBox) {
    aMetrics.mBoundingMetrics.ascent =
      NSToCoordCeil(PR_MAX(0, -textMetrics.mBoundingBox.Y()));
    aMetrics.mBoundingMetrics.descent =
      NSToCoordCeil(PR_MAX(0, textMetrics.mBoundingBox.YMost()));
    aMetrics.mBoundingMetrics.leftBearing =
      NSToCoordFloor(textMetrics.mBoundingBox.X());
    aMetrics.mBoundingMetrics.rightBearing =
      NSToCoordCeil(textMetrics.mBoundingBox.XMost());
    aMetrics.mBoundingMetrics.width = aMetrics.width;
  }
#endif

  
  
  

  lineLayout.SetUnderstandsWhiteSpace(PR_TRUE);
  if (charsFit > 0) {
    PRBool endsInWhitespace = IsTrimmableSpace(frag, offset + charsFit - 1);
    lineLayout.SetInWord(!endsInWhitespace);
    lineLayout.SetEndsInWhiteSpace(endsInWhitespace);
    PRBool wrapping = textStyle->WhiteSpaceCanWrap();
    lineLayout.SetTrailingTextFrame(this, wrapping);
    if (charsFit == length) {
      if (endsInWhitespace && wrapping) {
        
        lineLayout.NotifyOptionalBreakPosition(mContent, offset + length,
            textMetrics.mAdvanceWidth <= aReflowState.availableWidth);
      } else if (HasSoftHyphenBefore(frag, mTextRun, offset, end)) {
        
        lineLayout.NotifyOptionalBreakPosition(mContent, offset + length,
            textMetrics.mAdvanceWidth + provider.GetHyphenWidth() <= availWidth);
      }
    }
  } else {
    
    
    
    lineLayout.SetEndsInWhiteSpace(PR_FALSE);
    lineLayout.SetTrailingTextFrame(nsnull, PR_FALSE);
  }
  if (completedFirstLetter) {
    lineLayout.SetFirstLetterStyleOK(PR_FALSE);
  }

  
  aStatus = contentLength == maxContentLength
    ? NS_FRAME_COMPLETE : NS_FRAME_NOT_COMPLETE;

  if (charsFit == 0 && length > 0) {
    
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  } else if (contentLength > 0 && contentLength - 1 == newLineOffset) {
    
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    lineLayout.SetLineEndsInBR(PR_TRUE);
  }

  
  if (NS_STYLE_TEXT_ALIGN_JUSTIFY == textStyle->mTextAlign &&
      !textStyle->WhiteSpaceIsSignificant()) {
    
    
    PRInt32 numJustifiableCharacters =
      provider.ComputeJustifiableCharacters(offset, charsFit);
    
    
    
    if (canTrimTrailingWhitespace) {
      
      PRUint32 charIndex = transformedOffset + transformedCharsFit;
      while (charIndex > transformedOffset &&
             mTextRun->GetChar(charIndex - 1) == ' ') {
        --charIndex;
      }
    }

    NS_ASSERTION(numJustifiableCharacters <= charsFit,
                 "Justifiable characters combined???");
    lineLayout.SetTextJustificationWeights(numJustifiableCharacters,
        charsFit - numJustifiableCharacters);
  }

  SetLength(contentLength);

  Invalidate(nsRect(nsPoint(0, 0), GetSize()));

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

NS_IMETHODIMP
nsTextFrame::TrimTrailingWhiteSpace(nsPresContext* aPresContext,
                                    nsIRenderingContext& aRC,
                                    nscoord& aDeltaWidth,
                                    PRBool& aLastCharIsJustifiable)
{
  aLastCharIsJustifiable = PR_FALSE;
  aDeltaWidth = 0;

  AddStateBits(TEXT_END_OF_LINE);

  PRInt32 contentLength = GetContentLength();
  if (!contentLength)
    return NS_OK;

  gfxSkipCharsIterator start = EnsureTextRun(&aRC);
  if (!mTextRun)
    return NS_ERROR_FAILURE;
  PRUint32 trimmedStart = start.GetSkippedOffset();

  const nsTextFragment* frag = mContent->GetText();
  TrimmedOffsets trimmed = GetTrimmedOffsets(frag, PR_TRUE);
  gfxSkipCharsIterator iter = start;
  PRUint32 trimmedEnd = iter.ConvertOriginalToSkipped(trimmed.GetEnd());
  const nsStyleText* textStyle = GetStyleText();
  gfxFloat delta = 0;

  if (GetStateBits() & TEXT_TRIMMED_TRAILING_WHITESPACE) {
    aLastCharIsJustifiable = PR_TRUE;
  } else if (trimmed.GetEnd() < GetContentEnd()) {
    gfxSkipCharsIterator end = iter;
    PRUint32 endOffset = end.ConvertOriginalToSkipped(GetContentOffset() + contentLength);
    if (trimmedEnd < endOffset) {
      
      
      PropertyProvider provider(mTextRun, textStyle, frag, this, start, contentLength,
                                nsnull, 0);
      delta = mTextRun->GetAdvanceWidth(trimmedEnd, endOffset - trimmedEnd, &provider);
      
      
      
      aLastCharIsJustifiable = PR_TRUE;
    }
  }

  if (!aLastCharIsJustifiable &&
      NS_STYLE_TEXT_ALIGN_JUSTIFY == textStyle->mTextAlign) {
    
    PropertyProvider provider(mTextRun, textStyle, frag, this, start, contentLength,
                              nsnull, 0);
    PRBool isCJK = IsChineseJapaneseLangGroup(this);
    gfxSkipCharsIterator justificationEnd(iter);
    provider.FindEndOfJustificationRange(&justificationEnd);

    PRInt32 i;
    for (i = justificationEnd.GetOriginalOffset(); i < trimmed.GetEnd(); ++i) {
      if (IsJustifiableCharacter(frag, i, isCJK)) {
        aLastCharIsJustifiable = PR_TRUE;
      }
    }
  }

  gfxContext* ctx = static_cast<gfxContext*>
                               (aRC.GetNativeGraphicData(nsIRenderingContext::NATIVE_THEBES_CONTEXT));
  gfxFloat advanceDelta;
  mTextRun->SetLineBreaks(trimmedStart, trimmedEnd - trimmedStart,
                          (GetStateBits() & TEXT_START_OF_LINE) != 0, PR_TRUE,
                          &advanceDelta, ctx);

  
  
  
  aDeltaWidth = NSToCoordFloor(delta - advanceDelta);
  
  
  
  
  
  
  
  
  
  if (aDeltaWidth < 0) {
    NS_WARNING("Negative deltawidth, something odd is happening");
  }

  

#ifdef NOISY_TRIM
  ListTag(stdout);
  printf(": trim => %d\n", aDeltaWidth);
#endif
  return NS_OK;
}

static PRUnichar TransformChar(const nsStyleText* aStyle, gfxTextRun* aTextRun,
                               PRUint32 aSkippedOffset, PRUnichar aChar)
{
  if (aChar == '\n' || aChar == '\r') {
    return aStyle->WhiteSpaceIsSignificant() ? aChar : ' ';
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
  const nsTextFragment* textFrag = mContent->GetText();
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
nsTextFrame::ToCString(nsString& aBuf, PRInt32* aTotalContentLength) const
{
  
  const nsTextFragment* frag = mContent->GetText();
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
      aBuf.AppendLiteral("\\0");
      aBuf.AppendInt((PRInt32)ch, 8);
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
  
  
  if (GetStyleText()->WhiteSpaceIsSignificant()) {
    return PR_FALSE;
  }

  if (mState & TEXT_ISNOT_ONLY_WHITESPACE) {
    return PR_FALSE;
  }

  if (mState & TEXT_IS_ONLY_WHITESPACE) {
    return PR_TRUE;
  }
  
  PRBool isEmpty = mContent->TextIsOnlyWhitespace();
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
  nsAutoString tmp;
  ToCString(tmp, &totalContentLength);

  
  PRBool isComplete = GetContentEnd() == totalContentLength;
  fprintf(out, "[%d,%d,%c] ", 
          GetContentOffset(), GetContentLength(),
          isComplete ? 'T':'F');
  
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", static_cast<void*>(mNextSibling));
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
  fputs(NS_LossyConvertUTF16toASCII(tmp).get(), out);
  fputs("\"\n", out);

  aIndent--;
  IndentBy(out, aIndent);
  fputs(">\n", out);

  return NS_OK;
}
#endif

void nsTextFrame::AdjustSelectionPointsForBidi(SelectionDetails *sdptr,
                                               PRInt32 textLength,
                                               PRBool isRTLChars,
                                               PRBool isOddLevel,
                                               PRBool isBidiSystem)
{
  




















  if (isOddLevel ^ (isRTLChars && isBidiSystem)) {

    PRInt32 swap  = sdptr->mStart;
    sdptr->mStart = textLength - sdptr->mEnd;
    sdptr->mEnd   = textLength - swap;

    
    
    
    
    NS_ASSERTION((sdptr->mStart >= 0) , "mStart >= 0");
    if(sdptr->mStart < 0 )
      sdptr->mStart = 0;

    NS_ASSERTION((sdptr->mEnd >= 0) , "mEnd >= 0");
    if(sdptr->mEnd < 0 )
      sdptr->mEnd = 0;

    NS_ASSERTION((sdptr->mStart <= sdptr->mEnd), "mStart <= mEnd");
    if(sdptr->mStart > sdptr->mEnd)
      sdptr->mEnd = sdptr->mStart;
  }
  
  return;
}

void
nsTextFrame::AdjustOffsetsForBidi(PRInt32 aStart, PRInt32 aEnd)
{
  AddStateBits(NS_FRAME_IS_BIDI);

  nsTextFrame* prev = static_cast<nsTextFrame*>(GetPrevInFlow());
  if (prev) {
    
    
    PRInt32 prevOffset = prev->GetContentOffset();
    aStart = PR_MAX(aStart, prevOffset);
    aEnd = PR_MAX(aEnd, prevOffset);
  }
  if (mContentOffset != aStart) {
    mContentOffset = aStart;
    ClearTextRun();
    if (prev) {
      prev->ClearTextRun();
    }
  }

  SetLength(aEnd - aStart);
}





PRBool
nsTextFrame::HasTerminalNewline() const
{
  return ::HasTerminalNewline(this);
}
