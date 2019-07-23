

















































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
#include "prtime.h"
#include "nsVoidArray.h"
#include "prprf.h"
#include "nsIDOMText.h"
#include "nsIDocument.h"
#include "nsIDeviceContext.h"
#include "nsICaret.h"
#include "nsCSSPseudoElements.h"
#include "nsILineBreaker.h"
#include "nsCompatibility.h"
#include "nsCSSColorUtils.h"
#include "nsLayoutUtils.h"
#include "nsDisplayList.h"
#include "nsFrame.h"
#include "nsTextTransformer.h"

#include "nsTextFragment.h"
#include "nsGkAtoms.h"
#include "nsFrameSelection.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsILookAndFeel.h"
#include "nsCSSRendering.h"
#include "nsContentUtils.h"

#include "nsILineIterator.h"

#include "nsCompressedCharMap.h"

#include "nsIServiceManager.h"
#ifdef ACCESSIBILITY
#include "nsIAccessible.h"
#include "nsIAccessibilityService.h"
#endif
#include "nsGUIEvent.h"
#include "nsAutoPtr.h"
#include "nsStyleSet.h"

#include "nsBidiFrames.h"
#include "nsBidiPresUtils.h"
#include "nsBidiUtils.h"

#ifdef SUNCTL
#include "nsILE.h"
static NS_DEFINE_CID(kLECID, NS_ULE_CID);
#endif 

#ifdef NS_DEBUG
#undef NOISY_BLINK
#undef DEBUG_WORD_WRAPPING
#undef NOISY_REFLOW
#undef NOISY_TRIM
#else
#undef NOISY_BLINK
#undef DEBUG_WORD_WRAPPING
#undef NOISY_REFLOW
#undef NOISY_TRIM
#endif



#define kSZLIG 0x00DF


#define TEXT_BUF_SIZE 100



struct nsAutoIndexBuffer;
struct nsAutoPRUint8Buffer;

class nsTextStyle {
public:
  const nsStyleFont* mFont;
  const nsStyleText* mText;
  nsIFontMetrics* mNormalFont;
  nsIFontMetrics* mSmallFont;
  nsIFontMetrics* mLastFont;
  PRBool mSmallCaps;
  nscoord mWordSpacing;
  nscoord mLetterSpacing;
  nscoord mSpaceWidth;
  nscoord mAveCharWidth;
  PRBool mJustifying;
  PRBool mPreformatted;
  PRInt32 mNumJustifiableCharacterToRender;
  PRInt32 mNumJustifiableCharacterToMeasure;
  nscoord mExtraSpacePerJustifiableCharacter;
  PRInt32 mNumJustifiableCharacterReceivingExtraJot;

  nsTextStyle(nsPresContext* aPresContext,
              nsIRenderingContext& aRenderingContext,
              nsStyleContext* sc);

  ~nsTextStyle();
};


class nsTextPaintStyle : public nsTextStyle {
public:
  enum{
    eNormalSelection =
      nsISelectionController::SELECTION_NORMAL,
    eIMESelections =
      nsISelectionController::SELECTION_IME_RAWINPUT |
      nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT |
      nsISelectionController::SELECTION_IME_CONVERTEDTEXT |
      nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT,
    eAllSelections =
      eNormalSelection | eIMESelections
  };

  const nsStyleColor* mColor;

  nsTextPaintStyle(nsPresContext* aPresContext,
                   nsIRenderingContext& aRenderingContext,
                   nsStyleContext* aStyleContext,
                   nsIContent* aContent,
                   PRInt16 aSelectionStatus);
  ~nsTextPaintStyle();

  nscolor GetTextColor();
  void GetSelectionColors(nscolor* aForeColor,
                          nscolor* aBackColor,
                          PRBool*  aBackIsTransparent);
  void GetIMESelectionColors(SelectionType aSelectionType,
                             nscolor*      aForeColor,
                             nscolor*      aBackColor,
                             PRBool*       aBackIsTransparent);
  
  PRBool GetIMEUnderline(SelectionType aSelectionType,
                         nscolor*      aLineColor,
                         float*        aRelativeSize);
protected:
  nsPresContext* mPresContext;
  nsStyleContext* mStyleContext;
  nsIContent* mContent;
  PRInt16 mSelectionStatus; 

  
  PRBool mInitCommonColors;

  PRInt32 mSufficientContrast;
  nscolor mFrameBackgroundColor;

  
  PRBool mInitSelectionColors;

  nscolor mSelectionTextColor;
  nscolor mSelectionBGColor;
  PRBool  mSelectionBGIsTransparent;

  
  struct nsIMEColor {
    PRBool mInit;
    nscolor mTextColor;
    nscolor mBGColor;
    nscolor mBGIsTransparent;
    nscolor mUnderlineColor;
  };
  nsIMEColor mIMEColor[4];
  
  enum {
    eIndexRawInput = 0,
    eIndexSelRawText,
    eIndexConvText,
    eIndexSelConvText
  };
  float mIMEUnderlineRelativeSize;

  
  PRBool InitCommonColors();
  PRBool InitSelectionColors();

  nsIMEColor* GetIMEColor(SelectionType aSelectionType);
  PRBool InitIMEColors(SelectionType aSelectionType, nsIMEColor*);

  PRBool EnsureSufficientContrast(nscolor *aForeColor, nscolor *aBackColor);

  nscolor GetResolvedForeColor(nscolor aColor, nscolor aDefaultForeColor,
                               nscolor aBackColor);
};

class nsTextFrame : public nsFrame {
public:
  nsTextFrame(nsStyleContext* aContext) : nsFrame(aContext)
  {
    NS_ASSERTION(mContentOffset == 0, "Bogus content offset");
    NS_ASSERTION(mContentLength == 0, "Bogus content length");
  }
  
  
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
                              
  void PaintText(nsIRenderingContext& aRenderingContext, nsPoint aPt);
  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void Destroy();
  
  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);
  
  NS_IMETHOD CharacterDataChanged(nsPresContext* aPresContext,
                                  nsIContent*     aChild,
                                  PRBool          aAppend);
  
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
    
    
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }
  
#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  NS_IMETHOD_(nsFrameState) GetDebugStateBits() const ;
#endif
  
  NS_IMETHOD GetPositionHelper(const nsPoint&  aPoint,
                         nsIContent **   aNewContent,
                         PRInt32&        aContentOffset,
                         PRInt32&        aContentOffsetEnd);
  
  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint);
  
  NS_IMETHOD GetPositionSlowly(nsIRenderingContext * aRendContext,
                               const nsPoint&        aPoint,
                               nsIContent **         aNewContent,
                               PRInt32&              aOffset);
  
  
  NS_IMETHOD SetSelected(nsPresContext* aPresContext,
                         nsIDOMRange *aRange,
                         PRBool aSelected,
                         nsSpread aSpread);
  
  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                                PRInt32* aOffset, PRBool* aSawBeforeType);
    
  NS_IMETHOD CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aRecurse, PRBool *aFinished, PRBool *_retval);
  
  NS_IMETHOD GetOffsets(PRInt32 &start, PRInt32 &end)const;
  
  virtual void AdjustOffsetsForBidi(PRInt32 start, PRInt32 end);
  
  NS_IMETHOD GetPointFromOffset(nsPresContext*         inPresContext,
                                nsIRenderingContext*    inRendContext,
                                PRInt32                 inOffset,
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

  struct TextReflowData {
    PRInt32             mX;                   
    PRInt32             mOffset;              
    nscoord             mAscent;              
    nscoord             mDescent;             
    PRPackedBool        mWrapping;            
    PRPackedBool        mSkipWhitespace;      
    PRPackedBool        mMeasureText;         
    PRPackedBool        mInWord;              
    PRPackedBool        mFirstLetterOK;       
    PRPackedBool        mCanBreakBefore;         
    PRPackedBool        mTrailingSpaceTrimmed; 
    
    TextReflowData(PRInt32 aStartingOffset,
                   PRBool  aWrapping,
                   PRBool  aSkipWhitespace,
                   PRBool  aMeasureText,
                   PRBool  aInWord,
                   PRBool  aFirstLetterOK,
                   PRBool  aCanBreakBefore,
                   PRBool  aTrailingSpaceTrimmed)
      : mX(0),
      mOffset(aStartingOffset),
      mAscent(0),
      mDescent(0),
      mWrapping(aWrapping),
      mSkipWhitespace(aSkipWhitespace),
      mMeasureText(aMeasureText),
      mInWord(aInWord),
      mFirstLetterOK(aFirstLetterOK),
      mCanBreakBefore(aCanBreakBefore),
      mTrailingSpaceTrimmed(aTrailingSpaceTrimmed)
    {}
  };
  
  nsIDocument* GetDocument(nsPresContext* aPresContext);
  
  void PrepareUnicodeText(nsTextTransformer& aTransformer,
                          nsAutoIndexBuffer* aIndexBuffer,
                          nsAutoTextBuffer* aTextBuffer,
                          PRInt32* aTextLen,
                          PRBool aForceArabicShaping = PR_FALSE,
                          PRIntn* aJustifiableCharCount = nsnull,
                          PRBool aRemoveMultipleTrimmedWS = PR_FALSE);
  void ComputeExtraJustificationSpacing(nsIRenderingContext& aRenderingContext,
                                        nsTextStyle& aTextStyle,
                                        PRUnichar* aBuffer, PRInt32 aLength, PRInt32 aNumJustifiableCharacter);

  void SetupTextRunDirection(nsPresContext* aPresContext, nsIRenderingContext* aRenderingContext);

  



  void PaintTextDecorations(nsIRenderingContext& aRenderingContext,
                            nsStyleContext* aStyleContext,
                            nsPresContext* aPresContext,
                            nsTextPaintStyle& aStyle,
                            nscoord aX, nscoord aY, nscoord aWidth,
                            PRBool aRightToLeftText,
                            PRUnichar* aText = nsnull,
                            SelectionDetails *aDetails = nsnull,
                            PRUint32 aIndex = 0,
                            PRUint32 aLength = 0,
                            const nscoord* aSpacing = nsnull);
  
  void PaintTextSlowly(nsPresContext* aPresContext,
                       nsIRenderingContext& aRenderingContext,
                       nsStyleContext* aStyleContext,
                       nsTextPaintStyle& aStyle,
                       nscoord aX, nscoord aY);
  
  
  
  



  void RenderString(nsIRenderingContext& aRenderingContext,
                    nsStyleContext* aStyleContext,
                    nsPresContext* aPresContext,
                    nsTextPaintStyle& aStyle,
                    PRBool aRightToLeftText,
                    PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                    nscoord aX, nscoord aY,
                    nscoord aWidth,
                    SelectionDetails *aDetails = nsnull);
  
  void MeasureSmallCapsText(nsIRenderingContext* aRenderingContext,
                            nsTextStyle& aStyle,
                            PRUnichar* aWord,
                            PRInt32 aWordLength,
                            PRBool aIsEndOfFrame,
                            nsTextDimensions* aDimensionsResult);
  
  PRUint32 EstimateNumChars(PRUint32 aAvailableWidth,
                            PRUint32 aAverageCharWidth);
  
  nsReflowStatus MeasureText(nsPresContext*          aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             nsTextTransformer&       aTx,
                             nsTextStyle&               aTs,
                             TextReflowData&          aTextData);
  
  void GetTextDimensions(nsIRenderingContext& aRenderingContext,
                         nsTextStyle& aStyle,
                         PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                         nsTextDimensions* aDimensionsResult);
  
  
  
  
  PRInt32 GetLengthSlowly(nsIRenderingContext& aRenderingContext,
                          nsTextStyle& aStyle,
                          PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                          nscoord aWidth);
  
  
  
  PRBool IsTextInSelection();
  
  nsresult GetTextInfoForPainting(nsPresContext*           aPresContext,
                                  nsIPresShell**           aPresShell,
                                  nsISelectionController** aSelectionController,
                                  PRBool&                  aDisplayingSelection,
                                  PRBool&                  aIsPaginated,
                                  PRBool&                  aIsSelected,
                                  PRBool&                  aHideStandardSelection,
                                  PRInt16&                 aSelectionValue);

  nsresult GetSelectionStatus(nsPresContext* aPresContext,
                              PRInt16&       aSelectionValue);

  void PaintUnicodeText(nsPresContext* aPresContext,
                        nsIRenderingContext& aRenderingContext,
                        nsStyleContext* aStyleContext,
                        nsTextPaintStyle& aStyle,
                        nscoord dx, nscoord dy);
  
  void PaintAsciiText(nsPresContext* aPresContext,
                      nsIRenderingContext& aRenderingContext,
                      nsStyleContext* aStyleContext,
                      nsTextPaintStyle& aStyle,
                      nscoord dx, nscoord dy);

#ifdef DEBUG
  void ToCString(nsString& aBuf, PRInt32* aTotalContentLength) const;
#endif

  PRInt32 GetContentOffset() { return mContentOffset; }
  PRInt32 GetContentLength() { return mContentLength; }

protected:
    virtual ~nsTextFrame();
  
  nsIFrame* mNextContinuation;
  PRInt32   mContentOffset;
  PRInt32   mContentLength;
  PRInt32   mColumn;
  nscoord   mAscent;
  
  PRInt32 GetTextDimensionsOrLength(nsIRenderingContext& aRenderingContext,
                                    nsTextStyle& aStyle,
                                    PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                                    nsTextDimensions* aDimensionsResult,
                                    PRBool aGetTextDimensions);
  nsresult GetContentAndOffsetsForSelection(nsPresContext*  aPresContext,nsIContent **aContent, PRInt32 *aOffset, PRInt32 *aLength);
  
  void AdjustSelectionPointsForBidi(SelectionDetails *sdptr,
                                    PRInt32 textLength,
                                    PRBool isRTLChars,
                                    PRBool isOddLevel,
                                    PRBool isBidiSystem);
  
  void SetOffsets(PRInt32 start, PRInt32 end);
  
  PRBool IsChineseJapaneseLangGroup();
  PRBool IsJustifiableCharacter(PRUnichar aChar, PRBool aLangIsCJ);
  
  nsresult FillClusterBuffer(nsPresContext *aPresContext, const PRUnichar *aText,
                             PRUint32 aLength, nsAutoPRUint8Buffer& aClusterBuffer);
};





inline PRBool CanDarken(nsPresContext* aPresContext)
{
  PRBool darken;

  if (aPresContext->GetBackgroundColorDraw()) {
    darken = PR_FALSE;
  } else {
    if (aPresContext->GetBackgroundImageDraw()) {
      darken = PR_FALSE;
    } else {
      darken = PR_TRUE;
    }
  }

  return darken;
}


struct nsAutoIndexBuffer {
  nsAutoIndexBuffer();
  ~nsAutoIndexBuffer();

  nsresult GrowTo(PRInt32 aAtLeast);

  PRInt32* mBuffer;
  PRInt32 mBufferLen;
  PRInt32 mAutoBuffer[TEXT_BUF_SIZE];
};

nsAutoIndexBuffer::nsAutoIndexBuffer()
  : mBuffer(mAutoBuffer),
    mBufferLen(TEXT_BUF_SIZE)
{
#ifdef DEBUG
  memset(mAutoBuffer, 0xdd, sizeof(mAutoBuffer));
#endif 
}

nsAutoIndexBuffer::~nsAutoIndexBuffer()
{
  if (mBuffer && (mBuffer != mAutoBuffer)) {
    delete [] mBuffer;
  }
}

nsresult
nsAutoIndexBuffer::GrowTo(PRInt32 aAtLeast)
{
  if (aAtLeast > mBufferLen)
  {
    PRInt32 newSize = mBufferLen * 2;
    if (newSize < mBufferLen + aAtLeast) {
      newSize = mBufferLen * 2 + aAtLeast;
    }
    PRInt32* newBuffer = new PRInt32[newSize];
    if (!newBuffer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
#ifdef DEBUG
    memset(newBuffer, 0xdd, sizeof(PRInt32) * newSize);
#endif
    memcpy(newBuffer, mBuffer, sizeof(PRInt32) * mBufferLen);
    if (mBuffer != mAutoBuffer) {
      delete [] mBuffer;
    }
    mBuffer = newBuffer;
    mBufferLen = newSize;
  }
  return NS_OK;
}

struct nsAutoPRUint8Buffer {
  nsAutoPRUint8Buffer();
  ~nsAutoPRUint8Buffer();

  nsresult GrowTo(PRInt32 aAtLeast);

  PRUint8* mBuffer;
  PRInt32 mBufferLen;
  PRUint8 mAutoBuffer[TEXT_BUF_SIZE];
};

nsAutoPRUint8Buffer::nsAutoPRUint8Buffer()
  : mBuffer(mAutoBuffer),
    mBufferLen(TEXT_BUF_SIZE)
{
#ifdef DEBUG
  memset(mAutoBuffer, 0xdd, sizeof(mAutoBuffer));
#endif 
}

nsAutoPRUint8Buffer::~nsAutoPRUint8Buffer()
{
  if (mBuffer && (mBuffer != mAutoBuffer)) {
    delete [] mBuffer;
  }
}

nsresult
nsAutoPRUint8Buffer::GrowTo(PRInt32 aAtLeast)
{
  if (aAtLeast > mBufferLen)
  {
    PRInt32 newSize = mBufferLen * 2;
    if (newSize < mBufferLen + aAtLeast) {
      newSize = mBufferLen * 2 + aAtLeast;
    }
    PRUint8* newBuffer = new PRUint8[newSize];
    if (!newBuffer) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
#ifdef DEBUG
    memset(newBuffer, 0xdd, sizeof(PRUint8) * newSize);
#endif
    memcpy(newBuffer, mBuffer, sizeof(PRUint8) * mBufferLen);
    if (mBuffer != mAutoBuffer) {
      delete [] mBuffer;
    }
    mBuffer = newBuffer;
    mBufferLen = newSize;
  }
  return NS_OK;
}






class nsBlinkTimer : public nsITimerCallback
{
public:
  nsBlinkTimer();
  virtual ~nsBlinkTimer();

  NS_DECL_ISUPPORTS

  void AddFrame(nsIFrame* aFrame);

  PRBool RemoveFrame(nsIFrame* aFrame);

  PRInt32 FrameCount();

  void Start();

  void Stop();

  NS_DECL_NSITIMERCALLBACK

  static nsresult AddBlinkFrame(nsPresContext* aPresContext, nsIFrame* aFrame);
  static nsresult RemoveBlinkFrame(nsIFrame* aFrame);
  
  static PRBool   GetBlinkIsOff() { return sState == 3; }
  
protected:

  nsCOMPtr<nsITimer> mTimer;
  nsVoidArray     mFrames;

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

void nsBlinkTimer::AddFrame(nsIFrame* aFrame) {
  mFrames.AppendElement(aFrame);
  if (1 == mFrames.Count()) {
    Start();
  }
}

PRBool nsBlinkTimer::RemoveFrame(nsIFrame* aFrame) {
  PRBool rv = mFrames.RemoveElement(aFrame);
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
    nsIFrame* frame = (nsIFrame*) mFrames.ElementAt(i);

    
    
    nsRect bounds(nsPoint(0, 0), frame->GetSize());
    frame->Invalidate(bounds, PR_FALSE);
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

  sTextBlinker->AddFrame(aFrame);
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



nsTextStyle::nsTextStyle(nsPresContext* aPresContext,
                         nsIRenderingContext& aRenderingContext,
                         nsStyleContext* sc)
{
  
  mFont = sc->GetStyleFont();
  mText = sc->GetStyleText();
  
  
  
  nsFont* plainFont = (nsFont *)&mFont->mFont; 
  NS_ASSERTION(plainFont, "null plainFont: font problems in nsTextStyle::nsTextStyle");
  PRUint8 originalDecorations = plainFont->decorations;
  plainFont->decorations = NS_FONT_DECORATION_NONE;
  mAveCharWidth = 0;
  SetFontFromStyle(&aRenderingContext, sc); 
  aRenderingContext.GetFontMetrics(mNormalFont);
  mNormalFont->GetSpaceWidth(mSpaceWidth);
  mNormalFont->GetAveCharWidth(mAveCharWidth);
  mLastFont = mNormalFont;
  
  
  mSmallCaps = NS_STYLE_FONT_VARIANT_SMALL_CAPS == plainFont->variant;
  if (mSmallCaps) {
    nscoord originalSize = plainFont->size;
    plainFont->size = nscoord(0.8 * plainFont->size);
    mSmallFont = aPresContext->GetMetricsFor(*plainFont).get();  
                                                                 
    plainFont->size = originalSize;
  }
  else {
    mSmallFont = nsnull;
  }
  
  
  plainFont->decorations = originalDecorations; 
  
  
  PRIntn unit = mText->mWordSpacing.GetUnit();
  if (eStyleUnit_Coord == unit) {
    mWordSpacing = mText->mWordSpacing.GetCoordValue();
  } else {
    mWordSpacing = 0;
  }
  
  unit = mText->mLetterSpacing.GetUnit();
  if (eStyleUnit_Coord == unit) {
    mLetterSpacing = mText->mLetterSpacing.GetCoordValue();
  } else {
    mLetterSpacing = 0;
  }
  
  mNumJustifiableCharacterToRender = 0;
  mNumJustifiableCharacterToMeasure = 0;
  mNumJustifiableCharacterReceivingExtraJot = 0;
  mExtraSpacePerJustifiableCharacter = 0;
  mPreformatted = (NS_STYLE_WHITESPACE_PRE == mText->mWhiteSpace) ||
    (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == mText->mWhiteSpace);
  
  mJustifying = (NS_STYLE_TEXT_ALIGN_JUSTIFY == mText->mTextAlign) &&
    !mPreformatted;
}

nsTextStyle::~nsTextStyle() {
  NS_IF_RELEASE(mNormalFont);
  NS_IF_RELEASE(mSmallFont);
}



inline nscolor EnsureDifferentColors(nscolor colorA, nscolor colorB)
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



nsTextPaintStyle::nsTextPaintStyle(nsPresContext* aPresContext,
                                   nsIRenderingContext& aRenderingContext,
                                   nsStyleContext* aStyleContext,
                                   nsIContent* aContent,
                                   PRInt16 aSelectionStatus)
  : nsTextStyle(aPresContext, aRenderingContext, aStyleContext),
    mPresContext(nsnull),
    mStyleContext(nsnull),
    mContent(nsnull),
    mInitCommonColors(PR_FALSE),
    mInitSelectionColors(PR_FALSE)
{
  mPresContext = aPresContext;
  mStyleContext = aStyleContext;
  mContent = aContent;
  mSelectionStatus = aSelectionStatus;
  mColor = mStyleContext->GetStyleColor();
  for (int i = 0; i < 4; i++)
    mIMEColor[i].mInit = PR_FALSE;
  mIMEUnderlineRelativeSize = -1.0f;
}

nsTextPaintStyle::~nsTextPaintStyle()
{
  mColor = nsnull;
}

PRBool
nsTextPaintStyle::EnsureSufficientContrast(nscolor *aForeColor, nscolor *aBackColor)
{

  if (!aForeColor || !aBackColor)
    return PR_FALSE;

  
  
  if (!mInitCommonColors && !InitCommonColors())
    return PR_FALSE;

  
  
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
  return mColor->mColor;
}

void
nsTextPaintStyle::GetSelectionColors(nscolor* aForeColor,
                                     nscolor* aBackColor,
                                     PRBool*  aBackIsTransparent)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");
  NS_ASSERTION(aBackIsTransparent, "aBackIsTransparent is null");

  if (!mInitSelectionColors && !InitSelectionColors()) {
    NS_ERROR("Fail to initialize selection colors");
    return;
  }

  *aForeColor = mSelectionTextColor;
  *aBackColor = mSelectionBGColor;
  *aBackIsTransparent = mSelectionBGIsTransparent;
}

void
nsTextPaintStyle::GetIMESelectionColors(SelectionType aSelectionType,
                                        nscolor*      aForeColor,
                                        nscolor*      aBackColor,
                                        PRBool*       aBackIsTransparent)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");
  NS_ASSERTION(aBackIsTransparent, "aBackIsTransparent is null");

  nsIMEColor* IMEColor = GetIMEColor(aSelectionType);
  if (!IMEColor) {
    NS_ERROR("aSelectionType is invalid");
    return;
  }
  if (!IMEColor->mInit)
    return;
  *aForeColor = IMEColor->mTextColor;
  *aBackColor = IMEColor->mBGColor;
  *aBackIsTransparent = IMEColor->mBGIsTransparent;
}

PRBool
nsTextPaintStyle::GetIMEUnderline(SelectionType aSelectionType,
                                  nscolor*      aLineColor,
                                  float*        aRelativeSize)
{
  NS_ASSERTION(aLineColor, "aLineColor is null");
  NS_ASSERTION(aRelativeSize, "aRelativeSize is null");

  nsIMEColor* IMEColor = GetIMEColor(aSelectionType);
  if (!IMEColor) {
    NS_ERROR("aSelectionType is invalid");
    return PR_FALSE;
  }
  if (!IMEColor->mInit)
    return PR_FALSE;
  if (IMEColor->mUnderlineColor == NS_TRANSPARENT ||
      mIMEUnderlineRelativeSize <= 0.0f)
    return PR_FALSE;

  *aLineColor = IMEColor->mUnderlineColor;
  *aRelativeSize = mIMEUnderlineRelativeSize;
  return PR_TRUE;
}

PRBool
nsTextPaintStyle::InitCommonColors()
{
  if (!mPresContext || !mStyleContext)
    return PR_FALSE;

  if (mInitCommonColors)
    return PR_TRUE;

  const nsStyleBackground* bg =
    nsCSSRendering::FindNonTransparentBackground(mStyleContext);
  NS_ASSERTION(bg, "Cannot find NonTransparentBackground.");
  mFrameBackgroundColor = bg->mBackgroundColor;

  nsILookAndFeel* look = mPresContext->LookAndFeel();
  if (!look)
    return PR_FALSE;

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
  return PR_TRUE;
}

PRBool
nsTextPaintStyle::InitSelectionColors()
{
  if (!mPresContext || !mStyleContext)
    return PR_FALSE;
  if (mInitSelectionColors)
    return PR_TRUE;

  mSelectionBGIsTransparent = PR_FALSE;

  if (mContent &&
      mSelectionStatus == nsISelectionController::SELECTION_ON) {
    nsRefPtr<nsStyleContext> sc = nsnull;
    sc = mPresContext->StyleSet()->
      ProbePseudoStyleFor(mContent->GetParent(),
                          nsCSSPseudoElements::mozSelection, mStyleContext);
    
    if (sc) {
      const nsStyleBackground* bg = sc->GetStyleBackground();
      mSelectionBGIsTransparent =
        PRBool(bg->mBackgroundFlags & NS_STYLE_BG_COLOR_TRANSPARENT);
      if (!mSelectionBGIsTransparent)
        mSelectionBGColor = bg->mBackgroundColor;
      mSelectionTextColor = sc->GetStyleColor()->mColor;
      return PR_TRUE;
    }
  }

  nsILookAndFeel* look = mPresContext->LookAndFeel();
  if (!look)
    return PR_FALSE;

  nscolor selectionBGColor;
  look->GetColor(nsILookAndFeel::eColor_TextSelectBackground,
                 selectionBGColor);

  if (mSelectionStatus == nsISelectionController::SELECTION_ATTENTION) {
    look->GetColor(nsILookAndFeel::eColor_TextSelectBackgroundAttention,
                   mSelectionBGColor);
    mSelectionBGColor  = EnsureDifferentColors(mSelectionBGColor,
                                               selectionBGColor);
  } else if (mSelectionStatus != nsISelectionController::SELECTION_ON) {
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
    mSelectionTextColor = EnsureDifferentColors(mColor->mColor,
                                                mSelectionBGColor);
    return PR_TRUE;
  }

  EnsureSufficientContrast(&mSelectionTextColor, &mSelectionBGColor);

  mInitSelectionColors = PR_TRUE;
  return PR_TRUE;
}

nsTextPaintStyle::nsIMEColor*
nsTextPaintStyle::GetIMEColor(SelectionType aSelectionType)
{
  PRInt32 index;
  switch (aSelectionType) {
    case nsISelectionController::SELECTION_IME_RAWINPUT:
      index = eIndexRawInput;
      break;
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
      index = eIndexSelRawText;
      break;
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
      index = eIndexConvText;
      break;
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
      index = eIndexSelConvText;
      break;
    default:
      NS_ERROR("aSelectionType is Invalid");
      return nsnull;
  }
  nsIMEColor* IMEColor = &mIMEColor[index];
  if (!IMEColor->mInit && !InitIMEColors(aSelectionType, IMEColor))
    NS_ERROR("Fail to initialize IME color");
  return IMEColor;
}

PRBool
nsTextPaintStyle::InitIMEColors(SelectionType aSelectionType,
                                nsIMEColor*   aIMEColor)
{
  if (!mPresContext || !aIMEColor)
    return PR_FALSE;

  NS_ASSERTION(!aIMEColor->mInit, "this is already initialized");

  nsILookAndFeel::nsColorID foreColorID, backColorID, lineColorID;
  switch (aSelectionType) {
    case nsISelectionController::SELECTION_IME_RAWINPUT:
      foreColorID = nsILookAndFeel::eColor_IMERawInputForeground;
      backColorID = nsILookAndFeel::eColor_IMERawInputBackground;
      lineColorID = nsILookAndFeel::eColor_IMERawInputUnderline;
      break;
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
      foreColorID = nsILookAndFeel::eColor_IMESelectedRawTextForeground;
      backColorID = nsILookAndFeel::eColor_IMESelectedRawTextBackground;
      lineColorID = nsILookAndFeel::eColor_IMESelectedRawTextUnderline;
      break;
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
      foreColorID = nsILookAndFeel::eColor_IMEConvertedTextForeground;
      backColorID = nsILookAndFeel::eColor_IMEConvertedTextBackground;
      lineColorID = nsILookAndFeel::eColor_IMEConvertedTextUnderline;
      break;
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
      foreColorID = nsILookAndFeel::eColor_IMESelectedConvertedTextForeground;
      backColorID = nsILookAndFeel::eColor_IMESelectedConvertedTextBackground;
      lineColorID = nsILookAndFeel::eColor_IMESelectedConvertedTextUnderline;
      break;
    default:
      NS_ERROR("aSelectionType is Invalid");
      return PR_FALSE;
  }

  nsILookAndFeel* look = mPresContext->LookAndFeel();
  if (!look)
    return PR_FALSE;

  nscolor foreColor, backColor, lineColor;
  look->GetColor(foreColorID, foreColor);
  look->GetColor(backColorID, backColor);
  look->GetColor(lineColorID, lineColor);

  
  NS_ASSERTION(foreColor != NS_TRANSPARENT,
               "foreColor cannot be NS_TRANSPARENT");
  NS_ASSERTION(backColor != NS_SAME_AS_FOREGROUND_COLOR,
               "backColor cannot be NS_SAME_AS_FOREGROUND_COLOR");
  NS_ASSERTION(backColor != NS_40PERCENT_FOREGROUND_COLOR,
               "backColor cannot be NS_40PERCENT_FOREGROUND_COLOR");

  PRBool backIsTransparent = PR_FALSE;
  if (backColor == NS_TRANSPARENT)
    backIsTransparent = PR_TRUE;

  foreColor = GetResolvedForeColor(foreColor, GetTextColor(), backColor);

  if (!backIsTransparent)
    EnsureSufficientContrast(&foreColor, &backColor);

  lineColor = GetResolvedForeColor(lineColor, foreColor, backColor);

  aIMEColor->mTextColor       = foreColor;
  aIMEColor->mBGColor         = backColor;
  aIMEColor->mBGIsTransparent = backIsTransparent;
  aIMEColor->mUnderlineColor  = lineColor;
  aIMEColor->mInit            = PR_TRUE;

  if (mIMEUnderlineRelativeSize == -1.0f) {
    look->GetMetric(nsILookAndFeel::eMetricFloat_IMEUnderlineRelativeSize,
                    mIMEUnderlineRelativeSize);
    NS_ASSERTION(mIMEUnderlineRelativeSize >= 0.0f,
                 "underline size must be larger than 0");
  }

  return PR_TRUE;
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
    if (!mInitCommonColors && !InitCommonColors())
      return aDefaultForeColor;
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
      return accService->CreateHTMLTextAccessible(NS_STATIC_CAST(nsIFrame*, this), aAccessible);
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
  NS_PRECONDITION(aContent->IsNodeOfType(nsINode::eTEXT),
                  "Bogus content!");
  nsresult rv = nsFrame::Init(aContent, aParent, aPrevInFlow);
  if (NS_SUCCEEDED(rv) && !aPrevInFlow &&
      GetStyleText()->WhiteSpaceIsSignificant()) {
    
    
    

    
    
    mContentLength = mContent->TextLength();
  }
  return rv;
}

void
nsTextFrame::Destroy()
{
  if (mNextContinuation) {
    mNextContinuation->SetPrevInFlow(nsnull);
  }
  
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
  
protected:
  nsContinuingTextFrame(nsStyleContext* aContext) : nsTextFrame(aContext) {}
  nsIFrame* mPrevContinuation;
};

NS_IMETHODIMP
nsContinuingTextFrame::Init(nsIContent*      aContent,
                            nsIFrame*        aParent,
                            nsIFrame*        aPrevInFlow)
{
  nsresult rv = nsTextFrame::Init(aContent, aParent, aPrevInFlow);

  if (aPrevInFlow) {
    nsIFrame* nextContinuation = aPrevInFlow->GetNextContinuation();
    
    SetPrevInFlow(aPrevInFlow);
    aPrevInFlow->SetNextInFlow(this);
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
        mContentLength = PR_MAX(1, start - mContentOffset);
      }
      mState |= NS_FRAME_IS_BIDI;
    } 
#endif 
  }

  return rv;
}

void
nsContinuingTextFrame::Destroy()
{
  if (mPrevContinuation || mNextContinuation) {
    nsSplittableFrame::RemoveFromFlow(this);
  }
  
  nsFrame::Destroy();
}

nsIFrame*
nsContinuingTextFrame::GetFirstInFlow() const
{
  
  nsIFrame *firstInFlow,
           *previous = NS_CONST_CAST(nsIFrame*,
                                     NS_STATIC_CAST(const nsIFrame*, this));
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
  *previous = NS_CONST_CAST(nsIFrame*,
                            NS_STATIC_CAST(const nsIFrame*, mPrevContinuation));
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



class DrawSelectionIterator
{
public:
  DrawSelectionIterator(const SelectionDetails *aSelDetails, PRUnichar *aText,
                        PRUint32 aTextLength, nsTextPaintStyle *aTextStyle,
                        SelectionType aCareSelections);
  ~DrawSelectionIterator();
  PRBool      First();
  PRBool      Next();
  PRBool      IsDone();
  PRBool      IsLast();

  PRUnichar * CurrentTextUnicharPtr();
  char *      CurrentTextCStrPtr();
  PRUint32    CurrentLength();
  PRBool      IsBeforeOrAfter();

  












  PRBool GetSelectionColors(nscolor *aForeColor, nscolor *aBackColor, PRBool *aBackIsTransparent);
private:
  union {
    PRUnichar *mUniStr;
    char *mCStr;
  };
  PRUint32  mLength;
  PRUint32  mCurrentIdx;
  PRUint32  mCurrentLength;
  nsTextPaintStyle* mOldStyle;
  const SelectionDetails *mDetails;
  PRBool    mDone;
  PRUint8 * mTypes;
  PRBool    mInit;
  
  void FillCurrentData();
};

DrawSelectionIterator::DrawSelectionIterator(const SelectionDetails *aSelDetails, 
                                             PRUnichar *aText, 
                                             PRUint32 aTextLength, 
                                             nsTextPaintStyle* aTextStyle,
                                             SelectionType aCareSelections)
                                             :mOldStyle(aTextStyle)
{
  NS_ASSERTION(aCareSelections, "aCareSelection value must not be zero!");

  mDetails = aSelDetails;
  mCurrentIdx = 0;
  mUniStr = aText;
  mLength = aTextLength;
  mTypes = nsnull;
  mInit = PR_FALSE;

  if (!aSelDetails) {
    mDone = PR_TRUE;
    return;
  }
  mDone = (PRBool)(mCurrentIdx>=mLength);
  if (mDone)
    return;

  
  const SelectionDetails *details = aSelDetails;
  if (details->mNext) {
    
  } else if (details->mStart == details->mEnd) {
    
    mDone = PR_TRUE;
    return;
  } else if (!(details->mType & aCareSelections)) {
    
    mDone = PR_TRUE;
    return;
  }

  mTypes = new PRUint8[mLength];
  if (!mTypes)
    return;
  memset(mTypes, 0, mLength);
  while (details) {
    if ((details->mType & aCareSelections) &&
        (details->mStart != details->mEnd)) {
      mInit = PR_TRUE; 
      for (int i = details->mStart; i < details->mEnd; i++) {
        if ((PRUint32)i >= mLength) {
          NS_ASSERTION(0, "Selection Details out of range?");
          return;
        }
        mTypes[i] |= details->mType;
      }
    }
    details= details->mNext;
  }
  if (!mInit) {
    
    delete [] mTypes;
    mTypes = nsnull;
    mDone = PR_TRUE; 
    mInit = PR_TRUE;
  }
}

DrawSelectionIterator::~DrawSelectionIterator()
{
  if (mTypes)
    delete [] mTypes;
}

void
DrawSelectionIterator::FillCurrentData()
{
  if (mDone)
    return;
  mCurrentIdx += mCurrentLength; 
  mCurrentLength = 0;
  if (mCurrentIdx >= mLength)
  {
    mDone = PR_TRUE;
    return;
  }
  if (!mTypes)
  {
    if (mCurrentIdx < (PRUint32)mDetails->mStart)
    {
      mCurrentLength = mDetails->mStart;
    }
    else if (mCurrentIdx == (PRUint32)mDetails->mStart)
    {
        mCurrentLength = mDetails->mEnd-mCurrentIdx;
    }
    else if (mCurrentIdx > (PRUint32)mDetails->mStart)
    {
      mCurrentLength = mLength - mDetails->mEnd;
    }
  }
  else
  {
    uint8 typevalue = mTypes[mCurrentIdx];
    while (mCurrentIdx+mCurrentLength < mLength && typevalue == mTypes[mCurrentIdx+mCurrentLength])
    {
      mCurrentLength++;
    }
  }
  
  if (mCurrentIdx+mCurrentLength > mLength)
  {
    mCurrentLength = mLength - mCurrentIdx;
  }
}

PRBool
DrawSelectionIterator::First()
{
  if (!mInit)
    return PR_FALSE;
  mCurrentIdx = 0;
  mCurrentLength = 0;
  if (!mTypes && mDetails->mStart == mDetails->mEnd)
    mDone = PR_TRUE;
  mDone = (mCurrentIdx+mCurrentLength) >= mLength;
  FillCurrentData();
  return PR_TRUE;
}



PRBool
DrawSelectionIterator::Next()
{
  if (mDone || !mInit)
    return PR_FALSE;
  FillCurrentData();
  return PR_TRUE;
}

PRBool
DrawSelectionIterator::IsLast()
{
 return mDone || !mInit || mCurrentIdx + mCurrentLength >= mLength;
}

PRBool
DrawSelectionIterator::IsDone()
{
    return mDone || !mInit;
}


PRUnichar *
DrawSelectionIterator::CurrentTextUnicharPtr()
{
  return mUniStr+mCurrentIdx;
}

char *
DrawSelectionIterator::CurrentTextCStrPtr()
{
  return mCStr+mCurrentIdx;
}

PRUint32
DrawSelectionIterator::CurrentLength()
{
  return mCurrentLength;
}

PRBool
DrawSelectionIterator::GetSelectionColors(nscolor *aForeColor,
                                          nscolor *aBackColor,
                                          PRBool  *aBackIsTransparent)
{
  if (mTypes) {
    
    if (mTypes[mCurrentIdx] & nsTextPaintStyle::eNormalSelection) {
      mOldStyle->GetSelectionColors(aForeColor, aBackColor,
                                    aBackIsTransparent);
      return PR_TRUE;
    }

    
    if (mTypes[mCurrentIdx] & nsTextPaintStyle::eIMESelections) {
      mOldStyle->GetIMESelectionColors(mTypes[mCurrentIdx],
                                       aForeColor, aBackColor,
                                       aBackIsTransparent);
      return PR_TRUE;
    }
  }

  
  *aBackIsTransparent = PR_FALSE;
  *aForeColor = mOldStyle->GetTextColor();
  return PR_FALSE;
}

PRBool
DrawSelectionIterator::IsBeforeOrAfter()
{
  return mCurrentIdx != (PRUint32)mDetails->mStart;
}











#define TEXT_SKIP_LEADING_WS 0x01000000
#define TEXT_HAS_MULTIBYTE   0x02000000
#define TEXT_IN_WORD         0x04000000


#define TEXT_FIRST_LETTER    0x08000000
#define TEXT_WAS_TRANSFORMED 0x10000000


#define TEXT_REFLOW_FLAGS    0x1F000000

#define TEXT_TRIMMED_WS      0x20000000

#define TEXT_OPTIMIZE_RESIZE 0x40000000

#define TEXT_BLINK_ON        0x80000000

#define TEXT_IS_ONLY_WHITESPACE    0x00100000

#define TEXT_ISNOT_ONLY_WHITESPACE 0x00200000

#define TEXT_WHITESPACE_FLAGS      0x00300000

#define TEXT_IS_END_OF_LINE        0x00400000



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

nsIDocument*
nsTextFrame::GetDocument(nsPresContext* aPresContext)
{
  nsIDocument *result = nsnull;
  if (mContent) {
    result = mContent->GetDocument();
  }
  if (!result && aPresContext) {
    result = aPresContext->PresShell()->GetDocument();
  }
  return result;
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
  nsTextFrame* lastInFlow = NS_CONST_CAST(nsTextFrame*, this);
  while (lastInFlow->GetNextInFlow())  {
    lastInFlow = NS_STATIC_CAST(nsTextFrame*, lastInFlow->GetNextInFlow());
  }
  NS_POSTCONDITION(lastInFlow, "illegal state in flow chain.");
  return lastInFlow;
}
nsIFrame*
nsTextFrame::GetLastContinuation() const
{
  nsTextFrame* lastInFlow = NS_CONST_CAST(nsTextFrame*, this);
  while (lastInFlow->mNextContinuation)  {
    lastInFlow = NS_STATIC_CAST(nsTextFrame*, lastInFlow->mNextContinuation);
  }
  NS_POSTCONDITION(lastInFlow, "illegal state in continuation chain.");
  return lastInFlow;
}


NS_IMETHODIMP
nsTextFrame::CharacterDataChanged(nsPresContext* aPresContext,
                                  nsIContent*     aChild,
                                  PRBool          aAppend)
{
  nsIFrame* targetTextFrame = this;

  PRBool markAllDirty = PR_TRUE;
  if (aAppend) {
    markAllDirty = PR_FALSE;
    nsTextFrame* frame = NS_STATIC_CAST(nsTextFrame*, GetLastInFlow());
    frame->mState &= ~TEXT_WHITESPACE_FLAGS;
    frame->mState |= NS_FRAME_IS_DIRTY;
    targetTextFrame = frame;
  }

  if (markAllDirty) {
    
    
    
    nsTextFrame*  textFrame = this;
    while (textFrame) {
      textFrame->mState &= ~TEXT_WHITESPACE_FLAGS;
      textFrame->mState |= NS_FRAME_IS_DIRTY;
      textFrame->mContentOffset = 0;
      textFrame->mContentLength = 0;
      textFrame = NS_STATIC_CAST(nsTextFrame*, textFrame->GetNextContinuation());
    }
  }

  
  aPresContext->GetPresShell()->FrameNeedsReflow(targetTextFrame,
                                                 nsIPresShell::eStyleChange);

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

  virtual nsIFrame* HitTest(nsDisplayListBuilder* aBuilder, nsPoint aPt) { return mFrame; }
  virtual void Paint(nsDisplayListBuilder* aBuilder, nsIRenderingContext* aCtx,
     const nsRect& aDirtyRect);
  NS_DISPLAY_DECL_NAME("Text")
};

void
nsDisplayText::Paint(nsDisplayListBuilder* aBuilder,
     nsIRenderingContext* aCtx, const nsRect& aDirtyRect) {
  NS_STATIC_CAST(nsTextFrame*, mFrame)->
    PaintText(*aCtx, aBuilder->ToReferenceFrame(mFrame));
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

void
nsTextFrame::PaintText(nsIRenderingContext& aRenderingContext, nsPoint aPt)
{
  nsStyleContext* sc = mStyleContext;
  nsPresContext* presContext = PresContext();
  nsCOMPtr<nsIContent> content;
  PRInt32 offset, length;
  GetContentAndOffsetsForSelection(presContext,
                                   getter_AddRefs(content),
                                   &offset, &length);
  PRInt16 selectionValue;
  if (NS_FAILED(GetSelectionStatus(presContext, selectionValue)))
    selectionValue = nsISelectionController::SELECTION_NORMAL;
  
  nsTextPaintStyle ts(presContext, aRenderingContext, mStyleContext, content,
                      selectionValue);
  SetupTextRunDirection(presContext, &aRenderingContext);
  if (ts.mSmallCaps || (0 != ts.mWordSpacing) || (0 != ts.mLetterSpacing)
    || ts.mJustifying) {
    PaintTextSlowly(presContext, aRenderingContext, sc, ts, aPt.x, aPt.y);
  }
  else {
    
    const nsTextFragment* frag = mContent->GetText();
    if (!frag) {
      return;
    }

    
    
    
    PRBool   hasMultiByteChars = (0 != (mState & TEXT_HAS_MULTIBYTE));
    PRUint32 hints = 0;
    aRenderingContext.GetHints(hints);

#ifdef IBMBIDI
    PRBool bidiEnabled = presContext->BidiEnabled();
#else
    const PRBool bidiEnabled = PR_FALSE;
#endif 
    
    
    
    
    
    
    if (bidiEnabled || hasMultiByteChars ||
        ((0 == (hints & NS_RENDERING_HINT_FAST_8BIT_TEXT)) &&
         (frag->Is2b() || (0 != (mState & TEXT_WAS_TRANSFORMED))))) {
      PaintUnicodeText(presContext, aRenderingContext, sc, ts, aPt.x, aPt.y);
    }
    else {
      PaintAsciiText(presContext, aRenderingContext, sc, ts, aPt.x, aPt.y);
    }
  }
}

PRBool
nsTextFrame::IsChineseJapaneseLangGroup()
{
  const nsStyleVisibility* visibility = mStyleContext->GetStyleVisibility();
  if (visibility->mLangGroup == nsGkAtoms::Japanese
      || visibility->mLangGroup == nsGkAtoms::Chinese
      || visibility->mLangGroup == nsGkAtoms::Taiwanese
      || visibility->mLangGroup == nsGkAtoms::HongKongChinese)
    return PR_TRUE;

  return PR_FALSE;
}








PRBool
nsTextFrame::IsJustifiableCharacter(PRUnichar aChar, PRBool aLangIsCJ)
{
  if (0x20u == aChar || 0xa0u == aChar)
    return PR_TRUE;
  if (aChar < 0x2150u)
    return PR_FALSE;
  if (aLangIsCJ && (
       (0x2150u <= aChar && aChar <= 0x22ffu) || 
       (0x2460u <= aChar && aChar <= 0x24ffu) || 
       (0x2580u <= aChar && aChar <= 0x27bfu) || 
       (0x27f0u <= aChar && aChar <= 0x2bffu) || 
                                                 
                                                 
       (0x2e80u <= aChar && aChar <= 0x312fu) || 
                                                 
                                                 
       (0x3190u <= aChar && aChar <= 0xabffu) || 
                                                 
                                                 
                                                 
       (0xf900u <= aChar && aChar <= 0xfaffu) || 
       (0xff5eu <= aChar && aChar <= 0xff9fu)    
     ))
    return PR_TRUE;
  return PR_FALSE;
}

nsresult
nsTextFrame::FillClusterBuffer(nsPresContext *aPresContext, const PRUnichar *aText,
                               PRUint32 aLength, nsAutoPRUint8Buffer& aClusterBuffer)
{
  nsresult rv = aClusterBuffer.GrowTo(aLength);
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsIRenderingContext> acx;
  PRUint32 clusterHint = 0;

  nsIPresShell *shell = aPresContext->GetPresShell();
  if (shell) {
    rv = shell->CreateRenderingContext(this, getter_AddRefs(acx));
    NS_ENSURE_SUCCESS(rv, rv);

    
    SetFontFromStyle(acx, mStyleContext);

    acx->GetHints(clusterHint);
    clusterHint &= NS_RENDERING_HINT_TEXT_CLUSTERS;
  }

  if (clusterHint) {
    rv = acx->GetClusterInfo(aText, aLength, aClusterBuffer.mBuffer);
  }
  else {
    memset(aClusterBuffer.mBuffer, 1, sizeof(PRInt8) * aLength);
  }

  return rv;
}

inline PRBool IsEndOfLine(nsFrameState aState)
{
  return (aState & TEXT_IS_END_OF_LINE) ? PR_TRUE : PR_FALSE;
}

void nsTextFrame::SetupTextRunDirection(nsPresContext* aPresContext,
                                        nsIRenderingContext* aContext)
{
  PRBool isRTL = aPresContext->BidiEnabled() && (NS_GET_EMBEDDING_LEVEL(this) & 1);
  aContext->SetTextRunRTL(isRTL);
}






void
nsTextFrame::PrepareUnicodeText(nsTextTransformer& aTX,
                                nsAutoIndexBuffer* aIndexBuffer,
                                nsAutoTextBuffer* aTextBuffer,
                                PRInt32* aTextLen,
                                PRBool aForceArabicShaping,
                                PRIntn* aJustifiableCharCount,
                                PRBool aRemoveMultipleTrimmedWS)
{
  
  
  aTX.Init(this, mContent, mContentOffset, aForceArabicShaping);

  PRInt32 strInx = mContentOffset;
  PRInt32* indexp = aIndexBuffer ? aIndexBuffer->mBuffer : nsnull;

  
  PRInt32 n = mContentLength;
  if (0 != (mState & TEXT_SKIP_LEADING_WS)) {
    PRBool isWhitespace, wasTransformed;
    PRInt32 wordLen, contentLen;
    
    
    
    wordLen = mContentOffset + mContentLength;
    aTX.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, &wasTransformed);
    
    

    if (isWhitespace) {
      if (nsnull != indexp) {
        
        
        
        PRInt32 i = contentLen;
        while (--i >= 0) {
          *indexp++ = strInx;
        }
      }
      n -= contentLen;
      if(n<0)
        NS_WARNING("mContentLength is < FragmentLength");
    }
  }

  
  
  PRUint8 textTransform = GetStyleText()->mTextTransform;
  PRBool inWord = (TEXT_IN_WORD & mState) ? PR_TRUE : PR_FALSE;
  PRInt32 column = mColumn;
  PRInt32 textLength = 0;
  PRInt32 dstOffset = 0;

  nsAutoTextBuffer tmpTextBuffer;
  nsAutoTextBuffer* textBuffer = aTextBuffer;
  if (!textBuffer && aJustifiableCharCount)
    textBuffer = &tmpTextBuffer;

  while (n > 0) {
    PRUnichar* bp;
    PRBool isWhitespace, wasTransformed;
    PRInt32 wordLen, contentLen;

    
    
    
    wordLen = mContentOffset + mContentLength;
    
    bp = aTX.GetNextWord(inWord, &wordLen, &contentLen, &isWhitespace, &wasTransformed);
    if (nsnull == bp) {
      if (indexp) {
        while (--n >= 0) {
          *indexp++ = strInx;
        }
      }
      break;
    }
    inWord = PR_FALSE;
    if (isWhitespace) {
      if ('\t' == bp[0]) {
        PRInt32 spaces = 8 - (7 & column);
        PRUnichar* tp = bp;
        wordLen = spaces;
        while (--spaces >= 0) {
          *tp++ = ' ';
        }
        
        if (nsnull != indexp) {
          *indexp++ = strInx;
          strInx += wordLen;
        }
      }
      else if ('\n' == bp[0]) {
        if (nsnull != indexp) {
          *indexp++ = strInx;
        }
        break;
      }
      else if (nsnull != indexp) {
        if (1 == wordLen) {
          
          
          
          PRInt32 i = contentLen;
          while (--i >= 0) {
            *indexp++ = strInx;
          }
          strInx++;
        } else {
          
          PRInt32 i = contentLen;
          while (--i >= 0) {
            *indexp++ = strInx++;
          }
        }
      }
    }
    else {
      PRInt32 i = contentLen;
      if (nsnull != indexp) {
        
        if (!wasTransformed) {
          while (--i >= 0) {
            *indexp++ = strInx++;
          }
        } else {
          PRUnichar* tp = bp;
          PRBool caseChanged = 
            textTransform == NS_STYLE_TEXT_TRANSFORM_UPPERCASE ||
            textTransform == NS_STYLE_TEXT_TRANSFORM_CAPITALIZE;
          while (--i >= 0) {
            PRUnichar ch = aTX.GetContentCharAt(mContentOffset +
                             indexp - aIndexBuffer->mBuffer);
            if (IS_DISCARDED(ch) || ch == '\n') {
              *indexp++ = strInx;
              continue;
            }
            
            if (aTX.NeedsArabicShaping()) {
              if (IS_LAM(ch) && IS_LAMALEF(*tp)) {
                
                
                PRUnichar ch1 = aTX.GetContentCharAt(mContentOffset +
                                  indexp + 1 - aIndexBuffer->mBuffer);
                if (IS_ALEF(ch1)) {
                  *indexp++ = strInx;
                  --i;
                }
              }
            }
            *indexp++ = strInx++;
            
            if (caseChanged && ch == kSZLIG && *tp == PRUnichar('S')) {
              ++strInx;
              ++tp;
            }
            ++tp;
          }
        }
      }
    }

    
    if (textBuffer != nsnull && dstOffset + wordLen > textBuffer->mBufferLen) {
      nsresult rv = textBuffer->GrowBy(wordLen);
      if (NS_FAILED(rv)) {
        break;
      }
    }

    column += wordLen;
    textLength += wordLen;
    n -= contentLen;
    if (textBuffer != nsnull) {
      memcpy(textBuffer->mBuffer + dstOffset, bp,
             sizeof(PRUnichar)*wordLen);
    }
    dstOffset += wordLen;
  }

#ifdef DEBUG
  if (aIndexBuffer) {
    NS_ASSERTION(indexp <= aIndexBuffer->mBuffer + aIndexBuffer->mBufferLen,
                 "yikes - we just overwrote memory");
  }
  if (textBuffer) {
    NS_ASSERTION(dstOffset <= textBuffer->mBufferLen,
                 "yikes - we just overwrote memory");
  }

#endif

  
  
  
  if (TEXT_TRIMMED_WS & mState && textBuffer) {
    while (--dstOffset >= 0) {
      PRUnichar ch = textBuffer->mBuffer[dstOffset];
      if (XP_IS_SPACE(ch))
        textLength--;
      else
        break;
      if (!aRemoveMultipleTrimmedWS)
        break;
    }
  }

  if (aIndexBuffer) {
    PRInt32* ip = aIndexBuffer->mBuffer;
    
    for (PRInt32 i = mContentLength - 1; i >= 0; i--) {
      if (ip[i] > textLength + mContentOffset)
        ip[i] = textLength + mContentOffset;
      else
        break;
    }
    ip[mContentLength] = ip[mContentLength-1];
    if ((ip[mContentLength] - mContentOffset) < textLength) {
      
      ip[mContentLength] = textLength + mContentOffset;
    }
  }

  *aTextLen = textLength;

  if (aJustifiableCharCount && textBuffer) {
    PRBool isCJ = IsChineseJapaneseLangGroup();
    PRIntn numJustifiableCharacter = 0;
    PRInt32 justifiableRange = textLength;
    if (IsEndOfLine(mState))
      justifiableRange--;
    for (PRInt32 i = 0; i < justifiableRange; i++) {
      if (IsJustifiableCharacter(textBuffer->mBuffer[i], isCJ))
        numJustifiableCharacter++;
    }
    *aJustifiableCharCount = numJustifiableCharacter;
  }
}




#ifdef SHOW_SELECTION_CURSOR


#define CURSOR_COLOR NS_RGB(0,0,255)
static void
RenderSelectionCursor(nsIRenderingContext& aRenderingContext,
                      nscoord dx, nscoord dy, nscoord aHeight,
                      nscolor aCursorColor)
{
  nsPoint pnts[4];
  nscoord ox = aHeight / 4;
  nscoord oy = ox;
  nscoord x0 = dx;
  nscoord y0 = dy + aHeight;
  pnts[0].x = x0 - ox;
  pnts[0].y = y0;
  pnts[1].x = x0;
  pnts[1].y = y0 - oy;
  pnts[2].x = x0 + ox;
  pnts[2].y = y0;
  pnts[3].x = x0 - ox;
  pnts[3].y = y0;

  
  aRenderingContext.SetColor(aCursorColor);
  aRenderingContext.FillPolygon(pnts, 4);
}

#endif

void 
nsTextFrame::PaintTextDecorations(nsIRenderingContext& aRenderingContext,
                                  nsStyleContext* aStyleContext,
                                  nsPresContext* aPresContext,
                                  nsTextPaintStyle& aTextStyle,
                                  nscoord aX, nscoord aY, nscoord aWidth,
                                  PRBool aRightToLeftText,
                                  PRUnichar *aText, 
                                  SelectionDetails *aDetails,
                                  PRUint32 aIndex,  
                                  PRUint32 aLength, 
                                  const nscoord* aSpacing  )

{
  
  
  
  if (eCompatibility_NavQuirks == aPresContext->CompatibilityMode()) {
    nscolor overColor, underColor, strikeColor;
  
    PRBool useOverride = PR_FALSE;
    nscolor overrideColor;

    PRUint8 decorations = NS_STYLE_TEXT_DECORATION_NONE;
    
    PRUint8 decorMask = NS_STYLE_TEXT_DECORATION_UNDERLINE | 
                        NS_STYLE_TEXT_DECORATION_OVERLINE |
                        NS_STYLE_TEXT_DECORATION_LINE_THROUGH;    
    nsStyleContext* context = aStyleContext;
    PRBool hasDecorations = context->HasTextDecorations();

    while (hasDecorations) {
      const nsStyleTextReset* styleText = context->GetStyleTextReset();
      if (!useOverride && 
          (NS_STYLE_TEXT_DECORATION_OVERRIDE_ALL & 
           styleText->mTextDecoration)) {
        
        
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

    nscoord offset;
    nscoord size;
    nscoord baseline = mAscent;
    if (decorations & (NS_FONT_DECORATION_OVERLINE |
                       NS_FONT_DECORATION_UNDERLINE)) {
      aTextStyle.mNormalFont->GetUnderline(offset, size);
      if (decorations & NS_FONT_DECORATION_OVERLINE) {
        aRenderingContext.SetColor(overColor);
        aRenderingContext.FillRect(aX, aY, aWidth, size);
      }
      if (decorations & NS_FONT_DECORATION_UNDERLINE) {
        aRenderingContext.SetColor(underColor);
        aRenderingContext.FillRect(aX, aY + baseline - offset, aWidth, size);
      }
    }
    if (decorations & NS_FONT_DECORATION_LINE_THROUGH) {
      aTextStyle.mNormalFont->GetStrikeout(offset, size);
      aRenderingContext.SetColor(strikeColor);
      aRenderingContext.FillRect(aX, aY + baseline - offset, aWidth, size);
    }
  }

  if (aDetails){
    nsRect rect = GetRect();
    while(aDetails){
      const nscoord* sp= aSpacing;
      PRInt32 startOffset = 0;
      PRInt32 textWidth = 0;
      PRInt32 start = PR_MAX(0,(aDetails->mStart - (PRInt32)aIndex));
      PRInt32 end = PR_MIN((PRInt32)aLength,(aDetails->mEnd - (PRInt32)aIndex));
      PRInt32 i;
      if ((start < end) && ((aLength - start) > 0))
      {
        
        if (start < end){
          if (aLength == 1)
            textWidth = aWidth;
          else {
            if (aDetails->mStart > 0){
              if (sp)
              {
                for (i = 0; i < start;i ++){
                  startOffset += *sp ++;
                }
              }
              else
                aRenderingContext.GetWidth(aText, start, startOffset);
            }
            if (sp){
              for (i = start; i < end;i ++){
                textWidth += *sp ++;
              }
            }
            else
              aRenderingContext.GetWidth(aText + start,
                                           PRUint32(end - start), textWidth);
          }

          nscolor lineColor;
          float relativeSize;
          nscoord offset, size;
          nscoord baseline = mAscent;
          switch (aDetails->mType) {
            case nsISelectionController::SELECTION_NORMAL:
              break;
            case nsISelectionController::SELECTION_SPELLCHECK:
              aTextStyle.mNormalFont->GetUnderline(offset, size);
              aRenderingContext.SetLineStyle(nsLineStyle_kDotted);
              aRenderingContext.SetColor(NS_RGB(255,0,0));
              



              if (aRightToLeftText) {
                nscoord rightEdge = aX + aWidth;
                aRenderingContext.DrawLine(rightEdge - textWidth - startOffset,
                                           aY + baseline - offset,
                                           rightEdge - startOffset,
                                           aY + baseline - offset);
              }
              else {
                aRenderingContext.DrawLine(aX + startOffset,
                                           aY + baseline - offset,
                                           aX + startOffset + textWidth,
                                           aY + baseline - offset);
              }
              break;
            case nsISelectionController::SELECTION_IME_RAWINPUT:
            case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
            case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
            case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
              if (aTextStyle.GetIMEUnderline(aDetails->mType,
                                             &lineColor,
                                             &relativeSize)) {
                aTextStyle.mNormalFont->GetUnderline(offset, size);
                aRenderingContext.SetColor(lineColor);
                



                nscoord leftEdge = aRightToLeftText ?
                  aX + aWidth - startOffset - textWidth + size :
                  aX + startOffset + size;
                aRenderingContext.FillRect(leftEdge,
                                           aY + baseline - offset,
                                           textWidth - 2 * size,
                                           (nscoord)(relativeSize * size));
              }
              break;
            default:
              NS_ASSERTION(0,"what type of selection do i not know about?");
              break;
          }
        }
      }
      aDetails = aDetails->mNext;
    }
  }
}



nsresult
nsTextFrame::GetContentAndOffsetsForSelection(nsPresContext *aPresContext, nsIContent **aContent, PRInt32 *aOffset, PRInt32 *aLength)
{
  if (!aContent || !aOffset || !aLength)
    return NS_ERROR_NULL_POINTER;
  
  *aContent = nsnull;
  *aOffset = mContentOffset;
  *aLength = mContentLength;
  nsIFrame *parent = GetParent();
  if (parent)
  {
    if ((mState & NS_FRAME_GENERATED_CONTENT) != 0)
    {
      
      *aContent = parent->GetContent();
      if(!*aContent)
        return NS_ERROR_FAILURE;
      NS_ADDREF(*aContent);

      
      nsIFrame *grandParent = parent->GetParent();
      if (grandParent)
      {
        nsIFrame *firstParent = grandParent->GetFirstChild(nsnull);
        if (firstParent)
        {
          *aLength = 0;
          if (firstParent == parent) 
          {
            *aOffset = 0;
          }
          else
          {
            *aOffset = (*aContent)->GetChildCount();
          }
        }
        else
          return NS_OK;
      }
    }
  }
  
  if (!*aContent)
  {
    *aContent = mContent;
    NS_IF_ADDREF(*aContent);
  }

  return NS_OK;
}


nsresult nsTextFrame::GetTextInfoForPainting(nsPresContext*           aPresContext,
                                             nsIPresShell**           aPresShell,
                                             nsISelectionController** aSelectionController,
                                             PRBool&                  aDisplayingSelection,
                                             PRBool&                  aIsPaginated,
                                             PRBool&                  aIsSelected,
                                             PRBool&                  aHideStandardSelection,
                                             PRInt16&                 aSelectionValue)
{
  NS_ENSURE_ARG_POINTER(aPresContext);
  NS_ENSURE_ARG_POINTER(aPresShell);
  NS_ENSURE_ARG_POINTER(aSelectionController);

  
  NS_IF_ADDREF(*aPresShell = aPresContext->GetPresShell());
  if (!*aPresShell)
    return NS_ERROR_FAILURE;

  
  nsresult rv = GetSelectionController(aPresContext, aSelectionController);
  if (NS_FAILED(rv) || !(*aSelectionController))
    return NS_ERROR_FAILURE;

  (*aSelectionController)->GetDisplaySelection(&aSelectionValue);

  if (aPresContext->IsRenderingOnlySelection()) {
    aIsPaginated = PR_TRUE;
    aDisplayingSelection = PR_TRUE;
  } else {
    aIsPaginated = PR_FALSE;
    aDisplayingSelection =
      (aSelectionValue > nsISelectionController::SELECTION_HIDDEN);
  }

  PRInt16 textSel=0; 
  (*aSelectionController)->GetSelectionFlags(&textSel);
  if (!(textSel & nsISelectionDisplay::DISPLAY_TEXT))
    aDisplayingSelection = PR_FALSE;

  
  aHideStandardSelection = !aDisplayingSelection;
  if (!aDisplayingSelection){
    nsCOMPtr<nsISelection> spellcheckSelection;
    (*aSelectionController)->GetSelection(nsISelectionController::SELECTION_SPELLCHECK,
                                          getter_AddRefs(spellcheckSelection));
    if (spellcheckSelection){
      PRBool iscollapsed = PR_FALSE;
      spellcheckSelection->GetIsCollapsed(&iscollapsed);
      if (!iscollapsed)
        aDisplayingSelection = PR_TRUE;
    }
  }

  
  
  
  
  
  nsIDocument *doc = (*aPresShell)->GetDocument();
  if (!doc)
    return NS_ERROR_FAILURE;

  aIsSelected = (GetStateBits() & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;

  return NS_OK;
}

nsresult
nsTextFrame::GetSelectionStatus(nsPresContext* aPresContext,
                                PRInt16&       aSelectionValue)
{
  NS_ENSURE_ARG_POINTER(aPresContext);

  
  nsCOMPtr<nsISelectionController> selectionController;
  nsresult rv = GetSelectionController(aPresContext,
                                       getter_AddRefs(selectionController));
  if (NS_FAILED(rv) || !selectionController)
    return NS_ERROR_FAILURE;

  selectionController->GetDisplaySelection(&aSelectionValue);

  return NS_OK;
}

PRBool
nsTextFrame::IsTextInSelection()
{
  nsCOMPtr<nsISelectionController> selCon;
  nsCOMPtr<nsIPresShell> shell;
  PRBool  displaySelection;
  PRBool  isPaginated;
  PRBool  isSelected;
  PRBool  hideStandardSelection;
  PRInt16 selectionValue;
  nsPresContext* presContext = PresContext();
  if (NS_FAILED(GetTextInfoForPainting(presContext, 
                                       getter_AddRefs(shell),
                                       getter_AddRefs(selCon),
                                       displaySelection,
                                       isPaginated,
                                       isSelected,
                                       hideStandardSelection,
                                       selectionValue))) {
    return PR_FALSE;
  }

  
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
    return PR_FALSE;
  }

  
  
  
  
  

  nsTextTransformer tx(presContext);
  PRInt32 textLength;
  
  PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

  PRInt32* ip     = indexBuffer.mBuffer;
  PRUnichar* text = paintBuffer.mBuffer;

  isSelected = PR_FALSE;
  if (0 != textLength) {

    SelectionDetails *details = nsnull;

    nsCOMPtr<nsIContent> content;
    PRInt32 offset;
    PRInt32 length;

    nsresult rv = GetContentAndOffsetsForSelection(presContext,
                                                   getter_AddRefs(content),
                                                   &offset, &length);
    if (NS_SUCCEEDED(rv) && content) {
      details = GetFrameSelection()->LookUpSelection(content, mContentOffset,
                                                     mContentLength, PR_FALSE);
    }
      
    
    SelectionDetails *sdptr = details;
    while (sdptr){
      sdptr->mStart = ip[sdptr->mStart] - mContentOffset;
      sdptr->mEnd = ip[sdptr->mEnd]  - mContentOffset;
      sdptr = sdptr->mNext;
    }
    
    
    DrawSelectionIterator iter(details, text, (PRUint32)textLength, nsnull,
                               nsTextPaintStyle::eNormalSelection);
    if (!iter.IsDone() && iter.First()) {
      isSelected = PR_TRUE;
    }

    sdptr = details;
    if (details) {
      while ((sdptr = details->mNext) != nsnull) {
        delete details;
        details = sdptr;
      }
      delete details;
    }
  }
  return isSelected;
}

PRBool
nsTextFrame::IsVisibleInSelection(nsISelection* aSelection)
{
  
  PRBool isSelected = (mState & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT;
  if (!isSelected)
    return PR_FALSE;
    
  return IsTextInSelection();
}

void
nsTextFrame::PaintUnicodeText(nsPresContext* aPresContext,
                              nsIRenderingContext& aRenderingContext,
                              nsStyleContext* aStyleContext,
                              nsTextPaintStyle& aTextStyle,
                              nscoord dx, nscoord dy)
{
  nsCOMPtr<nsISelectionController> selCon;
  nsCOMPtr<nsIPresShell> shell;
  PRBool  displaySelection,canDarkenColor=PR_FALSE;
  PRBool  isPaginated;
  PRBool  isSelected;
  PRBool hideStandardSelection;
  PRInt16 selectionValue;
#ifdef IBMBIDI
  PRBool  isOddLevel = PR_FALSE;
#endif

  if (NS_FAILED(GetTextInfoForPainting(aPresContext, 
                                       getter_AddRefs(shell),
                                       getter_AddRefs(selCon),
                                       displaySelection,
                                       isPaginated,
                                       isSelected,
                                       hideStandardSelection,
                                       selectionValue))) {
     return;
  }

  if(isPaginated){
    canDarkenColor = CanDarken(aPresContext);
  }

  
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (displaySelection) {
    if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
      return;
    }
  }
  nscoord width = mRect.width;

  
  
  
  
  

  nsTextTransformer tx(aPresContext);
  PRInt32 textLength;

  
  
  
  PRBool removeMultipleTrimmedWS = NS_STYLE_WHITESPACE_MOZ_PRE_WRAP == GetStyleText()->mWhiteSpace;

  
  PrepareUnicodeText(tx, (displaySelection ? &indexBuffer : nsnull),
                     &paintBuffer, &textLength, PR_FALSE, nsnull, removeMultipleTrimmedWS);

  PRInt32* ip = indexBuffer.mBuffer;
  PRUnichar* text = paintBuffer.mBuffer;

  if (0 != textLength) 
  {
#ifdef IBMBIDI
    PRBool isRightToLeftOnBidiPlatform = PR_FALSE;
    PRBool isBidiSystem = PR_FALSE;
    nsCharType charType = eCharType_LeftToRight;
    if (aPresContext->BidiEnabled()) {
      isBidiSystem = aPresContext->IsBidiSystem();
      isOddLevel = NS_GET_EMBEDDING_LEVEL(this) & 1;
      charType = (nsCharType)NS_PTR_TO_INT32(aPresContext->PropertyTable()->GetProperty(this, nsGkAtoms::charType));

      isRightToLeftOnBidiPlatform = (isBidiSystem &&
                                     (eCharType_RightToLeft == charType ||
                                      eCharType_RightToLeftArabic == charType));
      if (isRightToLeftOnBidiPlatform) {
        
        
        
        aRenderingContext.SetRightToLeftText(PR_TRUE);
      }
      nsBidiPresUtils* bidiUtils = aPresContext->GetBidiUtils();
      if (bidiUtils) {
#ifdef DEBUG
        PRInt32 rememberTextLength = textLength;
#endif
        PRUint32 hints = 0;
        aRenderingContext.GetHints(hints);
        bidiUtils->ReorderUnicodeText(text, textLength,
                                      charType, isOddLevel, isBidiSystem,
                                      (hints & NS_RENDERING_HINT_NEW_TEXT_RUNS) != 0);
        NS_ASSERTION(rememberTextLength == textLength, "Bidi formatting changed text length");
      }
    }
#endif 
    if (!displaySelection || !isSelected ) 
    { 
      
      

      aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
      aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
      PaintTextDecorations(aRenderingContext, aStyleContext, aPresContext,
                           aTextStyle, dx, dy, width, PR_FALSE);
    }
    else 
    { 
      SelectionDetails *details = nsnull;
      nsCOMPtr<nsIContent> content;
      PRInt32 offset;
      PRInt32 length;
      nsresult rv = GetContentAndOffsetsForSelection(aPresContext,
                                                     getter_AddRefs(content),
                                                     &offset, &length);
      if (NS_SUCCEEDED(rv) && content) {
        details = GetFrameSelection()->LookUpSelection(content, mContentOffset,
                                                       mContentLength, PR_FALSE);
      }
        
      
      SelectionDetails *sdptr = details;
      while (sdptr){
        sdptr->mStart = ip[sdptr->mStart] - mContentOffset;
        sdptr->mEnd = ip[sdptr->mEnd]  - mContentOffset;
#ifdef SUNCTL
        nsCOMPtr<nsILE> ctlObj;
        ctlObj = do_CreateInstance(kLECID, &rv);
        if (NS_FAILED(rv)) {
          NS_WARNING("Cell based cursor movement will not be supported\n");
          ctlObj = nsnull;
        }
        else {
          PRInt32 start, end;
          PRBool  needsCTL = PR_FALSE;

          ctlObj->NeedsCTLFix(text, sdptr->mStart, sdptr->mEnd, &needsCTL);

          if (needsCTL && (sdptr->mEnd < textLength)) {
            ctlObj->GetRangeOfCluster(text, PRInt32(textLength), sdptr->mEnd,
                                      &start, &end);
            if (sdptr->mStart > sdptr->mEnd) 
              sdptr->mEnd = start;
            else
              sdptr->mEnd = end;
          }

          
          if (needsCTL && (sdptr->mStart > 0)) {
            ctlObj->GetRangeOfCluster(text, PRInt32(textLength),
                                      sdptr->mStart, &start, &end);
            sdptr->mStart = end;
          }
        }
#endif 
#ifdef IBMBIDI
        AdjustSelectionPointsForBidi(sdptr, textLength, CHARTYPE_IS_RTL(charType), isOddLevel, isBidiSystem);
#endif
        sdptr = sdptr->mNext;
      }
      if (!hideStandardSelection || displaySelection) {
      










      
      
      PRUint32 clusterHint = 0;
      aRenderingContext.GetHints(clusterHint);
      clusterHint &= NS_RENDERING_HINT_TEXT_CLUSTERS;

      
      
      DrawSelectionIterator iter(details, text, (PRUint32)textLength, &aTextStyle,
                                 nsTextPaintStyle::eAllSelections);
      if (!iter.IsDone() && iter.First())
      {
        nscoord currentX = dx;
        nscoord newWidth;
#ifdef IBMBIDI 
        nscoord FrameWidth = 0;
        if (isRightToLeftOnBidiPlatform)
          if (NS_SUCCEEDED(aRenderingContext.GetWidth(text, textLength, FrameWidth)))
            currentX = dx + FrameWidth;
#endif
        while (!iter.IsDone())
        {
          PRUnichar *currenttext  = iter.CurrentTextUnicharPtr();
          PRUint32   currentlength= iter.CurrentLength();
          nscolor    currentFGColor, currentBKColor;
          PRBool     isCurrentBKColorTransparent;

          PRBool     isSelection = iter.GetSelectionColors(&currentFGColor,
                                                           &currentBKColor,
                                                           &isCurrentBKColorTransparent);

          if (currentlength > 0)
          {
            if (clusterHint) {
              PRUint32 tmpWidth;
              rv = aRenderingContext.GetRangeWidth(text, textLength, currenttext - text,
                                                   (currenttext - text) + currentlength,
                                                   tmpWidth);
              newWidth = nscoord(tmpWidth);
            }
            else {
              rv = aRenderingContext.GetWidth(currenttext, currentlength,newWidth); 
            }
            if (NS_SUCCEEDED(rv)) {
              if (isRightToLeftOnBidiPlatform)
                currentX -= newWidth;
              if (isSelection && !isPaginated)
              {
                if (!isCurrentBKColorTransparent) {
                  aRenderingContext.SetColor(currentBKColor);
                  aRenderingContext.FillRect(currentX, dy, newWidth, mRect.height);
                }
             }
            }
            else {
              newWidth = 0;
            }
          }
          else {
            newWidth = 0;
          }

          aRenderingContext.PushState();

          nsRect rect(currentX, dy, newWidth, mRect.height);
          aRenderingContext.SetClipRect(rect, nsClipCombine_kIntersect);
                      
          if (isPaginated && !iter.IsBeforeOrAfter()) {
            aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
            aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
          } else if (!isPaginated) {
            aRenderingContext.SetColor(nsCSSRendering::TransformColor(currentFGColor,canDarkenColor));
            aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
          }

          aRenderingContext.PopState();

#ifdef IBMBIDI
          if (!isRightToLeftOnBidiPlatform)
#endif
          currentX += newWidth; 

          iter.Next();
        }
      }
      else if (!isPaginated)
      {
        aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
        aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
      }
      }
      PaintTextDecorations(aRenderingContext, aStyleContext, aPresContext,
                           aTextStyle, dx, dy, width,
                           isRightToLeftOnBidiPlatform, text, details, 0,
                           (PRUint32)textLength, nsnull);
      sdptr = details;
      if (details){
        while ((sdptr = details->mNext) != nsnull) {
          delete details;
          details = sdptr;
        }
        delete details;
      }
    }
#ifdef IBMBIDI
    if (isRightToLeftOnBidiPlatform) {
      
      
      aRenderingContext.SetRightToLeftText(PR_FALSE);
    }
#endif 
  }
}


nsresult
nsTextFrame::GetPositionSlowly(nsIRenderingContext* aRendContext,
                               const nsPoint& aPoint,
                               nsIContent** aNewContent,
                               PRInt32& aOffset)

{
  
  NS_PRECONDITION(aRendContext && aNewContent, "null arg");
  if (!aRendContext || !aNewContent) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aNewContent = nsnull;

  nsPresContext* presContext = PresContext();
  nsTextStyle ts(presContext, *aRendContext, mStyleContext);
  SetupTextRunDirection(presContext, aRendContext);
  if (!ts.mSmallCaps && !ts.mWordSpacing && !ts.mLetterSpacing && !ts.mJustifying) {
    return NS_ERROR_INVALID_ARG;
  }

  















  if (aPoint.x < 0)
  {
      *aNewContent = mContent;
      aOffset =0;
  }

  
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  nsresult rv = indexBuffer.GrowTo(mContentLength + 1);
  if (NS_FAILED(rv)) {
    
    
    *aNewContent = nsnull;
    return rv;
  }

  
  nsTextTransformer tx(PresContext());
  PRInt32 textLength;
  PRIntn numJustifiableCharacter;

  PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength, PR_TRUE, &numJustifiableCharacter);
  if (textLength <= 0) {
    
    
    
    
    
    *aNewContent = nsnull;
    return NS_ERROR_FAILURE;
  }

#ifdef IBMBIDI 
  PRBool isOddLevel = NS_GET_EMBEDDING_LEVEL(this) & 1;
  if (isOddLevel) {
    PRUnichar *tStart, *tEnd;
    PRUnichar tSwap;
    for (tStart = paintBuffer.mBuffer, tEnd = tStart + textLength - 1; tEnd > tStart; tStart++, tEnd--) {
      tSwap = *tStart;
      *tStart = *tEnd;
      *tEnd = tSwap;
    }
  }
#endif 

  ComputeExtraJustificationSpacing(*aRendContext, ts, paintBuffer.mBuffer, textLength, numJustifiableCharacter);
  {
    
    nscoord adjustedX = PR_MAX(0,aPoint.x);

#ifdef IBMBIDI
    if (isOddLevel)
      aOffset = mContentOffset + textLength -
                GetLengthSlowly(*aRendContext, ts, paintBuffer.mBuffer,
                                textLength, PR_TRUE, adjustedX);
    else
#endif
    aOffset = mContentOffset +
              GetLengthSlowly(*aRendContext, ts,paintBuffer.mBuffer,
                              textLength, PR_TRUE, adjustedX);
    PRInt32 i;
    PRInt32* ip = indexBuffer.mBuffer;
    for (i = 0;i <= mContentLength; i ++){
      if ((ip[i] >= aOffset) && 
          (! NS_IS_LOW_SURROGATE(paintBuffer.mBuffer[ip[i]-mContentOffset]))) {
          aOffset = i + mContentOffset;
          break;
      }
    }
  }

  *aNewContent = mContent;
  if (*aNewContent)
    (*aNewContent)->AddRef();
  return NS_OK;
}

void
nsTextFrame::RenderString(nsIRenderingContext& aRenderingContext,
                          nsStyleContext* aStyleContext,
                          nsPresContext* aPresContext,
                          nsTextPaintStyle& aTextStyle,
                          PRBool aRightToLeftText,
                          PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                          nscoord aX, nscoord aY,
                          nscoord aWidth, 
                          SelectionDetails *aDetails )
{
  PRUnichar buf[TEXT_BUF_SIZE];
  PRUnichar* bp0 = buf;

  nscoord spacingMem[TEXT_BUF_SIZE];
  nscoord* sp0 = spacingMem; 
  
  PRBool spacing = (0 != aTextStyle.mLetterSpacing) ||
    (0 != aTextStyle.mWordSpacing) || aTextStyle.mJustifying;

  PRBool justifying = aTextStyle.mJustifying &&
    (aTextStyle.mNumJustifiableCharacterReceivingExtraJot != 0 || aTextStyle.mExtraSpacePerJustifiableCharacter != 0);

  PRBool isCJ = IsChineseJapaneseLangGroup();
  PRBool isEndOfLine = aIsEndOfFrame && IsEndOfLine(mState);

  
  if (aTextStyle.mSmallCaps) {
     if (aLength*2 > TEXT_BUF_SIZE) {
       bp0 = new PRUnichar[aLength*2];
       if (spacing)
         sp0 = new nscoord[aLength*2];
     }
  }
  else if (aLength > TEXT_BUF_SIZE) {
    bp0 = new PRUnichar[aLength];
    if (spacing)
      sp0 = new nscoord[aLength];
  }

  PRUnichar* bp = bp0;
  nscoord* sp = sp0;

  nsIFontMetrics* lastFont = aTextStyle.mLastFont;
  PRInt32 pendingCount;
  PRUnichar* runStart = bp;
  nscoord charWidth, width = 0;
  PRInt32 countSoFar = 0;
  
  
  
  nscolor textColor;
  aRenderingContext.GetColor(textColor);
  for (; --aLength >= 0; aBuffer++) {
    nsIFontMetrics* nextFont;
    nscoord glyphWidth = 0;
    PRUnichar ch = *aBuffer;
    if (aTextStyle.mSmallCaps &&
        (IsLowerCase(ch) || (ch == kSZLIG))) {
      nextFont = aTextStyle.mSmallFont;
    }
    else {
      nextFont = aTextStyle.mNormalFont;
    }
    if (nextFont != lastFont) {
      pendingCount = bp - runStart;
      if (0 != pendingCount) {
        
        aRenderingContext.SetColor(textColor);
        
        aRenderingContext.DrawString(runStart, pendingCount,
                                     aX, aY + mAscent, -1,
                                     spacing ? sp0 : nsnull);

        
        
        PaintTextDecorations(aRenderingContext, aStyleContext, aPresContext,
                             aTextStyle, aX, aY, width,
                             aRightToLeftText, runStart, aDetails,
                             countSoFar, pendingCount, spacing ? sp0 : nsnull);
        countSoFar += pendingCount;
        aWidth -= width;
        aX += width;
        runStart = bp = bp0;
        sp = sp0;
        width = 0;
      }
      aRenderingContext.SetFont(nextFont);
      lastFont = nextFont;
    }
    if (nextFont == aTextStyle.mSmallFont) {
      PRUnichar upper_ch;
      
      if (ch == kSZLIG)
        upper_ch = (PRUnichar)'S';
      else
        upper_ch = ToUpperCase(ch);
      aRenderingContext.GetWidth(upper_ch, charWidth);
      glyphWidth += charWidth + aTextStyle.mLetterSpacing;
      if (ch == kSZLIG)   
      {
        *bp++ = upper_ch;
        if (spacing)
          *sp++ = glyphWidth;
        width += glyphWidth;
      }
      ch = upper_ch;
    }
    else if (ch == ' ') {
      glyphWidth += aTextStyle.mSpaceWidth + aTextStyle.mWordSpacing + aTextStyle.mLetterSpacing;
    }
    else if (NS_IS_HIGH_SURROGATE(ch) && aLength > 0 &&
           NS_IS_LOW_SURROGATE(*(aBuffer+1))) {
      
      
      aRenderingContext.GetWidth(aBuffer, 2, charWidth);
      glyphWidth += charWidth + aTextStyle.mLetterSpacing;
      
      *bp++ = ch;
      --aLength;
      aBuffer++;
      ch = *aBuffer;
      
      width += glyphWidth;
      if (spacing)
        *sp++ = glyphWidth;
      
      
      glyphWidth = 0;
    }
    else {
      aRenderingContext.GetWidth(ch, charWidth);
      glyphWidth += charWidth + aTextStyle.mLetterSpacing;
    }
    if (justifying && (!isEndOfLine || aLength > 0)
        && IsJustifiableCharacter(ch, isCJ)) {
      glyphWidth += aTextStyle.mExtraSpacePerJustifiableCharacter;
      if ((PRUint32)--aTextStyle.mNumJustifiableCharacterToRender
            < (PRUint32)aTextStyle.mNumJustifiableCharacterReceivingExtraJot) {
        glyphWidth++;
      }
    }
    *bp++ = ch;
    if (spacing)
      *sp++ = glyphWidth;
    width += glyphWidth;
  }
  pendingCount = bp - runStart;
  if (0 != pendingCount) {
    
    aRenderingContext.SetColor(textColor);
    
    aRenderingContext.DrawString(runStart, pendingCount, aX, aY + mAscent, -1,
                                 spacing ? sp0 : nsnull);

    
    
    PaintTextDecorations(aRenderingContext, aStyleContext, aPresContext,
                         aTextStyle, aX, aY, aWidth,
                         aRightToLeftText, runStart, aDetails,
                         countSoFar, pendingCount, spacing ? sp0 : nsnull);
  }
  aTextStyle.mLastFont = lastFont;

  if (bp0 != buf) {
    delete [] bp0;
  }
  if (sp0 != spacingMem) {
    delete [] sp0;
  }
}

inline void
nsTextFrame::MeasureSmallCapsText(nsIRenderingContext* aRenderingContext,
                                  nsTextStyle& aTextStyle,
                                  PRUnichar* aWord,
                                  PRInt32 aWordLength,
                                  PRBool aIsEndOfFrame,
                                  nsTextDimensions* aDimensionsResult)
{
  aDimensionsResult->Clear();
  GetTextDimensions(*aRenderingContext, aTextStyle, aWord, aWordLength,
                    aIsEndOfFrame, aDimensionsResult);
  if (aTextStyle.mLastFont != aTextStyle.mNormalFont) {
    aRenderingContext->SetFont(aTextStyle.mNormalFont);
    aTextStyle.mLastFont = aTextStyle.mNormalFont;
  }
}


PRInt32
nsTextFrame::GetTextDimensionsOrLength(nsIRenderingContext& aRenderingContext,
                nsTextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                nsTextDimensions* aDimensionsResult,
                PRBool aGetTextDimensions)
{
  PRUnichar *inBuffer = aBuffer;
  PRInt32 length = aLength;
  nsAutoTextBuffer dimensionsBuffer;
  if (NS_FAILED(dimensionsBuffer.GrowTo(length))) {
    aDimensionsResult->Clear();
    return 0;
  }
  PRUnichar* bp = dimensionsBuffer.mBuffer;

  nsIFontMetrics* lastFont = aStyle.mLastFont;
  nsTextDimensions sum, glyphDimensions;
  PRBool justifying = aStyle.mJustifying &&
    (aStyle.mNumJustifiableCharacterReceivingExtraJot != 0 || aStyle.mExtraSpacePerJustifiableCharacter != 0);
  PRBool isCJ = IsChineseJapaneseLangGroup();
  PRBool isEndOfLine = aIsEndOfFrame && IsEndOfLine(mState);

  for (PRInt32 prevLength = length; --length >= 0; prevLength = length) {
    PRUnichar ch = *inBuffer++;
    if (aStyle.mSmallCaps &&
        (IsLowerCase(ch) || (ch == kSZLIG))) {
      PRUnichar upper_ch;
      
      if (ch == kSZLIG)
        upper_ch = (PRUnichar)'S';
      else
        upper_ch = ToUpperCase(ch);
      if (lastFont != aStyle.mSmallFont) {
        lastFont = aStyle.mSmallFont;
        aRenderingContext.SetFont(lastFont);
      }
      aRenderingContext.GetTextDimensions(&upper_ch, (PRUint32)1, glyphDimensions);
      glyphDimensions.width += aStyle.mLetterSpacing;
      if (ch == kSZLIG)
        glyphDimensions.width += glyphDimensions.width;
    }
    else if (ch == ' ' || ch == CH_NBSP) {
      glyphDimensions.width = aStyle.mSpaceWidth + aStyle.mLetterSpacing
        + aStyle.mWordSpacing;
    }
    else {
      if (lastFont != aStyle.mNormalFont) {
        lastFont = aStyle.mNormalFont;
        aRenderingContext.SetFont(lastFont);
      }
      if (NS_IS_HIGH_SURROGATE(ch) && length > 0 &&
        NS_IS_LOW_SURROGATE(*inBuffer)) {
        aRenderingContext.GetTextDimensions(inBuffer-1, (PRUint32)2, glyphDimensions);
        length--;
        inBuffer++;
      } else {
        aRenderingContext.GetTextDimensions(&ch, (PRUint32)1, glyphDimensions);
      }
      glyphDimensions.width += aStyle.mLetterSpacing;
    }
    if (justifying && (!isEndOfLine || length > 0)
        && IsJustifiableCharacter(ch, isCJ)) {
      glyphDimensions.width += aStyle.mExtraSpacePerJustifiableCharacter;
      if ((PRUint32)--aStyle.mNumJustifiableCharacterToMeasure
            < (PRUint32)aStyle.mNumJustifiableCharacterReceivingExtraJot) {
        ++glyphDimensions.width;
      }
    }
    sum.Combine(glyphDimensions);
    *bp++ = ch;
    if (!aGetTextDimensions && sum.width >= aDimensionsResult->width) {
      PRInt32 result = aLength - length;
      if (2*(sum.width - aDimensionsResult->width) > glyphDimensions.width) 
        result = aLength - prevLength;
      aStyle.mLastFont = lastFont;
      return result;
    }
  }
  aStyle.mLastFont = lastFont;
  *aDimensionsResult = sum;
  return aLength;
}



void
nsTextFrame::GetTextDimensions(nsIRenderingContext& aRenderingContext,
                      nsTextStyle& aTextStyle,
                      PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                      nsTextDimensions* aDimensionsResult)
{
  GetTextDimensionsOrLength(aRenderingContext,aTextStyle,
                            aBuffer,aLength,aIsEndOfFrame,aDimensionsResult,PR_TRUE);
}

PRInt32 
nsTextFrame::GetLengthSlowly(nsIRenderingContext& aRenderingContext,
                nsTextStyle& aStyle,
                PRUnichar* aBuffer, PRInt32 aLength, PRBool aIsEndOfFrame,
                nscoord aWidth)
{
  nsTextDimensions dimensions;
  dimensions.width = aWidth;
  return GetTextDimensionsOrLength(aRenderingContext,aStyle,
                                   aBuffer,aLength,aIsEndOfFrame,&dimensions,PR_FALSE);
}

void
nsTextFrame::ComputeExtraJustificationSpacing(nsIRenderingContext& aRenderingContext,
                                              nsTextStyle& aTextStyle,
                                              PRUnichar* aBuffer, PRInt32 aLength,
                                              PRInt32 aNumJustifiableCharacter)
{
  if (aTextStyle.mJustifying) {
    nsTextDimensions trueDimensions;
    
    
    
    
    
    
    
    
    
    
    
    
    
    aTextStyle.mNumJustifiableCharacterToMeasure = 0;
    aTextStyle.mExtraSpacePerJustifiableCharacter = 0;
    aTextStyle.mNumJustifiableCharacterReceivingExtraJot = 0;
    
    GetTextDimensions(aRenderingContext, aTextStyle, aBuffer, aLength, PR_TRUE, &trueDimensions);

    aTextStyle.mNumJustifiableCharacterToMeasure = aNumJustifiableCharacter;
    aTextStyle.mNumJustifiableCharacterToRender = aNumJustifiableCharacter;

    nscoord extraSpace = mRect.width - trueDimensions.width;

    if (extraSpace > 0 && aNumJustifiableCharacter > 0) {
      aTextStyle.mExtraSpacePerJustifiableCharacter = extraSpace/aNumJustifiableCharacter;
      aTextStyle.mNumJustifiableCharacterReceivingExtraJot =
        extraSpace - aTextStyle.mExtraSpacePerJustifiableCharacter*aNumJustifiableCharacter;
    }
  }
}

void
nsTextFrame::PaintTextSlowly(nsPresContext* aPresContext,
                             nsIRenderingContext& aRenderingContext,
                             nsStyleContext* aStyleContext,
                             nsTextPaintStyle& aTextStyle,
                             nscoord dx, nscoord dy)
{
  nsCOMPtr<nsISelectionController> selCon;
  nsCOMPtr<nsIPresShell> shell;
  PRBool  displaySelection;
  PRBool  isPaginated,canDarkenColor=PR_FALSE;
  PRBool  isSelected;
  PRBool  hideStandardSelection;
  PRInt16 selectionValue;
  if (NS_FAILED(GetTextInfoForPainting(aPresContext, 
                                       getter_AddRefs(shell),
                                       getter_AddRefs(selCon),
                                       displaySelection,
                                       isPaginated,
                                       isSelected,
                                       hideStandardSelection,
                                       selectionValue))) {
     return;
  }


  if(isPaginated){
    canDarkenColor = CanDarken(aPresContext);
  }

  
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
    return;
  }
  nscoord width = mRect.width;
  PRInt32 textLength;

  nsTextTransformer tx(aPresContext);
  PRIntn numJustifiableCharacter;
  
  PrepareUnicodeText(tx, (displaySelection ? &indexBuffer : nsnull),
                     &paintBuffer, &textLength, PR_TRUE, &numJustifiableCharacter);

  PRInt32* ip = indexBuffer.mBuffer;
  PRUnichar* text = paintBuffer.mBuffer;

  if (0 != textLength) {
#ifdef IBMBIDI
    PRBool isRightToLeftOnBidiPlatform = PR_FALSE;
    PRBool isBidiSystem = PR_FALSE;
    PRBool isOddLevel = PR_FALSE;
    PRUint32 hints = 0;
    aRenderingContext.GetHints(hints);
    PRBool paintCharByChar = (0 == (hints & NS_RENDERING_HINT_REORDER_SPACED_TEXT)) &&
      ((0 != aTextStyle.mLetterSpacing) ||
       (0 != aTextStyle.mWordSpacing) ||
       aTextStyle.mJustifying);
    nsCharType charType = eCharType_LeftToRight;

    if (aPresContext->BidiEnabled()) {
      isBidiSystem = aPresContext->IsBidiSystem();
      nsBidiPresUtils* bidiUtils = aPresContext->GetBidiUtils();

      if (bidiUtils) {
        isOddLevel = NS_GET_EMBEDDING_LEVEL(this) & 1;
        charType = (nsCharType)NS_PTR_TO_INT32(aPresContext->PropertyTable()->GetProperty(this, nsGkAtoms::charType));
#ifdef DEBUG
        PRInt32 rememberTextLength = textLength;
#endif
        isRightToLeftOnBidiPlatform = (!paintCharByChar &&
                                       isBidiSystem &&
                                       (eCharType_RightToLeft == charType ||
                                        eCharType_RightToLeftArabic == charType));
        if (isRightToLeftOnBidiPlatform) {
          
          
          
          aRenderingContext.SetRightToLeftText(PR_TRUE);
        }
        PRUint32 hints = 0;
        aRenderingContext.GetHints(hints);
        
        bidiUtils->ReorderUnicodeText(text, textLength, charType,
                                      isOddLevel, (paintCharByChar) ? PR_FALSE : isBidiSystem,
                                      (hints & NS_RENDERING_HINT_NEW_TEXT_RUNS) != 0);
        NS_ASSERTION(rememberTextLength == textLength, "Bidi formatting changed text length");
      }
    }
#endif 
    ComputeExtraJustificationSpacing(aRenderingContext, aTextStyle, text, textLength, numJustifiableCharacter);
    if (!displaySelection || !isSelected) { 
      
      
      aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
      RenderString(aRenderingContext, aStyleContext, aPresContext, aTextStyle,
                   PR_FALSE, text, textLength, PR_TRUE, dx, dy, width);
    }
    else 
    {
      SelectionDetails *details = nsnull;
      nsCOMPtr<nsIContent> content;
      PRInt32 offset;
      PRInt32 length;
      nsresult rv = GetContentAndOffsetsForSelection(aPresContext,
                                                     getter_AddRefs(content),
                                                     &offset, &length);
      if (NS_SUCCEEDED(rv)) {
        details = GetFrameSelection()->LookUpSelection(content, mContentOffset,
                                                       mContentLength, PR_FALSE);
      }

      
      SelectionDetails *sdptr = details;
      while (sdptr){
        sdptr->mStart = ip[sdptr->mStart] - mContentOffset;
        sdptr->mEnd = ip[sdptr->mEnd]  - mContentOffset;
#ifdef IBMBIDI
        AdjustSelectionPointsForBidi(sdptr, textLength,
                                     CHARTYPE_IS_RTL(charType), isOddLevel,
                                     (paintCharByChar) ? PR_FALSE : isBidiSystem);
#endif
        sdptr = sdptr->mNext;
      }

      DrawSelectionIterator iter(details, text, (PRUint32)textLength, &aTextStyle,
                                 nsTextPaintStyle::eAllSelections);
      if (!iter.IsDone() && iter.First())
      {
        nscoord currentX = dx;
        nsTextDimensions newDimensions;
#ifdef IBMBIDI 
        if (isRightToLeftOnBidiPlatform)
          currentX = dx + mRect.width;
#endif
        while (!iter.IsDone())
        {
          PRUnichar *currenttext  = iter.CurrentTextUnicharPtr();
          PRUint32   currentlength= iter.CurrentLength();
          nscolor    currentFGColor, currentBKColor;
          PRBool     isCurrentBKColorTransparent;
          PRBool     isSelection = iter.GetSelectionColors(&currentFGColor,
                                                           &currentBKColor,
                                                           &isCurrentBKColorTransparent);
          PRBool     isEndOfFrame = iter.IsLast();
          GetTextDimensions(aRenderingContext, aTextStyle, currenttext,
                            (PRInt32)currentlength, isEndOfFrame, &newDimensions);
          if (newDimensions.width)
          {
#ifdef IBMBIDI
            if (isRightToLeftOnBidiPlatform)
              currentX -= newDimensions.width;
#endif
            if (isSelection && !isPaginated)
            {
              if (!isCurrentBKColorTransparent) {
                aRenderingContext.SetColor(currentBKColor);
                aRenderingContext.FillRect(currentX, dy, newDimensions.width, mRect.height);
              }
            }
          }

          if (isPaginated && !iter.IsBeforeOrAfter()) {
            aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor, canDarkenColor));
            RenderString(aRenderingContext, aStyleContext, aPresContext,
                         aTextStyle, isRightToLeftOnBidiPlatform, 
                         currenttext, currentlength, isEndOfFrame,
                         currentX, dy, newDimensions.width, details);
          } else if (!isPaginated) {
            aRenderingContext.SetColor(nsCSSRendering::TransformColor(currentFGColor, canDarkenColor));
            RenderString(aRenderingContext,aStyleContext, aPresContext,
                         aTextStyle, isRightToLeftOnBidiPlatform, 
                         currenttext, currentlength, isEndOfFrame,
                         currentX, dy, newDimensions.width, details);
          }

#ifdef IBMBIDI
          if (!isRightToLeftOnBidiPlatform)
#endif
          
          
          currentX += newDimensions.width; 

          iter.Next();
        }
      }
      else if (!isPaginated) 
      {
        aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
        RenderString(aRenderingContext, aStyleContext, aPresContext,
                     aTextStyle, isRightToLeftOnBidiPlatform, text, 
                     PRUint32(textLength), PR_TRUE, dx, dy, width, details);
      }
      sdptr = details;
      if (details){
        while ((sdptr = details->mNext) != nsnull) {
          delete details;
          details = sdptr;
        }
        delete details;
      }
    }
#ifdef IBMBIDI
    if (isRightToLeftOnBidiPlatform) {
      
      
      aRenderingContext.SetRightToLeftText(PR_FALSE);
    }
#endif 
  }
}

void
nsTextFrame::PaintAsciiText(nsPresContext* aPresContext,
                            nsIRenderingContext& aRenderingContext,
                            nsStyleContext* aStyleContext,
                            nsTextPaintStyle& aTextStyle,
                            nscoord dx, nscoord dy)
{
  NS_PRECONDITION(0 == (TEXT_HAS_MULTIBYTE & mState), "text is multi-byte");

  nsCOMPtr<nsISelectionController> selCon;
  nsCOMPtr<nsIPresShell> shell;
  PRBool  displaySelection,canDarkenColor=PR_FALSE;
  PRBool  isPaginated;
  PRBool  isSelected;
  PRBool  hideStandardSelection;
  PRInt16 selectionValue;
  if (NS_FAILED(GetTextInfoForPainting(aPresContext, 
                                       getter_AddRefs(shell),
                                       getter_AddRefs(selCon),
                                       displaySelection,
                                       isPaginated,
                                       isSelected,
                                       hideStandardSelection,
                                       selectionValue))) {
     return;
  }

  if(isPaginated){
    canDarkenColor = CanDarken(aPresContext);
  }

  
  const nsTextFragment* frag = mContent->GetText();
  if (!frag) {
    return;
  }

  
  nsAutoTextBuffer unicodePaintBuffer;
  nsAutoIndexBuffer indexBuffer;
  if (displaySelection) {
    if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1))) {
      return;
    }
  }

  nsTextTransformer tx(aPresContext);

  
  
  
  
  PRInt32     textLength = 0;
  const char* text;
  char        paintBufMem[TEXT_BUF_SIZE];
  char*       paintBuf = paintBufMem;
  if (frag->Is2b() ||
      (0 != (mState & TEXT_WAS_TRANSFORMED)) ||
      (displaySelection && isSelected)) {
    
    
    
    
    
    PrepareUnicodeText(tx, (displaySelection ? &indexBuffer : nsnull),
                       &unicodePaintBuffer, &textLength);


    
    if (textLength > TEXT_BUF_SIZE) {
      paintBuf = new char[textLength];
      if (!paintBuf) {
        return;
      }
    }
    char* dst = paintBuf;
    char* end = dst + textLength;
    PRUnichar* src = unicodePaintBuffer.mBuffer;
    while (dst < end) {
      *dst++ = (char) ((unsigned char) *src++);
    }

    text = paintBuf;

  }
  else if (mContentOffset + mContentLength <= frag->GetLength()) {
    text = frag->Get1b() + mContentOffset;
    textLength = mContentLength;

    
    if (0 != (mState & TEXT_SKIP_LEADING_WS)) {
      while ((textLength > 0) && XP_IS_SPACE(*text)) {
        text++;
        textLength--;
      }
    }

    
    if ((textLength > 0) && (text[textLength - 1] == '\n')) {
      textLength--;
    }
    NS_ASSERTION(textLength >= 0, "bad text length");
  }
  else {
    
    
    
    NS_WARNING("content length exceeds fragment length");
  }

  nscoord width = mRect.width;
  PRInt32* ip = indexBuffer.mBuffer;

  if (0 != textLength) {
    if (!displaySelection || !isSelected) { 
      
      
      
      aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
      aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
      PaintTextDecorations(aRenderingContext, aStyleContext,
                           aPresContext, aTextStyle, dx, dy, width, PR_FALSE);
    }
    else {
      SelectionDetails *details = nsnull;
      nsCOMPtr<nsIContent> content;
      PRInt32 offset;
      PRInt32 length;
      nsresult rv = GetContentAndOffsetsForSelection(aPresContext,
                                                     getter_AddRefs(content),
                                                     &offset, &length);
      if (NS_SUCCEEDED(rv)) {
        details = GetFrameSelection()->LookUpSelection(content, mContentOffset,
                                                       mContentLength, PR_FALSE);
      }
        
      
      SelectionDetails *sdptr = details;
      while (sdptr){
        sdptr->mStart = ip[sdptr->mStart] - mContentOffset;
        sdptr->mEnd = ip[sdptr->mEnd]  - mContentOffset;
        sdptr = sdptr->mNext;
      }

      if (!hideStandardSelection || displaySelection) {
        
        DrawSelectionIterator iter(details, (PRUnichar *)text,
                                   (PRUint32)textLength, &aTextStyle,
                                   nsTextPaintStyle::eAllSelections);

        
        
        PRUint32 clusterHint = 0;
        aRenderingContext.GetHints(clusterHint);
        clusterHint &= NS_RENDERING_HINT_TEXT_CLUSTERS;

        if (!iter.IsDone() && iter.First())
        {
          nscoord currentX = dx;
          nscoord newWidth;
          while (!iter.IsDone())
          {
            char *currenttext  = iter.CurrentTextCStrPtr();
            PRUint32   currentlength= iter.CurrentLength();
            nscolor    currentFGColor, currentBKColor;
            PRBool     isCurrentBKColorTransparent;

            if (clusterHint) {
              PRUint32 tmpWidth;
              rv = aRenderingContext.GetRangeWidth(text, textLength, currenttext - text,
                                                   (currenttext - text) + currentlength,
                                                   tmpWidth);
              newWidth = nscoord(tmpWidth);
            }
            else {
              rv = aRenderingContext.GetWidth(currenttext, currentlength,newWidth); 
            }

            PRBool     isSelection = iter.GetSelectionColors(&currentFGColor,
                                                             &currentBKColor,
                                                             &isCurrentBKColorTransparent);

            if (NS_SUCCEEDED(rv)) {
              if (isSelection && !isPaginated)
              {
                if (!isCurrentBKColorTransparent) {
                  aRenderingContext.SetColor(currentBKColor);
                  aRenderingContext.FillRect(currentX, dy, newWidth, mRect.height);
                }
              }
            }
            else {
              newWidth =0;
            }

            aRenderingContext.PushState();

            nsRect rect(currentX, dy, newWidth, mRect.height);
            aRenderingContext.SetClipRect(rect, nsClipCombine_kIntersect);

            if (isPaginated && !iter.IsBeforeOrAfter()) {
              aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
              aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
            } else if (!isPaginated) {
              aRenderingContext.SetColor(nsCSSRendering::TransformColor(currentFGColor,canDarkenColor));
              aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
            }

            aRenderingContext.PopState();

            currentX+=newWidth;

            iter.Next();
          }
        }
        else if (!isPaginated) 
        {
          aRenderingContext.SetColor(nsCSSRendering::TransformColor(aTextStyle.mColor->mColor,canDarkenColor));
          aRenderingContext.DrawString(text, PRUint32(textLength), dx, dy + mAscent);
        }
      }

      PaintTextDecorations(aRenderingContext, aStyleContext, aPresContext,
                           aTextStyle, dx, dy, width, PR_FALSE,
                           unicodePaintBuffer.mBuffer,
                           details, 0, textLength);
      sdptr = details;
      if (details){
        while ((sdptr = details->mNext) != nsnull) {
          delete details;
          details = sdptr;
        }
        delete details;
      }
    }
  }

  
  if (paintBuf != paintBufMem) {
    delete [] paintBuf;
  }
}



nsIFrame::ContentOffsets nsTextFrame::CalcContentOffsetsFromFramePoint(nsPoint aPoint) {
  ContentOffsets offsets;
  GetPositionHelper(aPoint, getter_AddRefs(offsets.content), offsets.offset,
                    offsets.secondaryOffset);
  offsets.associateWithNext = (mContentOffset == offsets.offset);
  return offsets;
}







NS_IMETHODIMP
nsTextFrame::GetPositionHelper(const nsPoint&  aPoint,
                         nsIContent **   aNewContent,
                         PRInt32&        aContentOffset,
                         PRInt32&        aContentOffsetEnd)

{
  
  NS_PRECONDITION(aNewContent, "null arg");
  if (!aNewContent) {
    return NS_ERROR_NULL_POINTER;
  }
  
  *aNewContent = nsnull;

  DEBUG_VERIFY_NOT_DIRTY(mState);
  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;

  nsPresContext *presContext = PresContext();
  nsIPresShell *shell = presContext->GetPresShell();
  if (shell) {
    nsCOMPtr<nsIRenderingContext> rendContext;      
    nsresult rv = shell->CreateRenderingContext(this, getter_AddRefs(rendContext));
    if (NS_SUCCEEDED(rv)) {
      nsTextStyle ts(presContext, *rendContext, mStyleContext);
      SetupTextRunDirection(presContext, rendContext);
      if (ts.mSmallCaps || ts.mWordSpacing || ts.mLetterSpacing || ts.mJustifying) {
        nsresult result = GetPositionSlowly(rendContext, aPoint, aNewContent,
                                 aContentOffset);
        aContentOffsetEnd = aContentOffset;
        return result;
      }

      
      nsAutoTextBuffer paintBuffer;
      nsAutoIndexBuffer indexBuffer;
      rv = indexBuffer.GrowTo(mContentLength + 1);
      if (NS_FAILED(rv)) {
        return rv;
      }

      
      SetFontFromStyle(rendContext, mStyleContext);

      
      nsTextTransformer tx(PresContext());
      PRInt32 textLength;
      
      PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);

      if (textLength <= 0) {
        aContentOffset = mContentOffset;
        aContentOffsetEnd = aContentOffset;
      }
      else
      {
        PRInt32* ip = indexBuffer.mBuffer;

        PRInt32 indx;
        PRInt32 textWidth = 0;
        PRUnichar* text = paintBuffer.mBuffer;

        
        PRUint32 clusterHint = 0;
        rendContext->GetHints(clusterHint);
        clusterHint &= NS_RENDERING_HINT_TEXT_CLUSTERS;
        if (clusterHint) {
          indx = rendContext->GetPosition(text, textLength, aPoint);
        }
        else {
#ifdef IBMBIDI
        PRBool getReversedPos = NS_GET_EMBEDDING_LEVEL(this) & 1;
        nscoord posX = (getReversedPos) ?
                       (mRect.width) - (aPoint.x) : aPoint.x;

        PRBool found = nsLayoutUtils::BinarySearchForPosition(rendContext, text, 0, 0, 0,
                                               PRInt32(textLength),
                                               PRInt32(posX) , 
                                               indx, textWidth);

#else
        PRBool found = nsLayoutUtils::BinarySearchForPosition(rendContext, text, 0, 0, 0,
                                               PRInt32(textLength),
                                               PRInt32(aPoint.x) , 
                                               indx, textWidth);
#endif 
        if (found) {
          PRInt32 charWidth;
          if (NS_IS_HIGH_SURROGATE(text[indx]))
            rendContext->GetWidth(&text[indx], 2, charWidth);
          else
            rendContext->GetWidth(text[indx], charWidth);
          charWidth /= 2;

#ifdef IBMBIDI
          if (getReversedPos) {
            if (mRect.width - aPoint.x> textWidth+charWidth ) {
              indx++;
            }
          }
          else
#endif 
          if ((aPoint.x) > textWidth+charWidth) {
            indx++;
          }
        }
        }

        aContentOffset = indx + mContentOffset;
        
        PRInt32 i;
        for (i = 0; i < mContentLength; i ++){
          if ((ip[i] >= aContentOffset) && 
              (! NS_IS_LOW_SURROGATE(paintBuffer.mBuffer[ip[i]-mContentOffset]))) {
              break;
          }
        }
        aContentOffset = i + mContentOffset;
#ifdef IBMBIDI
        PRInt32 bidiStopOffset = mContentOffset + mContentLength;

        if (aContentOffset >= mContentOffset && aContentOffset < bidiStopOffset) {
          PRInt32 curindx = ip[aContentOffset - mContentOffset] - mContentOffset;
          while (curindx < textLength && IS_BIDI_DIACRITIC(text[curindx])) {
            if (++aContentOffset >= bidiStopOffset)
              break;
            curindx = ip[aContentOffset - mContentOffset] - mContentOffset;
          }
        }
#endif 
        aContentOffsetEnd = aContentOffset;
        NS_ASSERTION(i<= mContentLength, "offset we got from binary search is messed up");
      }      
      *aNewContent = mContent;
      if (*aNewContent) {
        (*aNewContent)->AddRef();
      }
    }
  }
  return NS_OK;
}


void ForceDrawFrame(nsFrame * aFrame);


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

#if 0
  PRBool isSelected = ((GetStateBits() & NS_FRAME_SELECTED_CONTENT) == NS_FRAME_SELECTED_CONTENT);
  if (!aSelected && !isSelected) 
  {
    return NS_OK;
  }
#endif

  
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
      if ((mContentOffset + mContentLength) >= startOffset)
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
    SelectionDetails *details = nsnull;
    nsCOMPtr<nsIContent> content;
    PRInt32 offset;
    PRInt32 length;

    nsresult rv = GetContentAndOffsetsForSelection(aPresContext,
                                                   getter_AddRefs(content),
                                                   &offset, &length);
    if (NS_SUCCEEDED(rv) && content) {
      details = GetFrameSelection()->LookUpSelection(content, offset,
                                                     length, PR_TRUE);
      
    }
    if (!details)
      RemoveStateBits(NS_FRAME_SELECTED_CONTENT);
    else
    {
      SelectionDetails *sdptr = details;
      while ((sdptr = details->mNext) != nsnull) {
        delete details;
        details = sdptr;
      }
      delete details;
    }
  }
  if (found){ 
    
    
    
    Invalidate(nsRect(0, 0, mRect.width, mRect.height), PR_FALSE);
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
nsTextFrame::GetPointFromOffset(nsPresContext* aPresContext,
                                nsIRenderingContext* inRendContext,
                                PRInt32 inOffset,
                                nsPoint* outPoint)
{
  if (!aPresContext || !inRendContext || !outPoint)
    return NS_ERROR_NULL_POINTER;

  outPoint->x = 0;
  outPoint->y = 0;

  DEBUG_VERIFY_NOT_DIRTY(mState);
  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;

  if (mContentLength <= 0) {
    return NS_OK;
  }

  inOffset-=mContentOffset;
  if (inOffset < 0){
    NS_ASSERTION(0,"offset less than this frame has in GetPointFromOffset");
    inOffset = 0;
  }
  if (inOffset >= mContentLength)
    inOffset = mContentLength;

  nsTextStyle ts(aPresContext, *inRendContext, mStyleContext);
  SetupTextRunDirection(aPresContext, inRendContext);

  
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  nsresult rv = indexBuffer.GrowTo(mContentLength + 1);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  nsTextTransformer tx(aPresContext);
  PRInt32 textLength;
  PRIntn numJustifiableCharacter;

  PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength, PR_FALSE, &numJustifiableCharacter);

  ComputeExtraJustificationSpacing(*inRendContext, ts, paintBuffer.mBuffer, textLength, numJustifiableCharacter);


  PRInt32* ip = indexBuffer.mBuffer;
  if (inOffset > mContentLength){
    NS_ASSERTION(0, "invalid offset passed to GetPointFromOffset");
    inOffset = mContentLength;
  }

  while (inOffset >=0 && ip[inOffset] < mContentOffset) 
    inOffset --;
  nscoord width = mRect.width;
  if (inOffset <0)
  {
    NS_ASSERTION(0, "invalid offset passed to GetPointFromOffset");
    inOffset=0;
    width = 0;
  }
  else
  {
    PRInt32 hitLength = ip[inOffset] - mContentOffset;
    if (ts.mSmallCaps || (0 != ts.mWordSpacing) || (0 != ts.mLetterSpacing) || ts.mJustifying)
    {
      nsTextDimensions dimensions;
      GetTextDimensions(*inRendContext, ts, paintBuffer.mBuffer, hitLength,
                        textLength == hitLength, &dimensions);
      width = dimensions.width;
    }
    else
    {
      PRInt32 totalLength = mContent->TextLength(); 
      if ((hitLength == textLength) && (inOffset == mContentLength) &&
          (mContentOffset + mContentLength == totalLength)) {
        
      }
      else
        inRendContext->GetWidth(paintBuffer.mBuffer, hitLength, width);
    }
    if ((hitLength == textLength && inOffset > 0 && ip[inOffset] == ip[inOffset-1])
        && (TEXT_TRIMMED_WS & mState)) {
      
      
      
      
      
      
      
      
      width += ts.mSpaceWidth + ts.mWordSpacing + ts.mLetterSpacing;
    }
  }
  if (NS_GET_EMBEDDING_LEVEL(this) & 1)
    outPoint->x = mRect.width - width;
  else
    outPoint->x = width;
  outPoint->y = 0;

  return NS_OK;
}



NS_IMETHODIMP
nsTextFrame::GetChildFrameContainingOffset(PRInt32 inContentOffset,
                                           PRBool  inHint,
                                           PRInt32* outFrameContentOffset,
                                           nsIFrame **outChildFrame)
{
  DEBUG_VERIFY_NOT_DIRTY(mState);
#if 0 
  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;
#endif

  if (nsnull == outChildFrame)
    return NS_ERROR_NULL_POINTER;
  PRInt32 contentOffset = inContentOffset;
  
  if (contentOffset != -1) 
    contentOffset = inContentOffset - mContentOffset;

  if ((contentOffset > mContentLength) || ((contentOffset == mContentLength) && inHint) )
  {
    
    nsIFrame* nextContinuation = GetNextContinuation();
    if (nextContinuation)
    {
      return nextContinuation->GetChildFrameContainingOffset(inContentOffset, inHint, outFrameContentOffset, outChildFrame);
    }
    else {
      if (contentOffset != mContentLength) 
        return NS_ERROR_FAILURE;
    }
  }

  if (inContentOffset < mContentOffset) 
  {
    *outChildFrame = GetPrevInFlow();
    if (*outChildFrame)
      return (*outChildFrame)->GetChildFrameContainingOffset(inContentOffset, inHint,
        outFrameContentOffset,outChildFrame);
    else
      return NS_OK; 
  }
  
  *outFrameContentOffset = contentOffset;
  *outChildFrame = this;
  return NS_OK;
}

PRBool
nsTextFrame::PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= mContentLength, "aOffset out of range");
  return (!IsEmpty());
}

PRBool
nsTextFrame::PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset)
{
  NS_ASSERTION (aOffset && *aOffset <= mContentLength, "aOffset out of range");
  PRInt32 startOffset = *aOffset;
  
  if (startOffset < 0)
    startOffset = mContentLength;

  nsPresContext* presContext = PresContext();
  
  
  nsAutoTextBuffer paintBuffer;
  nsAutoIndexBuffer indexBuffer;
  nsresult rv = indexBuffer.GrowTo(mContentLength + 1);
  if (NS_FAILED(rv)) {
    return PR_FALSE;
  }  
  PRInt32 textLength;
  nsTextTransformer tx(presContext);
  PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);  
  PRInt32* ip = indexBuffer.mBuffer;

  PRBool found = PR_TRUE;
  
  PRBool selectable;
  PRUint8 selectStyle;
  
  IsSelectable(&selectable, &selectStyle);
  if (selectStyle == NS_STYLE_USER_SELECT_ALL)
    return PR_FALSE;

  if (!aForward) {
    *aOffset = 0;
    PRInt32 i;
    
    nsAutoPRUint8Buffer clusterBuffer;
    rv = FillClusterBuffer(presContext, paintBuffer.mBuffer,
                           (PRUint32)textLength, clusterBuffer);
    NS_ENSURE_SUCCESS(rv, rv);
    
    for (i = startOffset-1; i >= 0; i--){
      if ((ip[i] < ip[startOffset]) &&
          (clusterBuffer.mBuffer[ip[i] - mContentOffset]) &&
          (! NS_IS_LOW_SURROGATE(paintBuffer.mBuffer[ip[i]-mContentOffset])))
      {
        *aOffset = i;
        break;
      }
    }
    
#ifdef SUNCTL
    static NS_DEFINE_CID(kLECID, NS_ULE_CID);
    
    nsCOMPtr<nsILE> ctlObj;
    ctlObj = do_CreateInstance(kLECID, &rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Cell based cursor movement will not be supported\n");
      ctlObj = nsnull;
    }
    else {
      PRBool  needsCTL = PR_FALSE;
      PRInt32 previousOffset;
      
      ctlObj->NeedsCTLFix(NS_REINTERPRET_CAST(const PRUnichar*,
                                              paintBuffer.mBuffer),
                          mContentOffset + startOffset, -1, &needsCTL);
      
      if (needsCTL) {
        ctlObj->PrevCluster(NS_REINTERPRET_CAST(const PRUnichar*,
                                                paintBuffer.mBuffer),
                            textLength, mContentOffset + startOffset,
                            &previousOffset);
        *aOffset = i = previousOffset - mContentOffset;
      }
    }
#endif 
    
    if (i < 0)
      found = PR_FALSE;
  }
  else 
  {
    PRInt32 i;
    *aOffset = mContentLength;
    
    nsAutoPRUint8Buffer clusterBuffer;
    rv = FillClusterBuffer(presContext, paintBuffer.mBuffer,
                           (PRUint32)textLength, clusterBuffer);
    NS_ENSURE_SUCCESS(rv, rv);
    
    for (i = startOffset; i <= mContentLength; i++) {
      if ((ip[i] > ip[startOffset]) &&
          ((i == mContentLength) ||
           (!NS_IS_LOW_SURROGATE(paintBuffer.mBuffer[ip[i] - mContentOffset])) &&
           (clusterBuffer.mBuffer[ip[i] - mContentOffset]))) {
        *aOffset = i;
        break;
      }
    }
    
#ifdef SUNCTL
    static NS_DEFINE_CID(kLECID, NS_ULE_CID);
    
    nsCOMPtr<nsILE> ctlObj;
    ctlObj = do_CreateInstance(kLECID, &rv);
    if (NS_FAILED(rv)) {
      NS_WARNING("Cell based cursor movement will not be supported\n");
      ctlObj = nsnull;
    }
    else {
      PRBool needsCTL = PR_FALSE;
      PRInt32 nextOffset;
      
      ctlObj->NeedsCTLFix(NS_REINTERPRET_CAST(const PRUnichar*,
                                              paintBuffer.mBuffer),
                          mContentOffset + startOffset, 0, &needsCTL);
      
      if (needsCTL) {
        
        ctlObj->NextCluster(NS_REINTERPRET_CAST(const PRUnichar*,
                                                paintBuffer.mBuffer),
                            textLength, mContentOffset + startOffset,
                            &nextOffset);
        *aOffset = i = nextOffset - mContentOffset;
      }
    }
#endif 
    
    if (i > mContentLength)
      found = PR_FALSE;
  }
  return found;
}

PRBool
nsTextFrame::PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                            PRInt32* aOffset, PRBool* aSawBeforeType)
{
  NS_ASSERTION (aOffset && *aOffset <= mContentLength, "aOffset out of range");
  PRInt32 startOffset = *aOffset;
  if (startOffset < 0)
    startOffset = mContentLength;
  
  nsTextTransformer tx(PresContext());
  PRBool keepSearching = PR_TRUE; 
  PRBool found = PR_FALSE;
  PRBool isWhitespace, wasTransformed;
  PRInt32 wordLen, contentLen;  
  
  PRBool selectable;
  PRUint8 selectStyle;
  IsSelectable(&selectable, &selectStyle);
  if (selectStyle == NS_STYLE_USER_SELECT_ALL)
    found = PR_FALSE;
  else
  {
    tx.Init(this, mContent, mContentOffset + startOffset);
    if (!aForward) {
      *aOffset = 0; 
#ifdef IBMBIDI
      wordLen = (mState & NS_FRAME_IS_BIDI) ? mContentOffset : -1;
#endif 
      if (tx.GetPrevWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace,
                         PR_FALSE, aIsKeyboardSelect) &&
          contentLen <= startOffset) {
        if ((aWordSelectEatSpace ? isWhitespace : !isWhitespace) || !*aSawBeforeType){
          *aOffset = startOffset - contentLen;
          keepSearching = PR_TRUE;
          if (aWordSelectEatSpace ? isWhitespace : !isWhitespace)
            *aSawBeforeType = PR_TRUE;
#ifdef IBMBIDI
          wordLen = (mState & NS_FRAME_IS_BIDI) ? mContentOffset : -1;
#endif 
          while (tx.GetPrevWord(PR_FALSE, &wordLen, &contentLen,
                                &isWhitespace, PR_FALSE,
                                aIsKeyboardSelect)){
            if (aWordSelectEatSpace ? !isWhitespace : *aSawBeforeType)
              break;
            if (contentLen >= startOffset)
              goto TryNextFrame;
            *aOffset -= contentLen;
            if (aWordSelectEatSpace ? isWhitespace : !isWhitespace)
              *aSawBeforeType = PR_TRUE;
#ifdef IBMBIDI
            wordLen = (mState & NS_FRAME_IS_BIDI) ? mContentOffset : -1;
#endif 
          }
          keepSearching = *aOffset <= 0;
          if (!keepSearching)
            found = PR_TRUE;
        }
        else {
          *aOffset = mContentLength;
          found = PR_TRUE;
        }
      }
    }
    else { 
      *aOffset = mContentLength; 
        
#ifdef IBMBIDI
      wordLen = (mState & NS_FRAME_IS_BIDI) ? mContentOffset + mContentLength : -1;
#endif 
      if (tx.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, &wasTransformed, PR_TRUE, PR_FALSE, aIsKeyboardSelect) &&
          (startOffset + contentLen <= mContentLength)) {
        
        if ((aWordSelectEatSpace ? isWhitespace : !isWhitespace) || !*aSawBeforeType) {
          *aOffset = startOffset + contentLen;
          keepSearching = PR_TRUE;
          if (aWordSelectEatSpace ? isWhitespace : !isWhitespace)
            *aSawBeforeType = PR_TRUE;
#ifdef IBMBIDI
          wordLen = (mState & NS_FRAME_IS_BIDI) ? mContentOffset + mContentLength : -1;
#endif 
          while (tx.GetNextWord(PR_FALSE, &wordLen, &contentLen, &isWhitespace, &wasTransformed, PR_TRUE, PR_FALSE, aIsKeyboardSelect))
          {
            if (aWordSelectEatSpace ? !isWhitespace : *aSawBeforeType)
              break;
            if (startOffset + contentLen >= mContentLength)
              goto TryNextFrame;
            if (aWordSelectEatSpace ? isWhitespace : !isWhitespace)
              *aSawBeforeType = PR_TRUE;
            *aOffset += contentLen;
#ifdef IBMBIDI
            wordLen = (mState & NS_FRAME_IS_BIDI) ? mContentOffset + mContentLength : -1;
#endif 
          }
          keepSearching = *aOffset >= mContentLength;
          if (!keepSearching)
            found = PR_TRUE;
        }
        else
        {
          *aOffset = 0;
          found = PR_TRUE;
        }
      } 
    }
  }
TryNextFrame:
  if (!found ||
      *aOffset > mContentLength ||
      *aOffset < 0)
  {
    found = PR_FALSE;
    *aOffset = PR_MIN(*aOffset, mContentLength);
    *aOffset = PR_MAX(*aOffset, 0);    
  }
  return found;
}  

NS_IMETHODIMP
nsTextFrame::CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aRecurse, PRBool *aFinished, PRBool *_retval)
{
  if (!aFinished || !_retval)
    return NS_ERROR_NULL_POINTER;
  if (*aFinished)
    return NS_ERROR_FAILURE; 
  if (mContentOffset > aEndIndex)
    return NS_OK; 
  if (mContentOffset > aStartIndex)
    aStartIndex = mContentOffset;
  if (aStartIndex >= aEndIndex) 
    return NS_OK; 

  nsresult rv ;
  if (aStartIndex < (mContentOffset + mContentLength))
  {
  
    nsIPresShell *shell = aContext->GetPresShell();
    if (!shell) 
      return NS_ERROR_FAILURE;

  
    nsIDocument *doc = shell->GetDocument();
    if (!doc)
      return NS_ERROR_FAILURE;
  
    nsTextTransformer tx(aContext);
  
    nsAutoTextBuffer paintBuffer;
    nsAutoIndexBuffer indexBuffer;
    if (NS_FAILED(indexBuffer.GrowTo(mContentLength + 1)))
      return NS_ERROR_FAILURE;

    PRInt32 textLength;
    PrepareUnicodeText(tx, &indexBuffer, &paintBuffer, &textLength);
    if (textLength)
    {
      PRInt32 start = PR_MAX(aStartIndex,mContentOffset);
      PRInt32 end = PR_MIN(mContentOffset + mContentLength-1, aEndIndex); 
      while (start != end)
      { 
        if (indexBuffer.mBuffer[start] < indexBuffer.mBuffer[start+1]) 
        {
          *aFinished = PR_TRUE;
          *_retval = PR_TRUE;
          return NS_OK;
        }
        start++;
      }
      if (start == aEndIndex)
      {
        *aFinished = PR_TRUE;
      }
    }
  }
  if (aRecurse) 
  {
    nsIFrame *nextInFlow = this; 
    rv = NS_OK;
    while (!aFinished && nextInFlow && NS_SUCCEEDED(rv) && !*_retval) 
    {
      nextInFlow = nextInFlow->GetNextInFlow();
      if (nextInFlow)
      {
        rv = nextInFlow->CheckVisibility(aContext,aStartIndex,aEndIndex,PR_FALSE,aFinished,_retval);
      }
    }
  }
  return NS_OK;
}



NS_IMETHODIMP
nsTextFrame::GetOffsets(PRInt32 &start, PRInt32 &end) const
{
  start = mContentOffset;
  end = mContentOffset+mContentLength;
  return NS_OK;
}
  
#define TEXT_MAX_NUM_SEGMENTS 65

struct SegmentData {
  PRUint32  mIsWhitespace : 1;
  PRUint32  mContentLen : 31;  

  PRBool  IsWhitespace() {return PRBool(mIsWhitespace);}

  
  
  PRInt32 ContentLen() {return PRInt32(mContentLen);}
};

struct TextRun {
  
  PRInt32       mTotalNumChars, mTotalContentLen;

  
  PRInt32       mNumSegments;

  
  PRInt32       mBreaks[TEXT_MAX_NUM_SEGMENTS];

  
  SegmentData   mSegments[TEXT_MAX_NUM_SEGMENTS];

  TextRun()
  {
    Reset();
  }
  
  void Reset()
  {
    mNumSegments = 0;
    mTotalNumChars = 0;
    mTotalContentLen = 0;
  }

  
  PRBool IsBuffering()
  {
    return mNumSegments > 0;
  }

  void AddSegment(PRInt32 aNumChars, PRInt32 aContentLen, PRBool aIsWhitespace)
  {
    NS_PRECONDITION(mNumSegments < TEXT_MAX_NUM_SEGMENTS, "segment overflow");
#ifdef IBMBIDI
    if (mNumSegments >= TEXT_MAX_NUM_SEGMENTS) {
      return;
    }
#endif 
    mTotalNumChars += aNumChars;
    mBreaks[mNumSegments] = mTotalNumChars;
    mSegments[mNumSegments].mIsWhitespace = aIsWhitespace;
    mTotalContentLen += aContentLen;
    mSegments[mNumSegments].mContentLen = PRUint32(mTotalContentLen);
    mNumSegments++;
  }
};

PRUint32
nsTextFrame::EstimateNumChars(PRUint32 aAvailableWidth,
                              PRUint32 aAverageCharWidth)
{
  
  
  
  if (aAverageCharWidth == 0) {
    return PR_UINT32_MAX;
  }

  PRUint32 estimatedNumChars = aAvailableWidth / aAverageCharWidth;
  return estimatedNumChars + estimatedNumChars / 20;
}
  









#include "punct_marks.ccmap"
DEFINE_CCMAP(gPuncCharsCCMap, const);
  
#define IsPunctuationMark(ch) (CCMAP_HAS_CHAR(gPuncCharsCCMap, ch))

static PRBool CanBreakBetween(nsTextFrame* aBefore,
                              PRBool aBreakWhitespaceBefore,
                              nsTextFrame* aAfter,
                              PRBool aBreakWhitespaceAfter,
                              PRBool aSkipLeadingWhitespaceAfter,
                              nsIFrame* aLineContainer)
{
  
  const nsTextFragment* fragBefore = aBefore->GetContent()->GetText();
  const nsTextFragment* fragAfter = aAfter->GetContent()->GetText();
  NS_ASSERTION(fragBefore && fragAfter, "text frames with no text!");

  
  PRInt32 beforeOffset = aBefore->GetContentOffset() + aBefore->GetContentLength();
  
  PRInt32 afterOffset = aAfter->GetContentOffset();
  PRInt32 afterLength = fragAfter->GetLength() - afterOffset;
  
  if (beforeOffset <= 0 || afterLength <= 0) {
    
#if 0
    NS_WARNING("Textframe maps no content");
#endif
    return PR_FALSE;
  }

  PRUnichar lastBefore = fragBefore->CharAt(beforeOffset - 1);
  PRUnichar firstAfter = fragAfter->CharAt(afterOffset);
  
  
  
  
  
  
  if (aSkipLeadingWhitespaceAfter && XP_IS_SPACE(firstAfter))
    return PR_FALSE;

  while (IS_DISCARDED(firstAfter)) {
    ++afterOffset;
    --afterLength;
    if (afterLength == 0) {
      
      return PR_FALSE;
    }
    firstAfter = fragAfter->CharAt(afterOffset);
  }
  while (IS_DISCARDED(lastBefore)) {
    --beforeOffset;
    if (beforeOffset == 0) {
      
      NS_WARNING("Before-frame should not have called SetTrailingTextFrame");
      return PR_FALSE;
    }
    lastBefore = fragBefore->CharAt(beforeOffset - 1);
  }

  
  
  if ((XP_IS_SPACE(lastBefore) && aBreakWhitespaceBefore) ||
      (XP_IS_SPACE(firstAfter) && aBreakWhitespaceAfter))
    return PR_TRUE;
  
  
  
  
  if (!fragBefore->Is2b() && !fragAfter->Is2b())
    return PR_FALSE;

  nsIFrame* f =
    nsLayoutUtils::GetClosestCommonAncestorViaPlaceholders(aBefore, aAfter, aLineContainer);
  NS_ASSERTION(f, "Frames to check not in the same document???");
  
  if (f->GetStyleText()->WhiteSpaceCanWrap())
    return PR_FALSE;

  nsAutoString beforeStr;
  nsAutoString afterStr;
  fragBefore->AppendTo(beforeStr, 0, beforeOffset);
  fragAfter->AppendTo(afterStr, afterOffset, afterLength);

  return nsContentUtils::LineBreaker()->BreakInBetween(
    beforeStr.get(), beforeStr.Length(), afterStr.get(), afterStr.Length());
}

nsReflowStatus
nsTextFrame::MeasureText(nsPresContext*          aPresContext,
                         const nsHTMLReflowState& aReflowState,
                         nsTextTransformer&       aTx,
                         nsTextStyle&               aTs,
                         TextReflowData&          aTextData)
{
  PRBool firstThing = PR_TRUE;
  nscoord maxWidth = aReflowState.availableWidth;
  nsLineLayout& lineLayout = *aReflowState.mLineLayout;
  PRInt32 contentLength = aTx.GetContentLength();
  PRInt32 startingOffset = aTextData.mOffset;
  PRInt32 column = mColumn;
  nscoord prevAscent = 0, prevDescent = 0;
  PRInt32 lastWordLen = 0;
  PRUnichar* lastWordPtr = nsnull;
  PRBool  endsInWhitespace = PR_FALSE;
  PRBool  endsInNewline = PR_FALSE;
  PRBool  justDidFirstLetter = PR_FALSE;
  nsTextDimensions dimensions, lastWordDimensions;
  PRBool  measureTextRuns = PR_FALSE;

  
  PRBool trailingTextFrameCanWrap;
  nsIFrame* lastTextFrame = lineLayout.GetTrailingTextFrame(&trailingTextFrameCanWrap);
  PRBool canBreakBetweenTextFrames = PR_FALSE;
  if (lastTextFrame) {
    NS_ASSERTION(lastTextFrame->GetType() == nsGkAtoms::textFrame,
                 "Trailing text frame isn't text!");
    canBreakBetweenTextFrames =
      CanBreakBetween(NS_STATIC_CAST(nsTextFrame*, lastTextFrame),
                      trailingTextFrameCanWrap,
                      this, aTextData.mWrapping, aTextData.mSkipWhitespace,
                      lineLayout.GetLineContainerFrame());
  }

  if (contentLength == 0) {
    aTextData.mX = 0;
    aTextData.mAscent = 0;
    aTextData.mDescent = 0;
    return NS_FRAME_COMPLETE;
  }
#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11) || defined(XP_BEOS)
  
  PRUint32 hints = 0;
  aReflowState.rendContext->GetHints(hints);
  if (hints & NS_RENDERING_HINT_FAST_MEASURE) {
    measureTextRuns = !aTs.mPreformatted &&
                      !aTs.mSmallCaps && !aTs.mWordSpacing && !aTs.mLetterSpacing &&
                      aTextData.mWrapping;
  }
  
  
#endif 
  TextRun textRun;
  PRUint32 estimatedNumChars = EstimateNumChars(maxWidth - aTextData.mX,
                                                aTs.mAveCharWidth);

#ifdef IBMBIDI
  nsTextFrame* nextBidi = nsnull;
  PRInt32      start = -1, end;

  if (mState & NS_FRAME_IS_BIDI) {
    nextBidi = NS_STATIC_CAST(nsTextFrame*, GetLastInFlow()->GetNextContinuation());
    if (nextBidi) {
      if (mContentLength < 1) {
        mContentLength = 1;
      }
      nextBidi->GetOffsets(start, end);
      if (start <= mContentOffset) {
        nextBidi->AdjustOffsetsForBidi(mContentOffset + mContentLength, end);
      }
      else {
        mContentLength = start - mContentOffset;
      }
    }
  }
#endif 

  aTextData.mX = 0;
  if (aTextData.mMeasureText) {
    aTs.mNormalFont->GetMaxAscent(aTextData.mAscent);
    aTs.mNormalFont->GetMaxDescent(aTextData.mDescent);
  }
  PRBool firstWordDone = PR_FALSE;
  for (;;) {
#ifdef IBMBIDI
    if (nextBidi && (mContentLength <= 0) ) {
      if (textRun.IsBuffering()) {
        
        goto MeasureTextRun;
      }
      else {
        break;
      }
    }
#endif 
    
    PRBool isWhitespace, wasTransformed;
    PRInt32 wordLen, contentLen;
    union {
      char*       bp1;
      PRUnichar*  bp2;
    };
#ifdef IBMBIDI
    wordLen = start;
#endif 

    bp2 = aTx.GetNextWord(aTextData.mInWord, &wordLen, &contentLen, &isWhitespace,
                          &wasTransformed, textRun.mNumSegments == 0);

    
    
    
    
    if (!aTextData.mCanBreakBefore && !firstThing && !isWhitespace) {
      firstWordDone = PR_TRUE;
    }

#ifdef IBMBIDI
    if (nextBidi) {
      mContentLength -= contentLen;

      if (mContentLength < 0) {
        contentLen += mContentLength;
        wordLen = PR_MIN(wordLen, contentLen);
      }
    }
#endif 
    
    if (wasTransformed) {
      mState |= TEXT_WAS_TRANSFORMED;
    }
    
    if (bp2) {
      if (firstWordDone) {
        
        
        aTextData.mCanBreakBefore = PR_TRUE;
      }
    } else {
      if (textRun.IsBuffering()) {
        
        goto MeasureTextRun;
      }
      else {
        
        
        
        aTextData.mOffset += contentLen;
        break;
      }
    }

    lastWordLen = wordLen;
    lastWordPtr = bp2;
    aTextData.mInWord = PR_FALSE;

    
    PRUnichar firstChar;
    if (aTx.TransformedTextIsAscii()) {
      firstChar = *bp1;
    } else {
      firstChar = *bp2;
    }
    if (isWhitespace) {
      if ('\n' == firstChar) {
        
        NS_ASSERTION(aTs.mPreformatted, "newline w/o ts.mPreformatted");
        aTextData.mOffset++;
        endsInWhitespace = PR_TRUE;
        endsInNewline = PR_TRUE;
        break;
      }
      if (aTextData.mSkipWhitespace) {
        aTextData.mOffset += contentLen;
        aTextData.mSkipWhitespace = PR_FALSE; 

        if (wasTransformed) {
          
          
          if (wordLen == contentLen) {
            mState &= ~TEXT_WAS_TRANSFORMED;
          }
        }

        
        mState |= TEXT_SKIP_LEADING_WS;
        continue;
      }
      firstThing = PR_FALSE;

      
      
      aTextData.mCanBreakBefore = PR_TRUE;
      aTextData.mFirstLetterOK = PR_FALSE;
 
      if ('\t' == firstChar) {
        
        wordLen = 8 - (7 & column);
        
        dimensions.width = (aTs.mSpaceWidth + aTs.mWordSpacing + aTs.mLetterSpacing)*wordLen;

        
        
        mState |= TEXT_WAS_TRANSFORMED;
      }
      else if (textRun.IsBuffering()) {
        
        textRun.AddSegment(wordLen, contentLen, PR_TRUE);
        continue;
      }
      else {
        
        dimensions.width = wordLen*(aTs.mWordSpacing + aTs.mLetterSpacing + aTs.mSpaceWidth);
      }

      
      
      column += wordLen;
      endsInWhitespace = PR_TRUE;
      aTextData.mOffset += contentLen;

      if (aTextData.mMeasureText) {
        
        
        if (aTextData.mWrapping) {
          if (aTextData.mX + dimensions.width <= maxWidth) {
            aTextData.mX += dimensions.width;
          }
          else {
            
            
            aTextData.mTrailingSpaceTrimmed = PR_TRUE;
            
            
            
            
            
            
            break;
          }
        }
        else {
          
          
          aTextData.mX += dimensions.width;
        }
      } 
    }
    else {
      firstThing = PR_FALSE;

      aTextData.mSkipWhitespace = PR_FALSE;

      
      
      if (aTextData.mFirstLetterOK) {
        if (IsPunctuationMark(firstChar)) {
          if (contentLen > 1)
          {
            wordLen = 2;
            contentLen = 2;
          }
        }
        else {
          wordLen = 1;
          contentLen = 1;
        }
        justDidFirstLetter = PR_TRUE;
      }
      
      if (aTextData.mMeasureText) {
        if (measureTextRuns && !justDidFirstLetter) {
          
          textRun.AddSegment(wordLen, contentLen, PR_FALSE);

          
          if ((textRun.mTotalNumChars >= estimatedNumChars) ||
              (textRun.mNumSegments >= (TEXT_MAX_NUM_SEGMENTS - 1))) {
            goto MeasureTextRun;
          }
        }
        else {
          if (aTs.mSmallCaps) {
            MeasureSmallCapsText(aReflowState.rendContext, aTs, bp2, wordLen, PR_FALSE, &dimensions);
          }
          else {
            
            if (aTx.TransformedTextIsAscii()) {
              aReflowState.rendContext->GetTextDimensions(bp1, wordLen, dimensions);
            } else {
              aReflowState.rendContext->GetTextDimensions(bp2, wordLen, dimensions);
            }
#ifdef MOZ_MATHML
            
            
            
            if (justDidFirstLetter) {
              nsresult res;
              nsBoundingMetrics bm;
              if (aTx.TransformedTextIsAscii()) {
                res = aReflowState.rendContext->GetBoundingMetrics(bp1, wordLen, bm);
              } else {
                res = aReflowState.rendContext->GetBoundingMetrics(bp2, wordLen, bm);
              }
              if (NS_SUCCEEDED(res)) {
                aTextData.mAscent = dimensions.ascent = bm.ascent;
                aTextData.mDescent = dimensions.descent = bm.descent;
              }
            }
#endif
            if (aTs.mLetterSpacing) {
              dimensions.width += aTs.mLetterSpacing * wordLen;
            }

            if (aTs.mWordSpacing) {
              if (aTx.TransformedTextIsAscii()) {
                for (char* bp = bp1; bp < bp1 + wordLen; bp++) {
                  if (*bp == ' ') 
                    dimensions.width += aTs.mWordSpacing;
                }
              } else {
                for (PRUnichar* bp = bp2; bp < bp2 + wordLen; bp++) {
                  if (*bp == ' ') 
                    dimensions.width += aTs.mWordSpacing;
                }
              }
            }
          }

          lastWordDimensions = dimensions;

          PRBool canBreak;
          if (0 == aTextData.mX) {
            canBreak = canBreakBetweenTextFrames;
            
            
            
            
            
          } else {
            canBreak = aTextData.mWrapping;
          }
          if (canBreak) {
            
            PRBool forceBreak =
              lineLayout.NotifyOptionalBreakPosition(GetContent(), aTextData.mOffset, PR_TRUE);
            
            if (forceBreak || (aTextData.mX + dimensions.width > maxWidth)) {
              
              break;
            }
          }
          prevAscent = aTextData.mAscent;
          prevDescent =  aTextData.mDescent;

          aTextData.mX += dimensions.width;
          if (aTextData.mAscent < dimensions.ascent) {
            aTextData.mAscent = dimensions.ascent;
          }
          if (aTextData.mDescent < dimensions.descent) {
            aTextData.mDescent = dimensions.descent;
          }

          column += wordLen;
          endsInWhitespace = PR_FALSE;
          aTextData.mOffset += contentLen;
          if (justDidFirstLetter) {
            
            break;
          }
        }
      }
      else {
        
        PRBool canBreak;
        if (aTextData.mOffset == startingOffset) {
          canBreak = canBreakBetweenTextFrames;
        } else {
          canBreak = aTextData.mWrapping;
        }
        if (canBreak) {
#ifdef DEBUG
          PRBool forceBreak =
#endif
            lineLayout.NotifyOptionalBreakPosition(GetContent(), aTextData.mOffset, PR_TRUE);
          NS_ASSERTION(!forceBreak, "If we're supposed to break, we should be "
                       "really measuring");
        }
          
        
        column += wordLen;
        endsInWhitespace = PR_FALSE;
        aTextData.mOffset += contentLen;
        if (justDidFirstLetter) {
          
          break;
        }
      }
    }
    continue;

  MeasureTextRun:
#if defined(_WIN32) || defined(XP_OS2) || defined(MOZ_X11) || defined(XP_BEOS)
    
    if (hints & NS_RENDERING_HINT_FAST_MEASURE) {
      PRInt32 numCharsFit;

      PRInt32 forcedOffset = lineLayout.GetForcedBreakPosition(GetContent());
      PRInt32 measureChars = textRun.mTotalNumChars;
      if (forcedOffset == -1) {
        forcedOffset -= aTextData.mOffset;
#ifdef MOZ_CAIRO_GFX
        NS_ASSERTION(forcedOffset >= 0,
                     "Overshot forced offset, we should have already exited");
#endif
        if (forcedOffset >= 0 && forcedOffset < textRun.mTotalNumChars) {
          
          
          
          
          measureChars = forcedOffset;
        }
      }

      
      if (aTx.TransformedTextIsAscii()) {
        aReflowState.rendContext->GetTextDimensions((char*)aTx.GetWordBuffer(), measureChars,
                                           maxWidth - aTextData.mX,
                                           textRun.mBreaks, textRun.mNumSegments,
                                           dimensions, numCharsFit, lastWordDimensions);
      } else {
        aReflowState.rendContext->GetTextDimensions(aTx.GetWordBuffer(), measureChars,
                                           maxWidth - aTextData.mX,
                                           textRun.mBreaks, textRun.mNumSegments,
                                           dimensions, numCharsFit, lastWordDimensions);
      }
      
      
      PRBool canBreak;
      if (0 == aTextData.mX) {
        canBreak = canBreakBetweenTextFrames;
      } else {
        canBreak = aTextData.mWrapping;
      }
      if (canBreak && aTextData.mX + dimensions.width > maxWidth) {
        
#ifdef IBMBIDI
        nextBidi = nsnull;
#endif 
        break;
      }
  
      
      PRInt32 lastSegment;
      if (numCharsFit >= textRun.mTotalNumChars) { 
        NS_ASSERTION(numCharsFit == textRun.mTotalNumChars, "shouldn't overshoot");
        lastSegment = textRun.mNumSegments - 1;
      } else {
        for (lastSegment = 0; textRun.mBreaks[lastSegment] < numCharsFit; lastSegment++) ;
        NS_ASSERTION(lastSegment < textRun.mNumSegments, "failed to find segment");
        
        








      }
  
      





  
      aTextData.mX += dimensions.width;
      if (aTextData.mAscent < dimensions.ascent) {
        aTextData.mAscent = dimensions.ascent;
      }
      if (aTextData.mDescent < dimensions.descent) {
        aTextData.mDescent = dimensions.descent;
      }
      
      prevAscent = aTextData.mAscent;
      prevDescent = aTextData.mDescent;
      
      if (aTextData.mAscent < lastWordDimensions.ascent) {
        aTextData.mAscent = lastWordDimensions.ascent;
      }
      if (aTextData.mDescent < lastWordDimensions.descent) {
        aTextData.mDescent = lastWordDimensions.descent;
      }
  
      endsInWhitespace = textRun.mSegments[lastSegment].IsWhitespace();
      
      
      
      
      
      PRInt32 lastWhitespaceSegment =
        endsInWhitespace ? lastSegment : lastSegment - 1;
      if (lastWhitespaceSegment >= 0) {
        lineLayout.NotifyOptionalBreakPosition(GetContent(),
            aTextData.mOffset + textRun.mSegments[lastWhitespaceSegment].ContentLen(),
            PR_TRUE);
      }
  
      column += numCharsFit;
      aTextData.mOffset += textRun.mSegments[lastSegment].ContentLen();
  
      
      if (numCharsFit != textRun.mTotalNumChars) {
#ifdef IBMBIDI
        nextBidi = nsnull;
#endif 
        break;
      }
      
#ifdef IBMBIDI
      if (nextBidi && (mContentLength <= 0) ) {
        break;
      }
#endif 
  
      if (nsnull == bp2) {
        
        
        aTextData.mOffset += contentLen;
        break;
      }
  
      
      textRun.Reset();
  
      
      estimatedNumChars = EstimateNumChars(maxWidth - aTextData.mX,
                                           aTs.mAveCharWidth);
    }
#endif 
    ;
  }

  
  
  if (!aTextData.mMeasureText) {
    aTextData.mAscent = mAscent;
    aTextData.mDescent = mRect.height - aTextData.mAscent;
    aTextData.mX = mRect.width;
    if (mState & TEXT_TRIMMED_WS) {
      
      
      aTextData.mX += aTs.mSpaceWidth + aTs.mWordSpacing + aTs.mLetterSpacing;
    }
  }
  
  
  
  
  
  
  
  lineLayout.SetColumn(column);
  lineLayout.SetUnderstandsWhiteSpace(PR_TRUE);
  if (0 != aTextData.mX) {
    lineLayout.SetTrailingTextFrame(this, aTextData.mWrapping);
    lineLayout.SetEndsInWhiteSpace(endsInWhitespace);
    lineLayout.SetInWord(!endsInWhitespace);
  } else {
    
    
    
    lineLayout.SetTrailingTextFrame(nsnull, PR_FALSE);
    lineLayout.SetInWord(PR_FALSE);
  }
  if (justDidFirstLetter) {
    lineLayout.SetFirstLetterFrame(this);
    lineLayout.SetFirstLetterStyleOK(PR_FALSE);
    mState |= TEXT_FIRST_LETTER;
  }

  
  nsReflowStatus rs = (aTextData.mOffset == contentLength)
#ifdef IBMBIDI
                      || (aTextData.mOffset == start)
#endif 
    ? NS_FRAME_COMPLETE
    : NS_FRAME_NOT_COMPLETE;

  if (canBreakBetweenTextFrames && aTextData.mOffset == startingOffset) {
    
    return NS_INLINE_LINE_BREAK_BEFORE();
  }

  if (endsInNewline) {
    lineLayout.SetLineEndsInBR(PR_TRUE);
    return NS_INLINE_LINE_BREAK_AFTER(rs);
  }

  if (aTextData.mTrailingSpaceTrimmed && rs == NS_FRAME_COMPLETE) {
    
    lineLayout.SetLineEndsInSoftBR(PR_TRUE);
  } else if (lineLayout.GetLineEndsInSoftBR() && !lineLayout.GetEndsInWhiteSpace()) {
    
    return NS_INLINE_LINE_BREAK_BEFORE();
  }

  if (rs == NS_FRAME_COMPLETE && 0 != aTextData.mX && endsInWhitespace &&
      aTextData.mWrapping) {
    
    if (lineLayout.NotifyOptionalBreakPosition(GetContent(), aTextData.mOffset,
                                               aTextData.mX <= maxWidth))
      return NS_INLINE_LINE_BREAK_AFTER(rs);
  }

  if ((aTextData.mOffset != contentLength) && (aTextData.mOffset == startingOffset)) {
    
    return NS_INLINE_LINE_BREAK_BEFORE();
  }

  return rs;
}

 void
nsTextFrame::MarkIntrinsicWidthsDirty()
{
  
  
  RemoveStateBits(TEXT_OPTIMIZE_RESIZE);

  nsFrame::MarkIntrinsicWidthsDirty();
}


 void
nsTextFrame::AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinWidthData *aData)
{
  nsresult rv;

  nsPresContext *presContext = PresContext();
  nsTextStyle ts(presContext, *aRenderingContext, mStyleContext);
  SetupTextRunDirection(presContext, aRenderingContext);
  if (!ts.mFont->mSize)
    
    
    return;

  const nsStyleText *styleText = GetStyleText();
  PRBool wrapping = styleText->WhiteSpaceCanWrap();
  PRBool wsSignificant = styleText->WhiteSpaceIsSignificant();
  PRBool atStart = PR_TRUE;
  PRBool forceArabicShaping = (ts.mSmallCaps ||
                               (0 != ts.mWordSpacing) ||
                               (0 != ts.mLetterSpacing) ||
                               ts.mJustifying);
  nsTextTransformer tx(presContext);
  
  
  
  rv = tx.Init(this, mContent, mContentOffset, forceArabicShaping,
               !ts.mSmallCaps);
  if (NS_FAILED(rv)) {
    NS_NOTREACHED("failure initializing text transformer");
    return;
  }

  if (aData->trailingTextFrame &&
      CanBreakBetween(NS_STATIC_CAST(nsTextFrame*, aData->trailingTextFrame),
                      aData->trailingTextFrame->
                        GetStyleText()->WhiteSpaceCanWrap(),
                      this, wrapping,
                      aData->skipWhitespace, 
                      nsnull)) 
  {
    aData->Break(aRenderingContext);
  }

  for (;;) {
    union {
      char*       bp1;
      PRUnichar*  bp2;
    };
    PRInt32 wordLen = -1, contentLen;
    PRBool isWhitespace, wasTransformed;
    
    
    bp2 = tx.GetNextWord(!aData->skipWhitespace, &wordLen, &contentLen,
                         &isWhitespace, &wasTransformed);
    if (!bp2)
      break;
    

    if (isWhitespace) {
      PRUnichar firstChar;
      if (tx.TransformedTextIsAscii()) {
        firstChar = *bp1;
      } else {
        firstChar = *bp2;
      }
      if ('\n' == firstChar) {
        aData->Break(aRenderingContext);
        aData->skipWhitespace = PR_TRUE;
        aData->trailingWhitespace = 0;
      } else if (!aData->skipWhitespace || wsSignificant) {
        atStart = PR_FALSE;
        nscoord width;
        if ('\t' == firstChar) {
          
          wordLen = 8;
          
          width =
            (ts.mSpaceWidth + ts.mWordSpacing + ts.mLetterSpacing)*wordLen;
        } else {
          
          width =
            wordLen*(ts.mWordSpacing + ts.mLetterSpacing + ts.mSpaceWidth);
        }
        aData->currentLine += width;
        if (wsSignificant) {
          aData->trailingWhitespace = 0;
          aData->skipWhitespace = PR_FALSE;
        } else {
          aData->trailingWhitespace += width;
          aData->skipWhitespace = PR_TRUE;
        }

        if (wrapping) {
          aData->Break(aRenderingContext);
        }
      }
    } else {
      if (!atStart && wrapping) {
        aData->Break(aRenderingContext);
      }

      atStart = PR_FALSE;

      nscoord width;
      if (ts.mSmallCaps) {
        nsTextDimensions dimensions;
        
        
        
        aRenderingContext->SetTextRunRTL(PR_FALSE);
        MeasureSmallCapsText(aRenderingContext, ts, bp2, wordLen, PR_FALSE,
                             &dimensions);
        width = dimensions.width;
      } else {
        
        
        
        if (tx.TransformedTextIsAscii()) {
          
          aRenderingContext->SetTextRunRTL(PR_FALSE);
          aRenderingContext->GetWidth(bp1, wordLen, width);
        } else {
          
          
          width =
            nsLayoutUtils::GetStringWidth(this, aRenderingContext, bp2, wordLen);
        }
        width += ts.mLetterSpacing * wordLen;
      }

      aData->currentLine += width;
      aData->skipWhitespace = PR_FALSE;
      aData->trailingWhitespace = 0;
    }
  }

  aData->trailingTextFrame = this;
}

 void
nsTextFrame::AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefWidthData *aData)
{
  nsresult rv;

  nsPresContext *presContext = PresContext();
  nsTextStyle ts(presContext, *aRenderingContext, mStyleContext);
  if (!ts.mFont->mSize)
    
    
    return;

  PRBool forceArabicShaping = (ts.mSmallCaps ||
                               (0 != ts.mWordSpacing) ||
                               (0 != ts.mLetterSpacing) ||
                               ts.mJustifying);
  nsTextTransformer tx(presContext);
  
  
  
  rv = tx.Init(this, mContent, mContentOffset, forceArabicShaping,
               !ts.mSmallCaps);
  if (NS_FAILED(rv)) {
    NS_NOTREACHED("failure initializing text transformer");
    return;
  }

  for (;;) {
    union {
      char*       bp1;
      PRUnichar*  bp2;
    };
    PRInt32 wordLen = -1, contentLen;
    PRBool isWhitespace, wasTransformed;
    
    
    
    bp2 = tx.GetNextWord(!aData->skipWhitespace, &wordLen, &contentLen,
                         &isWhitespace, &wasTransformed);
    if (!bp2)
      break;
    

    if (isWhitespace) {
      PRUnichar firstChar;
      if (tx.TransformedTextIsAscii()) {
        firstChar = *bp1;
      } else {
        firstChar = *bp2;
      }
      if ('\n' == firstChar) {
        aData->Break(aRenderingContext);
      } else if (!aData->skipWhitespace) {
        nscoord width;
        if ('\t' == firstChar) {
          
          wordLen = 8;
          
          width =
            (ts.mSpaceWidth + ts.mWordSpacing + ts.mLetterSpacing)*wordLen;
        } else {
          
          width =
            wordLen*(ts.mWordSpacing + ts.mLetterSpacing + ts.mSpaceWidth);
        }
        aData->currentLine += width;
        if (GetStyleText()->WhiteSpaceIsSignificant())
          
          
          aData->trailingWhitespace = 0;
        else
          aData->trailingWhitespace += width;
      }
    } else {
      nscoord width;
      if (ts.mSmallCaps) {
        nsTextDimensions dimensions;
        
        
        
        aRenderingContext->SetTextRunRTL(PR_FALSE);
        MeasureSmallCapsText(aRenderingContext, ts, bp2, wordLen, PR_FALSE,
                             &dimensions);
        width = dimensions.width;
      } else {
        
        
        
        
        if (tx.TransformedTextIsAscii()) {
          
          aRenderingContext->SetTextRunRTL(PR_FALSE);
          aRenderingContext->GetWidth(bp1, wordLen, width);
        } else {
          
          
          width =
            nsLayoutUtils::GetStringWidth(this, aRenderingContext, bp2, wordLen);
        }
        width += ts.mLetterSpacing * wordLen;
      }

      aData->currentLine += width;
      aData->skipWhitespace = PR_FALSE;
      aData->trailingWhitespace = 0;
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

NS_IMETHODIMP
nsTextFrame::Reflow(nsPresContext*          aPresContext,
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

  mState &= ~TEXT_IS_END_OF_LINE;

  
  
  
  if (nsnull == aReflowState.mLineLayout) {
    
    aMetrics.width = 0;
    aMetrics.height = 0;
    aMetrics.ascent = 0;
#ifdef MOZ_MATHML
    if (NS_REFLOW_CALC_BOUNDING_METRICS & aMetrics.mFlags)
      aMetrics.mBoundingMetrics.Clear();
#endif
    return NS_OK;
  }

  
  PRInt32 startingOffset = 0;
  nsIFrame* prevInFlow = GetPrevInFlow();
  if (nsnull != prevInFlow) {
    nsTextFrame* prev = NS_STATIC_CAST(nsTextFrame*, prevInFlow);
    startingOffset = prev->mContentOffset + prev->mContentLength;

    
    
    
    if (startingOffset != mContentOffset) {
      mState &= ~TEXT_OPTIMIZE_RESIZE;
    }
  }
  nsLineLayout& lineLayout = *aReflowState.mLineLayout;
  nsTextStyle ts(aPresContext, *aReflowState.rendContext, mStyleContext);
  SetupTextRunDirection(aPresContext, aReflowState.rendContext);

  if ( (mContentLength > 0) && (mState & NS_FRAME_IS_BIDI) ) {
    startingOffset = mContentOffset;
  }

  if (aPresContext->BidiEnabled()) {
    nsCharType charType = eCharType_LeftToRight;
    PRUint32 hints = 0;
    aReflowState.rendContext->GetHints(hints);
    charType = (nsCharType)NS_PTR_TO_INT32(aPresContext->PropertyTable()->GetProperty(this, nsGkAtoms::charType));
    if ((eCharType_RightToLeftArabic == charType &&
        (hints & NS_RENDERING_HINT_ARABIC_SHAPING) == NS_RENDERING_HINT_ARABIC_SHAPING) ||
        (eCharType_RightToLeft == charType &&
        (hints & NS_RENDERING_HINT_BIDI_REORDERING) == NS_RENDERING_HINT_BIDI_REORDERING)) {
      
      aPresContext->SetIsBidiSystem(PR_TRUE);
    }
  }

  
  
  PRBool lastTimeWeSkippedLeadingWS = 0 != (mState & TEXT_SKIP_LEADING_WS);
  mState &= ~TEXT_REFLOW_FLAGS;
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

  PRBool wrapping = ts.mText->WhiteSpaceCanWrap();

  
  PRBool skipWhitespace = PR_FALSE;
  if (!ts.mPreformatted) {
    if (lineLayout.GetEndsInWhiteSpace()) {
      skipWhitespace = PR_TRUE;
    }
  }

  nscoord maxWidth = aReflowState.availableWidth;

  
  nsIDocument* doc = mContent->GetDocument();
  if (!doc) {
    NS_WARNING("Content has no document.");
    return NS_ERROR_FAILURE; 
  }
  PRBool forceArabicShaping = (ts.mSmallCaps ||
                               (0 != ts.mWordSpacing) ||
                               (0 != ts.mLetterSpacing) ||
                               ts.mJustifying);
  nsTextTransformer tx(aPresContext);
  
  
  
  nsresult rv = tx.Init(this, mContent, startingOffset, forceArabicShaping, !ts.mSmallCaps);
  if (NS_OK != rv) {
    return rv;
  }
  

  
  
  PRBool inWord = lineLayout.InWord();
  if (inWord) {
    mState |= TEXT_IN_WORD;
  }
  mState &= ~TEXT_FIRST_LETTER;

  PRInt32 column = lineLayout.GetColumn();
  PRInt32 prevColumn = mColumn;
  mColumn = column;
  PRBool measureText = PR_TRUE;
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  nscoord realWidth = mRect.width;
  if (mState & TEXT_TRIMMED_WS) {
    
    realWidth += ts.mSpaceWidth + ts.mWordSpacing + ts.mLetterSpacing;
  }
  if (!GetNextInFlow() &&
      (mState & TEXT_OPTIMIZE_RESIZE) &&
      lineLayout.GetForcedBreakPosition(GetContent()) == -1 &&
      (lastTimeWeSkippedLeadingWS == skipWhitespace) &&
      ((wrapping && (maxWidth >= realWidth)) ||
       (!wrapping && (prevColumn == column))) &&
#ifdef IBMBIDI
      (0 == (mState & NS_FRAME_IS_BIDI) ) &&
#endif 
      !ts.mJustifying) {
    
    
    measureText = PR_FALSE;
#ifdef NOISY_REFLOW
    printf("  => measureText=%s wrapping=%s skipWhitespace=%s",
           measureText ? "yes" : "no",
           wrapping ? "yes" : "no",
           skipWhitespace ? "yes" : "no");
    printf(" realWidth=%d maxWidth=%d\n",
           realWidth, maxWidth);
#endif
  }

  
  TextReflowData  textData(startingOffset, wrapping, skipWhitespace, 
                           measureText, inWord, lineLayout.GetFirstLetterStyleOK(),
                           lineLayout.LineIsBreakable(), PR_FALSE);
  
  
  
  if (ts.mFont->mSize)
    aStatus = MeasureText(aPresContext, aReflowState, tx, ts, textData);
  else {
    textData.mX = 0;
    textData.mAscent = 0;
    textData.mDescent = 0;
    aStatus = NS_FRAME_COMPLETE;
  }
  if (textData.mTrailingSpaceTrimmed)
    mState |= TEXT_TRIMMED_WS;
  else
    mState &= ~TEXT_TRIMMED_WS;

  if (tx.HasMultibyte()) {
    mState |= TEXT_HAS_MULTIBYTE;
  }

  
  aMetrics.width = textData.mX;
  if ((0 == textData.mX) && !ts.mPreformatted) {
    aMetrics.height = 0;
    aMetrics.ascent = 0;
  }
  else {
    aMetrics.ascent = textData.mAscent;
    aMetrics.height = textData.mAscent + textData.mDescent;
  }
  mAscent = aMetrics.ascent;

  
  mContentOffset = startingOffset;
  mContentLength = textData.mOffset - startingOffset;

  
  
  
  PRBool calcMathMLMetrics = PR_FALSE;
  nsAutoTextBuffer* textBufferPtr = nsnull;
#ifdef MOZ_MATHML
  nsAutoTextBuffer textBuffer;
  calcMathMLMetrics = (NS_REFLOW_CALC_BOUNDING_METRICS & aMetrics.mFlags) != 0;
  if (calcMathMLMetrics) {
    textBufferPtr = &textBuffer;
    
    mState |= TEXT_HAS_MULTIBYTE;
  }
#endif
  if (ts.mJustifying || calcMathMLMetrics) {
    PRIntn numJustifiableCharacter;
    PRInt32 textLength;

    
    

    
    
    
    
    PrepareUnicodeText(tx, nsnull, textBufferPtr, &textLength, PR_TRUE, &numJustifiableCharacter);
    lineLayout.SetTextJustificationWeights(numJustifiableCharacter, textLength - numJustifiableCharacter);

#ifdef MOZ_MATHML
    if (calcMathMLMetrics) {
      SetFontFromStyle(aReflowState.rendContext, mStyleContext);
      nsBoundingMetrics bm;
      rv = aReflowState.rendContext->GetBoundingMetrics(textBuffer.mBuffer, textLength, bm);
      if (NS_SUCCEEDED(rv))
        aMetrics.mBoundingMetrics = bm;
      else {
        
        aMetrics.mBoundingMetrics.ascent = aMetrics.ascent;
        aMetrics.mBoundingMetrics.descent = aMetrics.height - aMetrics.ascent;
        aMetrics.mBoundingMetrics.width = aMetrics.width;
        aMetrics.mBoundingMetrics.rightBearing = aMetrics.width;
      }
    }
#endif
  }

  nscoord maxFrameWidth  = mRect.width;
  nscoord maxFrameHeight = mRect.height;

  
  
  
  
  
  
  
  if (NS_FRAME_IS_COMPLETE(aStatus) && !NS_INLINE_IS_BREAK(aStatus)  && 
      (aMetrics.width <= maxWidth)) {
    mState |= TEXT_OPTIMIZE_RESIZE;
    mRect.width = aMetrics.width;
  }
  else {
    mState &= ~TEXT_OPTIMIZE_RESIZE;
  }
 
  
  
  
  
  

    
    
    
    
    
    nscoord onePixel = nsPresContext::CSSPixelsToAppUnits(1);

    maxFrameWidth  = PR_MAX(maxFrameWidth,  mRect.width) + onePixel; 
    maxFrameHeight = PR_MAX(maxFrameHeight, mRect.height);
    nsRect damage(0,0,maxFrameWidth,maxFrameHeight);
    Invalidate(damage);
  


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
  mState |= TEXT_IS_END_OF_LINE;

  
  
  
  if (mState & TEXT_TRIMMED_WS) {
    aDeltaWidth = 0;
    return NS_OK;
  }

  nscoord dw = 0;
  const nsStyleText* textStyle = GetStyleText();
  if (mContentLength &&
      (NS_STYLE_WHITESPACE_PRE != textStyle->mWhiteSpace) &&
      (NS_STYLE_WHITESPACE_MOZ_PRE_WRAP != textStyle->mWhiteSpace)) {

    
    const nsTextFragment* frag = mContent->GetText();
    if (frag) {
      PRInt32 lastCharIndex = mContentOffset + mContentLength - 1;
      if (lastCharIndex < frag->GetLength()) {
        PRUnichar ch = frag->CharAt(lastCharIndex);
        if (XP_IS_SPACE(ch)) {
          
          
          SetFontFromStyle(&aRC, mStyleContext);

          aRC.GetWidth(' ', dw);
          
          nsStyleUnit unit;
          unit = textStyle->mWordSpacing.GetUnit();
          if (eStyleUnit_Coord == unit) {
            dw += textStyle->mWordSpacing.GetCoordValue();
          }
          unit = textStyle->mLetterSpacing.GetUnit();
          if (eStyleUnit_Coord == unit) {
            dw += textStyle->mLetterSpacing.GetCoordValue();
          }
          aLastCharIsJustifiable = PR_TRUE;
        } else if (IsJustifiableCharacter(ch, IsChineseJapaneseLangGroup())) {
          aLastCharIsJustifiable = PR_TRUE;
        }
      }
    }
  }
#ifdef NOISY_TRIM
  ListTag(stdout);
  printf(": trim => %d\n", dw);
#endif
  if (0 != dw) {
    mState |= TEXT_TRIMMED_WS;
  }
  else {
    mState &= ~TEXT_TRIMMED_WS;
  }
  aDeltaWidth = dw;
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

  
  if (0 == mContentLength) {
    return;
  }
  PRInt32 fragOffset = mContentOffset;
  PRInt32 n = fragOffset + mContentLength;
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
    fprintf(out, " [view=%p]", NS_STATIC_CAST(void*, GetView()));
  }

  PRInt32 totalContentLength;
  nsAutoString tmp;
  ToCString(tmp, &totalContentLength);

  
  PRBool isComplete = (mContentOffset + mContentLength) == totalContentLength;
  fprintf(out, "[%d,%d,%c] ", 
          mContentOffset, mContentLength,
          isComplete ? 'T':'F');
  
  if (nsnull != mNextSibling) {
    fprintf(out, " next=%p", NS_STATIC_CAST(void*, mNextSibling));
  }
  nsIFrame* prevContinuation = GetPrevContinuation();
  if (nsnull != prevContinuation) {
    fprintf(out, " prev-continuation=%p", NS_STATIC_CAST(void*, prevContinuation));
  }
  if (nsnull != mNextContinuation) {
    fprintf(out, " next-continuation=%p", NS_STATIC_CAST(void*, mNextContinuation));
  }

  
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  if (0 != mState) {
    if (mState & NS_FRAME_SELECTED_CONTENT) {
      fprintf(out, " [state=%08x] SELECTED", mState);
    } else {
      fprintf(out, " [state=%08x]", mState);
    }
  }
  fprintf(out, " [content=%p]", NS_STATIC_CAST(void*, mContent));
  fprintf(out, " sc=%p", NS_STATIC_CAST(void*, mStyleContext));
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
  SetOffsets(aStart, aEnd);
}

void
nsTextFrame::SetOffsets(PRInt32 aStart, PRInt32 aEnd)
{
  mContentOffset = aStart;
  mContentLength = aEnd - aStart;
}





PRBool
nsTextFrame::HasTerminalNewline() const
{
  const nsTextFragment* frag = mContent->GetText();
  if (frag && mContentLength > 0) {
    PRUnichar ch = frag->CharAt(mContentOffset + mContentLength - 1);
    if (ch == '\n')
      return PR_TRUE;
  }
  return PR_FALSE;
}
