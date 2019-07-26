






#include "nsTextFrame.h"

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "mozilla/Likely.h"
#include "mozilla/MathAlgorithms.h"

#include "nsCOMPtr.h"
#include "nsBlockFrame.h"
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
#include "nsStyleStruct.h"
#include "nsStyleStructInlines.h"
#include "nsCoord.h"
#include "nsRenderingContext.h"
#include "nsIPresShell.h"
#include "nsITimer.h"
#include "nsTArray.h"
#include "nsIDocument.h"
#include "nsCSSPseudoElements.h"
#include "nsCSSFrameConstructor.h"
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
#include "nsUnicodeProperties.h"
#include "nsUnicharUtilCIID.h"

#include "nsTextFragment.h"
#include "nsGkAtoms.h"
#include "nsFrameSelection.h"
#include "nsISelection.h"
#include "nsIDOMRange.h"
#include "nsRange.h"
#include "nsCSSRendering.h"
#include "nsContentUtils.h"
#include "nsLineBreaker.h"
#include "nsIWordBreaker.h"
#include "nsGenericDOMDataNode.h"

#include "nsILineIterator.h"

#include "nsIServiceManager.h"
#include <algorithm>
#ifdef ACCESSIBILITY
#include "nsAccessibilityService.h"
#endif
#include "nsAutoPtr.h"

#include "nsBidiUtils.h"
#include "nsPrintfCString.h"

#include "gfxFont.h"
#include "gfxContext.h"
#include "gfxImageSurface.h"

#include "mozilla/dom/Element.h"
#include "mozilla/LookAndFeel.h"

#include "GeckoProfiler.h"

#ifdef DEBUG
#undef NOISY_BLINK
#undef NOISY_REFLOW
#undef NOISY_TRIM
#else
#undef NOISY_BLINK
#undef NOISY_REFLOW
#undef NOISY_TRIM
#endif

using namespace mozilla;
using namespace mozilla::dom;

struct TabWidth {
  TabWidth(uint32_t aOffset, uint32_t aWidth)
    : mOffset(aOffset), mWidth(float(aWidth))
  { }

  uint32_t mOffset; 
  float    mWidth;  
};

struct TabWidthStore {
  TabWidthStore(int32_t aValidForContentOffset)
    : mLimit(0)
    , mValidForContentOffset(aValidForContentOffset)
  { }

  
  
  
  void ApplySpacing(gfxTextRun::PropertyProvider::Spacing *aSpacing,
                    uint32_t aOffset, uint32_t aLength);

  
  
  
  uint32_t mLimit;
 
  
  int32_t mValidForContentOffset;

  
  nsTArray<TabWidth> mWidths;
};

void
TabWidthStore::ApplySpacing(gfxTextRun::PropertyProvider::Spacing *aSpacing,
                            uint32_t aOffset, uint32_t aLength)
{
  
  
  
  
  for (uint32_t i = 0; i < mWidths.Length(); ++i) {
    TabWidth& tw = mWidths[i];
    if (tw.mOffset < aOffset) {
      continue;
    }
    if (tw.mOffset - aOffset >= aLength) {
      break;
    }
    aSpacing[tw.mOffset - aOffset].mAfter += tw.mWidth;
  }
}

static void DestroyTabWidth(void* aPropertyValue)
{
  delete static_cast<TabWidthStore*>(aPropertyValue);
}

NS_DECLARE_FRAME_PROPERTY(TabWidthProperty, DestroyTabWidth)

NS_DECLARE_FRAME_PROPERTY(OffsetToFrameProperty, nullptr)


NS_DECLARE_FRAME_PROPERTY(UninflatedTextRunProperty, nullptr)

NS_DECLARE_FRAME_PROPERTY(FontSizeInflationProperty, nullptr)





#define TEXT_FIRST_LETTER    NS_FRAME_STATE_BIT(20)


#define TEXT_START_OF_LINE   NS_FRAME_STATE_BIT(21)


#define TEXT_END_OF_LINE     NS_FRAME_STATE_BIT(22)

#define TEXT_HYPHEN_BREAK    NS_FRAME_STATE_BIT(23)


#define TEXT_TRIMMED_TRAILING_WHITESPACE NS_FRAME_STATE_BIT(24)



#define TEXT_JUSTIFICATION_ENABLED       NS_FRAME_STATE_BIT(25)

#define TEXT_SELECTION_UNDERLINE_OVERFLOWED NS_FRAME_STATE_BIT(26)

#define TEXT_REFLOW_FLAGS    \
  (TEXT_FIRST_LETTER|TEXT_START_OF_LINE|TEXT_END_OF_LINE|TEXT_HYPHEN_BREAK| \
   TEXT_TRIMMED_TRAILING_WHITESPACE|TEXT_JUSTIFICATION_ENABLED| \
   TEXT_HAS_NONCOLLAPSED_CHARACTERS|TEXT_SELECTION_UNDERLINE_OVERFLOWED)



#define TEXT_IS_ONLY_WHITESPACE    NS_FRAME_STATE_BIT(27)

#define TEXT_ISNOT_ONLY_WHITESPACE NS_FRAME_STATE_BIT(28)

#define TEXT_WHITESPACE_FLAGS      (TEXT_IS_ONLY_WHITESPACE | \
                                    TEXT_ISNOT_ONLY_WHITESPACE)

#define TEXT_BLINK_ON              NS_FRAME_STATE_BIT(29)


#define TEXT_IN_TEXTRUN_USER_DATA  NS_FRAME_STATE_BIT(30)









#define TEXT_IN_UNINFLATED_TEXTRUN_USER_DATA NS_FRAME_STATE_BIT(60)







#define TEXT_STYLE_MATCHES_PREV_CONTINUATION NS_FRAME_STATE_BIT(62)


#define TEXT_IN_OFFSET_CACHE       NS_FRAME_STATE_BIT(63)







































struct TextRunMappedFlow {
  nsTextFrame* mStartFrame;
  int32_t      mDOMOffsetToBeforeTransformOffset;
  
  uint32_t     mContentLength;
};








struct TextRunUserData {
  TextRunMappedFlow* mMappedFlows;
  uint32_t           mMappedFlowCount;
  uint32_t           mLastFlowIndex;
};






class nsTextPaintStyle {
public:
  nsTextPaintStyle(nsTextFrame* aFrame);

  void SetResolveColors(bool aResolveColors) {
    NS_ASSERTION(mFrame->IsSVGText() || aResolveColors,
                 "must resolve colors is frame is not for SVG text");
    mResolveColors = aResolveColors;
  }

  nscolor GetTextColor();
  



  bool GetSelectionColors(nscolor* aForeColor,
                            nscolor* aBackColor);
  void GetHighlightColors(nscolor* aForeColor,
                          nscolor* aBackColor);
  void GetURLSecondaryColor(nscolor* aForeColor);
  void GetIMESelectionColors(int32_t  aIndex,
                             nscolor* aForeColor,
                             nscolor* aBackColor);
  
  bool GetSelectionUnderlineForPaint(int32_t  aIndex,
                                       nscolor* aLineColor,
                                       float*   aRelativeSize,
                                       uint8_t* aStyle);

  
  static bool GetSelectionUnderline(nsPresContext* aPresContext,
                                      int32_t aIndex,
                                      nscolor* aLineColor,
                                      float* aRelativeSize,
                                      uint8_t* aStyle);

  
  
  bool GetSelectionShadow(nsCSSShadowArray** aShadow);

  nsPresContext* PresContext() const { return mPresContext; }

  enum {
    eIndexRawInput = 0,
    eIndexSelRawText,
    eIndexConvText,
    eIndexSelConvText,
    eIndexSpellChecker
  };

  static int32_t GetUnderlineStyleIndexForSelectionType(int32_t aSelectionType)
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
  bool           mInitCommonColors;
  bool           mInitSelectionColorsAndShadow;
  bool           mResolveColors;

  

  int16_t      mSelectionStatus; 
  nscolor      mSelectionTextColor;
  nscolor      mSelectionBGColor;
  nsRefPtr<nsCSSShadowArray> mSelectionShadow;
  bool                       mHasSelectionShadow;

  

  int32_t mSufficientContrast;
  nscolor mFrameBackgroundColor;

  
  
  
  
  struct nsSelectionStyle {
    bool    mInit;
    nscolor mTextColor;
    nscolor mBGColor;
    nscolor mUnderlineColor;
    uint8_t mUnderlineStyle;
    float   mUnderlineRelativeSize;
  };
  nsSelectionStyle mSelectionStyle[5];

  
  void InitCommonColors();
  bool InitSelectionColorsAndShadow();

  nsSelectionStyle* GetSelectionStyle(int32_t aIndex);
  void InitSelectionStyle(int32_t aIndex);

  bool EnsureSufficientContrast(nscolor *aForeColor, nscolor *aBackColor);

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








static bool
ClearAllTextRunReferences(nsTextFrame* aFrame, gfxTextRun* aTextRun,
                          nsTextFrame* aStartContinuation,
                          nsFrameState aWhichTextRunState)
{
  NS_PRECONDITION(aFrame, "");
  NS_PRECONDITION(!aStartContinuation ||
                  (!aStartContinuation->GetTextRun(nsTextFrame::eInflated) ||
                   aStartContinuation->GetTextRun(nsTextFrame::eInflated) == aTextRun) ||
                  (!aStartContinuation->GetTextRun(nsTextFrame::eNotInflated) ||
                   aStartContinuation->GetTextRun(nsTextFrame::eNotInflated) == aTextRun),
                  "wrong aStartContinuation for this text run");

  if (!aStartContinuation || aStartContinuation == aFrame) {
    aFrame->RemoveStateBits(aWhichTextRunState);
  } else {
    do {
      NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame, "Bad frame");
      aFrame = static_cast<nsTextFrame*>(aFrame->GetNextContinuation());
    } while (aFrame && aFrame != aStartContinuation);
  }
  bool found = aStartContinuation == aFrame;
  while (aFrame) {
    NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame, "Bad frame");
    if (!aFrame->RemoveTextRun(aTextRun)) {
      break;
    }
    aFrame = static_cast<nsTextFrame*>(aFrame->GetNextContinuation());
  }
  NS_POSTCONDITION(!found || aStartContinuation, "how did we find null?");
  return found;
}











static void
UnhookTextRunFromFrames(gfxTextRun* aTextRun, nsTextFrame* aStartContinuation)
{
  if (!aTextRun->GetUserData())
    return;

  if (aTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW) {
    nsTextFrame* userDataFrame = static_cast<nsTextFrame*>(
      static_cast<nsIFrame*>(aTextRun->GetUserData()));
    nsFrameState whichTextRunState =
      userDataFrame->GetTextRun(nsTextFrame::eInflated) == aTextRun
        ? TEXT_IN_TEXTRUN_USER_DATA
        : TEXT_IN_UNINFLATED_TEXTRUN_USER_DATA;
    DebugOnly<bool> found =
      ClearAllTextRunReferences(userDataFrame, aTextRun,
                                aStartContinuation, whichTextRunState);
    NS_ASSERTION(!aStartContinuation || found,
                 "aStartContinuation wasn't found in simple flow text run");
    if (!(userDataFrame->GetStateBits() & whichTextRunState)) {
      aTextRun->SetUserData(nullptr);
    }
  } else {
    TextRunUserData* userData =
      static_cast<TextRunUserData*>(aTextRun->GetUserData());
    int32_t destroyFromIndex = aStartContinuation ? -1 : 0;
    for (uint32_t i = 0; i < userData->mMappedFlowCount; ++i) {
      nsTextFrame* userDataFrame = userData->mMappedFlows[i].mStartFrame;
      nsFrameState whichTextRunState =
        userDataFrame->GetTextRun(nsTextFrame::eInflated) == aTextRun
          ? TEXT_IN_TEXTRUN_USER_DATA
          : TEXT_IN_UNINFLATED_TEXTRUN_USER_DATA;
      bool found =
        ClearAllTextRunReferences(userDataFrame, aTextRun,
                                  aStartContinuation, whichTextRunState);
      if (found) {
        if (userDataFrame->GetStateBits() & whichTextRunState) {
          destroyFromIndex = i + 1;
        }
        else {
          destroyFromIndex = i;
        }
        aStartContinuation = nullptr;
      }
    }
    NS_ASSERTION(destroyFromIndex >= 0,
                 "aStartContinuation wasn't found in multi flow text run");
    if (destroyFromIndex == 0) {
      DestroyUserData(userData);
      aTextRun->SetUserData(nullptr);
    }
    else {
      userData->mMappedFlowCount = uint32_t(destroyFromIndex);
      if (userData->mLastFlowIndex >= uint32_t(destroyFromIndex)) {
        userData->mLastFlowIndex = uint32_t(destroyFromIndex) - 1;
      }
    }
  }
}

class FrameTextRunCache;

static FrameTextRunCache *gTextRuns = nullptr;




class FrameTextRunCache MOZ_FINAL : public nsExpirationTracker<gfxTextRun,3> {
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
  }

  
  virtual void NotifyExpired(gfxTextRun* aTextRun) {
    UnhookTextRunFromFrames(aTextRun, nullptr);
    RemoveFromCache(aTextRun);
    delete aTextRun;
  }
};



template<typename T>
gfxTextRun *
MakeTextRun(const T *aText, uint32_t aLength,
            gfxFontGroup *aFontGroup, const gfxFontGroup::Parameters* aParams,
            uint32_t aFlags)
{
    nsAutoPtr<gfxTextRun> textRun(aFontGroup->MakeTextRun(aText, aLength,
                                                          aParams, aFlags));
    if (!textRun) {
        return nullptr;
    }
    nsresult rv = gTextRuns->AddObject(textRun);
    if (NS_FAILED(rv)) {
        gTextRuns->RemoveFromCache(textRun);
        return nullptr;
    }
#ifdef NOISY_BIDI
    printf("Created textrun\n");
#endif
    return textRun.forget();
}

void
nsTextFrameTextRunCache::Init() {
    gTextRuns = new FrameTextRunCache();
}

void
nsTextFrameTextRunCache::Shutdown() {
    delete gTextRuns;
    gTextRuns = nullptr;
}

int32_t nsTextFrame::GetContentEnd() const {
  nsTextFrame* next = static_cast<nsTextFrame*>(GetNextContinuation());
  return next ? next->GetContentOffset() : mContent->GetText()->GetLength();
}

struct FlowLengthProperty {
  int32_t mStartOffset;
  
  
  int32_t mEndFlowOffset;

  static void Destroy(void* aObject, nsIAtom* aPropertyName,
                      void* aPropertyValue, void* aData)
  {
    delete static_cast<FlowLengthProperty*>(aPropertyValue);
  }
};

int32_t nsTextFrame::GetInFlowContentLength() {
  if (!(mState & NS_FRAME_IS_BIDI)) {
    return mContent->TextLength() - mContentOffset;
  }

  FlowLengthProperty* flowLength =
    static_cast<FlowLengthProperty*>(mContent->GetProperty(nsGkAtoms::flowlength));

  




  if (flowLength && 
      (flowLength->mStartOffset < mContentOffset ||
       (flowLength->mStartOffset == mContentOffset && GetContentEnd() > mContentOffset)) &&
      flowLength->mEndFlowOffset > mContentOffset) {
#ifdef DEBUG
    NS_ASSERTION(flowLength->mEndFlowOffset >= GetContentEnd(),
		 "frame crosses fixed continuation boundary");
#endif
    return flowLength->mEndFlowOffset - mContentOffset;
  }

  nsTextFrame* nextBidi = static_cast<nsTextFrame*>(GetLastInFlow()->GetNextContinuation());
  int32_t endFlow = nextBidi ? nextBidi->GetContentOffset() : mContent->TextLength();

  if (!flowLength) {
    flowLength = new FlowLengthProperty;
    if (NS_FAILED(mContent->SetProperty(nsGkAtoms::flowlength, flowLength,
                                        FlowLengthProperty::Destroy))) {
      delete flowLength;
      flowLength = nullptr;
    }
  }
  if (flowLength) {
    flowLength->mStartOffset = mContentOffset;
    flowLength->mEndFlowOffset = endFlow;
  }

  return endFlow - mContentOffset;
}






static bool IsSpaceCombiningSequenceTail(const nsTextFragment* aFrag, uint32_t aPos)
{
  NS_ASSERTION(aPos <= aFrag->GetLength(), "Bad offset");
  if (!aFrag->Is2b())
    return false;
  return nsTextFrameUtils::IsSpaceCombiningSequenceTail(
    aFrag->Get2b() + aPos, aFrag->GetLength() - aPos);
}


static bool IsCSSWordSpacingSpace(const nsTextFragment* aFrag,
                                    uint32_t aPos, const nsStyleText* aStyleText)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");

  PRUnichar ch = aFrag->CharAt(aPos);
  switch (ch) {
  case ' ':
  case CH_NBSP:
    return !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  case '\r':
  case '\t': return !aStyleText->WhiteSpaceIsSignificant();
  case '\n': return !aStyleText->NewlineIsSignificant() &&
                    !aStyleText->NewlineIsDiscarded();
  default: return false;
  }
}



static bool IsTrimmableSpace(const PRUnichar* aChars, uint32_t aLength)
{
  NS_ASSERTION(aLength > 0, "No text for IsSpace!");

  PRUnichar ch = *aChars;
  if (ch == ' ')
    return !nsTextFrameUtils::IsSpaceCombiningSequenceTail(aChars + 1, aLength - 1);
  return ch == '\t' || ch == '\f' || ch == '\n' || ch == '\r';
}



static bool IsTrimmableSpace(char aCh)
{
  return aCh == ' ' || aCh == '\t' || aCh == '\f' || aCh == '\n' || aCh == '\r';
}

static bool IsTrimmableSpace(const nsTextFragment* aFrag, uint32_t aPos,
                               const nsStyleText* aStyleText)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");

  switch (aFrag->CharAt(aPos)) {
  case ' ': return !aStyleText->WhiteSpaceIsSignificant() &&
                   !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  case '\n': return !aStyleText->NewlineIsSignificant() &&
                    !aStyleText->NewlineIsDiscarded();
  case '\t':
  case '\r':
  case '\f': return !aStyleText->WhiteSpaceIsSignificant();
  default: return false;
  }
}

static bool IsSelectionSpace(const nsTextFragment* aFrag, uint32_t aPos)
{
  NS_ASSERTION(aPos < aFrag->GetLength(), "No text for IsSpace!");
  PRUnichar ch = aFrag->CharAt(aPos);
  if (ch == ' ' || ch == CH_NBSP)
    return !IsSpaceCombiningSequenceTail(aFrag, aPos + 1);
  return ch == '\t' || ch == '\n' || ch == '\f' || ch == '\r';
}






static uint32_t
GetTrimmableWhitespaceCount(const nsTextFragment* aFrag,
                            int32_t aStartOffset, int32_t aLength,
                            int32_t aDirection)
{
  int32_t count = 0;
  if (aFrag->Is2b()) {
    const PRUnichar* str = aFrag->Get2b() + aStartOffset;
    int32_t fragLen = aFrag->GetLength() - aStartOffset;
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

static bool
IsAllWhitespace(const nsTextFragment* aFrag, bool aAllowNewline)
{
  if (aFrag->Is2b())
    return false;
  int32_t len = aFrag->GetLength();
  const char* str = aFrag->Get1b();
  for (int32_t i = 0; i < len; ++i) {
    char ch = str[i];
    if (ch == ' ' || ch == '\t' || ch == '\r' || (ch == '\n' && aAllowNewline))
      continue;
    return false;
  }
  return true;
}

static bool
IsAllNewlines(const nsTextFragment* aFrag)
{
  if (aFrag->Is2b())
    return false;
  int32_t len = aFrag->GetLength();
  const char* str = aFrag->Get1b();
  for (int32_t i = 0; i < len; ++i) {
    char ch = str[i];
    if (ch != '\n')
      return false;
  }
  return true;
}










class BuildTextRunsScanner {
public:
  BuildTextRunsScanner(nsPresContext* aPresContext, gfxContext* aContext,
      nsIFrame* aLineContainer, nsTextFrame::TextRunType aWhichTextRun) :
    mCurrentFramesAllSameTextRun(nullptr),
    mContext(aContext),
    mLineContainer(aLineContainer),
    mBidiEnabled(aPresContext->BidiEnabled()),
    mSkipIncompleteTextRuns(false),
    mWhichTextRun(aWhichTextRun),
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
    mStartOfLine = true;
    mCanStopOnThisLine = false;
  }
  void SetSkipIncompleteTextRuns(bool aSkip) {
    mSkipIncompleteTextRuns = aSkip;
  }
  void SetCommonAncestorWithLastFrame(nsIFrame* aFrame) {
    mCommonAncestorWithLastFrame = aFrame;
  }
  bool CanStopOnThisLine() {
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
  bool IsTextRunValidForMappedFlows(gfxTextRun* aTextRun);
  void FlushFrames(bool aFlushLineBreaks, bool aSuppressTrailingBreak);
  void FlushLineBreaks(gfxTextRun* aTrailingTextRun);
  void ResetRunInfo() {
    mLastFrame = nullptr;
    mMappedFlows.Clear();
    mLineBreakBeforeFrames.Clear();
    mMaxTextLength = 0;
    mDoubleByteText = false;
  }
  void AccumulateRunInfo(nsTextFrame* aFrame);
  




  gfxTextRun* BuildTextRunForFrames(void* aTextBuffer);
  bool SetupLineBreakerContext(gfxTextRun *aTextRun);
  void AssignTextRun(gfxTextRun* aTextRun, float aInflation);
  nsTextFrame* GetNextBreakBeforeFrame(uint32_t* aIndex);
  enum SetupBreakSinksFlags {
    SBS_DOUBLE_BYTE =      (1 << 0),
    SBS_EXISTING_TEXTRUN = (1 << 1),
    SBS_SUPPRESS_SINK    = (1 << 2)
  };
  void SetupBreakSinksForTextRun(gfxTextRun* aTextRun,
                                 const void* aTextPtr,
                                 uint32_t    aFlags);
  struct FindBoundaryState {
    nsIFrame*    mStopAtFrame;
    nsTextFrame* mFirstTextFrame;
    nsTextFrame* mLastTextFrame;
    bool mSeenTextRunBoundaryOnLaterLine;
    bool mSeenTextRunBoundaryOnThisLine;
    bool mSeenSpaceForLineBreakingOnThisLine;
  };
  enum FindBoundaryResult {
    FB_CONTINUE,
    FB_STOPPED_AT_STOP_FRAME,
    FB_FOUND_VALID_TEXTRUN_BOUNDARY
  };
  FindBoundaryResult FindBoundaries(nsIFrame* aFrame, FindBoundaryState* aState);

  bool ContinueTextRunAcrossFrames(nsTextFrame* aFrame1, nsTextFrame* aFrame2);

  
  
  
  struct MappedFlow {
    nsTextFrame* mStartFrame;
    nsTextFrame* mEndFrame;
    
    
    
    
    
    nsIFrame*    mAncestorControllingInitialBreak;
    
    int32_t GetContentEnd() {
      return mEndFrame ? mEndFrame->GetContentOffset()
          : mStartFrame->GetContent()->GetText()->GetLength();
    }
  };

  class BreakSink MOZ_FINAL : public nsILineBreakSink {
  public:
    BreakSink(gfxTextRun* aTextRun, gfxContext* aContext, uint32_t aOffsetIntoTextRun,
              bool aExistingTextRun) :
                mTextRun(aTextRun), mContext(aContext),
                mOffsetIntoTextRun(aOffsetIntoTextRun),
                mChangedBreaks(false), mExistingTextRun(aExistingTextRun) {}

    virtual void SetBreaks(uint32_t aOffset, uint32_t aLength,
                           uint8_t* aBreakBefore) {
      if (mTextRun->SetPotentialLineBreaks(aOffset + mOffsetIntoTextRun, aLength,
                                           aBreakBefore, mContext)) {
        mChangedBreaks = true;
        
        mTextRun->ClearFlagBits(nsTextFrameUtils::TEXT_NO_BREAKS);
      }
    }
    
    virtual void SetCapitalization(uint32_t aOffset, uint32_t aLength,
                                   bool* aCapitalize) {
      NS_ASSERTION(mTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_TRANSFORMED,
                   "Text run should be transformed!");
      nsTransformedTextRun* transformedTextRun =
        static_cast<nsTransformedTextRun*>(mTextRun);
      transformedTextRun->SetCapitalization(aOffset + mOffsetIntoTextRun, aLength,
                                            aCapitalize, mContext);
    }

    void Finish() {
      NS_ASSERTION(!(mTextRun->GetFlags() &
                     (gfxTextRunFactory::TEXT_UNUSED_FLAGS |
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
    uint32_t     mOffsetIntoTextRun;
    bool mChangedBreaks;
    bool mExistingTextRun;
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
  
  
  uint32_t                      mMaxTextLength;
  bool                          mDoubleByteText;
  bool                          mBidiEnabled;
  bool                          mStartOfLine;
  bool                          mSkipIncompleteTextRuns;
  bool                          mCanStopOnThisLine;
  nsTextFrame::TextRunType      mWhichTextRun;
  uint8_t                       mNextRunContextInfo;
  uint8_t                       mCurrentRunContextInfo;
};

static nsIFrame*
FindLineContainer(nsIFrame* aFrame)
{
  while (aFrame && aFrame->CanContinueTextRun()) {
    aFrame = aFrame->GetParent();
  }
  return aFrame;
}

static bool
IsLineBreakingWhiteSpace(PRUnichar aChar)
{
  
  
  
  
  
  
  return nsLineBreaker::IsSpace(aChar) || aChar == 0x0A;
}

static bool
TextContainsLineBreakerWhiteSpace(const void* aText, uint32_t aLength,
                                  bool aIsDoubleByte)
{
  if (aIsDoubleByte) {
    const PRUnichar* chars = static_cast<const PRUnichar*>(aText);
    for (uint32_t i = 0; i < aLength; ++i) {
      if (IsLineBreakingWhiteSpace(chars[i]))
        return true;
    }
    return false;
  } else {
    const uint8_t* chars = static_cast<const uint8_t*>(aText);
    for (uint32_t i = 0; i < aLength; ++i) {
      if (IsLineBreakingWhiteSpace(chars[i]))
        return true;
    }
    return false;
  }
}

struct FrameTextTraversal {
  
  
  nsIFrame*    mFrameToScan;
  
  nsIFrame*    mOverflowFrameToScan;
  
  bool mScanSiblings;

  
  
  bool mLineBreakerCanCrossFrameBoundary;
  bool mTextRunCanCrossFrameBoundary;

  nsIFrame* NextFrameToScan() {
    nsIFrame* f;
    if (mFrameToScan) {
      f = mFrameToScan;
      mFrameToScan = mScanSiblings ? f->GetNextSibling() : nullptr;
    } else if (mOverflowFrameToScan) {
      f = mOverflowFrameToScan;
      mOverflowFrameToScan = mScanSiblings ? f->GetNextSibling() : nullptr;
    } else {
      f = nullptr;
    }
    return f;
  }
};

static FrameTextTraversal
CanTextCrossFrameBoundary(nsIFrame* aFrame, nsIAtom* aType)
{
  NS_ASSERTION(aType == aFrame->GetType(), "Wrong type");

  FrameTextTraversal result;

  bool continuesTextRun = aFrame->CanContinueTextRun();
  if (aType == nsGkAtoms::placeholderFrame) {
    
    
    result.mLineBreakerCanCrossFrameBoundary = true;
    result.mOverflowFrameToScan = nullptr;
    if (continuesTextRun) {
      
      
      
      
      
      result.mFrameToScan =
        (static_cast<nsPlaceholderFrame*>(aFrame))->GetOutOfFlowFrame();
      result.mScanSiblings = false;
      result.mTextRunCanCrossFrameBoundary = false;
    } else {
      result.mFrameToScan = nullptr;
      result.mTextRunCanCrossFrameBoundary = true;
    }
  } else {
    if (continuesTextRun) {
      result.mFrameToScan = aFrame->GetFirstPrincipalChild();
      result.mOverflowFrameToScan =
        aFrame->GetFirstChild(nsIFrame::kOverflowList);
      NS_WARN_IF_FALSE(!result.mOverflowFrameToScan,
                       "Scanning overflow inline frames is something we should avoid");
      result.mScanSiblings = true;
      result.mTextRunCanCrossFrameBoundary = true;
      result.mLineBreakerCanCrossFrameBoundary = true;
    } else {
      result.mFrameToScan = nullptr;
      result.mOverflowFrameToScan = nullptr;
      result.mTextRunCanCrossFrameBoundary = false;
      result.mLineBreakerCanCrossFrameBoundary = false;
    }
  }    
  return result;
}

BuildTextRunsScanner::FindBoundaryResult
BuildTextRunsScanner::FindBoundaries(nsIFrame* aFrame, FindBoundaryState* aState)
{
  nsIAtom* frameType = aFrame->GetType();
  nsTextFrame* textFrame = frameType == nsGkAtoms::textFrame
    ? static_cast<nsTextFrame*>(aFrame) : nullptr;
  if (textFrame) {
    if (aState->mLastTextFrame &&
        textFrame != aState->mLastTextFrame->GetNextInFlow() &&
        !ContinueTextRunAcrossFrames(aState->mLastTextFrame, textFrame)) {
      aState->mSeenTextRunBoundaryOnThisLine = true;
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
      uint32_t start = textFrame->GetContentOffset();
      const void* text = frag->Is2b()
          ? static_cast<const void*>(frag->Get2b() + start)
          : static_cast<const void*>(frag->Get1b() + start);
      if (TextContainsLineBreakerWhiteSpace(text, textFrame->GetContentLength(),
                                            frag->Is2b())) {
        aState->mSeenSpaceForLineBreakingOnThisLine = true;
        if (aState->mSeenTextRunBoundaryOnLaterLine)
          return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
      }
    }
    return FB_CONTINUE; 
  }

  FrameTextTraversal traversal =
    CanTextCrossFrameBoundary(aFrame, frameType);
  if (!traversal.mTextRunCanCrossFrameBoundary) {
    aState->mSeenTextRunBoundaryOnThisLine = true;
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
    aState->mSeenTextRunBoundaryOnThisLine = true;
    if (aState->mSeenSpaceForLineBreakingOnThisLine)
      return FB_FOUND_VALID_TEXTRUN_BOUNDARY;
  }

  return FB_CONTINUE;
}



#define NUM_LINES_TO_BUILD_TEXT_RUNS 200















static void
BuildTextRuns(gfxContext* aContext, nsTextFrame* aForFrame,
              nsIFrame* aLineContainer,
              const nsLineList::iterator* aForFrameLine,
              nsTextFrame::TextRunType aWhichTextRun)
{
  NS_ASSERTION(aForFrame || aLineContainer,
               "One of aForFrame or aLineContainer must be set!");
  NS_ASSERTION(!aForFrameLine || aLineContainer,
               "line but no line container");
  
  nsIFrame* lineContainerChild = aForFrame;
  if (!aLineContainer) {
    if (aForFrame->IsFloatingFirstLetterChild()) {
      lineContainerChild = aForFrame->PresContext()->PresShell()->
        GetPlaceholderFrameFor(aForFrame->GetParent());
    }
    aLineContainer = FindLineContainer(lineContainerChild);
  } else {
    NS_ASSERTION(!aForFrame ||
                 (aLineContainer == FindLineContainer(aForFrame) ||
                  (aLineContainer->GetType() == nsGkAtoms::letterFrame &&
                   aLineContainer->IsFloating())),
                 "Wrong line container hint");
  }

  nsPresContext* presContext = aLineContainer->PresContext();
  BuildTextRunsScanner scanner(presContext, aContext, aLineContainer,
                               aWhichTextRun);

  nsBlockFrame* block = nsLayoutUtils::GetAsBlock(aLineContainer);

  if (!block) {
    NS_ASSERTION(!aLineContainer->GetPrevInFlow() && !aLineContainer->GetNextInFlow(),
                 "Breakable non-block line containers not supported");
    
    
    scanner.SetAtStartOfLine();
    scanner.SetCommonAncestorWithLastFrame(nullptr);
    nsIFrame* child = aLineContainer->GetFirstPrincipalChild();
    while (child) {
      scanner.ScanFrame(child);
      child = child->GetNextSibling();
    }
    
    scanner.SetAtStartOfLine();
    scanner.FlushFrames(true, false);
    return;
  }

  

  bool isValid = true;
  nsBlockInFlowLineIterator backIterator(block, &isValid);
  if (aForFrameLine) {
    backIterator = nsBlockInFlowLineIterator(block, *aForFrameLine);
  } else {
    backIterator = nsBlockInFlowLineIterator(block, lineContainerChild, &isValid);
    NS_ASSERTION(isValid, "aForFrame not found in block, someone lied to us");
    NS_ASSERTION(backIterator.GetContainer() == block,
                 "Someone lied to us about the block");
  }
  nsBlockFrame::line_iterator startLine = backIterator.GetLine();

  
  
  
  
  
  
  
  
  
  
  
  
  
  nsBlockInFlowLineIterator forwardIterator = backIterator;
  nsIFrame* stopAtFrame = lineContainerChild;
  nsTextFrame* nextLineFirstTextFrame = nullptr;
  bool seenTextRunBoundaryOnLaterLine = false;
  bool mayBeginInTextRun = true;
  while (true) {
    forwardIterator = backIterator;
    nsBlockFrame::line_iterator line = backIterator.GetLine();
    if (!backIterator.Prev() || backIterator.GetLine()->IsBlock()) {
      mayBeginInTextRun = false;
      break;
    }

    BuildTextRunsScanner::FindBoundaryState state = { stopAtFrame, nullptr, nullptr,
      bool(seenTextRunBoundaryOnLaterLine), false, false };
    nsIFrame* child = line->mFirstChild;
    bool foundBoundary = false;
    for (int32_t i = line->GetChildCount() - 1; i >= 0; --i) {
      BuildTextRunsScanner::FindBoundaryResult result =
          scanner.FindBoundaries(child, &state);
      if (result == BuildTextRunsScanner::FB_FOUND_VALID_TEXTRUN_BOUNDARY) {
        foundBoundary = true;
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
      seenTextRunBoundaryOnLaterLine = true;
    } else if (state.mSeenTextRunBoundaryOnThisLine) {
      seenTextRunBoundaryOnLaterLine = true;
    }
    stopAtFrame = nullptr;
    if (state.mFirstTextFrame) {
      nextLineFirstTextFrame = state.mFirstTextFrame;
    }
  }
  scanner.SetSkipIncompleteTextRuns(mayBeginInTextRun);

  
  
  
  
  bool seenStartLine = false;
  uint32_t linesAfterStartLine = 0;
  do {
    nsBlockFrame::line_iterator line = forwardIterator.GetLine();
    if (line->IsBlock())
      break;
    line->SetInvalidateTextRuns(false);
    scanner.SetAtStartOfLine();
    scanner.SetCommonAncestorWithLastFrame(nullptr);
    nsIFrame* child = line->mFirstChild;
    for (int32_t i = line->GetChildCount() - 1; i >= 0; --i) {
      scanner.ScanFrame(child);
      child = child->GetNextSibling();
    }
    if (line.get() == startLine.get()) {
      seenStartLine = true;
    }
    if (seenStartLine) {
      ++linesAfterStartLine;
      if (linesAfterStartLine >= NUM_LINES_TO_BUILD_TEXT_RUNS && scanner.CanStopOnThisLine()) {
        
        
        
        
        
        scanner.FlushLineBreaks(nullptr);
        
        
        scanner.ResetRunInfo();
        return;
      }
    }
  } while (forwardIterator.Next());

  
  scanner.SetAtStartOfLine();
  scanner.FlushFrames(true, false);
}

static PRUnichar*
ExpandBuffer(PRUnichar* aDest, uint8_t* aSrc, uint32_t aCount)
{
  while (aCount) {
    *aDest = *aSrc;
    ++aDest;
    ++aSrc;
    --aCount;
  }
  return aDest;
}

bool BuildTextRunsScanner::IsTextRunValidForMappedFlows(gfxTextRun* aTextRun)
{
  if (aTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW)
    return mMappedFlows.Length() == 1 &&
      mMappedFlows[0].mStartFrame == static_cast<nsTextFrame*>(aTextRun->GetUserData()) &&
      mMappedFlows[0].mEndFrame == nullptr;

  TextRunUserData* userData = static_cast<TextRunUserData*>(aTextRun->GetUserData());
  if (userData->mMappedFlowCount != mMappedFlows.Length())
    return false;
  for (uint32_t i = 0; i < mMappedFlows.Length(); ++i) {
    if (userData->mMappedFlows[i].mStartFrame != mMappedFlows[i].mStartFrame ||
        int32_t(userData->mMappedFlows[i].mContentLength) !=
            mMappedFlows[i].GetContentEnd() - mMappedFlows[i].mStartFrame->GetContentOffset())
      return false;
  }
  return true;
}





void BuildTextRunsScanner::FlushFrames(bool aFlushLineBreaks, bool aSuppressTrailingBreak)
{
  gfxTextRun* textRun = nullptr;
  if (!mMappedFlows.IsEmpty()) {
    if (!mSkipIncompleteTextRuns && mCurrentFramesAllSameTextRun &&
        ((mCurrentFramesAllSameTextRun->GetFlags() & nsTextFrameUtils::TEXT_INCOMING_WHITESPACE) != 0) ==
        ((mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_WHITESPACE) != 0) &&
        ((mCurrentFramesAllSameTextRun->GetFlags() & gfxTextRunFactory::TEXT_INCOMING_ARABICCHAR) != 0) ==
        ((mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_ARABICCHAR) != 0) &&
        IsTextRunValidForMappedFlows(mCurrentFramesAllSameTextRun)) {
      
      textRun = mCurrentFramesAllSameTextRun;

      
      if (!SetupLineBreakerContext(textRun)) {
        return;
      }
 
      
      mNextRunContextInfo = nsTextFrameUtils::INCOMING_NONE;
      if (textRun->GetFlags() & nsTextFrameUtils::TEXT_TRAILING_WHITESPACE) {
        mNextRunContextInfo |= nsTextFrameUtils::INCOMING_WHITESPACE;
      }
      if (textRun->GetFlags() & gfxTextRunFactory::TEXT_TRAILING_ARABICCHAR) {
        mNextRunContextInfo |= nsTextFrameUtils::INCOMING_ARABICCHAR;
      }
    } else {
      AutoFallibleTArray<uint8_t,BIG_TEXT_NODE_SIZE> buffer;
      uint32_t bufferSize = mMaxTextLength*(mDoubleByteText ? 2 : 1);
      if (bufferSize < mMaxTextLength || bufferSize == UINT32_MAX ||
          !buffer.AppendElements(bufferSize)) {
        return;
      }
      textRun = BuildTextRunForFrames(buffer.Elements());
    }
  }

  if (aFlushLineBreaks) {
    FlushLineBreaks(aSuppressTrailingBreak ? nullptr : textRun);
  }

  mCanStopOnThisLine = true;
  ResetRunInfo();
}

void BuildTextRunsScanner::FlushLineBreaks(gfxTextRun* aTrailingTextRun)
{
  bool trailingLineBreak;
  nsresult rv = mLineBreaker.Reset(&trailingLineBreak);
  
  
  
  if (NS_SUCCEEDED(rv) && trailingLineBreak && aTrailingTextRun) {
    aTrailingTextRun->SetFlagBits(nsTextFrameUtils::TEXT_HAS_TRAILING_BREAK);
  }

  for (uint32_t i = 0; i < mBreakSinks.Length(); ++i) {
    if (!mBreakSinks[i]->mExistingTextRun || mBreakSinks[i]->mChangedBreaks) {
      
      
    }
    mBreakSinks[i]->Finish();
  }
  mBreakSinks.Clear();

  for (uint32_t i = 0; i < mTextRunsToDelete.Length(); ++i) {
    gfxTextRun* deleteTextRun = mTextRunsToDelete[i];
    gTextRuns->RemoveFromCache(deleteTextRun);
    delete deleteTextRun;
  }
  mTextRunsToDelete.Clear();
}

void BuildTextRunsScanner::AccumulateRunInfo(nsTextFrame* aFrame)
{
  if (mMaxTextLength != UINT32_MAX) {
    NS_ASSERTION(mMaxTextLength < UINT32_MAX - aFrame->GetContentLength(), "integer overflow");
    if (mMaxTextLength >= UINT32_MAX - aFrame->GetContentLength()) {
      mMaxTextLength = UINT32_MAX;
    } else {
      mMaxTextLength += aFrame->GetContentLength();
    }
  }
  mDoubleByteText |= aFrame->GetContent()->GetText()->Is2b();
  mLastFrame = aFrame;
  mCommonAncestorWithLastFrame = aFrame->GetParent();

  MappedFlow* mappedFlow = &mMappedFlows[mMappedFlows.Length() - 1];
  NS_ASSERTION(mappedFlow->mStartFrame == aFrame ||
               mappedFlow->GetContentEnd() == aFrame->GetContentOffset(),
               "Overlapping or discontiguous frames => BAD");
  mappedFlow->mEndFrame = static_cast<nsTextFrame*>(aFrame->GetNextContinuation());
  if (mCurrentFramesAllSameTextRun != aFrame->GetTextRun(mWhichTextRun)) {
    mCurrentFramesAllSameTextRun = nullptr;
  }

  if (mStartOfLine) {
    mLineBreakBeforeFrames.AppendElement(aFrame);
    mStartOfLine = false;
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

static bool
HasTerminalNewline(const nsTextFrame* aFrame)
{
  if (aFrame->GetContentLength() == 0)
    return false;
  const nsTextFragment* frag = aFrame->GetContent()->GetText();
  return frag->CharAt(aFrame->GetContentEnd() - 1) == '\n';
}

static nscoord
LetterSpacing(nsIFrame* aFrame, const nsStyleText* aStyleText = nullptr)
{
  if (aFrame->IsSVGText()) {
    return 0;
  }
  if (!aStyleText) {
    aStyleText = aFrame->StyleText();
  }
  return StyleToCoord(aStyleText->mLetterSpacing);
}

static nscoord
WordSpacing(nsIFrame* aFrame, const nsStyleText* aStyleText = nullptr)
{
  if (aFrame->IsSVGText()) {
    return 0;
  }
  if (!aStyleText) {
    aStyleText = aFrame->StyleText();
  }
  return aStyleText->mWordSpacing;
}

bool
BuildTextRunsScanner::ContinueTextRunAcrossFrames(nsTextFrame* aFrame1, nsTextFrame* aFrame2)
{
  
  
  
  
  
  
  if (mBidiEnabled &&
      (NS_GET_EMBEDDING_LEVEL(aFrame1) != NS_GET_EMBEDDING_LEVEL(aFrame2) ||
       NS_GET_PARAGRAPH_DEPTH(aFrame1) != NS_GET_PARAGRAPH_DEPTH(aFrame2)))
    return false;

  nsStyleContext* sc1 = aFrame1->StyleContext();
  const nsStyleText* textStyle1 = sc1->StyleText();
  
  
  
  
  
  
  if (textStyle1->NewlineIsSignificant() && HasTerminalNewline(aFrame1))
    return false;

  if (aFrame1->GetContent() == aFrame2->GetContent() &&
      aFrame1->GetNextInFlow() != aFrame2) {
    
    
    
    
    
    
    return false;
  }

  nsStyleContext* sc2 = aFrame2->StyleContext();
  const nsStyleText* textStyle2 = sc2->StyleText();
  if (sc1 == sc2)
    return true;

  const nsStyleFont* fontStyle1 = sc1->StyleFont();
  const nsStyleFont* fontStyle2 = sc2->StyleFont();
  nscoord letterSpacing1 = LetterSpacing(aFrame1);
  nscoord letterSpacing2 = LetterSpacing(aFrame2);
  return fontStyle1->mFont.BaseEquals(fontStyle2->mFont) &&
    sc1->StyleFont()->mLanguage == sc2->StyleFont()->mLanguage &&
    textStyle1->mTextTransform == textStyle2->mTextTransform &&
    nsLayoutUtils::GetTextRunFlagsForStyle(sc1, fontStyle1, letterSpacing1) ==
      nsLayoutUtils::GetTextRunFlagsForStyle(sc2, fontStyle2, letterSpacing2);
}

void BuildTextRunsScanner::ScanFrame(nsIFrame* aFrame)
{
  
  if (mMappedFlows.Length() > 0) {
    MappedFlow* mappedFlow = &mMappedFlows[mMappedFlows.Length() - 1];
    if (mappedFlow->mEndFrame == aFrame &&
        (aFrame->GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION)) {
      NS_ASSERTION(aFrame->GetType() == nsGkAtoms::textFrame,
                   "Flow-sibling of a text frame is not a text frame?");

      
      
      
      if (mLastFrame->StyleContext() == aFrame->StyleContext() &&
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
        FlushFrames(false, false);
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
      mCurrentFramesAllSameTextRun = frame->GetTextRun(mWhichTextRun);
      mCurrentRunContextInfo = mNextRunContextInfo;
    }
    return;
  }

  FrameTextTraversal traversal =
    CanTextCrossFrameBoundary(aFrame, frameType);
  bool isBR = frameType == nsGkAtoms::brFrame;
  if (!traversal.mLineBreakerCanCrossFrameBoundary) {
    
    
    FlushFrames(true, isBR);
    mCommonAncestorWithLastFrame = aFrame;
    mNextRunContextInfo &= ~nsTextFrameUtils::INCOMING_WHITESPACE;
    mStartOfLine = false;
  } else if (!traversal.mTextRunCanCrossFrameBoundary) {
    FlushFrames(false, false);
  }

  for (nsIFrame* f = traversal.NextFrameToScan(); f;
       f = traversal.NextFrameToScan()) {
    ScanFrame(f);
  }

  if (!traversal.mLineBreakerCanCrossFrameBoundary) {
    
    
    FlushFrames(true, isBR);
    mCommonAncestorWithLastFrame = aFrame;
    mNextRunContextInfo &= ~nsTextFrameUtils::INCOMING_WHITESPACE;
  } else if (!traversal.mTextRunCanCrossFrameBoundary) {
    FlushFrames(false, false);
  }

  LiftCommonAncestorWithLastFrameToParent(aFrame->GetParent());
}

nsTextFrame*
BuildTextRunsScanner::GetNextBreakBeforeFrame(uint32_t* aIndex)
{
  uint32_t index = *aIndex;
  if (index >= mLineBreakBeforeFrames.Length())
    return nullptr;
  *aIndex = index + 1;
  return static_cast<nsTextFrame*>(mLineBreakBeforeFrames.ElementAt(index));
}

static uint32_t
GetSpacingFlags(nscoord spacing)
{
  return spacing ? gfxTextRunFactory::TEXT_ENABLE_SPACING : 0;
}

static gfxFontGroup*
GetFontGroupForFrame(nsIFrame* aFrame, float aFontSizeInflation,
                     nsFontMetrics** aOutFontMetrics = nullptr)
{
  if (aOutFontMetrics)
    *aOutFontMetrics = nullptr;

  nsRefPtr<nsFontMetrics> metrics;
  nsLayoutUtils::GetFontMetricsForFrame(aFrame, getter_AddRefs(metrics),
                                        aFontSizeInflation);

  if (!metrics)
    return nullptr;

  if (aOutFontMetrics) {
    *aOutFontMetrics = metrics;
    NS_ADDREF(*aOutFontMetrics);
  }
  
  
  
  
  return metrics->GetThebesFontGroup();
}

static already_AddRefed<gfxContext>
GetReferenceRenderingContext(nsTextFrame* aTextFrame, nsRenderingContext* aRC)
{
  nsRefPtr<nsRenderingContext> tmp = aRC;
  if (!tmp) {
    tmp = aTextFrame->PresContext()->PresShell()->GetReferenceRenderingContext();
    if (!tmp)
      return nullptr;
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
    ctx = GetReferenceRenderingContext(aTextFrame, nullptr);
  }
  if (!ctx)
    return nullptr;

  gfxFontGroup* fontGroup = aTextRun->GetFontGroup();
  uint32_t flags = gfxFontGroup::TEXT_IS_PERSISTENT;

  
  
  
  static const PRUnichar unicodeHyphen = 0x2010;
  gfxFont *font = fontGroup->GetFontAt(0);
  if (font && font->HasCharacter(unicodeHyphen)) {
    return fontGroup->MakeTextRun(&unicodeHyphen, 1, ctx,
                                  aTextRun->GetAppUnitsPerDevUnit(), flags);
  }

  static const uint8_t dash = '-';
  return fontGroup->MakeTextRun(&dash, 1, ctx,
                                aTextRun->GetAppUnitsPerDevUnit(), flags);
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
PR_STATIC_ASSERT(NS_STYLE_WHITESPACE_PRE_DISCARD_NEWLINES == 5);

static const nsTextFrameUtils::CompressionMode CSSWhitespaceToCompressionMode[] =
{
  nsTextFrameUtils::COMPRESS_WHITESPACE_NEWLINE, 
  nsTextFrameUtils::COMPRESS_NONE,               
  nsTextFrameUtils::COMPRESS_WHITESPACE_NEWLINE, 
  nsTextFrameUtils::COMPRESS_NONE,               
  nsTextFrameUtils::COMPRESS_WHITESPACE,         
  nsTextFrameUtils::DISCARD_NEWLINE              
};

gfxTextRun*
BuildTextRunsScanner::BuildTextRunForFrames(void* aTextBuffer)
{
  gfxSkipCharsBuilder builder;

  const void* textPtr = aTextBuffer;
  bool anySmallcapsStyle = false;
  bool anyTextTransformStyle = false;
  uint32_t textFlags = nsTextFrameUtils::TEXT_NO_BREAKS;

  if (mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_WHITESPACE) {
    textFlags |= nsTextFrameUtils::TEXT_INCOMING_WHITESPACE;
  }
  if (mCurrentRunContextInfo & nsTextFrameUtils::INCOMING_ARABICCHAR) {
    textFlags |= gfxTextRunFactory::TEXT_INCOMING_ARABICCHAR;
  }

  nsAutoTArray<int32_t,50> textBreakPoints;
  TextRunUserData dummyData;
  TextRunMappedFlow dummyMappedFlow;

  TextRunUserData* userData;
  TextRunUserData* userDataToDestroy;
  
  
  if (mMappedFlows.Length() == 1 && !mMappedFlows[0].mEndFrame &&
      mMappedFlows[0].mStartFrame->GetContentOffset() == 0) {
    userData = &dummyData;
    userDataToDestroy = nullptr;
    dummyData.mMappedFlows = &dummyMappedFlow;
  } else {
    userData = static_cast<TextRunUserData*>
      (nsMemory::Alloc(sizeof(TextRunUserData) + mMappedFlows.Length()*sizeof(TextRunMappedFlow)));
    userDataToDestroy = userData;
    userData->mMappedFlows = reinterpret_cast<TextRunMappedFlow*>(userData + 1);
  }
  userData->mMappedFlowCount = mMappedFlows.Length();
  userData->mLastFlowIndex = 0;

  uint32_t currentTransformedTextOffset = 0;

  uint32_t nextBreakIndex = 0;
  nsTextFrame* nextBreakBeforeFrame = GetNextBreakBeforeFrame(&nextBreakIndex);
  bool enabledJustification = mLineContainer &&
    (mLineContainer->StyleText()->mTextAlign == NS_STYLE_TEXT_ALIGN_JUSTIFY ||
     mLineContainer->StyleText()->mTextAlignLast == NS_STYLE_TEXT_ALIGN_JUSTIFY) &&
    !mLineContainer->IsSVGText();

  
  switch (mLineContainer->StyleText()->mWordBreak) {
    case NS_STYLE_WORDBREAK_BREAK_ALL:
      mLineBreaker.SetWordBreak(nsILineBreaker::kWordBreak_BreakAll);
      break;
    case NS_STYLE_WORDBREAK_KEEP_ALL:
      mLineBreaker.SetWordBreak(nsILineBreaker::kWordBreak_KeepAll);
      break;
    default:
      mLineBreaker.SetWordBreak(nsILineBreaker::kWordBreak_Normal);
      break;
  }

  const nsStyleText* textStyle = nullptr;
  const nsStyleFont* fontStyle = nullptr;
  nsStyleContext* lastStyleContext = nullptr;
  for (uint32_t i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    nsTextFrame* f = mappedFlow->mStartFrame;

    lastStyleContext = f->StyleContext();
    
    textStyle = f->StyleText();
    if (NS_STYLE_TEXT_TRANSFORM_NONE != textStyle->mTextTransform) {
      anyTextTransformStyle = true;
    }
    textFlags |= GetSpacingFlags(LetterSpacing(f));
    textFlags |= GetSpacingFlags(WordSpacing(f));
    nsTextFrameUtils::CompressionMode compression =
      CSSWhitespaceToCompressionMode[textStyle->mWhiteSpace];
    if (enabledJustification && !textStyle->WhiteSpaceIsSignificant()) {
      textFlags |= gfxTextRunFactory::TEXT_ENABLE_SPACING;
    }
    fontStyle = f->StyleFont();
    if (NS_STYLE_FONT_VARIANT_SMALL_CAPS == fontStyle->mFont.variant) {
      anySmallcapsStyle = true;
    }

    
    nsIContent* content = f->GetContent();
    const nsTextFragment* frag = content->GetText();
    int32_t contentStart = mappedFlow->mStartFrame->GetContentOffset();
    int32_t contentEnd = mappedFlow->GetContentEnd();
    int32_t contentLength = contentEnd - contentStart;

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

    uint32_t analysisFlags;
    if (frag->Is2b()) {
      NS_ASSERTION(mDoubleByteText, "Wrong buffer char size!");
      PRUnichar* bufStart = static_cast<PRUnichar*>(aTextBuffer);
      PRUnichar* bufEnd = nsTextFrameUtils::TransformText(
          frag->Get2b() + contentStart, contentLength, bufStart,
          compression, &mNextRunContextInfo, &builder, &analysisFlags);
      aTextBuffer = bufEnd;
    } else {
      if (mDoubleByteText) {
        
        
        AutoFallibleTArray<uint8_t,BIG_TEXT_NODE_SIZE> tempBuf;
        uint8_t* bufStart = tempBuf.AppendElements(contentLength);
        if (!bufStart) {
          DestroyUserData(userDataToDestroy);
          return nullptr;
        }
        uint8_t* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const uint8_t*>(frag->Get1b()) + contentStart, contentLength,
            bufStart, compression, &mNextRunContextInfo, &builder, &analysisFlags);
        aTextBuffer = ExpandBuffer(static_cast<PRUnichar*>(aTextBuffer),
                                   tempBuf.Elements(), end - tempBuf.Elements());
      } else {
        uint8_t* bufStart = static_cast<uint8_t*>(aTextBuffer);
        uint8_t* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const uint8_t*>(frag->Get1b()) + contentStart, contentLength,
            bufStart, compression, &mNextRunContextInfo, &builder, &analysisFlags);
        aTextBuffer = end;
      }
    }
    textFlags |= analysisFlags;

    currentTransformedTextOffset =
      (static_cast<const uint8_t*>(aTextBuffer) - static_cast<const uint8_t*>(textPtr)) >> mDoubleByteText;
  }

  
  if (!builder.IsOK()) {
    DestroyUserData(userDataToDestroy);
    return nullptr;
  }

  void* finalUserData;
  if (userData == &dummyData) {
    textFlags |= nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW;
    userData = nullptr;
    finalUserData = mMappedFlows[0].mStartFrame;
  } else {
    finalUserData = userData;
  }

  uint32_t transformedLength = currentTransformedTextOffset;

  
  nsTextFrame* firstFrame = mMappedFlows[0].mStartFrame;
  float fontInflation;
  if (mWhichTextRun == nsTextFrame::eNotInflated) {
    fontInflation = 1.0f;
  } else {
    fontInflation = nsLayoutUtils::FontSizeInflationFor(firstFrame);
  }

  gfxFontGroup* fontGroup = GetFontGroupForFrame(firstFrame, fontInflation);
  if (!fontGroup) {
    DestroyUserData(userDataToDestroy);
    return nullptr;
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
    textFlags |= gfxTextRunFactory::TEXT_TRAILING_ARABICCHAR;
  }
  
  
  
  textFlags |= nsLayoutUtils::GetTextRunFlagsForStyle(lastStyleContext,
      fontStyle, LetterSpacing(firstFrame, textStyle));
  
  
  if (!(textFlags & gfxTextRunFactory::TEXT_OPTIMIZE_SPEED)) {
    textFlags |= gfxTextRunFactory::TEXT_NEED_BOUNDING_BOX;
  }

  gfxSkipChars skipChars;
  skipChars.TakeFrom(&builder);
  
  NS_ASSERTION(nextBreakIndex == mLineBreakBeforeFrames.Length(),
               "Didn't find all the frames to break-before...");
  gfxSkipCharsIterator iter(skipChars);
  nsAutoTArray<uint32_t,50> textBreakPointsAfterTransform;
  for (uint32_t i = 0; i < textBreakPoints.Length(); ++i) {
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
    for (uint32_t i = 0; i < mMappedFlows.Length(); ++i) {
      MappedFlow* mappedFlow = &mMappedFlows[i];
      nsTextFrame* f;
      for (f = mappedFlow->mStartFrame; f != mappedFlow->mEndFrame;
           f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
        uint32_t offset = iter.GetSkippedOffset();
        iter.AdvanceOriginal(f->GetContentLength());
        uint32_t end = iter.GetSkippedOffset();
        nsStyleContext* sc = f->StyleContext();
        uint32_t j;
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
        textBreakPointsAfterTransform.Elements(),
        textBreakPointsAfterTransform.Length(),
        int32_t(firstFrame->PresContext()->AppUnitsPerDevPixel())};

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
    const uint8_t* text = static_cast<const uint8_t*>(textPtr);
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
    return nullptr;
  }

  
  
  
  
  uint32_t flags = 0;
  if (mDoubleByteText) {
    flags |= SBS_DOUBLE_BYTE;
  }
  if (mSkipIncompleteTextRuns) {
    flags |= SBS_SUPPRESS_SINK;
  }
  SetupBreakSinksForTextRun(textRun, textPtr, flags);

  if (mSkipIncompleteTextRuns) {
    mSkipIncompleteTextRuns = !TextContainsLineBreakerWhiteSpace(textPtr,
        transformedLength, mDoubleByteText);
    
    
    mTextRunsToDelete.AppendElement(textRun);
    
    
    
    
    
    textRun->SetUserData(nullptr);
    DestroyUserData(userDataToDestroy);
    return nullptr;
  }

  
  
  AssignTextRun(textRun, fontInflation);
  return textRun;
}





bool
BuildTextRunsScanner::SetupLineBreakerContext(gfxTextRun *aTextRun)
{
  AutoFallibleTArray<uint8_t,BIG_TEXT_NODE_SIZE> buffer;
  uint32_t bufferSize = mMaxTextLength*(mDoubleByteText ? 2 : 1);
  if (bufferSize < mMaxTextLength || bufferSize == UINT32_MAX) {
    return false;
  }
  void *textPtr = buffer.AppendElements(bufferSize);
  if (!textPtr) {
    return false;
  }

  gfxSkipCharsBuilder builder;

  nsAutoTArray<int32_t,50> textBreakPoints;
  TextRunUserData dummyData;
  TextRunMappedFlow dummyMappedFlow;

  TextRunUserData* userData;
  TextRunUserData* userDataToDestroy;
  
  
  if (mMappedFlows.Length() == 1 && !mMappedFlows[0].mEndFrame &&
      mMappedFlows[0].mStartFrame->GetContentOffset() == 0) {
    userData = &dummyData;
    userDataToDestroy = nullptr;
    dummyData.mMappedFlows = &dummyMappedFlow;
  } else {
    userData = static_cast<TextRunUserData*>
      (nsMemory::Alloc(sizeof(TextRunUserData) + mMappedFlows.Length()*sizeof(TextRunMappedFlow)));
    userDataToDestroy = userData;
    userData->mMappedFlows = reinterpret_cast<TextRunMappedFlow*>(userData + 1);
  }
  userData->mMappedFlowCount = mMappedFlows.Length();
  userData->mLastFlowIndex = 0;

  uint32_t nextBreakIndex = 0;
  nsTextFrame* nextBreakBeforeFrame = GetNextBreakBeforeFrame(&nextBreakIndex);

  const nsStyleText* textStyle = nullptr;
  for (uint32_t i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    nsTextFrame* f = mappedFlow->mStartFrame;

    textStyle = f->StyleText();
    nsTextFrameUtils::CompressionMode compression =
      CSSWhitespaceToCompressionMode[textStyle->mWhiteSpace];

    
    nsIContent* content = f->GetContent();
    const nsTextFragment* frag = content->GetText();
    int32_t contentStart = mappedFlow->mStartFrame->GetContentOffset();
    int32_t contentEnd = mappedFlow->GetContentEnd();
    int32_t contentLength = contentEnd - contentStart;

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

    uint32_t analysisFlags;
    if (frag->Is2b()) {
      NS_ASSERTION(mDoubleByteText, "Wrong buffer char size!");
      PRUnichar* bufStart = static_cast<PRUnichar*>(textPtr);
      PRUnichar* bufEnd = nsTextFrameUtils::TransformText(
          frag->Get2b() + contentStart, contentLength, bufStart,
          compression, &mNextRunContextInfo, &builder, &analysisFlags);
      textPtr = bufEnd;
    } else {
      if (mDoubleByteText) {
        
        
        AutoFallibleTArray<uint8_t,BIG_TEXT_NODE_SIZE> tempBuf;
        uint8_t* bufStart = tempBuf.AppendElements(contentLength);
        if (!bufStart) {
          DestroyUserData(userDataToDestroy);
          return false;
        }
        uint8_t* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const uint8_t*>(frag->Get1b()) + contentStart, contentLength,
            bufStart, compression, &mNextRunContextInfo, &builder, &analysisFlags);
        textPtr = ExpandBuffer(static_cast<PRUnichar*>(textPtr),
                               tempBuf.Elements(), end - tempBuf.Elements());
      } else {
        uint8_t* bufStart = static_cast<uint8_t*>(textPtr);
        uint8_t* end = nsTextFrameUtils::TransformText(
            reinterpret_cast<const uint8_t*>(frag->Get1b()) + contentStart, contentLength,
            bufStart, compression, &mNextRunContextInfo, &builder, &analysisFlags);
        textPtr = end;
      }
    }
  }

  
  
  
  
  uint32_t flags = 0;
  if (mDoubleByteText) {
    flags |= SBS_DOUBLE_BYTE;
  }
  if (mSkipIncompleteTextRuns) {
    flags |= SBS_SUPPRESS_SINK;
  }
  SetupBreakSinksForTextRun(aTextRun, buffer.Elements(), flags);

  DestroyUserData(userDataToDestroy);

  return true;
}

static bool
HasCompressedLeadingWhitespace(nsTextFrame* aFrame, const nsStyleText* aStyleText,
                               int32_t aContentEndOffset,
                               const gfxSkipCharsIterator& aIterator)
{
  if (!aIterator.IsOriginalCharSkipped())
    return false;

  gfxSkipCharsIterator iter = aIterator;
  int32_t frameContentOffset = aFrame->GetContentOffset();
  const nsTextFragment* frag = aFrame->GetContent()->GetText();
  while (frameContentOffset < aContentEndOffset && iter.IsOriginalCharSkipped()) {
    if (IsTrimmableSpace(frag, frameContentOffset, aStyleText))
      return true;
    ++frameContentOffset;
    iter.AdvanceOriginal(1);
  }
  return false;
}

void
BuildTextRunsScanner::SetupBreakSinksForTextRun(gfxTextRun* aTextRun,
                                                const void* aTextPtr,
                                                uint32_t    aFlags)
{
  
  const nsStyleFont *styleFont = mMappedFlows[0].mStartFrame->StyleFont();
  
  
  nsIAtom* hyphenationLanguage =
    styleFont->mExplicitLanguage ? styleFont->mLanguage : nullptr;
  
  
  
  gfxSkipCharsIterator iter(aTextRun->GetSkipChars());

  for (uint32_t i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    uint32_t offset = iter.GetSkippedOffset();
    gfxSkipCharsIterator iterNext = iter;
    iterNext.AdvanceOriginal(mappedFlow->GetContentEnd() -
            mappedFlow->mStartFrame->GetContentOffset());

    nsAutoPtr<BreakSink>* breakSink = mBreakSinks.AppendElement(
      new BreakSink(aTextRun, mContext, offset,
                    (aFlags & SBS_EXISTING_TEXTRUN) != 0));
    if (!breakSink || !*breakSink)
      return;

    uint32_t length = iterNext.GetSkippedOffset() - offset;
    uint32_t flags = 0;
    nsIFrame* initialBreakController = mappedFlow->mAncestorControllingInitialBreak;
    if (!initialBreakController) {
      initialBreakController = mLineContainer;
    }
    if (!initialBreakController->StyleText()->WhiteSpaceCanWrap()) {
      flags |= nsLineBreaker::BREAK_SUPPRESS_INITIAL;
    }
    nsTextFrame* startFrame = mappedFlow->mStartFrame;
    const nsStyleText* textStyle = startFrame->StyleText();
    if (!textStyle->WhiteSpaceCanWrap()) {
      flags |= nsLineBreaker::BREAK_SUPPRESS_INSIDE;
    }
    if (aTextRun->GetFlags() & nsTextFrameUtils::TEXT_NO_BREAKS) {
      flags |= nsLineBreaker::BREAK_SKIP_SETTING_NO_BREAKS;
    }
    if (textStyle->mTextTransform == NS_STYLE_TEXT_TRANSFORM_CAPITALIZE) {
      flags |= nsLineBreaker::BREAK_NEED_CAPITALIZATION;
    }
    if (textStyle->mHyphens == NS_STYLE_HYPHENS_AUTO) {
      flags |= nsLineBreaker::BREAK_USE_AUTO_HYPHENATION;
    }

    if (HasCompressedLeadingWhitespace(startFrame, textStyle,
                                       mappedFlow->GetContentEnd(), iter)) {
      mLineBreaker.AppendInvisibleWhitespace(flags);
    }

    if (length > 0) {
      BreakSink* sink =
        (aFlags & SBS_SUPPRESS_SINK) ? nullptr : (*breakSink).get();
      if (aFlags & SBS_DOUBLE_BYTE) {
        const PRUnichar* text = reinterpret_cast<const PRUnichar*>(aTextPtr);
        mLineBreaker.AppendText(hyphenationLanguage, text + offset,
                                length, flags, sink);
      } else {
        const uint8_t* text = reinterpret_cast<const uint8_t*>(aTextPtr);
        mLineBreaker.AppendText(hyphenationLanguage, text + offset,
                                length, flags, sink);
      }
    }
    
    iter = iterNext;
  }
}


static inline TextRunMappedFlow*
FindFlowForContent(TextRunUserData* aUserData, nsIContent* aContent)
{
  
  int32_t i = aUserData->mLastFlowIndex;
  int32_t delta = 1;
  int32_t sign = 1;
  
  
  while (i >= 0 && uint32_t(i) < aUserData->mMappedFlowCount) {
    TextRunMappedFlow* flow = &aUserData->mMappedFlows[i];
    if (flow->mStartFrame->GetContent() == aContent) {
      return flow;
    }

    i += delta;
    sign = -sign;
    delta = -delta + sign;
  }

  
  
  
  i += delta;
  if (sign > 0) {
    for (; i < int32_t(aUserData->mMappedFlowCount); ++i) {
      TextRunMappedFlow* flow = &aUserData->mMappedFlows[i];
      if (flow->mStartFrame->GetContent() == aContent) {
        return flow;
      }
    }
  } else {
    for (; i >= 0; --i) {
      TextRunMappedFlow* flow = &aUserData->mMappedFlows[i];
      if (flow->mStartFrame->GetContent() == aContent) {
        return flow;
      }
    }
  }

  return nullptr;
}

void
BuildTextRunsScanner::AssignTextRun(gfxTextRun* aTextRun, float aInflation)
{
  for (uint32_t i = 0; i < mMappedFlows.Length(); ++i) {
    MappedFlow* mappedFlow = &mMappedFlows[i];
    nsTextFrame* startFrame = mappedFlow->mStartFrame;
    nsTextFrame* endFrame = mappedFlow->mEndFrame;
    nsTextFrame* f;
    for (f = startFrame; f != endFrame;
         f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
#ifdef DEBUG_roc
      if (f->GetTextRun(mWhichTextRun)) {
        gfxTextRun* textRun = f->GetTextRun(mWhichTextRun);
        if (textRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW) {
          if (mMappedFlows[0].mStartFrame != static_cast<nsTextFrame*>(textRun->GetUserData())) {
            NS_WARNING("REASSIGNING SIMPLE FLOW TEXT RUN!");
          }
        } else {
          TextRunUserData* userData =
            static_cast<TextRunUserData*>(textRun->GetUserData());
         
          if (userData->mMappedFlowCount >= mMappedFlows.Length() ||
              userData->mMappedFlows[userData->mMappedFlowCount - 1].mStartFrame !=
              mMappedFlows[userData->mMappedFlowCount - 1].mStartFrame) {
            NS_WARNING("REASSIGNING MULTIFLOW TEXT RUN (not append)!");
          }
        }
      }
#endif

      gfxTextRun* oldTextRun = f->GetTextRun(mWhichTextRun);
      if (oldTextRun) {
        nsTextFrame* firstFrame = nullptr;
        uint32_t startOffset = 0;
        if (oldTextRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW) {
          firstFrame = static_cast<nsTextFrame*>(oldTextRun->GetUserData());
        }
        else {
          TextRunUserData* userData = static_cast<TextRunUserData*>(oldTextRun->GetUserData());
          firstFrame = userData->mMappedFlows[0].mStartFrame;
          if (MOZ_UNLIKELY(f != firstFrame)) {
            TextRunMappedFlow* flow = FindFlowForContent(userData, f->GetContent());
            if (flow) {
              startOffset = flow->mDOMOffsetToBeforeTransformOffset;
            }
            else {
              NS_ERROR("Can't find flow containing frame 'f'");
            }
          }
        }

        
        
        nsTextFrame* clearFrom = nullptr;
        if (MOZ_UNLIKELY(f != firstFrame)) {
          
          
          gfxSkipCharsIterator iter(oldTextRun->GetSkipChars(), startOffset, f->GetContentOffset());
          uint32_t textRunOffset = iter.ConvertOriginalToSkipped(f->GetContentOffset());
          clearFrom = textRunOffset == oldTextRun->GetLength() ? f : nullptr;
        }
        f->ClearTextRun(clearFrom, mWhichTextRun);

#ifdef DEBUG
        if (firstFrame && !firstFrame->GetTextRun(mWhichTextRun)) {
          
          for (uint32_t j = 0; j < mBreakSinks.Length(); ++j) {
            NS_ASSERTION(oldTextRun != mBreakSinks[j]->mTextRun,
                         "destroyed text run is still in use");
          }
        }
#endif
      }
      f->SetTextRun(aTextRun, mWhichTextRun, aInflation);
    }
    
    
    nsFrameState whichTextRunState =
      startFrame->GetTextRun(nsTextFrame::eInflated) == aTextRun
        ? TEXT_IN_TEXTRUN_USER_DATA
        : TEXT_IN_UNINFLATED_TEXTRUN_USER_DATA;
    startFrame->AddStateBits(whichTextRunState);
  }
}

NS_QUERYFRAME_HEAD(nsTextFrame)
  NS_QUERYFRAME_ENTRY(nsTextFrame)
NS_QUERYFRAME_TAIL_INHERITING(nsTextFrameBase)

gfxSkipCharsIterator
nsTextFrame::EnsureTextRun(TextRunType aWhichTextRun,
                           gfxContext* aReferenceContext,
                           nsIFrame* aLineContainer,
                           const nsLineList::iterator* aLine,
                           uint32_t* aFlowEndInTextRun)
{
  gfxTextRun *textRun = GetTextRun(aWhichTextRun);
  if (textRun && (!aLine || !(*aLine)->GetInvalidateTextRuns())) {
    if (textRun->GetExpirationState()->IsTracked()) {
      gTextRuns->MarkUsed(textRun);
    }
  } else {
    nsRefPtr<gfxContext> ctx = aReferenceContext;
    if (!ctx) {
      ctx = GetReferenceRenderingContext(this, nullptr);
    }
    if (ctx) {
      BuildTextRuns(ctx, this, aLineContainer, aLine, aWhichTextRun);
    }
    textRun = GetTextRun(aWhichTextRun);
    if (!textRun) {
      
      
      static const gfxSkipChars emptySkipChars;
      return gfxSkipCharsIterator(emptySkipChars, 0);
    }
    TabWidthStore* tabWidths =
      static_cast<TabWidthStore*>(Properties().Get(TabWidthProperty()));
    if (tabWidths && tabWidths->mValidForContentOffset != GetContentOffset()) {
      Properties().Delete(TabWidthProperty());
    }
  }

  if (textRun->GetFlags() & nsTextFrameUtils::TEXT_IS_SIMPLE_FLOW) {
    if (aFlowEndInTextRun) {
      *aFlowEndInTextRun = textRun->GetLength();
    }
    return gfxSkipCharsIterator(textRun->GetSkipChars(), 0, mContentOffset);
  }

  TextRunUserData* userData = static_cast<TextRunUserData*>(textRun->GetUserData());
  TextRunMappedFlow* flow = FindFlowForContent(userData, mContent);
  if (flow) {
    
    
    uint32_t flowIndex = flow - userData->mMappedFlows;
    userData->mLastFlowIndex = flowIndex;
    gfxSkipCharsIterator iter(textRun->GetSkipChars(),
                              flow->mDOMOffsetToBeforeTransformOffset, mContentOffset);
    if (aFlowEndInTextRun) {
      if (flowIndex + 1 < userData->mMappedFlowCount) {
        gfxSkipCharsIterator end(textRun->GetSkipChars());
        *aFlowEndInTextRun = end.ConvertOriginalToSkipped(
              flow[1].mStartFrame->GetContentOffset() + flow[1].mDOMOffsetToBeforeTransformOffset);
      } else {
        *aFlowEndInTextRun = textRun->GetLength();
      }
    }
    return iter;
  }

  NS_ERROR("Can't find flow containing this frame???");
  static const gfxSkipChars emptySkipChars;
  return gfxSkipCharsIterator(emptySkipChars, 0);
}

static uint32_t
GetEndOfTrimmedText(const nsTextFragment* aFrag, const nsStyleText* aStyleText,
                    uint32_t aStart, uint32_t aEnd,
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
                               bool aTrimAfter)
{
  NS_ASSERTION(mTextRun, "Need textrun here");
  
  
  
  NS_ASSERTION(!(GetStateBits() & NS_FRAME_FIRST_REFLOW) ||
               (GetParent()->GetStateBits() & NS_FRAME_TOO_DEEP_IN_FRAME_TREE),
               "Can only call this on frames that have been reflowed");
  NS_ASSERTION(!(GetStateBits() & NS_FRAME_IN_REFLOW),
               "Can only call this on frames that are not being reflowed");

  TrimmedOffsets offsets = { GetContentOffset(), GetContentLength() };
  const nsStyleText* textStyle = StyleText();
  
  
  if (textStyle->WhiteSpaceIsSignificant())
    return offsets;

  if (GetStateBits() & TEXT_START_OF_LINE) {
    int32_t whitespaceCount =
      GetTrimmableWhitespaceCount(aFrag,
                                  offsets.mStart, offsets.mLength, 1);
    offsets.mStart += whitespaceCount;
    offsets.mLength -= whitespaceCount;
  }

  if (aTrimAfter && (GetStateBits() & TEXT_END_OF_LINE)) {
    
    
    
    int32_t whitespaceCount =
      GetTrimmableWhitespaceCount(aFrag,
                                  offsets.GetEnd() - 1, offsets.mLength, -1);
    offsets.mLength -= whitespaceCount;
  }
  return offsets;
}








static bool IsJustifiableCharacter(const nsTextFragment* aFrag, int32_t aPos,
                                     bool aLangIsCJ)
{
  PRUnichar ch = aFrag->CharAt(aPos);
  if (ch == '\n' || ch == '\t' || ch == '\r')
    return true;
  if (ch == ' ' || ch == CH_NBSP) {
    
    if (!aFrag->Is2b())
      return true;
    return !nsTextFrameUtils::IsSpaceCombiningSequenceTail(
        aFrag->Get2b() + aPos + 1, aFrag->GetLength() - (aPos + 1));
  }
  if (ch < 0x2150u)
    return false;
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
    return true;
  return false;
}

void
nsTextFrame::ClearMetrics(nsHTMLReflowMetrics& aMetrics)
{
  aMetrics.width = 0;
  aMetrics.height = 0;
  aMetrics.ascent = 0;
  mAscent = 0;
}

static int32_t FindChar(const nsTextFragment* frag,
                        int32_t aOffset, int32_t aLength, PRUnichar ch)
{
  int32_t i = 0;
  if (frag->Is2b()) {
    const PRUnichar* str = frag->Get2b() + aOffset;
    for (; i < aLength; ++i) {
      if (*str == ch)
        return i + aOffset;
      ++str;
    }
  } else {
    if (uint16_t(ch) <= 0xFF) {
      const char* str = frag->Get1b() + aOffset;
      const void* p = memchr(str, ch, aLength);
      if (p)
        return (static_cast<const char*>(p) - str) + aOffset;
    }
  }
  return -1;
}

static bool IsChineseOrJapanese(nsIFrame* aFrame)
{
  nsIAtom* language = aFrame->StyleFont()->mLanguage;
  if (!language) {
    return false;
  }
  const PRUnichar *lang = language->GetUTF16String();
  return (!nsCRT::strncmp(lang, NS_LITERAL_STRING("ja").get(), 2) ||
          !nsCRT::strncmp(lang, NS_LITERAL_STRING("zh").get(), 2)) &&
         (language->GetLength() == 2 || lang[2] == '-');
}

#ifdef DEBUG
static bool IsInBounds(const gfxSkipCharsIterator& aStart, int32_t aContentLength,
                         uint32_t aOffset, uint32_t aLength) {
  if (aStart.GetSkippedOffset() > aOffset)
    return false;
  if (aContentLength == INT32_MAX)
    return true;
  gfxSkipCharsIterator iter(aStart);
  iter.AdvanceOriginal(aContentLength);
  return iter.GetSkippedOffset() >= aOffset + aLength;
}
#endif

class NS_STACK_CLASS PropertyProvider : public gfxTextRun::PropertyProvider {
public:
  








  PropertyProvider(gfxTextRun* aTextRun, const nsStyleText* aTextStyle,
                   const nsTextFragment* aFrag, nsTextFrame* aFrame,
                   const gfxSkipCharsIterator& aStart, int32_t aLength,
                   nsIFrame* aLineContainer,
                   nscoord aOffsetFromBlockOriginForTabs,
                   nsTextFrame::TextRunType aWhichTextRun)
    : mTextRun(aTextRun), mFontGroup(nullptr),
      mTextStyle(aTextStyle), mFrag(aFrag),
      mLineContainer(aLineContainer),
      mFrame(aFrame), mStart(aStart), mTempIterator(aStart),
      mTabWidths(nullptr), mTabWidthsAnalyzedLimit(0),
      mLength(aLength),
      mWordSpacing(WordSpacing(aFrame, aTextStyle)),
      mLetterSpacing(LetterSpacing(aFrame, aTextStyle)),
      mJustificationSpacing(0),
      mHyphenWidth(-1),
      mOffsetFromBlockOriginForTabs(aOffsetFromBlockOriginForTabs),
      mReflowing(true),
      mWhichTextRun(aWhichTextRun)
  {
    NS_ASSERTION(mStart.IsInitialized(), "Start not initialized?");
  }

  




  PropertyProvider(nsTextFrame* aFrame, const gfxSkipCharsIterator& aStart,
                   nsTextFrame::TextRunType aWhichTextRun)
    : mTextRun(aFrame->GetTextRun(aWhichTextRun)), mFontGroup(nullptr),
      mTextStyle(aFrame->StyleText()),
      mFrag(aFrame->GetContent()->GetText()),
      mLineContainer(nullptr),
      mFrame(aFrame), mStart(aStart), mTempIterator(aStart),
      mTabWidths(nullptr), mTabWidthsAnalyzedLimit(0),
      mLength(aFrame->GetContentLength()),
      mWordSpacing(WordSpacing(aFrame)),
      mLetterSpacing(LetterSpacing(aFrame)),
      mJustificationSpacing(0),
      mHyphenWidth(-1),
      mOffsetFromBlockOriginForTabs(0),
      mReflowing(false),
      mWhichTextRun(aWhichTextRun)
  {
    NS_ASSERTION(mTextRun, "Textrun not initialized!");
  }

  
  void InitializeForDisplay(bool aTrimAfter);

  virtual void GetSpacing(uint32_t aStart, uint32_t aLength, Spacing* aSpacing);
  virtual gfxFloat GetHyphenWidth();
  virtual void GetHyphenationBreaks(uint32_t aStart, uint32_t aLength,
                                    bool* aBreakBefore);
  virtual int8_t GetHyphensOption() {
    return mTextStyle->mHyphens;
  }

  void GetSpacingInternal(uint32_t aStart, uint32_t aLength, Spacing* aSpacing,
                          bool aIgnoreTabs);

  


  uint32_t ComputeJustifiableCharacters(int32_t aOffset, int32_t aLength);
  




  void FindJustificationRange(gfxSkipCharsIterator* aStart,
                              gfxSkipCharsIterator* aEnd);

  const nsStyleText* StyleText() { return mTextStyle; }
  nsTextFrame* GetFrame() { return mFrame; }
  
  
  
  const gfxSkipCharsIterator& GetStart() { return mStart; }
  
  uint32_t GetOriginalLength() {
    NS_ASSERTION(mLength != INT32_MAX, "Length not known");
    return mLength;
  }
  const nsTextFragment* GetFragment() { return mFrag; }

  gfxFontGroup* GetFontGroup() {
    if (!mFontGroup)
      InitFontGroupAndFontMetrics();
    return mFontGroup;
  }

  nsFontMetrics* GetFontMetrics() {
    if (!mFontMetrics)
      InitFontGroupAndFontMetrics();
    return mFontMetrics;
  }

  void CalcTabWidths(uint32_t aTransformedStart, uint32_t aTransformedLength);

  const gfxSkipCharsIterator& GetEndHint() { return mTempIterator; }

protected:
  void SetupJustificationSpacing();

  void InitFontGroupAndFontMetrics() {
    float inflation = (mWhichTextRun == nsTextFrame::eInflated)
      ? mFrame->GetFontSizeInflation() : 1.0f;
    mFontGroup = GetFontGroupForFrame(mFrame, inflation,
                                      getter_AddRefs(mFontMetrics));
  }

  gfxTextRun*           mTextRun;
  gfxFontGroup*         mFontGroup;
  nsRefPtr<nsFontMetrics> mFontMetrics;
  const nsStyleText*    mTextStyle;
  const nsTextFragment* mFrag;
  nsIFrame*             mLineContainer;
  nsTextFrame*          mFrame;
  gfxSkipCharsIterator  mStart;  
  gfxSkipCharsIterator  mTempIterator;
  
  
  TabWidthStore*        mTabWidths;
  
  
  
  uint32_t              mTabWidthsAnalyzedLimit;

  int32_t               mLength; 
  gfxFloat              mWordSpacing;     
  gfxFloat              mLetterSpacing;   
  gfxFloat              mJustificationSpacing;
  gfxFloat              mHyphenWidth;
  gfxFloat              mOffsetFromBlockOriginForTabs;
  bool                  mReflowing;
  nsTextFrame::TextRunType mWhichTextRun;
};

uint32_t
PropertyProvider::ComputeJustifiableCharacters(int32_t aOffset, int32_t aLength)
{
  
  nsSkipCharsRunIterator
    run(mStart, nsSkipCharsRunIterator::LENGTH_INCLUDES_SKIPPED, aLength);
  run.SetOriginalOffset(aOffset);
  uint32_t justifiableChars = 0;
  bool isCJK = IsChineseOrJapanese(mFrame);
  while (run.NextRun()) {
    for (int32_t i = 0; i < run.GetRunLength(); ++i) {
      justifiableChars +=
        IsJustifiableCharacter(mFrag, run.GetOriginalOffset() + i, isCJK);
    }
  }
  return justifiableChars;
}




static void FindClusterStart(gfxTextRun* aTextRun, int32_t aOriginalStart,
                             gfxSkipCharsIterator* aPos)
{
  while (aPos->GetOriginalOffset() > aOriginalStart) {
    if (aPos->IsOriginalCharSkipped() ||
        aTextRun->IsClusterStart(aPos->GetSkippedOffset())) {
      break;
    }
    aPos->AdvanceOriginal(-1);
  }
}




static void FindClusterEnd(gfxTextRun* aTextRun, int32_t aOriginalEnd,
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
PropertyProvider::GetSpacing(uint32_t aStart, uint32_t aLength,
                             Spacing* aSpacing)
{
  GetSpacingInternal(aStart, aLength, aSpacing,
                     (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TAB) == 0);
}

static bool
CanAddSpacingAfter(gfxTextRun* aTextRun, uint32_t aOffset)
{
  if (aOffset + 1 >= aTextRun->GetLength())
    return true;
  return aTextRun->IsClusterStart(aOffset + 1) &&
    aTextRun->IsLigatureGroupStart(aOffset + 1);
}

void
PropertyProvider::GetSpacingInternal(uint32_t aStart, uint32_t aLength,
                                     Spacing* aSpacing, bool aIgnoreTabs)
{
  NS_PRECONDITION(IsInBounds(mStart, mLength, aStart, aLength), "Range out of bounds");

  uint32_t index;
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
      uint32_t runOffsetInSubstring = run.GetSkippedOffset() - aStart;
      gfxSkipCharsIterator iter = run.GetPos();
      for (int32_t i = 0; i < run.GetRunLength(); ++i) {
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
    aIgnoreTabs = mFrame->StyleText()->mTabSize == 0;

  
  if (!aIgnoreTabs) {
    CalcTabWidths(aStart, aLength);
    if (mTabWidths) {
      mTabWidths->ApplySpacing(aSpacing,
                               aStart - mStart.GetSkippedOffset(), aLength);
    }
  }

  
  if (mJustificationSpacing) {
    gfxFloat halfJustificationSpace = mJustificationSpacing/2;
    
    
    bool isCJK = IsChineseOrJapanese(mFrame);
    gfxSkipCharsIterator justificationStart(mStart), justificationEnd(mStart);
    FindJustificationRange(&justificationStart, &justificationEnd);

    nsSkipCharsRunIterator
      run(start, nsSkipCharsRunIterator::LENGTH_UNSKIPPED_ONLY, aLength);
    while (run.NextRun()) {
      gfxSkipCharsIterator iter = run.GetPos();
      int32_t runOriginalOffset = run.GetOriginalOffset();
      for (int32_t i = 0; i < run.GetRunLength(); ++i) {
        int32_t iterOriginalOffset = runOriginalOffset + i;
        if (IsJustifiableCharacter(mFrag, iterOriginalOffset, isCJK)) {
          iter.SetOriginalOffset(iterOriginalOffset);
          FindClusterStart(mTextRun, runOriginalOffset, &iter);
          uint32_t clusterFirstChar = iter.GetSkippedOffset();
          FindClusterEnd(mTextRun, runOriginalOffset + run.GetRunLength(), &iter);
          uint32_t clusterLastChar = iter.GetSkippedOffset();
          
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

static gfxFloat
ComputeTabWidthAppUnits(nsIFrame* aFrame, gfxTextRun* aTextRun)
{
  
  const nsStyleText* textStyle = aFrame->StyleText();
  
  
  
  gfxFloat spaceWidthAppUnits =
    NS_round(GetFirstFontMetrics(aTextRun->GetFontGroup()).spaceWidth *
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

  
  
  
  return ceil((aX + 1)/(*aCachedTabWidth))*(*aCachedTabWidth);
}

void
PropertyProvider::CalcTabWidths(uint32_t aStart, uint32_t aLength)
{
  if (!mTabWidths) {
    if (mReflowing && !mLineContainer) {
      
      
      return;
    }
    if (!mReflowing) {
      mTabWidths = static_cast<TabWidthStore*>
        (mFrame->Properties().Get(TabWidthProperty()));
#ifdef DEBUG
      
      
      
      for (uint32_t i = aStart + aLength; i > aStart; --i) {
        if (mTextRun->CharIsTab(i - 1)) {
          uint32_t startOffset = mStart.GetSkippedOffset();
          NS_ASSERTION(mTabWidths && mTabWidths->mLimit + startOffset >= i,
                       "Precomputed tab widths are missing!");
          break;
        }
      }
#endif
      return;
    }
  }

  uint32_t startOffset = mStart.GetSkippedOffset();
  MOZ_ASSERT(aStart >= startOffset, "wrong start offset");
  MOZ_ASSERT(aStart + aLength <= startOffset + mLength, "beyond the end");
  uint32_t tabsEnd =
    (mTabWidths ? mTabWidths->mLimit : mTabWidthsAnalyzedLimit) + startOffset;
  if (tabsEnd < aStart + aLength) {
    NS_ASSERTION(mReflowing,
                 "We need precomputed tab widths, but don't have enough.");

    gfxFloat tabWidth = -1;
    for (uint32_t i = tabsEnd; i < aStart + aLength; ++i) {
      Spacing spacing;
      GetSpacingInternal(i, 1, &spacing, true);
      mOffsetFromBlockOriginForTabs += spacing.mBefore;

      if (!mTextRun->CharIsTab(i)) {
        if (mTextRun->IsClusterStart(i)) {
          uint32_t clusterEnd = i + 1;
          while (clusterEnd < mTextRun->GetLength() &&
                 !mTextRun->IsClusterStart(clusterEnd)) {
            ++clusterEnd;
          }
          mOffsetFromBlockOriginForTabs +=
            mTextRun->GetAdvanceWidth(i, clusterEnd - i, nullptr);
        }
      } else {
        if (!mTabWidths) {
          mTabWidths = new TabWidthStore(mFrame->GetContentOffset());
          mFrame->Properties().Set(TabWidthProperty(), mTabWidths);
        }
        double nextTab = AdvanceToNextTab(mOffsetFromBlockOriginForTabs,
                mFrame, mTextRun, &tabWidth);
        mTabWidths->mWidths.AppendElement(TabWidth(i - startOffset, 
                NSToIntRound(nextTab - mOffsetFromBlockOriginForTabs)));
        mOffsetFromBlockOriginForTabs = nextTab;
      }

      mOffsetFromBlockOriginForTabs += spacing.mAfter;
    }

    if (mTabWidths) {
      mTabWidths->mLimit = aStart + aLength - startOffset;
    }
  }

  if (!mTabWidths) {
    
    mFrame->Properties().Delete(TabWidthProperty());
    mTabWidthsAnalyzedLimit = std::max(mTabWidthsAnalyzedLimit,
                                       aStart + aLength - startOffset);
  }
}

gfxFloat
PropertyProvider::GetHyphenWidth()
{
  if (mHyphenWidth < 0) {
    nsAutoPtr<gfxTextRun> hyphenTextRun(GetHyphenTextRun(mTextRun, nullptr, mFrame));
    mHyphenWidth = mLetterSpacing;
    if (hyphenTextRun.get()) {
      mHyphenWidth += hyphenTextRun->GetAdvanceWidth(0, hyphenTextRun->GetLength(), nullptr);
    }
  }
  return mHyphenWidth;
}

void
PropertyProvider::GetHyphenationBreaks(uint32_t aStart, uint32_t aLength,
                                       bool* aBreakBefore)
{
  NS_PRECONDITION(IsInBounds(mStart, mLength, aStart, aLength), "Range out of bounds");
  NS_PRECONDITION(mLength != INT32_MAX, "Can't call this with undefined length");

  if (!mTextStyle->WhiteSpaceCanWrap() ||
      mTextStyle->mHyphens == NS_STYLE_HYPHENS_NONE)
  {
    memset(aBreakBefore, false, aLength*sizeof(bool));
    return;
  }

  
  nsSkipCharsRunIterator
    run(mStart, nsSkipCharsRunIterator::LENGTH_UNSKIPPED_ONLY, aLength);
  run.SetSkippedOffset(aStart);
  
  run.SetVisitSkipped();

  int32_t prevTrailingCharOffset = run.GetPos().GetOriginalOffset() - 1;
  bool allowHyphenBreakBeforeNextChar =
    prevTrailingCharOffset >= mStart.GetOriginalOffset() &&
    prevTrailingCharOffset < mStart.GetOriginalOffset() + mLength &&
    mFrag->CharAt(prevTrailingCharOffset) == CH_SHY;

  while (run.NextRun()) {
    NS_ASSERTION(run.GetRunLength() > 0, "Shouldn't return zero-length runs");
    if (run.IsSkipped()) {
      
      
      
      allowHyphenBreakBeforeNextChar =
        mFrag->CharAt(run.GetOriginalOffset() + run.GetRunLength() - 1) == CH_SHY;
    } else {
      int32_t runOffsetInSubstring = run.GetSkippedOffset() - aStart;
      memset(aBreakBefore + runOffsetInSubstring, false, run.GetRunLength()*sizeof(bool));
      
      aBreakBefore[runOffsetInSubstring] = allowHyphenBreakBeforeNextChar &&
          (!(mFrame->GetStateBits() & TEXT_START_OF_LINE) ||
           run.GetSkippedOffset() > mStart.GetSkippedOffset());
      allowHyphenBreakBeforeNextChar = false;
    }
  }

  if (mTextStyle->mHyphens == NS_STYLE_HYPHENS_AUTO) {
    for (uint32_t i = 0; i < aLength; ++i) {
      if (mTextRun->CanHyphenateBefore(aStart + i)) {
        aBreakBefore[i] = true;
      }
    }
  }
}

void
PropertyProvider::InitializeForDisplay(bool aTrimAfter)
{
  nsTextFrame::TrimmedOffsets trimmed =
    mFrame->GetTrimmedOffsets(mFrag, aTrimAfter);
  mStart.SetOriginalOffset(trimmed.mStart);
  mLength = trimmed.mLength;
  SetupJustificationSpacing();
}

static uint32_t GetSkippedDistance(const gfxSkipCharsIterator& aStart,
                                   const gfxSkipCharsIterator& aEnd)
{
  return aEnd.GetSkippedOffset() - aStart.GetSkippedOffset();
}

void
PropertyProvider::FindJustificationRange(gfxSkipCharsIterator* aStart,
                                         gfxSkipCharsIterator* aEnd)
{
  NS_PRECONDITION(mLength != INT32_MAX, "Can't call this with undefined length");
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
  NS_PRECONDITION(mLength != INT32_MAX, "Can't call this with undefined length");

  if (!(mFrame->GetStateBits() & TEXT_JUSTIFICATION_ENABLED))
    return;

  gfxSkipCharsIterator start(mStart), end(mStart);
  
  
  
  nsTextFrame::TrimmedOffsets trimmed =
    mFrame->GetTrimmedOffsets(mFrag, true);
  end.AdvanceOriginal(trimmed.mLength);
  gfxSkipCharsIterator realEnd(end);
  FindJustificationRange(&start, &end);

  int32_t justifiableCharacters =
    ComputeJustifiableCharacters(start.GetOriginalOffset(),
                                 end.GetOriginalOffset() - start.GetOriginalOffset());
  if (justifiableCharacters == 0) {
    
    
    return;
  }

  gfxFloat naturalWidth =
    mTextRun->GetAdvanceWidth(mStart.GetSkippedOffset(),
                              GetSkippedDistance(mStart, realEnd), this);
  if (mFrame->GetStateBits() & TEXT_HYPHEN_BREAK) {
    nsAutoPtr<gfxTextRun> hyphenTextRun(GetHyphenTextRun(mTextRun, nullptr, mFrame));
    if (hyphenTextRun.get()) {
      naturalWidth +=
        hyphenTextRun->GetAdvanceWidth(0, hyphenTextRun->GetLength(), nullptr);
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

  bool RemoveFrame(nsIFrame* aFrame);

  int32_t FrameCount();

  void Start();

  void Stop();

  NS_DECL_NSITIMERCALLBACK

  static void AddBlinkFrame(nsPresContext* aPresContext, nsIFrame* aFrame);
  static void RemoveBlinkFrame(nsIFrame* aFrame);
  
  static bool     GetBlinkIsOff() { return sState == 3; }
  
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
      bool Equals(const FrameData& aTimer, nsIFrame* const& aFrame) const {
        return aTimer.mFrame == aFrame;
      }
  };

  nsCOMPtr<nsITimer> mTimer;
  nsTArray<FrameData> mFrames;
  nsPresContext* mPresContext;

protected:

  static nsBlinkTimer* sTextBlinker;
  static uint32_t      sState; 
  
};

nsBlinkTimer* nsBlinkTimer::sTextBlinker = nullptr;
uint32_t      nsBlinkTimer::sState = 0;

#ifdef NOISY_BLINK
static PRTime gLastTick;
#endif

nsBlinkTimer::nsBlinkTimer()
{
}

nsBlinkTimer::~nsBlinkTimer()
{
  Stop();
  sTextBlinker = nullptr;
}

void nsBlinkTimer::Start()
{
  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_OK == rv) {
    mTimer->InitWithCallback(this, 250, nsITimer::TYPE_REPEATING_PRECISE_CAN_SKIP);
  }
}

void nsBlinkTimer::Stop()
{
  if (nullptr != mTimer) {
    mTimer->Cancel();
    mTimer = nullptr;
  }
}

NS_IMPL_ISUPPORTS1(nsBlinkTimer, nsITimerCallback)

void nsBlinkTimer::AddFrame(nsPresContext* aPresContext, nsIFrame* aFrame) {
  mFrames.AppendElement(FrameData(aPresContext, aFrame));
  if (1 == mFrames.Length()) {
    Start();
  }
}

bool nsBlinkTimer::RemoveFrame(nsIFrame* aFrame) {
  mFrames.RemoveElement(aFrame, FrameDataComparator());
  
  if (mFrames.IsEmpty()) {
    Stop();
  }
  return true;
}

int32_t nsBlinkTimer::FrameCount() {
  return int32_t(mFrames.Length());
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
  delta = now - gLastTick;
  gLastTick = now;
  PR_snprintf(buf, sizeof(buf), "%lldusec", delta);
  printf("%s\n", buf);
#endif

  uint32_t n = mFrames.Length();
  for (uint32_t i = 0; i < n; i++) {
    FrameData& frameData = mFrames.ElementAt(i);

    
    
    frameData.mFrame->InvalidateFrame();
  }
  return NS_OK;
}



void nsBlinkTimer::AddBlinkFrame(nsPresContext* aPresContext, nsIFrame* aFrame)
{
  if (!sTextBlinker)
  {
    sTextBlinker = new nsBlinkTimer;
  }

  NS_ADDREF(sTextBlinker);

  sTextBlinker->AddFrame(aPresContext, aFrame);
}



void nsBlinkTimer::RemoveBlinkFrame(nsIFrame* aFrame)
{
  NS_ASSERTION(sTextBlinker, "Should have blink timer here");

  nsBlinkTimer* blinkTimer = sTextBlinker;    

  blinkTimer->RemoveFrame(aFrame);  
  NS_RELEASE(blinkTimer);
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



nsTextPaintStyle::nsTextPaintStyle(nsTextFrame* aFrame)
  : mFrame(aFrame),
    mPresContext(aFrame->PresContext()),
    mInitCommonColors(false),
    mInitSelectionColorsAndShadow(false),
    mResolveColors(true),
    mHasSelectionShadow(false)
{
  for (uint32_t i = 0; i < ArrayLength(mSelectionStyle); i++)
    mSelectionStyle[i].mInit = false;
}

bool
nsTextPaintStyle::EnsureSufficientContrast(nscolor *aForeColor, nscolor *aBackColor)
{
  InitCommonColors();

  
  
  int32_t backLuminosityDifference =
            NS_LUMINOSITY_DIFFERENCE(*aBackColor, mFrameBackgroundColor);
  if (backLuminosityDifference >= mSufficientContrast)
    return false;

  
  
  int32_t foreLuminosityDifference =
            NS_LUMINOSITY_DIFFERENCE(*aForeColor, mFrameBackgroundColor);
  if (backLuminosityDifference < foreLuminosityDifference) {
    nscolor tmpColor = *aForeColor;
    *aForeColor = *aBackColor;
    *aBackColor = tmpColor;
    return true;
  }
  return false;
}

nscolor
nsTextPaintStyle::GetTextColor()
{
  if (mFrame->IsSVGText()) {
    if (!mResolveColors)
      return NS_SAME_AS_FOREGROUND_COLOR;

    const nsStyleSVG* style = mFrame->StyleSVG();
    switch (style->mFill.mType) {
      case eStyleSVGPaintType_None:
        return NS_RGBA(0, 0, 0, 0);
      case eStyleSVGPaintType_Color:
        return nsLayoutUtils::GetColor(mFrame, eCSSProperty_fill);
      default:
        NS_ERROR("cannot resolve SVG paint to nscolor");
        return NS_RGBA(0, 0, 0, 255);
    }
  }
  return nsLayoutUtils::GetColor(mFrame, eCSSProperty_color);
}

bool
nsTextPaintStyle::GetSelectionColors(nscolor* aForeColor,
                                     nscolor* aBackColor)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");

  if (!InitSelectionColorsAndShadow())
    return false;

  *aForeColor = mSelectionTextColor;
  *aBackColor = mSelectionBGColor;
  return true;
}

void
nsTextPaintStyle::GetHighlightColors(nscolor* aForeColor,
                                     nscolor* aBackColor)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");
  NS_ASSERTION(aBackColor, "aBackColor is null");
  
  nscolor backColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextHighlightBackground);
  nscolor foreColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextHighlightForeground);
  EnsureSufficientContrast(&foreColor, &backColor);
  *aForeColor = foreColor;
  *aBackColor = backColor;
}

void
nsTextPaintStyle::GetURLSecondaryColor(nscolor* aForeColor)
{
  NS_ASSERTION(aForeColor, "aForeColor is null");

  nscolor textColor = GetTextColor();
  textColor = NS_RGBA(NS_GET_R(textColor),
                      NS_GET_G(textColor),
                      NS_GET_B(textColor),
                      (uint8_t)(255 * 0.5f));
  
  InitCommonColors();
  *aForeColor = NS_ComposeColors(mFrameBackgroundColor, textColor);
}

void
nsTextPaintStyle::GetIMESelectionColors(int32_t  aIndex,
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

bool
nsTextPaintStyle::GetSelectionUnderlineForPaint(int32_t  aIndex,
                                                nscolor* aLineColor,
                                                float*   aRelativeSize,
                                                uint8_t* aStyle)
{
  NS_ASSERTION(aLineColor, "aLineColor is null");
  NS_ASSERTION(aRelativeSize, "aRelativeSize is null");
  NS_ASSERTION(aIndex >= 0 && aIndex < 5, "Index out of range");

  nsSelectionStyle* selectionStyle = GetSelectionStyle(aIndex);
  if (selectionStyle->mUnderlineStyle == NS_STYLE_BORDER_STYLE_NONE ||
      selectionStyle->mUnderlineColor == NS_TRANSPARENT ||
      selectionStyle->mUnderlineRelativeSize <= 0.0f)
    return false;

  *aLineColor = selectionStyle->mUnderlineColor;
  *aRelativeSize = selectionStyle->mUnderlineRelativeSize;
  *aStyle = selectionStyle->mUnderlineStyle;
  return true;
}

void
nsTextPaintStyle::InitCommonColors()
{
  if (mInitCommonColors)
    return;

  nsIFrame* bgFrame =
    nsCSSRendering::FindNonTransparentBackgroundFrame(mFrame);
  NS_ASSERTION(bgFrame, "Cannot find NonTransparentBackgroundFrame.");
  nscolor bgColor =
    bgFrame->GetVisitedDependentColor(eCSSProperty_background_color);

  nscolor defaultBgColor = mPresContext->DefaultBackgroundColor();
  mFrameBackgroundColor = NS_ComposeColors(defaultBgColor, bgColor);

  if (bgFrame->IsThemed()) {
    
    mSufficientContrast = 0;
    mInitCommonColors = true;
    return;
  }

  NS_ASSERTION(NS_GET_A(defaultBgColor) == 255,
               "default background color is not opaque");

  nscolor defaultWindowBackgroundColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_WindowBackground);
  nscolor selectionTextColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectForeground);
  nscolor selectionBGColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectBackground);

  mSufficientContrast =
    std::min(std::min(NS_SUFFICIENT_LUMINOSITY_DIFFERENCE,
                  NS_LUMINOSITY_DIFFERENCE(selectionTextColor,
                                           selectionBGColor)),
                  NS_LUMINOSITY_DIFFERENCE(defaultWindowBackgroundColor,
                                           selectionBGColor));

  mInitCommonColors = true;
}

static Element*
FindElementAncestorForMozSelection(nsIContent* aContent)
{
  NS_ENSURE_TRUE(aContent, nullptr);
  while (aContent && aContent->IsInNativeAnonymousSubtree()) {
    aContent = aContent->GetBindingParent();
  }
  NS_ASSERTION(aContent, "aContent isn't in non-anonymous tree?");
  while (aContent && !aContent->IsElement()) {
    aContent = aContent->GetParent();
  }
  return aContent ? aContent->AsElement() : nullptr;
}

bool
nsTextPaintStyle::InitSelectionColorsAndShadow()
{
  if (mInitSelectionColorsAndShadow)
    return true;

  int16_t selectionFlags;
  int16_t selectionStatus = mFrame->GetSelectionStatus(&selectionFlags);
  if (!(selectionFlags & nsISelectionDisplay::DISPLAY_TEXT) ||
      selectionStatus < nsISelectionController::SELECTION_ON) {
    
    
    
    return false;
  }

  mInitSelectionColorsAndShadow = true;

  nsIFrame* nonGeneratedAncestor = nsLayoutUtils::GetNonGeneratedAncestor(mFrame);
  Element* selectionElement =
    FindElementAncestorForMozSelection(nonGeneratedAncestor->GetContent());

  if (selectionElement &&
      selectionStatus == nsISelectionController::SELECTION_ON) {
    nsRefPtr<nsStyleContext> sc = nullptr;
    sc = mPresContext->StyleSet()->
      ProbePseudoElementStyle(selectionElement,
                              nsCSSPseudoElements::ePseudo_mozSelection,
                              mFrame->StyleContext());
    
    if (sc) {
      mSelectionBGColor =
        sc->GetVisitedDependentColor(eCSSProperty_background_color);
      mSelectionTextColor = sc->GetVisitedDependentColor(eCSSProperty_color);
      mHasSelectionShadow =
        nsRuleNode::HasAuthorSpecifiedRules(sc,
                                            NS_AUTHOR_SPECIFIED_TEXT_SHADOW,
                                            true);
      if (mHasSelectionShadow) {
        mSelectionShadow = sc->StyleText()->mTextShadow;
      }
      return true;
    }
  }

  nscolor selectionBGColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectBackground);

  if (selectionStatus == nsISelectionController::SELECTION_ATTENTION) {
    mSelectionBGColor =
      LookAndFeel::GetColor(
        LookAndFeel::eColorID_TextSelectBackgroundAttention);
    mSelectionBGColor  = EnsureDifferentColors(mSelectionBGColor,
                                               selectionBGColor);
  } else if (selectionStatus != nsISelectionController::SELECTION_ON) {
    mSelectionBGColor =
      LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectBackgroundDisabled);
    mSelectionBGColor  = EnsureDifferentColors(mSelectionBGColor,
                                               selectionBGColor);
  } else {
    mSelectionBGColor = selectionBGColor;
  }

  mSelectionTextColor =
    LookAndFeel::GetColor(LookAndFeel::eColorID_TextSelectForeground);

  if (mResolveColors) {
    
    if (mSelectionTextColor == NS_DONT_CHANGE_COLOR) {
      nsCSSProperty property = mFrame->IsSVGText() ? eCSSProperty_fill :
                                                     eCSSProperty_color;
      nscoord frameColor = mFrame->GetVisitedDependentColor(property);
      mSelectionTextColor = EnsureDifferentColors(frameColor, mSelectionBGColor);
    } else {
      EnsureSufficientContrast(&mSelectionTextColor, &mSelectionBGColor);
    }
  } else {
    if (mSelectionTextColor == NS_DONT_CHANGE_COLOR) {
      mSelectionTextColor = NS_SAME_AS_FOREGROUND_COLOR;
    }
  }
  return true;
}

nsTextPaintStyle::nsSelectionStyle*
nsTextPaintStyle::GetSelectionStyle(int32_t aIndex)
{
  InitSelectionStyle(aIndex);
  return &mSelectionStyle[aIndex];
}

struct StyleIDs {
  LookAndFeel::ColorID mForeground, mBackground, mLine;
  LookAndFeel::IntID mLineStyle;
  LookAndFeel::FloatID mLineRelativeSize;
};
static StyleIDs SelectionStyleIDs[] = {
  { LookAndFeel::eColorID_IMERawInputForeground,
    LookAndFeel::eColorID_IMERawInputBackground,
    LookAndFeel::eColorID_IMERawInputUnderline,
    LookAndFeel::eIntID_IMERawInputUnderlineStyle,
    LookAndFeel::eFloatID_IMEUnderlineRelativeSize },
  { LookAndFeel::eColorID_IMESelectedRawTextForeground,
    LookAndFeel::eColorID_IMESelectedRawTextBackground,
    LookAndFeel::eColorID_IMESelectedRawTextUnderline,
    LookAndFeel::eIntID_IMESelectedRawTextUnderlineStyle,
    LookAndFeel::eFloatID_IMEUnderlineRelativeSize },
  { LookAndFeel::eColorID_IMEConvertedTextForeground,
    LookAndFeel::eColorID_IMEConvertedTextBackground,
    LookAndFeel::eColorID_IMEConvertedTextUnderline,
    LookAndFeel::eIntID_IMEConvertedTextUnderlineStyle,
    LookAndFeel::eFloatID_IMEUnderlineRelativeSize },
  { LookAndFeel::eColorID_IMESelectedConvertedTextForeground,
    LookAndFeel::eColorID_IMESelectedConvertedTextBackground,
    LookAndFeel::eColorID_IMESelectedConvertedTextUnderline,
    LookAndFeel::eIntID_IMESelectedConvertedTextUnderline,
    LookAndFeel::eFloatID_IMEUnderlineRelativeSize },
  { LookAndFeel::eColorID_LAST_COLOR,
    LookAndFeel::eColorID_LAST_COLOR,
    LookAndFeel::eColorID_SpellCheckerUnderline,
    LookAndFeel::eIntID_SpellCheckerUnderlineStyle,
    LookAndFeel::eFloatID_SpellCheckerUnderlineRelativeSize }
};

void
nsTextPaintStyle::InitSelectionStyle(int32_t aIndex)
{
  NS_ASSERTION(aIndex >= 0 && aIndex < 5, "aIndex is invalid");
  nsSelectionStyle* selectionStyle = &mSelectionStyle[aIndex];
  if (selectionStyle->mInit)
    return;

  StyleIDs* styleIDs = &SelectionStyleIDs[aIndex];

  nscolor foreColor, backColor;
  if (styleIDs->mForeground == LookAndFeel::eColorID_LAST_COLOR) {
    foreColor = NS_SAME_AS_FOREGROUND_COLOR;
  } else {
    foreColor = LookAndFeel::GetColor(styleIDs->mForeground);
  }
  if (styleIDs->mBackground == LookAndFeel::eColorID_LAST_COLOR) {
    backColor = NS_TRANSPARENT;
  } else {
    backColor = LookAndFeel::GetColor(styleIDs->mBackground);
  }

  
  NS_ASSERTION(foreColor != NS_TRANSPARENT,
               "foreColor cannot be NS_TRANSPARENT");
  NS_ASSERTION(backColor != NS_SAME_AS_FOREGROUND_COLOR,
               "backColor cannot be NS_SAME_AS_FOREGROUND_COLOR");
  NS_ASSERTION(backColor != NS_40PERCENT_FOREGROUND_COLOR,
               "backColor cannot be NS_40PERCENT_FOREGROUND_COLOR");

  if (mResolveColors) {
    foreColor = GetResolvedForeColor(foreColor, GetTextColor(), backColor);

    if (NS_GET_A(backColor) > 0)
      EnsureSufficientContrast(&foreColor, &backColor);
  }

  nscolor lineColor;
  float relativeSize;
  uint8_t lineStyle;
  GetSelectionUnderline(mPresContext, aIndex,
                        &lineColor, &relativeSize, &lineStyle);

  if (mResolveColors)
    lineColor = GetResolvedForeColor(lineColor, foreColor, backColor);

  selectionStyle->mTextColor       = foreColor;
  selectionStyle->mBGColor         = backColor;
  selectionStyle->mUnderlineColor  = lineColor;
  selectionStyle->mUnderlineStyle  = lineStyle;
  selectionStyle->mUnderlineRelativeSize = relativeSize;
  selectionStyle->mInit            = true;
}

 bool
nsTextPaintStyle::GetSelectionUnderline(nsPresContext* aPresContext,
                                        int32_t aIndex,
                                        nscolor* aLineColor,
                                        float* aRelativeSize,
                                        uint8_t* aStyle)
{
  NS_ASSERTION(aPresContext, "aPresContext is null");
  NS_ASSERTION(aRelativeSize, "aRelativeSize is null");
  NS_ASSERTION(aStyle, "aStyle is null");
  NS_ASSERTION(aIndex >= 0 && aIndex < 5, "Index out of range");

  StyleIDs& styleID = SelectionStyleIDs[aIndex];

  nscolor color = LookAndFeel::GetColor(styleID.mLine);
  int32_t style = LookAndFeel::GetInt(styleID.mLineStyle);
  if (style > NS_STYLE_TEXT_DECORATION_STYLE_MAX) {
    NS_ERROR("Invalid underline style value is specified");
    style = NS_STYLE_TEXT_DECORATION_STYLE_SOLID;
  }
  float size = LookAndFeel::GetFloat(styleID.mLineRelativeSize);

  NS_ASSERTION(size, "selection underline relative size must be larger than 0");

  if (aLineColor) {
    *aLineColor = color;
  }
  *aRelativeSize = size;
  *aStyle = style;

  return style != NS_STYLE_TEXT_DECORATION_STYLE_NONE &&
         color != NS_TRANSPARENT &&
         size > 0.0f;
}

bool
nsTextPaintStyle::GetSelectionShadow(nsCSSShadowArray** aShadow)
{
  if (!InitSelectionColorsAndShadow()) {
    return false;
  }

  if (mHasSelectionShadow) {
    *aShadow = mSelectionShadow;
    return true;
  }

  return false;
}

inline nscolor Get40PercentColor(nscolor aForeColor, nscolor aBackColor)
{
  nscolor foreColor = NS_RGBA(NS_GET_R(aForeColor),
                              NS_GET_G(aForeColor),
                              NS_GET_B(aForeColor),
                              (uint8_t)(255 * 0.4f));
  
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
a11y::AccType
nsTextFrame::AccessibleType()
{
  if (IsEmpty()) {
    nsAutoString renderedWhitespace;
    GetRenderedText(&renderedWhitespace, nullptr, nullptr, 0, 1);
    if (renderedWhitespace.IsEmpty()) {
      return a11y::eNoType;
    }
  }

  return a11y::eTextLeafType;
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

  
  
  aContent->DeleteProperty(nsGkAtoms::newline);
  if (PresContext()->BidiEnabled()) {
    aContent->DeleteProperty(nsGkAtoms::flowlength);
  }

  
  aContent->UnsetFlags(NS_CREATE_FRAME_IF_NON_WHITESPACE);

  
  
  return nsFrame::Init(aContent, aParent, aPrevInFlow);
}

void
nsTextFrame::ClearFrameOffsetCache()
{
  
  if (GetStateBits() & TEXT_IN_OFFSET_CACHE) {
    nsIFrame* primaryFrame = mContent->GetPrimaryFrame();
    if (primaryFrame) {
      
      
      
      
      primaryFrame->Properties().Delete(OffsetToFrameProperty());
    }
    RemoveStateBits(TEXT_IN_OFFSET_CACHE);
  }
}

void
nsTextFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  ClearFrameOffsetCache();

  
  
  
  ClearTextRuns();
  if (mNextContinuation) {
    mNextContinuation->SetPrevInFlow(nullptr);
  }
  
  nsFrame::DestroyFrom(aDestructRoot);
}

class nsContinuingTextFrame : public nsTextFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewContinuingTextFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

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
    return (GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? mPrevContinuation : nullptr;
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

  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  
  virtual nsresult GetRenderedText(nsAString* aString = nullptr,
                                   gfxSkipChars* aSkipChars = nullptr,
                                   gfxSkipCharsIterator* aSkipIter = nullptr,
                                   uint32_t aSkippedStartOffset = 0,
                                   uint32_t aSkippedMaxLength = UINT32_MAX)
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

#ifdef IBMBIDI
  nsTextFrame* nextContinuation =
    static_cast<nsTextFrame*>(aPrevInFlow->GetNextContinuation());
#endif 
  
  SetPrevInFlow(aPrevInFlow);
  aPrevInFlow->SetNextInFlow(this);
  nsTextFrame* prev = static_cast<nsTextFrame*>(aPrevInFlow);
  mContentOffset = prev->GetContentOffset() + prev->GetContentLengthHint();
  NS_ASSERTION(mContentOffset < int32_t(aContent->GetText()->GetLength()),
               "Creating ContinuingTextFrame, but there is no more content");
  if (prev->StyleContext() != StyleContext()) {
    
    
    prev->ClearTextRuns();
  } else {
    float inflation = prev->GetFontSizeInflation();
    SetFontSizeInflation(inflation);
    mTextRun = prev->GetTextRun(nsTextFrame::eInflated);
    if (inflation != 1.0f) {
      gfxTextRun *uninflatedTextRun =
        prev->GetTextRun(nsTextFrame::eNotInflated);
      if (uninflatedTextRun) {
        SetTextRun(uninflatedTextRun, nsTextFrame::eNotInflated, 1.0f);
      }
    }
  }
#ifdef IBMBIDI
  if (aPrevInFlow->GetStateBits() & NS_FRAME_IS_BIDI) {
    FramePropertyTable *propTable = PresContext()->PropertyTable();
    
    
    void* embeddingLevel = propTable->Get(aPrevInFlow, EmbeddingLevelProperty());
    void* baseLevel = propTable->Get(aPrevInFlow, BaseLevelProperty());
    void* paragraphDepth = propTable->Get(aPrevInFlow, ParagraphDepthProperty());
    propTable->Set(this, EmbeddingLevelProperty(), embeddingLevel);
    propTable->Set(this, BaseLevelProperty(), baseLevel);
    propTable->Set(this, ParagraphDepthProperty(), paragraphDepth);

    if (nextContinuation) {
      SetNextContinuation(nextContinuation);
      nextContinuation->SetPrevContinuation(this);
      
      while (nextContinuation &&
             nextContinuation->GetContentOffset() < mContentOffset) {
        NS_ASSERTION(
          embeddingLevel == propTable->Get(nextContinuation, EmbeddingLevelProperty()) &&
          baseLevel == propTable->Get(nextContinuation, BaseLevelProperty()) &&
          paragraphDepth == propTable->Get(nextContinuation, ParagraphDepthProperty()),
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
nsContinuingTextFrame::DestroyFrom(nsIFrame* aDestructRoot)
{
  ClearFrameOffsetCache();

  
  
  
  
  
  
  
  
  
  if ((GetStateBits() & TEXT_IN_TEXTRUN_USER_DATA) ||
      (GetStateBits() & TEXT_IN_UNINFLATED_TEXTRUN_USER_DATA) ||
      (!mPrevContinuation &&
       !(GetStateBits() & TEXT_STYLE_MATCHES_PREV_CONTINUATION)) ||
      (mPrevContinuation &&
       mPrevContinuation->StyleContext() != StyleContext())) {
    ClearTextRuns();
    
    
    if (mPrevContinuation) {
      nsTextFrame *prevContinuationText =
        static_cast<nsTextFrame*>(mPrevContinuation);
      prevContinuationText->ClearTextRuns();
    }
  }
  nsSplittableFrame::RemoveFromFlow(this);
  
  nsFrame::DestroyFrom(aDestructRoot);
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
nsTextFrame::GetMinWidth(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::MinWidthFromInline(this, aRenderingContext);
}


 nscoord
nsTextFrame::GetPrefWidth(nsRenderingContext *aRenderingContext)
{
  return nsLayoutUtils::PrefWidthFromInline(this, aRenderingContext);
}

 void
nsContinuingTextFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                         InlineMinWidthData *aData)
{
  
  return;
}

 void
nsContinuingTextFrame::AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
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
  bool isZero = state & NS_FRAME_FIRST_REFLOW;
  bool isDirty = state & NS_FRAME_IS_DIRTY;
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
  if (0 != (mState & TEXT_BLINK_ON))
  {
    nsBlinkTimer::RemoveBlinkFrame(this);
  }
}

NS_IMETHODIMP
nsTextFrame::GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor)
{
  FillCursorInformationFromStyle(StyleUserInterface(), aCursor);  
  if (NS_STYLE_CURSOR_AUTO == aCursor.mCursor) {
    aCursor.mCursor = NS_STYLE_CURSOR_TEXT;
    
    if (mContent->IsEditable()) {
      return NS_OK;
    }

    
    nsIFrame *ancestorFrame = this;
    while ((ancestorFrame = ancestorFrame->GetParent()) != nullptr) {
      nsIContent *ancestorContent = ancestorFrame->GetContent();
      if (ancestorContent && ancestorContent->HasAttr(kNameSpaceID_None, nsGkAtoms::tabindex)) {
        nsAutoString tabIndexStr;
        ancestorContent->GetAttr(kNameSpaceID_None, nsGkAtoms::tabindex, tabIndexStr);
        if (!tabIndexStr.IsEmpty()) {
          nsresult rv;
          int32_t tabIndexVal = tabIndexStr.ToInteger(&rv);
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
nsTextFrame::InvalidateFrame(uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    nsIFrame* svgTextFrame =
      nsLayoutUtils::GetClosestFrameOfType(GetParent(),
                                           nsGkAtoms::svgTextFrame2);
    svgTextFrame->InvalidateFrame();
    return;
  }
  nsTextFrameBase::InvalidateFrame(aDisplayItemKey);
}

void
nsTextFrame::InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey)
{
  if (IsSVGText()) {
    nsIFrame* svgTextFrame =
      nsLayoutUtils::GetClosestFrameOfType(GetParent(),
                                           nsGkAtoms::svgTextFrame2);
    svgTextFrame->InvalidateFrame();
    return;
  }
  nsTextFrameBase::InvalidateFrameWithRect(aRect, aDisplayItemKey);
}

gfxTextRun*
nsTextFrame::GetUninflatedTextRun()
{
  return static_cast<gfxTextRun*>(
           Properties().Get(UninflatedTextRunProperty()));
}

void
nsTextFrame::SetTextRun(gfxTextRun* aTextRun, TextRunType aWhichTextRun,
                        float aInflation)
{
  NS_ASSERTION(aTextRun, "must have text run");

  
  
  
  
  if (aWhichTextRun == eInflated) {
    if (HasFontSizeInflation() && aInflation == 1.0f) {
      
      
      ClearTextRun(nullptr, nsTextFrame::eNotInflated);
    }
    SetFontSizeInflation(aInflation);
  } else {
    NS_ABORT_IF_FALSE(aInflation == 1.0f, "unexpected inflation");
    if (HasFontSizeInflation()) {
      Properties().Set(UninflatedTextRunProperty(), aTextRun);
      return;
    }
    
  }

  mTextRun = aTextRun;

  
  
  
}

bool
nsTextFrame::RemoveTextRun(gfxTextRun* aTextRun)
{
  if (aTextRun == mTextRun) {
    mTextRun = nullptr;
    return true;
  }
  FrameProperties props = Properties();
  if ((GetStateBits() & TEXT_HAS_FONT_INFLATION) &&
      props.Get(UninflatedTextRunProperty()) == aTextRun) {
    props.Delete(UninflatedTextRunProperty());
    return true;
  }
  return false;
}

void
nsTextFrame::ClearTextRun(nsTextFrame* aStartContinuation,
                          TextRunType aWhichTextRun)
{
  gfxTextRun* textRun = GetTextRun(aWhichTextRun);
  if (!textRun) {
    return;
  }

  DebugOnly<bool> checkmTextrun = textRun == mTextRun;
  UnhookTextRunFromFrames(textRun, aStartContinuation);
  MOZ_ASSERT(checkmTextrun ? !mTextRun
                           : !Properties().Get(UninflatedTextRunProperty()));

  











  if (!textRun->GetUserData()) {
    
    gTextRuns->RemoveFromCache(textRun);
    delete textRun;
  }
}

NS_IMETHODIMP
nsTextFrame::CharacterDataChanged(CharacterDataChangeInfo* aInfo)
{
  mContent->DeleteProperty(nsGkAtoms::newline);
  if (PresContext()->BidiEnabled()) {
    mContent->DeleteProperty(nsGkAtoms::flowlength);
  }

  
  
  nsTextFrame* next;
  nsTextFrame* textFrame = this;
  while (true) {
    next = static_cast<nsTextFrame*>(textFrame->GetNextContinuation());
    if (!next || next->GetContentOffset() > int32_t(aInfo->mChangeStart))
      break;
    textFrame = next;
  }

  int32_t endOfChangedText = aInfo->mChangeStart + aInfo->mReplaceLength;
  nsTextFrame* lastDirtiedFrame = nullptr;

  nsIPresShell* shell = PresContext()->GetPresShell();
  do {
    
    
    textFrame->mState &= ~TEXT_WHITESPACE_FLAGS;
    textFrame->ClearTextRuns();
    if (!lastDirtiedFrame ||
        lastDirtiedFrame->GetParent() != textFrame->GetParent()) {
      
      shell->FrameNeedsReflow(textFrame, nsIPresShell::eStyleChange,
                              NS_FRAME_IS_DIRTY);
      lastDirtiedFrame = textFrame;
    } else {
      
      
      
      textFrame->AddStateBits(NS_FRAME_IS_DIRTY);
    }
    textFrame->InvalidateFrame();

    
    
    
    
    
    
    
    if (textFrame->mContentOffset > endOfChangedText) {
      textFrame->mContentOffset = endOfChangedText;
    }

    textFrame = static_cast<nsTextFrame*>(textFrame->GetNextContinuation());
  } while (textFrame && textFrame->GetContentOffset() < int32_t(aInfo->mChangeEnd));

  
  
  int32_t sizeChange =
    aInfo->mChangeStart + aInfo->mReplaceLength - aInfo->mChangeEnd;

  if (sizeChange) {
    
    
    while (textFrame) {
      textFrame->mContentOffset += sizeChange;
      
      
      textFrame->ClearTextRuns();
      textFrame = static_cast<nsTextFrame*>(textFrame->GetNextContinuation());
    }
  }

  return NS_OK;
}

 void
nsTextFrame::DidSetStyleContext(nsStyleContext* aOldStyleContext)
{
  nsFrame::DidSetStyleContext(aOldStyleContext);
  ClearTextRuns();
}

class nsDisplayTextGeometry : public nsDisplayItemGenericGeometry
{
public:
  nsDisplayTextGeometry(nsDisplayItem* aItem, nsDisplayListBuilder* aBuilder)
    : nsDisplayItemGenericGeometry(aItem, aBuilder)
  {
    nsTextFrame* f = static_cast<nsTextFrame*>(aItem->GetUnderlyingFrame());
    f->GetTextDecorations(f->PresContext(), nsTextFrame::eResolvedColors, mDecorations);
  }
 
  




  nsTextFrame::TextDecorations mDecorations;
};

class nsDisplayText : public nsCharClipDisplayItem {
public:
  nsDisplayText(nsDisplayListBuilder* aBuilder, nsTextFrame* aFrame) :
    nsCharClipDisplayItem(aBuilder, aFrame),
    mDisableSubpixelAA(false) {
    MOZ_COUNT_CTOR(nsDisplayText);
  }
#ifdef NS_BUILD_REFCNT_LOGGING
  virtual ~nsDisplayText() {
    MOZ_COUNT_DTOR(nsDisplayText);
  }
#endif

  virtual nsRect GetBounds(nsDisplayListBuilder* aBuilder, bool* aSnap) {
    *aSnap = false;
    nsRect temp = mFrame->GetVisualOverflowRectRelativeToSelf() + ToReferenceFrame();
    
    temp.Inflate(mFrame->PresContext()->AppUnitsPerDevPixel());
    return temp;
  }
  virtual void HitTest(nsDisplayListBuilder* aBuilder, const nsRect& aRect,
                       HitTestState* aState, nsTArray<nsIFrame*> *aOutFrames) {
    if (nsRect(ToReferenceFrame(), mFrame->GetSize()).Intersects(aRect)) {
      aOutFrames->AppendElement(mFrame);
    }
  }
  virtual void Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx);
  NS_DISPLAY_DECL_NAME("Text", TYPE_TEXT)

  virtual nsRect GetComponentAlphaBounds(nsDisplayListBuilder* aBuilder)
  {
    bool snap;
    return GetBounds(aBuilder, &snap);
  }

  virtual nsDisplayItemGeometry* AllocateGeometry(nsDisplayListBuilder* aBuilder)
  {
    return new nsDisplayTextGeometry(this, aBuilder);
  }

  virtual void ComputeInvalidationRegion(nsDisplayListBuilder* aBuilder,
                                         const nsDisplayItemGeometry* aGeometry,
                                         nsRegion *aInvalidRegion)
  {
    const nsDisplayTextGeometry* geometry = static_cast<const nsDisplayTextGeometry*>(aGeometry);
    nsTextFrame* f = static_cast<nsTextFrame*>(mFrame);

    nsTextFrame::TextDecorations decorations;
    f->GetTextDecorations(f->PresContext(), nsTextFrame::eResolvedColors, decorations);

    bool snap;
    nsRect newRect = geometry->mBounds;
    nsRect oldRect = GetBounds(aBuilder, &snap);
    if (decorations != geometry->mDecorations ||
        !oldRect.IsEqualInterior(newRect) ||
        !geometry->mBorderRect.IsEqualInterior(GetBorderRect())) {
      aInvalidRegion->Or(oldRect, newRect);
    }
  }
  
  virtual void DisableComponentAlpha() { mDisableSubpixelAA = true; }

  bool mDisableSubpixelAA;
};

void
nsDisplayText::Paint(nsDisplayListBuilder* aBuilder,
                     nsRenderingContext* aCtx) {
  PROFILER_LABEL("nsDisplayText", "Paint");
  
  
  
  nsRect extraVisible = mVisibleRect;
  nscoord appUnitsPerDevPixel = mFrame->PresContext()->AppUnitsPerDevPixel();
  extraVisible.Inflate(appUnitsPerDevPixel, appUnitsPerDevPixel);
  nsTextFrame* f = static_cast<nsTextFrame*>(mFrame);

  gfxContextAutoDisableSubpixelAntialiasing disable(aCtx->ThebesContext(),
                                                    mDisableSubpixelAA);
  NS_ASSERTION(mLeftEdge >= 0, "illegal left edge");
  NS_ASSERTION(mRightEdge >= 0, "illegal right edge");
  f->PaintText(aCtx, ToReferenceFrame(), extraVisible, *this);
}

void
nsTextFrame::BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists)
{
  if (!IsVisibleForPainting(aBuilder))
    return;
  
  DO_GLOBAL_REFLOW_COUNT_DSP("nsTextFrame");

  if ((0 != (mState & TEXT_BLINK_ON)) && nsBlinkTimer::GetBlinkIsOff() &&
      PresContext()->IsDynamic() && !aBuilder->IsForEventDelivery())
    return;
    
  aLists.Content()->AppendNewToTop(
    new (aBuilder) nsDisplayText(aBuilder, this));
}

static nsIFrame*
GetGeneratedContentOwner(nsIFrame* aFrame, bool* aIsBefore)
{
  *aIsBefore = false;
  while (aFrame && (aFrame->GetStateBits() & NS_FRAME_GENERATED_CONTENT)) {
    if (aFrame->StyleContext()->GetPseudo() == nsCSSPseudoElements::before) {
      *aIsBefore = true;
    }
    aFrame = aFrame->GetParent();
  }
  return aFrame;
}

SelectionDetails*
nsTextFrame::GetSelectionDetails()
{
  const nsFrameSelection* frameSelection = GetConstFrameSelection();
  if (frameSelection->GetTableCellSelection()) {
    return nullptr;
  }
  if (!(GetStateBits() & NS_FRAME_GENERATED_CONTENT)) {
    SelectionDetails* details =
      frameSelection->LookUpSelection(mContent, GetContentOffset(),
                                      GetContentLength(), false);
    SelectionDetails* sd;
    for (sd = details; sd; sd = sd->mNext) {
      sd->mStart += mContentOffset;
      sd->mEnd += mContentOffset;
    }
    return details;
  }

  
  
  bool isBefore;
  nsIFrame* owner = GetGeneratedContentOwner(this, &isBefore);
  if (!owner || !owner->GetContent())
    return nullptr;

  SelectionDetails* details =
    frameSelection->LookUpSelection(owner->GetContent(),
        isBefore ? 0 : owner->GetContent()->GetChildCount(), 0, false);
  SelectionDetails* sd;
  for (sd = details; sd; sd = sd->mNext) {
    
    sd->mStart = GetContentOffset();
    sd->mEnd = GetContentEnd();
  }
  return details;
}

static void
PaintSelectionBackground(gfxContext* aCtx, nsPresContext* aPresContext,
                         nscolor aColor, const gfxRect& aDirtyRect,
                         const gfxRect& aRect,
                         nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  if (aCallbacks) {
    aCallbacks->NotifyBeforeSelectionBackground(aColor);
  }

  gfxRect r = aRect.Intersect(aDirtyRect);
  
  int32_t app = aPresContext->AppUnitsPerDevPixel();
  aCtx->NewPath();
  
  aCtx->Rectangle(gfxRect(r.X() / app, r.Y() / app,
                          r.Width() / app, r.Height() / app), true);

  if (aCallbacks) {
    aCallbacks->NotifySelectionBackgroundPathEmitted();
  } else {
    aCtx->SetColor(gfxRGBA(aColor));
    aCtx->Fill();
  }
}

void
nsTextFrame::GetTextDecorations(
                    nsPresContext* aPresContext,
                    nsTextFrame::TextDecorationColorResolution aColorResolution,
                    nsTextFrame::TextDecorations& aDecorations)
{
  const nsCompatibility compatMode = aPresContext->CompatibilityMode();

  bool useOverride = false;
  nscolor overrideColor = NS_RGBA(0, 0, 0, 0);

  
  
  
  
  
  nscoord frameTopOffset = mAscent,
          baselineOffset = 0;

  bool nearestBlockFound = false;

  for (nsIFrame* f = this, *fChild = nullptr;
       f;
       fChild = f,
       f = nsLayoutUtils::GetParentOrPlaceholderFor(f))
  {
    nsStyleContext *const context = f->StyleContext();
    if (!context->HasTextDecorationLines()) {
      break;
    }

    const nsStyleTextReset *const styleText = context->StyleTextReset();
    const uint8_t textDecorations = styleText->mTextDecorationLine;

    if (!useOverride &&
        (NS_STYLE_TEXT_DECORATION_LINE_OVERRIDE_ALL & textDecorations)) {
      
      
      useOverride = true;
      overrideColor =
        nsLayoutUtils::GetColor(f, eCSSProperty_text_decoration_color);
    }

    const bool firstBlock = !nearestBlockFound && nsLayoutUtils::GetAsBlock(f);

    
    
    
    
    
    
    if (firstBlock) {
      
      if (fChild->VerticalAlignEnum() != NS_STYLE_VERTICAL_ALIGN_BASELINE) {
        
        
        
        baselineOffset =
          frameTopOffset - (fChild->GetRect().y - fChild->GetRelativeOffset().y)
          - NS_PTR_TO_INT32(
              fChild->Properties().Get(nsIFrame::LineBaselineOffset()));
      }
    }
    else if (!nearestBlockFound) {
      baselineOffset = frameTopOffset - f->GetBaseline();
    }

    nearestBlockFound = nearestBlockFound || firstBlock;
    frameTopOffset += f->GetRect().y - f->GetRelativeOffset().y;

    const uint8_t style = styleText->GetDecorationStyle();
    if (textDecorations) {
      nscolor color;
      if (useOverride) {
        color = overrideColor;
      } else if (IsSVGText()) {
        
        
        
        
        
        
        
        color = aColorResolution == eResolvedColors ?
                  nsLayoutUtils::GetColor(f, eCSSProperty_fill) :
                  NS_SAME_AS_FOREGROUND_COLOR;
      } else {
        color = nsLayoutUtils::GetColor(f, eCSSProperty_text_decoration_color);
      }

      if (textDecorations & NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE) {
        aDecorations.mUnderlines.AppendElement(
          nsTextFrame::LineDecoration(f, baselineOffset, color, style));
      }
      if (textDecorations & NS_STYLE_TEXT_DECORATION_LINE_OVERLINE) {
        aDecorations.mOverlines.AppendElement(
          nsTextFrame::LineDecoration(f, baselineOffset, color, style));
      }
      if (textDecorations & NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH) {
        aDecorations.mStrikes.AppendElement(
          nsTextFrame::LineDecoration(f, baselineOffset, color, style));
      }
    }

    
    
    uint8_t display = f->GetDisplay();
    if (display != NS_STYLE_DISPLAY_INLINE &&
        nsStyleDisplay::IsDisplayTypeInlineOutside(display)) {
      break;
    }

    if (compatMode == eCompatibility_NavQuirks) {
      
      if (f->GetContent()->IsHTML(nsGkAtoms::table)) {
        break;
      }
    } else {
      
      
      if (f->IsFloating() || f->IsAbsolutelyPositioned()) {
        break;
      }
    }
  }
}

void
nsTextFrame::UnionAdditionalOverflow(nsPresContext* aPresContext,
                                     const nsHTMLReflowState& aBlockReflowState,
                                     PropertyProvider& aProvider,
                                     nsRect* aVisualOverflowRect,
                                     bool aIncludeTextDecorations)
{
  
  nsRect shadowRect =
    nsLayoutUtils::GetTextShadowRectsUnion(*aVisualOverflowRect, this);
  aVisualOverflowRect->UnionRect(*aVisualOverflowRect, shadowRect);

  if (IsFloatingFirstLetterChild()) {
    
    
    nsIFrame* firstLetterFrame = aBlockReflowState.frame;
    uint8_t decorationStyle = firstLetterFrame->StyleContext()->
                                StyleTextReset()->GetDecorationStyle();
    
    
    
    if (decorationStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
      decorationStyle = NS_STYLE_TEXT_DECORATION_STYLE_SOLID;
    }
    nsFontMetrics* fontMetrics = aProvider.GetFontMetrics();
    nscoord underlineOffset, underlineSize;
    fontMetrics->GetUnderline(underlineOffset, underlineSize);
    nscoord maxAscent = fontMetrics->MaxAscent();

    gfxFloat appUnitsPerDevUnit = aPresContext->AppUnitsPerDevPixel();
    gfxFloat gfxWidth = aVisualOverflowRect->width / appUnitsPerDevUnit;
    gfxFloat gfxAscent = gfxFloat(mAscent) / appUnitsPerDevUnit;
    gfxFloat gfxMaxAscent = maxAscent / appUnitsPerDevUnit;
    gfxFloat gfxUnderlineSize = underlineSize / appUnitsPerDevUnit;
    gfxFloat gfxUnderlineOffset = underlineOffset / appUnitsPerDevUnit;
    nsRect underlineRect =
      nsCSSRendering::GetTextDecorationRect(aPresContext,
        gfxSize(gfxWidth, gfxUnderlineSize), gfxAscent, gfxUnderlineOffset,
        NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE, decorationStyle);
    nsRect overlineRect =
      nsCSSRendering::GetTextDecorationRect(aPresContext,
        gfxSize(gfxWidth, gfxUnderlineSize), gfxAscent, gfxMaxAscent,
        NS_STYLE_TEXT_DECORATION_LINE_OVERLINE, decorationStyle);

    aVisualOverflowRect->UnionRect(*aVisualOverflowRect, underlineRect);
    aVisualOverflowRect->UnionRect(*aVisualOverflowRect, overlineRect);

    
    
    
  }
  if (aIncludeTextDecorations) {
    
    
    
    
    TextDecorations textDecs;
    GetTextDecorations(aPresContext, eResolvedColors, textDecs);
    if (textDecs.HasDecorationLines()) {
      nscoord inflationMinFontSize =
        nsLayoutUtils::InflationMinFontSizeFor(aBlockReflowState.frame);

      const nscoord width = GetSize().width;
      const gfxFloat appUnitsPerDevUnit = aPresContext->AppUnitsPerDevPixel(),
                     gfxWidth = width / appUnitsPerDevUnit,
                     ascent = gfxFloat(mAscent) / appUnitsPerDevUnit;
      nscoord top(nscoord_MAX), bottom(nscoord_MIN);
      
      
      for (uint32_t i = 0; i < textDecs.mUnderlines.Length(); ++i) {
        const LineDecoration& dec = textDecs.mUnderlines[i];
        uint8_t decorationStyle = dec.mStyle;
        
        
        
        if (decorationStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
          decorationStyle = NS_STYLE_TEXT_DECORATION_STYLE_SOLID;
        }

        float inflation = nsLayoutUtils::FontSizeInflationInner(dec.mFrame,
                            inflationMinFontSize);
        const gfxFont::Metrics metrics =
          GetFirstFontMetrics(GetFontGroupForFrame(dec.mFrame, inflation));

        const nsRect decorationRect =
          nsCSSRendering::GetTextDecorationRect(aPresContext,
            gfxSize(gfxWidth, metrics.underlineSize),
            ascent, metrics.underlineOffset,
            NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE, decorationStyle) +
          nsPoint(0, -dec.mBaselineOffset);

        top = std::min(decorationRect.y, top);
        bottom = std::max(decorationRect.YMost(), bottom);
      }
      for (uint32_t i = 0; i < textDecs.mOverlines.Length(); ++i) {
        const LineDecoration& dec = textDecs.mOverlines[i];
        uint8_t decorationStyle = dec.mStyle;
        
        
        
        if (decorationStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
          decorationStyle = NS_STYLE_TEXT_DECORATION_STYLE_SOLID;
        }

        float inflation = nsLayoutUtils::FontSizeInflationInner(dec.mFrame,
                            inflationMinFontSize);
        const gfxFont::Metrics metrics =
          GetFirstFontMetrics(GetFontGroupForFrame(dec.mFrame, inflation));

        const nsRect decorationRect =
          nsCSSRendering::GetTextDecorationRect(aPresContext,
            gfxSize(gfxWidth, metrics.underlineSize),
            ascent, metrics.maxAscent,
            NS_STYLE_TEXT_DECORATION_LINE_OVERLINE, decorationStyle) +
          nsPoint(0, -dec.mBaselineOffset);

        top = std::min(decorationRect.y, top);
        bottom = std::max(decorationRect.YMost(), bottom);
      }
      for (uint32_t i = 0; i < textDecs.mStrikes.Length(); ++i) {
        const LineDecoration& dec = textDecs.mStrikes[i];
        uint8_t decorationStyle = dec.mStyle;
        
        
        
        if (decorationStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
          decorationStyle = NS_STYLE_TEXT_DECORATION_STYLE_SOLID;
        }

        float inflation = nsLayoutUtils::FontSizeInflationInner(dec.mFrame,
                            inflationMinFontSize);
        const gfxFont::Metrics metrics =
          GetFirstFontMetrics(GetFontGroupForFrame(dec.mFrame, inflation));

        const nsRect decorationRect =
          nsCSSRendering::GetTextDecorationRect(aPresContext,
            gfxSize(gfxWidth, metrics.strikeoutSize),
            ascent, metrics.strikeoutOffset,
            NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH, decorationStyle) +
          nsPoint(0, -dec.mBaselineOffset);
        top = std::min(decorationRect.y, top);
        bottom = std::max(decorationRect.YMost(), bottom);
      }

      aVisualOverflowRect->UnionRect(*aVisualOverflowRect,
                                     nsRect(0, top, width, bottom - top));
    }
  }
  
  
  if (!IsSelected() ||
      !CombineSelectionUnderlineRect(aPresContext, *aVisualOverflowRect))
    return;
  AddStateBits(TEXT_SELECTION_UNDERLINE_OVERFLOWED);
}

static gfxFloat
ComputeDescentLimitForSelectionUnderline(nsPresContext* aPresContext,
                                         nsTextFrame* aFrame,
                                         const gfxFont::Metrics& aFontMetrics)
{
  gfxFloat app = aPresContext->AppUnitsPerDevPixel();
  nscoord lineHeightApp =
    nsHTMLReflowState::CalcLineHeight(aFrame->StyleContext(), NS_AUTOHEIGHT,
                                      aFrame->GetFontSizeInflation());
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
      
      
      
      
      
      
      int32_t defaultFontSize =
        aPresContext->AppUnitsToDevPixels(nsStyleFont(aPresContext).mFont.size);
      gfxFloat fontSize = std::min(gfxFloat(defaultFontSize),
                                 aFontMetrics.emHeight);
      fontSize = std::max(fontSize, 1.0);
      return ceil(fontSize / 20);
    }
    default:
      NS_WARNING("Requested underline style is not valid");
      return aFontMetrics.underlineSize;
  }
}

enum DecorationType {
  eNormalDecoration,
  eSelectionDecoration
};

static void
PaintDecorationLine(nsIFrame* aFrame,
                    gfxContext* const aCtx,
                    const gfxRect& aDirtyRect,
                    nscolor aColor,
                    const nscolor* aOverrideColor,
                    const gfxPoint& aPt,
                    gfxFloat aXInFrame,
                    const gfxSize& aLineSize,
                    gfxFloat aAscent,
                    gfxFloat aOffset,
                    uint8_t aDecoration,
                    uint8_t aStyle,
                    DecorationType aDecorationType,
                    nsTextFrame::DrawPathCallbacks* aCallbacks,
                    gfxFloat aDescentLimit = -1.0)
{
  nscolor lineColor = aOverrideColor ? *aOverrideColor : aColor;
  if (aCallbacks) {
    if (aDecorationType == eNormalDecoration) {
      aCallbacks->NotifyBeforeDecorationLine(lineColor);
    } else {
      aCallbacks->NotifyBeforeSelectionDecorationLine(lineColor);
    }
    nsCSSRendering::DecorationLineToPath(aFrame, aCtx, aDirtyRect, lineColor,
      aPt, aXInFrame, aLineSize, aAscent, aOffset, aDecoration, aStyle,
      aDescentLimit);
    if (aDecorationType == eNormalDecoration) {
      aCallbacks->NotifyDecorationLinePathEmitted();
    } else {
      aCallbacks->NotifySelectionDecorationLinePathEmitted();
    }
  } else {
    nsCSSRendering::PaintDecorationLine(aFrame, aCtx, aDirtyRect, lineColor,
      aPt, aXInFrame, aLineSize, aAscent, aOffset, aDecoration, aStyle,
      aDescentLimit);
  }
}





static void DrawSelectionDecorations(gfxContext* aContext,
    const gfxRect& aDirtyRect,
    SelectionType aType,
    nsTextFrame* aFrame,
    nsTextPaintStyle& aTextPaintStyle,
    const nsTextRangeStyle &aRangeStyle,
    const gfxPoint& aPt, gfxFloat aXInFrame, gfxFloat aWidth,
    gfxFloat aAscent, const gfxFont::Metrics& aFontMetrics,
    nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  gfxPoint pt(aPt);
  gfxSize size(aWidth,
               ComputeSelectionUnderlineHeight(aTextPaintStyle.PresContext(),
                                               aFontMetrics, aType));
  gfxFloat descentLimit =
    ComputeDescentLimitForSelectionUnderline(aTextPaintStyle.PresContext(),
                                             aFrame, aFontMetrics);

  float relativeSize;
  uint8_t style;
  nscolor color;
  int32_t index =
    nsTextPaintStyle::GetUnderlineStyleIndexForSelectionType(aType);
  bool weDefineSelectionUnderline =
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
          style = aRangeStyle.mLineStyle;
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
  PaintDecorationLine(aFrame, aContext, aDirtyRect, color, nullptr, pt,
    pt.x - aPt.x + aXInFrame, size, aAscent, aFontMetrics.underlineOffset,
    NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE, style, eSelectionDecoration,
    aCallbacks, descentLimit);
}









static bool GetSelectionTextColors(SelectionType aType,
                                     nsTextPaintStyle& aTextPaintStyle,
                                     const nsTextRangeStyle &aRangeStyle,
                                     nscolor* aForeground, nscolor* aBackground)
{
  switch (aType) {
    case nsISelectionController::SELECTION_NORMAL:
      return aTextPaintStyle.GetSelectionColors(aForeground, aBackground);
    case nsISelectionController::SELECTION_FIND:
      aTextPaintStyle.GetHighlightColors(aForeground, aBackground);
      return true;
    case nsISelectionController::SELECTION_URLSECONDARY:
      aTextPaintStyle.GetURLSecondaryColor(aForeground);
      *aBackground = NS_RGBA(0,0,0,0);
      return true;
    case nsISelectionController::SELECTION_IME_RAWINPUT:
    case nsISelectionController::SELECTION_IME_SELECTEDRAWTEXT:
    case nsISelectionController::SELECTION_IME_CONVERTEDTEXT:
    case nsISelectionController::SELECTION_IME_SELECTEDCONVERTEDTEXT:
      if (aRangeStyle.IsDefined()) {
        *aForeground = aTextPaintStyle.GetTextColor();
        *aBackground = NS_RGBA(0,0,0,0);
        if (!aRangeStyle.IsForegroundColorDefined() &&
            !aRangeStyle.IsBackgroundColorDefined()) {
          return false;
        }
        if (aRangeStyle.IsForegroundColorDefined()) {
          *aForeground = aRangeStyle.mForegroundColor;
        }
        if (aRangeStyle.IsBackgroundColorDefined()) {
          *aBackground = aRangeStyle.mBackgroundColor;
        }
        return true;
      }
      aTextPaintStyle.GetIMESelectionColors(
        nsTextPaintStyle::GetUnderlineStyleIndexForSelectionType(aType),
        aForeground, aBackground);
      return true;
    default:
      *aForeground = aTextPaintStyle.GetTextColor();
      *aBackground = NS_RGBA(0,0,0,0);
      return false;
  }
}







static bool GetSelectionTextShadow(nsIFrame* aFrame,
                                   SelectionType aType,
                                   nsTextPaintStyle& aTextPaintStyle,
                                   nsCSSShadowArray** aShadow)
{
  if (aFrame->IsSVGText()) {
    return false;
  }
  switch (aType) {
    case nsISelectionController::SELECTION_NORMAL:
      return aTextPaintStyle.GetSelectionShadow(aShadow);
    default:
      return false;
  }
}








class SelectionIterator {
public:
  






  SelectionIterator(SelectionDetails** aSelectionDetails,
                    int32_t aStart, int32_t aLength,
                    PropertyProvider& aProvider, gfxTextRun* aTextRun,
                    gfxFloat aXOffset);

  












  bool GetNextSegment(gfxFloat* aXOffset, uint32_t* aOffset, uint32_t* aLength,
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
  int32_t                 mOriginalStart;
  int32_t                 mOriginalEnd;
  gfxFloat                mXOffset;
};

SelectionIterator::SelectionIterator(SelectionDetails** aSelectionDetails,
    int32_t aStart, int32_t aLength, PropertyProvider& aProvider,
    gfxTextRun* aTextRun, gfxFloat aXOffset)
  : mSelectionDetails(aSelectionDetails), mProvider(aProvider),
    mTextRun(aTextRun), mIterator(aProvider.GetStart()),
    mOriginalStart(aStart), mOriginalEnd(aStart + aLength),
    mXOffset(aXOffset)
{
  mIterator.SetOriginalOffset(aStart);
}

bool SelectionIterator::GetNextSegment(gfxFloat* aXOffset,
    uint32_t* aOffset, uint32_t* aLength, gfxFloat* aHyphenWidth,
    SelectionType* aType, nsTextRangeStyle* aStyle)
{
  if (mIterator.GetOriginalOffset() >= mOriginalEnd)
    return false;
  
  
  uint32_t runOffset = mIterator.GetSkippedOffset();
  
  int32_t index = mIterator.GetOriginalOffset() - mOriginalStart;
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

  bool haveHyphenBreak =
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
  return true;
}

static void
AddHyphenToMetrics(nsTextFrame* aTextFrame, gfxTextRun* aBaseTextRun,
                   gfxTextRun::Metrics* aMetrics,
                   gfxFont::BoundingBoxType aBoundingBoxType,
                   gfxContext* aContext)
{
  
  nsAutoPtr<gfxTextRun> hyphenTextRun(
    GetHyphenTextRun(aBaseTextRun, aContext, aTextFrame));
  if (!hyphenTextRun.get())
    return;

  gfxTextRun::Metrics hyphenMetrics =
    hyphenTextRun->MeasureText(0, hyphenTextRun->GetLength(),
                               aBoundingBoxType, aContext, nullptr);
  aMetrics->CombineWith(hyphenMetrics, aBaseTextRun->IsRightToLeft());
}

void
nsTextFrame::PaintOneShadow(uint32_t aOffset, uint32_t aLength,
                            nsCSSShadowItem* aShadowDetails,
                            PropertyProvider* aProvider, const nsRect& aDirtyRect,
                            const gfxPoint& aFramePt, const gfxPoint& aTextBaselinePt,
                            gfxContext* aCtx, const nscolor& aForegroundColor,
                            const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                            nscoord aLeftSideOffset, gfxRect& aBoundingBox)
{
  PROFILER_LABEL("nsTextFrame", "PaintOneShadow");
  gfxPoint shadowOffset(aShadowDetails->mXOffset, aShadowDetails->mYOffset);
  nscoord blurRadius = std::max(aShadowDetails->mRadius, 0);

  
  
  
  gfxRect shadowGfxRect = aBoundingBox +
    gfxPoint(aFramePt.x + aLeftSideOffset, aTextBaselinePt.y) + shadowOffset;
  nsRect shadowRect(NSToCoordRound(shadowGfxRect.X()),
                    NSToCoordRound(shadowGfxRect.Y()),
                    NSToCoordRound(shadowGfxRect.Width()),
                    NSToCoordRound(shadowGfxRect.Height()));

  nsContextBoxBlur contextBoxBlur;
  gfxContext* shadowContext = contextBoxBlur.Init(shadowRect, 0, blurRadius,
                                                  PresContext()->AppUnitsPerDevPixel(),
                                                  aCtx, aDirtyRect, nullptr);
  if (!shadowContext)
    return;

  nscolor shadowColor;
  const nscolor* decorationOverrideColor;
  if (aShadowDetails->mHasColor) {
    shadowColor = aShadowDetails->mColor;
    decorationOverrideColor = &shadowColor;
  } else {
    shadowColor = aForegroundColor;
    decorationOverrideColor = nullptr;
  }

  aCtx->Save();
  aCtx->NewPath();
  aCtx->SetColor(gfxRGBA(shadowColor));

  
  
  
  gfxFloat advanceWidth;
  gfxRect dirtyRect(aDirtyRect.x, aDirtyRect.y,
                    aDirtyRect.width, aDirtyRect.height);
  DrawText(shadowContext, dirtyRect, aFramePt + shadowOffset,
           aTextBaselinePt + shadowOffset, aOffset, aLength, *aProvider,
           nsTextPaintStyle(this),
           aCtx == shadowContext ? shadowColor : NS_RGB(0, 0, 0), aClipEdges,
           advanceWidth, (GetStateBits() & TEXT_HYPHEN_BREAK) != 0,
           decorationOverrideColor);

  contextBoxBlur.DoPaint();
  aCtx->Restore();
}



bool
nsTextFrame::PaintTextWithSelectionColors(gfxContext* aCtx,
    const gfxPoint& aFramePt, const gfxPoint& aTextBaselinePt,
    const gfxRect& aDirtyRect,
    PropertyProvider& aProvider,
    uint32_t aContentOffset, uint32_t aContentLength,
    nsTextPaintStyle& aTextPaintStyle, SelectionDetails* aDetails,
    SelectionType* aAllTypes,
    const nsCharClipDisplayItem::ClipEdges& aClipEdges,
    nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  
  AutoFallibleTArray<SelectionDetails*,BIG_TEXT_NODE_SIZE> prevailingSelectionsBuffer;
  SelectionDetails** prevailingSelections =
    prevailingSelectionsBuffer.AppendElements(aContentLength);
  if (!prevailingSelections) {
    return false;
  }

  SelectionType allTypes = 0;
  for (uint32_t i = 0; i < aContentLength; ++i) {
    prevailingSelections[i] = nullptr;
  }

  SelectionDetails *sdptr = aDetails;
  bool anyBackgrounds = false;
  while (sdptr) {
    int32_t start = std::max(0, sdptr->mStart - int32_t(aContentOffset));
    int32_t end = std::min(int32_t(aContentLength),
                         sdptr->mEnd - int32_t(aContentOffset));
    SelectionType type = sdptr->mType;
    if (start < end) {
      allTypes |= type;
      
      nscolor foreground, background;
      if (GetSelectionTextColors(type, aTextPaintStyle, sdptr->mTextRangeStyle,
                                 &foreground, &background)) {
        if (NS_GET_A(background) > 0) {
          anyBackgrounds = true;
        }
        for (int32_t i = start; i < end; ++i) {
          
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

  if (!allTypes) {
    
    return false;
  }

  const gfxFloat startXOffset = aTextBaselinePt.x - aFramePt.x;
  gfxFloat xOffset, hyphenWidth;
  uint32_t offset, length; 
  SelectionType type;
  nsTextRangeStyle rangeStyle;
  
  if (anyBackgrounds) {
    SelectionIterator iterator(prevailingSelections, aContentOffset, aContentLength,
                               aProvider, mTextRun, startXOffset);
    while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth,
                                   &type, &rangeStyle)) {
      nscolor foreground, background;
      GetSelectionTextColors(type, aTextPaintStyle, rangeStyle,
                             &foreground, &background);
      
      gfxFloat advance = hyphenWidth +
        mTextRun->GetAdvanceWidth(offset, length, &aProvider);
      if (NS_GET_A(background) > 0) {
        gfxFloat x = xOffset - (mTextRun->IsRightToLeft() ? advance : 0);
        PaintSelectionBackground(aCtx, aTextPaintStyle.PresContext(),
                                 background, aDirtyRect,
                                 gfxRect(aFramePt.x + x, aFramePt.y, advance,
                                 GetSize().height), aCallbacks);
      }
      iterator.UpdateWithAdvance(advance);
    }
  }
  
  
  const nsStyleText* textStyle = StyleText();
  nsRect dirtyRect(aDirtyRect.x, aDirtyRect.y,
                   aDirtyRect.width, aDirtyRect.height);
  SelectionIterator iterator(prevailingSelections, aContentOffset, aContentLength,
                             aProvider, mTextRun, startXOffset);
  while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth,
                                 &type, &rangeStyle)) {
    nscolor foreground, background;
    GetSelectionTextColors(type, aTextPaintStyle, rangeStyle,
                           &foreground, &background);
    gfxPoint textBaselinePt(aFramePt.x + xOffset, aTextBaselinePt.y);

    
    
    nsCSSShadowArray *shadow = textStyle->GetTextShadow(this);
    GetSelectionTextShadow(this, type, aTextPaintStyle, &shadow);

    
    if (shadow) {
      gfxTextRun::Metrics shadowMetrics =
        mTextRun->MeasureText(offset, length, gfxFont::LOOSE_INK_EXTENTS,
                              nullptr, &aProvider);
      if (GetStateBits() & TEXT_HYPHEN_BREAK) {
        AddHyphenToMetrics(this, mTextRun, &shadowMetrics,
                           gfxFont::LOOSE_INK_EXTENTS, aCtx);
      }
      for (uint32_t i = shadow->Length(); i > 0; --i) {
        PaintOneShadow(offset, length,
                       shadow->ShadowAt(i - 1), &aProvider,
                       dirtyRect, aFramePt, textBaselinePt, aCtx,
                       foreground, aClipEdges, 
                       xOffset - (mTextRun->IsRightToLeft() ?
                                  shadowMetrics.mBoundingBox.width : 0),
                       shadowMetrics.mBoundingBox);
      }
    }

    
    gfxFloat advance;

    DrawText(aCtx, aDirtyRect, aFramePt, textBaselinePt,
             offset, length, aProvider, aTextPaintStyle, foreground, aClipEdges,
             advance, hyphenWidth > 0, nullptr, nullptr, aCallbacks);
    if (hyphenWidth) {
      advance += hyphenWidth;
    }
    iterator.UpdateWithAdvance(advance);
  }
  return true;
}

void
nsTextFrame::PaintTextSelectionDecorations(gfxContext* aCtx,
    const gfxPoint& aFramePt,
    const gfxPoint& aTextBaselinePt, const gfxRect& aDirtyRect,
    PropertyProvider& aProvider,
    uint32_t aContentOffset, uint32_t aContentLength,
    nsTextPaintStyle& aTextPaintStyle, SelectionDetails* aDetails,
    SelectionType aSelectionType,
    nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  
  if (aProvider.GetFontGroup()->ShouldSkipDrawing())
    return;

  
  AutoFallibleTArray<SelectionDetails*, BIG_TEXT_NODE_SIZE> selectedCharsBuffer;
  SelectionDetails** selectedChars =
    selectedCharsBuffer.AppendElements(aContentLength);
  if (!selectedChars) {
    return;
  }
  for (uint32_t i = 0; i < aContentLength; ++i) {
    selectedChars[i] = nullptr;
  }

  SelectionDetails *sdptr = aDetails;
  while (sdptr) {
    if (sdptr->mType == aSelectionType) {
      int32_t start = std::max(0, sdptr->mStart - int32_t(aContentOffset));
      int32_t end = std::min(int32_t(aContentLength),
                           sdptr->mEnd - int32_t(aContentOffset));
      for (int32_t i = start; i < end; ++i) {
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

  gfxFloat startXOffset = aTextBaselinePt.x - aFramePt.x;
  SelectionIterator iterator(selectedChars, aContentOffset, aContentLength,
                             aProvider, mTextRun, startXOffset);
  gfxFloat xOffset, hyphenWidth;
  uint32_t offset, length;
  int32_t app = aTextPaintStyle.PresContext()->AppUnitsPerDevPixel();
  
  gfxPoint pt(0.0, (aTextBaselinePt.y - mAscent) / app);
  gfxRect dirtyRect(aDirtyRect.x / app, aDirtyRect.y / app,
                    aDirtyRect.width / app, aDirtyRect.height / app);
  SelectionType type;
  nsTextRangeStyle selectedStyle;
  while (iterator.GetNextSegment(&xOffset, &offset, &length, &hyphenWidth,
                                 &type, &selectedStyle)) {
    gfxFloat advance = hyphenWidth +
      mTextRun->GetAdvanceWidth(offset, length, &aProvider);
    if (type == aSelectionType) {
      pt.x = (aFramePt.x + xOffset -
             (mTextRun->IsRightToLeft() ? advance : 0)) / app;
      gfxFloat width = Abs(advance) / app;
      gfxFloat xInFrame = pt.x - (aFramePt.x / app);
      DrawSelectionDecorations(aCtx, dirtyRect, aSelectionType, this,
                               aTextPaintStyle, selectedStyle, pt, xInFrame,
                               width, mAscent / app, decorationMetrics,
                               aCallbacks);
    }
    iterator.UpdateWithAdvance(advance);
  }
}

bool
nsTextFrame::PaintTextWithSelection(gfxContext* aCtx,
    const gfxPoint& aFramePt,
    const gfxPoint& aTextBaselinePt, const gfxRect& aDirtyRect,
    PropertyProvider& aProvider,
    uint32_t aContentOffset, uint32_t aContentLength,
    nsTextPaintStyle& aTextPaintStyle,
    const nsCharClipDisplayItem::ClipEdges& aClipEdges,
    gfxTextObjectPaint* aObjectPaint,
    nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  NS_ASSERTION(GetContent()->IsSelectionDescendant(), "wrong paint path");

  SelectionDetails* details = GetSelectionDetails();
  if (!details) {
    return false;
  }

  SelectionType allTypes;
  if (!PaintTextWithSelectionColors(aCtx, aFramePt, aTextBaselinePt, aDirtyRect,
                                    aProvider, aContentOffset, aContentLength,
                                    aTextPaintStyle, details, &allTypes,
                                    aClipEdges, aCallbacks)) {
    DestroySelectionDetails(details);
    return false;
  }
  
  
  
  
  allTypes &= SelectionTypesWithDecorations;
  for (int32_t i = nsISelectionController::NUM_SELECTIONTYPES - 1;
       i >= 1; --i) {
    SelectionType type = 1 << (i - 1);
    if (allTypes & type) {
      
      
      
      PaintTextSelectionDecorations(aCtx, aFramePt, aTextBaselinePt, aDirtyRect,
                                    aProvider, aContentOffset, aContentLength,
                                    aTextPaintStyle, details, type,
                                    aCallbacks);
    }
  }

  DestroySelectionDetails(details);
  return true;
}

nscolor
nsTextFrame::GetCaretColorAt(int32_t aOffset)
{
  NS_PRECONDITION(aOffset >= 0, "aOffset must be positive");

  nscolor result = nsFrame::GetCaretColorAt(aOffset);
  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  PropertyProvider provider(this, iter, nsTextFrame::eInflated);
  int32_t contentOffset = provider.GetStart().GetOriginalOffset();
  int32_t contentLength = provider.GetOriginalLength();
  NS_PRECONDITION(aOffset >= contentOffset &&
                  aOffset <= contentOffset + contentLength,
                  "aOffset must be in the frame's range");
  int32_t offsetInFrame = aOffset - contentOffset;
  if (offsetInFrame < 0 || offsetInFrame >= contentLength) {
    return result;
  }

  bool isSolidTextColor = true;
  if (IsSVGText()) {
    const nsStyleSVG* style = StyleSVG();
    if (style->mFill.mType != eStyleSVGPaintType_None &&
        style->mFill.mType != eStyleSVGPaintType_Color) {
      isSolidTextColor = false;
    }
  }

  nsTextPaintStyle textPaintStyle(this);
  textPaintStyle.SetResolveColors(isSolidTextColor);
  SelectionDetails* details = GetSelectionDetails();
  SelectionDetails* sdptr = details;
  SelectionType type = 0;
  while (sdptr) {
    int32_t start = std::max(0, sdptr->mStart - contentOffset);
    int32_t end = std::min(contentLength, sdptr->mEnd - contentOffset);
    if (start <= offsetInFrame && offsetInFrame < end &&
        (type == 0 || sdptr->mType < type)) {
      nscolor foreground, background;
      if (GetSelectionTextColors(sdptr->mType, textPaintStyle,
                                 sdptr->mTextRangeStyle,
                                 &foreground, &background)) {
        if (!isSolidTextColor &&
            NS_IS_SELECTION_SPECIAL_COLOR(foreground)) {
          result = NS_RGBA(0, 0, 0, 255);
        } else {
          result = foreground;
        }
        type = sdptr->mType;
      }
    }
    sdptr = sdptr->mNext;
  }

  DestroySelectionDetails(details);
  return result;
}

static uint32_t
ComputeTransformedLength(PropertyProvider& aProvider)
{
  gfxSkipCharsIterator iter(aProvider.GetStart());
  uint32_t start = iter.GetSkippedOffset();
  iter.AdvanceOriginal(aProvider.GetOriginalLength());
  return iter.GetSkippedOffset() - start;
}

bool
nsTextFrame::MeasureCharClippedText(nscoord aLeftEdge, nscoord aRightEdge,
                                    nscoord* aSnappedLeftEdge,
                                    nscoord* aSnappedRightEdge)
{
  
  
  
  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return false;

  PropertyProvider provider(this, iter, nsTextFrame::eInflated);
  
  provider.InitializeForDisplay(true);

  uint32_t startOffset = provider.GetStart().GetSkippedOffset();
  uint32_t maxLength = ComputeTransformedLength(provider);
  return MeasureCharClippedText(provider, aLeftEdge, aRightEdge,
                                &startOffset, &maxLength,
                                aSnappedLeftEdge, aSnappedRightEdge);
}

static uint32_t GetClusterLength(gfxTextRun* aTextRun,
                                 uint32_t    aStartOffset,
                                 uint32_t    aMaxLength,
                                 bool        aIsRTL)
{
  uint32_t clusterLength = aIsRTL ? 0 : 1;
  while (clusterLength < aMaxLength) {
    if (aTextRun->IsClusterStart(aStartOffset + clusterLength)) {
      if (aIsRTL) {
        ++clusterLength;
      }
      break;
    }
    ++clusterLength;
  }
  return clusterLength;
}

bool
nsTextFrame::MeasureCharClippedText(PropertyProvider& aProvider,
                                    nscoord aLeftEdge, nscoord aRightEdge,
                                    uint32_t* aStartOffset,
                                    uint32_t* aMaxLength,
                                    nscoord*  aSnappedLeftEdge,
                                    nscoord*  aSnappedRightEdge)
{
  *aSnappedLeftEdge = 0;
  *aSnappedRightEdge = 0;
  if (aLeftEdge <= 0 && aRightEdge <= 0) {
    return true;
  }

  uint32_t offset = *aStartOffset;
  uint32_t maxLength = *aMaxLength;
  const nscoord frameWidth = GetSize().width;
  const bool rtl = mTextRun->IsRightToLeft();
  gfxFloat advanceWidth = 0;
  const nscoord startEdge = rtl ? aRightEdge : aLeftEdge;
  if (startEdge > 0) {
    const gfxFloat maxAdvance = gfxFloat(startEdge);
    while (maxLength > 0) {
      uint32_t clusterLength =
        GetClusterLength(mTextRun, offset, maxLength, rtl);
      advanceWidth +=
        mTextRun->GetAdvanceWidth(offset, clusterLength, &aProvider);
      maxLength -= clusterLength;
      offset += clusterLength;
      if (advanceWidth >= maxAdvance) {
        break;
      }
    }
    nscoord* snappedStartEdge = rtl ? aSnappedRightEdge : aSnappedLeftEdge;
    *snappedStartEdge = NSToCoordFloor(advanceWidth);
    *aStartOffset = offset;
  }

  const nscoord endEdge = rtl ? aLeftEdge : aRightEdge;
  if (endEdge > 0) {
    const gfxFloat maxAdvance = gfxFloat(frameWidth - endEdge);
    while (maxLength > 0) {
      uint32_t clusterLength =
        GetClusterLength(mTextRun, offset, maxLength, rtl);
      gfxFloat nextAdvance = advanceWidth +
        mTextRun->GetAdvanceWidth(offset, clusterLength, &aProvider);
      if (nextAdvance > maxAdvance) {
        break;
      }
      
      advanceWidth = nextAdvance;
      maxLength -= clusterLength;
      offset += clusterLength;
    }
    maxLength = offset - *aStartOffset;
    nscoord* snappedEndEdge = rtl ? aSnappedLeftEdge : aSnappedRightEdge;
    *snappedEndEdge = NSToCoordFloor(gfxFloat(frameWidth) - advanceWidth);
  }
  *aMaxLength = maxLength;
  return maxLength != 0;
}

void
nsTextFrame::PaintText(nsRenderingContext* aRenderingContext, nsPoint aPt,
                       const nsRect& aDirtyRect,
                       const nsCharClipDisplayItem& aItem,
                       gfxTextObjectPaint* aObjectPaint,
                       nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  
  
  
  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return;

  PropertyProvider provider(this, iter, nsTextFrame::eInflated);
  
  provider.InitializeForDisplay(true);

  gfxContext* ctx = aRenderingContext->ThebesContext();
  const bool rtl = mTextRun->IsRightToLeft();
  const nscoord frameWidth = GetSize().width;
  gfxPoint framePt(aPt.x, aPt.y);
  gfxPoint textBaselinePt(rtl ? gfxFloat(aPt.x + frameWidth) : framePt.x,
             nsLayoutUtils::GetSnappedBaselineY(this, ctx, aPt.y, mAscent));
  uint32_t startOffset = provider.GetStart().GetSkippedOffset();
  uint32_t maxLength = ComputeTransformedLength(provider);
  nscoord snappedLeftEdge, snappedRightEdge;
  if (!MeasureCharClippedText(provider, aItem.mLeftEdge, aItem.mRightEdge,
         &startOffset, &maxLength, &snappedLeftEdge, &snappedRightEdge)) {
    return;
  }
  textBaselinePt.x += rtl ? -snappedRightEdge : snappedLeftEdge;
  nsCharClipDisplayItem::ClipEdges clipEdges(aItem, snappedLeftEdge,
                                             snappedRightEdge);
  nsTextPaintStyle textPaintStyle(this);
  textPaintStyle.SetResolveColors(!aCallbacks);

  gfxRect dirtyRect(aDirtyRect.x, aDirtyRect.y,
                    aDirtyRect.width, aDirtyRect.height);
  
  if (IsSelected()) {
    gfxSkipCharsIterator tmp(provider.GetStart());
    int32_t contentOffset = tmp.ConvertSkippedToOriginal(startOffset);
    int32_t contentLength =
      tmp.ConvertSkippedToOriginal(startOffset + maxLength) - contentOffset;
    if (PaintTextWithSelection(ctx, framePt, textBaselinePt, dirtyRect,
                               provider, contentOffset, contentLength,
                               textPaintStyle, clipEdges, aObjectPaint,
                               aCallbacks)) {
      return;
    }
  }

  nscolor foregroundColor = textPaintStyle.GetTextColor();
  if (!aCallbacks) {
    const nsStyleText* textStyle = StyleText();
    if (textStyle->HasTextShadow(this)) {
      
      
      gfxTextRun::Metrics shadowMetrics = 
        mTextRun->MeasureText(startOffset, maxLength, gfxFont::LOOSE_INK_EXTENTS,
                              nullptr, &provider);
      for (uint32_t i = textStyle->mTextShadow->Length(); i > 0; --i) {
        PaintOneShadow(startOffset, maxLength,
                       textStyle->mTextShadow->ShadowAt(i - 1), &provider,
                       aDirtyRect, framePt, textBaselinePt, ctx,
                       foregroundColor, clipEdges,
                       snappedLeftEdge, shadowMetrics.mBoundingBox);
      }
    }
  }

  gfxFloat advanceWidth;
  DrawText(ctx, dirtyRect, framePt, textBaselinePt, startOffset, maxLength, provider,
           textPaintStyle, foregroundColor, clipEdges, advanceWidth,
           (GetStateBits() & TEXT_HYPHEN_BREAK) != 0,
           nullptr, aObjectPaint, aCallbacks);
}

static void
DrawTextRun(gfxTextRun* aTextRun,
            gfxContext* const aCtx,
            const gfxPoint& aTextBaselinePt,
            uint32_t aOffset, uint32_t aLength,
            PropertyProvider* aProvider,
            nscolor aTextColor,
            gfxFloat* aAdvanceWidth,
            gfxTextObjectPaint* aObjectPaint,
            nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  gfxFont::DrawMode drawMode = aCallbacks ? gfxFont::GLYPH_PATH :
                                            gfxFont::GLYPH_FILL;
  if (aCallbacks) {
    aCallbacks->NotifyBeforeText(aTextColor);
    aTextRun->Draw(aCtx, aTextBaselinePt, drawMode, aOffset, aLength,
                   aProvider, aAdvanceWidth, aObjectPaint, aCallbacks);
    aCallbacks->NotifyAfterText();
  } else {
    aCtx->SetColor(gfxRGBA(aTextColor));
    aTextRun->Draw(aCtx, aTextBaselinePt, drawMode, aOffset, aLength,
                   aProvider, aAdvanceWidth, aObjectPaint);
  }
}

void
nsTextFrame::DrawTextRun(gfxContext* const aCtx,
                         const gfxPoint& aTextBaselinePt,
                         uint32_t aOffset, uint32_t aLength,
                         PropertyProvider& aProvider,
                         nscolor aTextColor,
                         gfxFloat& aAdvanceWidth,
                         bool aDrawSoftHyphen,
                         gfxTextObjectPaint* aObjectPaint,
                         nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  ::DrawTextRun(mTextRun, aCtx, aTextBaselinePt, aOffset, aLength, &aProvider,
                aTextColor, &aAdvanceWidth, aObjectPaint, aCallbacks);

  if (aDrawSoftHyphen) {
    
    
    nsAutoPtr<gfxTextRun> hyphenTextRun(GetHyphenTextRun(mTextRun, nullptr, this));
    if (hyphenTextRun.get()) {
      
      
      gfxFloat hyphenBaselineX = aTextBaselinePt.x + mTextRun->GetDirection() * aAdvanceWidth -
        (mTextRun->IsRightToLeft() ? hyphenTextRun->GetAdvanceWidth(0, hyphenTextRun->GetLength(), nullptr) : 0);
      ::DrawTextRun(hyphenTextRun.get(), aCtx,
                    gfxPoint(hyphenBaselineX, aTextBaselinePt.y),
                    0, hyphenTextRun->GetLength(),
                    nullptr, aTextColor, nullptr, aObjectPaint, aCallbacks);
    }
  }
}

void
nsTextFrame::DrawTextRunAndDecorations(
    gfxContext* const aCtx, const gfxRect& aDirtyRect,
    const gfxPoint& aFramePt, const gfxPoint& aTextBaselinePt,
    uint32_t aOffset, uint32_t aLength,
    PropertyProvider& aProvider,
    const nsTextPaintStyle& aTextStyle,
    nscolor aTextColor,
    const nsCharClipDisplayItem::ClipEdges& aClipEdges,
    gfxFloat& aAdvanceWidth,
    bool aDrawSoftHyphen,
    const TextDecorations& aDecorations,
    const nscolor* const aDecorationOverrideColor,
    gfxTextObjectPaint* aObjectPaint,
    nsTextFrame::DrawPathCallbacks* aCallbacks)
{
    const gfxFloat app = aTextStyle.PresContext()->AppUnitsPerDevPixel();

    
    nscoord x = NSToCoordRound(aFramePt.x);
    nscoord width = GetRect().width;
    aClipEdges.Intersect(&x, &width);

    gfxPoint decPt(x / app, 0);
    gfxSize decSize(width / app, 0);
    const gfxFloat ascent = gfxFloat(mAscent) / app;
    const gfxFloat frameTop = aFramePt.y;

    gfxRect dirtyRect(aDirtyRect.x / app, aDirtyRect.y / app,
                      aDirtyRect.Width() / app, aDirtyRect.Height() / app);

    nscoord inflationMinFontSize =
      nsLayoutUtils::InflationMinFontSizeFor(this);

    
    for (uint32_t i = aDecorations.mUnderlines.Length(); i-- > 0; ) {
      const LineDecoration& dec = aDecorations.mUnderlines[i];
      if (dec.mStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
        continue;
      }

      float inflation = nsLayoutUtils::FontSizeInflationInner(dec.mFrame,
                          inflationMinFontSize);
      const gfxFont::Metrics metrics =
        GetFirstFontMetrics(GetFontGroupForFrame(dec.mFrame, inflation));

      decSize.height = metrics.underlineSize;
      decPt.y = (frameTop - dec.mBaselineOffset) / app;

      PaintDecorationLine(this, aCtx, dirtyRect, dec.mColor,
        aDecorationOverrideColor, decPt, 0.0, decSize, ascent,
        metrics.underlineOffset, NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE,
        dec.mStyle, eNormalDecoration, aCallbacks);
    }
    
    for (uint32_t i = aDecorations.mOverlines.Length(); i-- > 0; ) {
      const LineDecoration& dec = aDecorations.mOverlines[i];
      if (dec.mStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
        continue;
      }

      float inflation = nsLayoutUtils::FontSizeInflationInner(dec.mFrame,
                          inflationMinFontSize);
      const gfxFont::Metrics metrics =
        GetFirstFontMetrics(GetFontGroupForFrame(dec.mFrame, inflation));

      decSize.height = metrics.underlineSize;
      decPt.y = (frameTop - dec.mBaselineOffset) / app;

      PaintDecorationLine(this, aCtx, dirtyRect, dec.mColor,
        aDecorationOverrideColor, decPt, 0.0, decSize, ascent,
        metrics.maxAscent, NS_STYLE_TEXT_DECORATION_LINE_OVERLINE, dec.mStyle,
        eNormalDecoration, aCallbacks);
    }

    
    
    DrawTextRun(aCtx, aTextBaselinePt, aOffset, aLength, aProvider, aTextColor,
                aAdvanceWidth, aDrawSoftHyphen, aObjectPaint, aCallbacks);

    
    for (uint32_t i = aDecorations.mStrikes.Length(); i-- > 0; ) {
      const LineDecoration& dec = aDecorations.mStrikes[i];
      if (dec.mStyle == NS_STYLE_TEXT_DECORATION_STYLE_NONE) {
        continue;
      }

      float inflation = nsLayoutUtils::FontSizeInflationInner(dec.mFrame,
                          inflationMinFontSize);
      const gfxFont::Metrics metrics =
        GetFirstFontMetrics(GetFontGroupForFrame(dec.mFrame, inflation));

      decSize.height = metrics.strikeoutSize;
      decPt.y = (frameTop - dec.mBaselineOffset) / app;

      PaintDecorationLine(this, aCtx, dirtyRect, dec.mColor,
        aDecorationOverrideColor, decPt, 0.0, decSize, ascent,
        metrics.strikeoutOffset, NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH,
        dec.mStyle, eNormalDecoration, aCallbacks);
    }
}

void
nsTextFrame::DrawText(
    gfxContext* const aCtx, const gfxRect& aDirtyRect,
    const gfxPoint& aFramePt, const gfxPoint& aTextBaselinePt,
    uint32_t aOffset, uint32_t aLength,
    PropertyProvider& aProvider,
    const nsTextPaintStyle& aTextStyle,
    nscolor aTextColor,
    const nsCharClipDisplayItem::ClipEdges& aClipEdges,
    gfxFloat& aAdvanceWidth,
    bool aDrawSoftHyphen,
    const nscolor* const aDecorationOverrideColor,
    gfxTextObjectPaint* aObjectPaint,
    nsTextFrame::DrawPathCallbacks* aCallbacks)
{
  TextDecorations decorations;
  GetTextDecorations(aTextStyle.PresContext(),
                     aCallbacks ? eUnresolvedColors : eResolvedColors,
                     decorations);

  
  const bool drawDecorations = !aProvider.GetFontGroup()->ShouldSkipDrawing() &&
                               decorations.HasDecorationLines();
  if (drawDecorations) {
    DrawTextRunAndDecorations(aCtx, aDirtyRect, aFramePt, aTextBaselinePt, aOffset, aLength,
                              aProvider, aTextStyle, aTextColor, aClipEdges, aAdvanceWidth,
                              aDrawSoftHyphen, decorations,
                              aDecorationOverrideColor, aObjectPaint, aCallbacks);
  } else {
    DrawTextRun(aCtx, aTextBaselinePt, aOffset, aLength, aProvider,
                aTextColor, aAdvanceWidth, aDrawSoftHyphen, aObjectPaint, aCallbacks);
  }
}

int16_t
nsTextFrame::GetSelectionStatus(int16_t* aSelectionFlags)
{
  
  nsCOMPtr<nsISelectionController> selectionController;
  nsresult rv = GetSelectionController(PresContext(),
                                       getter_AddRefs(selectionController));
  if (NS_FAILED(rv) || !selectionController)
    return nsISelectionController::SELECTION_OFF;

  selectionController->GetSelectionFlags(aSelectionFlags);

  int16_t selectionValue;
  selectionController->GetDisplaySelection(&selectionValue);

  return selectionValue;
}

bool
nsTextFrame::IsVisibleInSelection(nsISelection* aSelection)
{
  
  if (!GetContent()->IsSelectionDescendant())
    return false;
    
  SelectionDetails* details = GetSelectionDetails();
  bool found = false;
    
  
  SelectionDetails *sdptr = details;
  while (sdptr) {
    if (sdptr->mEnd > GetContentOffset() &&
        sdptr->mStart < GetContentEnd() &&
        sdptr->mType == nsISelectionController::SELECTION_NORMAL) {
      found = true;
      break;
    }
    sdptr = sdptr->mNext;
  }
  DestroySelectionDetails(details);

  return found;
}





static uint32_t
CountCharsFit(gfxTextRun* aTextRun, uint32_t aStart, uint32_t aLength,
              gfxFloat aWidth, PropertyProvider* aProvider,
              gfxFloat* aFitWidth)
{
  uint32_t last = 0;
  gfxFloat width = 0;
  for (uint32_t i = 1; i <= aLength; ++i) {
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
  return GetCharacterOffsetAtFramePointInternal(aPoint, true);
}

nsIFrame::ContentOffsets
nsTextFrame::GetCharacterOffsetAtFramePoint(const nsPoint &aPoint)
{
  return GetCharacterOffsetAtFramePointInternal(aPoint, false);
}

nsIFrame::ContentOffsets
nsTextFrame::GetCharacterOffsetAtFramePointInternal(nsPoint aPoint,
                                                    bool aForInsertionPoint)
{
  ContentOffsets offsets;
  
  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return offsets;
  
  PropertyProvider provider(this, iter, nsTextFrame::eInflated);
  
  provider.InitializeForDisplay(false);
  gfxFloat width = mTextRun->IsRightToLeft() ? mRect.width - aPoint.x : aPoint.x;
  gfxFloat fitWidth;
  uint32_t skippedLength = ComputeTransformedLength(provider);

  uint32_t charsFit = CountCharsFit(mTextRun,
      provider.GetStart().GetSkippedOffset(), skippedLength, width, &provider, &fitWidth);

  int32_t selectedOffset;
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
    
    
    
    if (StyleText()->NewlineIsSignificant() &&
        HasTerminalNewline()) {
      --selectedOffset;
    }
  }

  offsets.content = GetContent();
  offsets.offset = offsets.secondaryOffset = selectedOffset;
  offsets.associateWithNext = mContentOffset == offsets.offset;
  return offsets;
}

bool
nsTextFrame::CombineSelectionUnderlineRect(nsPresContext* aPresContext,
                                           nsRect& aRect)
{
  if (aRect.IsEmpty())
    return false;

  nsRect givenRect = aRect;

  nsRefPtr<nsFontMetrics> fm;
  nsLayoutUtils::GetFontMetricsForFrame(this, getter_AddRefs(fm),
                                        GetFontSizeInflation());
  gfxFontGroup* fontGroup = fm->GetThebesFontGroup();
  gfxFont* firstFont = fontGroup->GetFontAt(0);
  if (!firstFont)
    return false; 
  const gfxFont::Metrics& metrics = firstFont->GetMetrics();
  gfxFloat underlineOffset = fontGroup->GetUnderlineOffset();
  gfxFloat ascent = aPresContext->AppUnitsToGfxUnits(mAscent);
  gfxFloat descentLimit =
    ComputeDescentLimitForSelectionUnderline(aPresContext, this, metrics);

  SelectionDetails *details = GetSelectionDetails();
  for (SelectionDetails *sd = details; sd; sd = sd->mNext) {
    if (sd->mStart == sd->mEnd || !(sd->mType & SelectionTypesWithDecorations))
      continue;

    uint8_t style;
    float relativeSize;
    int32_t index =
      nsTextPaintStyle::GetUnderlineStyleIndexForSelectionType(sd->mType);
    if (sd->mType == nsISelectionController::SELECTION_SPELLCHECK) {
      if (!nsTextPaintStyle::GetSelectionUnderline(aPresContext, index, nullptr,
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
        style = rangeStyle.mLineStyle;
        relativeSize = rangeStyle.mIsBoldLine ? 2.0f : 1.0f;
      } else if (!nsTextPaintStyle::GetSelectionUnderline(aPresContext, index,
                                                          nullptr, &relativeSize,
                                                          &style)) {
        continue;
      }
    }
    nsRect decorationArea;
    gfxSize size(aPresContext->AppUnitsToGfxUnits(aRect.width),
                 ComputeSelectionUnderlineHeight(aPresContext,
                                                 metrics, sd->mType));
    relativeSize = std::max(relativeSize, 1.0f);
    size.height *= relativeSize;
    decorationArea =
      nsCSSRendering::GetTextDecorationRect(aPresContext, size,
        ascent, underlineOffset, NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE,
        style, descentLimit);
    aRect.UnionRect(aRect, decorationArea);
  }
  DestroySelectionDetails(details);

  return !aRect.IsEmpty() && !givenRect.Contains(aRect);
}

bool
nsTextFrame::IsFrameSelected() const
{
  NS_ASSERTION(!GetContent() || GetContent()->IsSelectionDescendant(),
               "use the public IsSelected() instead");
  return nsRange::IsNodeSelected(GetContent(), GetContentOffset(),
                                 GetContentEnd());
}

void
nsTextFrame::SetSelectedRange(uint32_t aStart, uint32_t aEnd, bool aSelected,
                              SelectionType aType)
{
  NS_ASSERTION(!GetPrevContinuation(), "Should only be called for primary frame");
  DEBUG_VERIFY_NOT_DIRTY(mState);

  
  if (aStart == aEnd)
    return;

  nsTextFrame* f = this;
  while (f && f->GetContentEnd() <= int32_t(aStart)) {
    f = static_cast<nsTextFrame*>(f->GetNextContinuation());
  }

  nsPresContext* presContext = PresContext();
  while (f && f->GetContentOffset() < int32_t(aEnd)) {
    
    
    
    if (aType & SelectionTypesWithDecorations) {
      bool didHaveOverflowingSelection =
        (f->GetStateBits() & TEXT_SELECTION_UNDERLINE_OVERFLOWED) != 0;
      nsRect r(nsPoint(0, 0), GetSize());
      bool willHaveOverflowingSelection =
        aSelected && f->CombineSelectionUnderlineRect(presContext, r);
      if (didHaveOverflowingSelection || willHaveOverflowingSelection) {
        presContext->PresShell()->FrameNeedsReflow(f,
                                                   nsIPresShell::eStyleChange,
                                                   NS_FRAME_IS_DIRTY);
      }
    }
    
    f->InvalidateFrame();

    f = static_cast<nsTextFrame*>(f->GetNextContinuation());
  }
}

NS_IMETHODIMP
nsTextFrame::GetPointFromOffset(int32_t inOffset,
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

  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return NS_ERROR_FAILURE;

  PropertyProvider properties(this, iter, nsTextFrame::eInflated);
  
  
  properties.InitializeForDisplay(false);  

  if (inOffset < GetContentOffset()){
    NS_WARNING("offset before this frame's content");
    inOffset = GetContentOffset();
  } else if (inOffset > GetContentEnd()) {
    NS_WARNING("offset after this frame's content");
    inOffset = GetContentEnd();
  }
  int32_t trimmedOffset = properties.GetStart().GetOriginalOffset();
  int32_t trimmedEnd = trimmedOffset + properties.GetOriginalLength();
  inOffset = std::max(inOffset, trimmedOffset);
  inOffset = std::min(inOffset, trimmedEnd);

  iter.SetOriginalOffset(inOffset);

  if (inOffset < trimmedEnd &&
      !iter.IsOriginalCharSkipped() &&
      !mTextRun->IsClusterStart(iter.GetSkippedOffset())) {
    NS_WARNING("GetPointFromOffset called for non-cluster boundary");
    FindClusterStart(mTextRun, trimmedOffset, &iter);
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
nsTextFrame::GetChildFrameContainingOffset(int32_t   aContentOffset,
                                           bool      aHint,
                                           int32_t*  aOutOffset,
                                           nsIFrame**aOutFrame)
{
  DEBUG_VERIFY_NOT_DIRTY(mState);
#if 0 
  if (mState & NS_FRAME_IS_DIRTY)
    return NS_ERROR_UNEXPECTED;
#endif

  NS_ASSERTION(aOutOffset && aOutFrame, "Bad out parameters");
  NS_ASSERTION(aContentOffset >= 0, "Negative content offset, existing code was very broken!");
  nsIFrame* primaryFrame = mContent->GetPrimaryFrame();
  if (this != primaryFrame) {
    
    return primaryFrame->GetChildFrameContainingOffset(aContentOffset, aHint,
                                                       aOutOffset, aOutFrame);
  }

  nsTextFrame* f = this;
  int32_t offset = mContentOffset;

  
  nsTextFrame* cachedFrame = static_cast<nsTextFrame*>
    (Properties().Get(OffsetToFrameProperty()));

  if (cachedFrame) {
    f = cachedFrame;
    offset = f->GetContentOffset();

    f->RemoveStateBits(TEXT_IN_OFFSET_CACHE);
  }

  if ((aContentOffset >= offset) &&
      (aHint || aContentOffset != offset)) {
    while (true) {
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
    while (true) {
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

  
  Properties().Set(OffsetToFrameProperty(), f);
  f->AddStateBits(TEXT_IN_OFFSET_CACHE);

  return NS_OK;
}

bool
nsTextFrame::PeekOffsetNoAmount(bool aForward, int32_t* aOffset)
{
  NS_ASSERTION(aOffset && *aOffset <= GetContentLength(), "aOffset out of range");

  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return false;

  TrimmedOffsets trimmed = GetTrimmedOffsets(mContent->GetText(), true);
  
  return iter.ConvertOriginalToSkipped(trimmed.GetEnd()) >
         iter.ConvertOriginalToSkipped(trimmed.mStart);
}









class NS_STACK_CLASS ClusterIterator {
public:
  ClusterIterator(nsTextFrame* aTextFrame, int32_t aPosition, int32_t aDirection,
                  nsString& aContext);

  bool NextCluster();
  bool IsWhitespace();
  bool IsPunctuation();
  bool HaveWordBreakBefore() { return mHaveWordBreak; }
  int32_t GetAfterOffset();
  int32_t GetBeforeOffset();

private:
  gfxSkipCharsIterator        mIterator;
  const nsTextFragment*       mFrag;
  nsTextFrame*                mTextFrame;
  int32_t                     mDirection;
  int32_t                     mCharIndex;
  nsTextFrame::TrimmedOffsets mTrimmed;
  nsTArray<bool>      mWordBreaks;
  bool                        mHaveWordBreak;
};

static bool
IsAcceptableCaretPosition(const gfxSkipCharsIterator& aIter,
                          bool aRespectClusters,
                          gfxTextRun* aTextRun,
                          nsIFrame* aFrame)
{
  if (aIter.IsOriginalCharSkipped())
    return false;
  uint32_t index = aIter.GetSkippedOffset();
  if (aRespectClusters && !aTextRun->IsClusterStart(index))
    return false;
  if (index > 0) {
    
    
    
    
    
    if (aTextRun->CharIsLowSurrogate(index)) {
      return false;
    }
  }
  return true;
}

bool
nsTextFrame::PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                 bool aRespectClusters)
{
  int32_t contentLength = GetContentLength();
  NS_ASSERTION(aOffset && *aOffset <= contentLength, "aOffset out of range");

  bool selectable;
  uint8_t selectStyle;  
  IsSelectable(&selectable, &selectStyle);
  if (selectStyle == NS_STYLE_USER_SELECT_ALL)
    return false;

  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return false;

  TrimmedOffsets trimmed = GetTrimmedOffsets(mContent->GetText(), false);

  
  int32_t startOffset = GetContentOffset() + (*aOffset < 0 ? contentLength : *aOffset);

  if (!aForward) {
    
    for (int32_t i = std::min(trimmed.GetEnd(), startOffset) - 1;
         i >= trimmed.mStart; --i) {
      iter.SetOriginalOffset(i);
      if (IsAcceptableCaretPosition(iter, aRespectClusters, mTextRun, this)) {
        *aOffset = i - mContentOffset;
        return true;
      }
    }
    *aOffset = 0;
  } else {
    
    iter.SetOriginalOffset(startOffset);
    if (startOffset <= trimmed.GetEnd() &&
        !(startOffset < trimmed.GetEnd() &&
          StyleText()->NewlineIsSignificant() &&
          iter.GetSkippedOffset() < mTextRun->GetLength() &&
          mTextRun->CharIsNewline(iter.GetSkippedOffset()))) {
      for (int32_t i = startOffset + 1; i <= trimmed.GetEnd(); ++i) {
        iter.SetOriginalOffset(i);
        if (i == trimmed.GetEnd() ||
            IsAcceptableCaretPosition(iter, aRespectClusters, mTextRun, this)) {
          *aOffset = i - mContentOffset;
          return true;
        }
      }
    }
    *aOffset = contentLength;
  }
  
  return false;
}

bool
ClusterIterator::IsWhitespace()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  return IsSelectionSpace(mFrag, mCharIndex);
}

bool
ClusterIterator::IsPunctuation()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  nsIUGenCategory::nsUGenCategory c =
    mozilla::unicode::GetGenCategory(mFrag->CharAt(mCharIndex));
  return c == nsIUGenCategory::kPunctuation || c == nsIUGenCategory::kSymbol;
}

int32_t
ClusterIterator::GetBeforeOffset()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  return mCharIndex + (mDirection > 0 ? 0 : 1);
}

int32_t
ClusterIterator::GetAfterOffset()
{
  NS_ASSERTION(mCharIndex >= 0, "No cluster selected");
  return mCharIndex + (mDirection > 0 ? 1 : 0);
}

bool
ClusterIterator::NextCluster()
{
  if (!mDirection)
    return false;
  gfxTextRun* textRun = mTextFrame->GetTextRun(nsTextFrame::eInflated);

  mHaveWordBreak = false;
  while (true) {
    bool keepGoing = false;
    if (mDirection > 0) {
      if (mIterator.GetOriginalOffset() >= mTrimmed.GetEnd())
        return false;
      keepGoing = mIterator.IsOriginalCharSkipped() ||
          mIterator.GetOriginalOffset() < mTrimmed.mStart ||
          !textRun->IsClusterStart(mIterator.GetSkippedOffset());
      mCharIndex = mIterator.GetOriginalOffset();
      mIterator.AdvanceOriginal(1);
    } else {
      if (mIterator.GetOriginalOffset() <= mTrimmed.mStart)
        return false;
      mIterator.AdvanceOriginal(-1);
      keepGoing = mIterator.IsOriginalCharSkipped() ||
          mIterator.GetOriginalOffset() >= mTrimmed.GetEnd() ||
          !textRun->IsClusterStart(mIterator.GetSkippedOffset());
      mCharIndex = mIterator.GetOriginalOffset();
    }

    if (mWordBreaks[GetBeforeOffset() - mTextFrame->GetContentOffset()]) {
      mHaveWordBreak = true;
    }
    if (!keepGoing)
      return true;
  }
}

ClusterIterator::ClusterIterator(nsTextFrame* aTextFrame, int32_t aPosition,
                                 int32_t aDirection, nsString& aContext)
  : mTextFrame(aTextFrame), mDirection(aDirection), mCharIndex(-1)
{
  mIterator = aTextFrame->EnsureTextRun(nsTextFrame::eInflated);
  if (!aTextFrame->GetTextRun(nsTextFrame::eInflated)) {
    mDirection = 0; 
    return;
  }
  mIterator.SetOriginalOffset(aPosition);

  mFrag = aTextFrame->GetContent()->GetText();
  mTrimmed = aTextFrame->GetTrimmedOffsets(mFrag, true);

  int32_t textOffset = aTextFrame->GetContentOffset();
  int32_t textLen = aTextFrame->GetContentLength();
  if (!mWordBreaks.AppendElements(textLen + 1)) {
    mDirection = 0; 
    return;
  }
  memset(mWordBreaks.Elements(), false, (textLen + 1)*sizeof(bool));
  int32_t textStart;
  if (aDirection > 0) {
    if (aContext.IsEmpty()) {
      
      mWordBreaks[0] = true;
    }
    textStart = aContext.Length();
    mFrag->AppendTo(aContext, textOffset, textLen);
  } else {
    if (aContext.IsEmpty()) {
      
      mWordBreaks[textLen] = true;
    }
    textStart = 0;
    nsAutoString str;
    mFrag->AppendTo(str, textOffset, textLen);
    aContext.Insert(str, 0);
  }
  nsIWordBreaker* wordBreaker = nsContentUtils::WordBreaker();
  for (int32_t i = 0; i <= textLen; ++i) {
    int32_t indexInText = i + textStart;
    mWordBreaks[i] |=
      wordBreaker->BreakInBetween(aContext.get(), indexInText,
                                  aContext.get() + indexInText,
                                  aContext.Length() - indexInText);
  }
}

bool
nsTextFrame::PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
                            int32_t* aOffset, PeekWordState* aState)
{
  int32_t contentLength = GetContentLength();
  NS_ASSERTION (aOffset && *aOffset <= contentLength, "aOffset out of range");

  bool selectable;
  uint8_t selectStyle;
  IsSelectable(&selectable, &selectStyle);
  if (selectStyle == NS_STYLE_USER_SELECT_ALL)
    return false;

  int32_t offset = GetContentOffset() + (*aOffset < 0 ? contentLength : *aOffset);
  ClusterIterator cIter(this, offset, aForward ? 1 : -1, aState->mContext);

  if (!cIter.NextCluster())
    return false;

  do {
    bool isPunctuation = cIter.IsPunctuation();
    bool isWhitespace = cIter.IsWhitespace();
    bool isWordBreakBefore = cIter.HaveWordBreakBefore();
    if (aWordSelectEatSpace == isWhitespace && !aState->mSawBeforeType) {
      aState->SetSawBeforeType();
      aState->Update(isPunctuation, isWhitespace);
      continue;
    }
    
    if (!aState->mAtStart) {
      bool canBreak;
      if (isPunctuation != aState->mLastCharWasPunctuation) {
        canBreak = BreakWordBetweenPunctuation(aState, aForward,
                     isPunctuation, isWhitespace, aIsKeyboardSelect);
      } else if (!aState->mLastCharWasWhitespace &&
                 !isWhitespace && !isPunctuation && isWordBreakBefore) {
        
        
        
        
        
        canBreak = true;
      } else {
        canBreak = isWordBreakBefore && aState->mSawBeforeType;
      }
      if (canBreak) {
        *aOffset = cIter.GetBeforeOffset() - mContentOffset;
        return true;
      }
    }
    aState->Update(isPunctuation, isWhitespace);
  } while (cIter.NextCluster());

  *aOffset = cIter.GetAfterOffset() - mContentOffset;
  return false;
}

 

NS_IMETHODIMP
nsTextFrame::CheckVisibility(nsPresContext* aContext, int32_t aStartIndex,
    int32_t aEndIndex, bool aRecurse, bool *aFinished, bool *aRetval)
{
  if (!aRetval)
    return NS_ERROR_NULL_POINTER;

  
  
  
  for (nsTextFrame* f = this; f;
       f = static_cast<nsTextFrame*>(GetNextContinuation())) {
    int32_t dummyOffset = 0;
    if (f->PeekOffsetNoAmount(true, &dummyOffset)) {
      *aRetval = true;
      return NS_OK;
    }
  }

  *aRetval = false;
  return NS_OK;
}

NS_IMETHODIMP
nsTextFrame::GetOffsets(int32_t &start, int32_t &end) const
{
  start = GetContentOffset();
  end = GetContentEnd();
  return NS_OK;
}

static int32_t
FindEndOfPunctuationRun(const nsTextFragment* aFrag,
                        gfxTextRun* aTextRun,
                        gfxSkipCharsIterator* aIter,
                        int32_t aOffset,
                        int32_t aStart,
                        int32_t aEnd)
{
  int32_t i;

  for (i = aStart; i < aEnd - aOffset; ++i) {
    if (nsContentUtils::IsFirstLetterPunctuationAt(aFrag, aOffset + i)) {
      aIter->SetOriginalOffset(aOffset + i);
      FindClusterEnd(aTextRun, aEnd, aIter);
      i = aIter->GetOriginalOffset() - aOffset;
    } else {
      break;
    }
  }
  return i;
}














static bool
FindFirstLetterRange(const nsTextFragment* aFrag,
                     gfxTextRun* aTextRun,
                     int32_t aOffset, const gfxSkipCharsIterator& aIter,
                     int32_t* aLength)
{
  int32_t i;
  int32_t length = *aLength;
  int32_t endOffset = aOffset + length;
  gfxSkipCharsIterator iter(aIter);

  
  i = FindEndOfPunctuationRun(aFrag, aTextRun, &iter, aOffset, 
                              GetTrimmableWhitespaceCount(aFrag, aOffset, length, 1),
                              endOffset);
  if (i == length)
    return false;

  
  
  if (!nsContentUtils::IsAlphanumericAt(aFrag, aOffset + i)) {
    *aLength = 0;
    return true;
  }

  
  iter.SetOriginalOffset(aOffset + i);
  FindClusterEnd(aTextRun, endOffset, &iter);
  i = iter.GetOriginalOffset() - aOffset;
  if (i + 1 == length)
    return true;

  
  i = FindEndOfPunctuationRun(aFrag, aTextRun, &iter, aOffset, i + 1, endOffset);
  if (i < length)
    *aLength = i;
  return true;
}

static uint32_t
FindStartAfterSkippingWhitespace(PropertyProvider* aProvider,
                                 nsIFrame::InlineIntrinsicWidthData* aData,
                                 const nsStyleText* aTextStyle,
                                 gfxSkipCharsIterator* aIterator,
                                 uint32_t aFlowEndInTextRun)
{
  if (aData->skipWhitespace) {
    while (aIterator->GetSkippedOffset() < aFlowEndInTextRun &&
           IsTrimmableSpace(aProvider->GetFragment(), aIterator->GetOriginalOffset(), aTextStyle)) {
      aIterator->AdvanceOriginal(1);
    }
  }
  return aIterator->GetSkippedOffset();
}

union VoidPtrOrFloat {
  VoidPtrOrFloat() : p(nullptr) {}

  void *p;
  float f;
};

float
nsTextFrame::GetFontSizeInflation() const
{
  if (!HasFontSizeInflation()) {
    return 1.0f;
  }
  VoidPtrOrFloat u;
  u.p = Properties().Get(FontSizeInflationProperty());
  return u.f;
}

void
nsTextFrame::SetFontSizeInflation(float aInflation)
{
  if (aInflation == 1.0f) {
    if (HasFontSizeInflation()) {
      RemoveStateBits(TEXT_HAS_FONT_INFLATION);
      Properties().Delete(FontSizeInflationProperty());
    }
    return;
  }

  AddStateBits(TEXT_HAS_FONT_INFLATION);
  VoidPtrOrFloat u;
  u.f = aInflation;
  Properties().Set(FontSizeInflationProperty(), u.p);
}

 
void nsTextFrame::MarkIntrinsicWidthsDirty()
{
  ClearTextRuns();
  nsFrame::MarkIntrinsicWidthsDirty();
}



void
nsTextFrame::AddInlineMinWidthForFlow(nsRenderingContext *aRenderingContext,
                                      nsIFrame::InlineMinWidthData *aData,
                                      TextRunType aTextRunType)
{
  uint32_t flowEndInTextRun;
  gfxContext* ctx = aRenderingContext->ThebesContext();
  gfxSkipCharsIterator iter =
    EnsureTextRun(aTextRunType, ctx, aData->lineContainer,
                  aData->line, &flowEndInTextRun);
  gfxTextRun *textRun = GetTextRun(aTextRunType);
  if (!textRun)
    return;

  
  
  const nsStyleText* textStyle = StyleText();
  const nsTextFragment* frag = mContent->GetText();

  
  
  int32_t len = INT32_MAX;
  bool hyphenating = frag->GetLength() > 0 &&
    (textStyle->mHyphens == NS_STYLE_HYPHENS_AUTO ||
     (textStyle->mHyphens == NS_STYLE_HYPHENS_MANUAL &&
      (textRun->GetFlags() & gfxTextRunFactory::TEXT_ENABLE_HYPHEN_BREAKS) != 0));
  if (hyphenating) {
    gfxSkipCharsIterator tmp(iter);
    len = std::min<int32_t>(GetContentOffset() + GetInFlowContentLength(),
                 tmp.ConvertSkippedToOriginal(flowEndInTextRun)) - iter.GetOriginalOffset();
  }
  PropertyProvider provider(textRun, textStyle, frag, this,
                            iter, len, nullptr, 0, aTextRunType);

  bool collapseWhitespace = !textStyle->WhiteSpaceIsSignificant();
  bool preformatNewlines = textStyle->NewlineIsSignificant();
  bool preformatTabs = textStyle->WhiteSpaceIsSignificant();
  gfxFloat tabWidth = -1;
  uint32_t start =
    FindStartAfterSkippingWhitespace(&provider, aData, textStyle, &iter, flowEndInTextRun);

  AutoFallibleTArray<bool,BIG_TEXT_NODE_SIZE> hyphBuffer;
  bool *hyphBreakBefore = nullptr;
  if (hyphenating) {
    hyphBreakBefore = hyphBuffer.AppendElements(flowEndInTextRun - start);
    if (hyphBreakBefore) {
      provider.GetHyphenationBreaks(start, flowEndInTextRun - start,
                                    hyphBreakBefore);
    }
  }

  for (uint32_t i = start, wordStart = start; i <= flowEndInTextRun; ++i) {
    bool preformattedNewline = false;
    bool preformattedTab = false;
    if (i < flowEndInTextRun) {
      
      
      
      preformattedNewline = preformatNewlines && textRun->CharIsNewline(i);
      preformattedTab = preformatTabs && textRun->CharIsTab(i);
      if (!textRun->CanBreakLineBefore(i) &&
          !preformattedNewline &&
          !preformattedTab &&
          (!hyphBreakBefore || !hyphBreakBefore[i - start]))
      {
        
        continue;
      }
    }

    if (i > wordStart) {
      nscoord width =
        NSToCoordCeilClamped(textRun->GetAdvanceWidth(wordStart, i - wordStart, &provider));
      aData->currentLine = NSCoordSaturatingAdd(aData->currentLine, width);
      aData->atStartOfLine = false;

      if (collapseWhitespace) {
        uint32_t trimStart = GetEndOfTrimmedText(frag, textStyle, wordStart, i, &iter);
        if (trimStart == start) {
          
          
          aData->trailingWhitespace += width;
        } else {
          
          aData->trailingWhitespace =
            NSToCoordCeilClamped(textRun->GetAdvanceWidth(trimStart, i - trimStart, &provider));
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
                         textRun, &tabWidth);
      aData->currentLine = nscoord(afterTab + spacing.mAfter);
      wordStart = i + 1;
    } else if (i < flowEndInTextRun ||
        (i == textRun->GetLength() &&
         (textRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TRAILING_BREAK))) {
      if (preformattedNewline) {
        aData->ForceBreak(aRenderingContext);
      } else if (i < flowEndInTextRun && hyphBreakBefore &&
                 hyphBreakBefore[i - start])
      {
        aData->OptionallyBreak(aRenderingContext, 
                               NSToCoordRound(provider.GetHyphenWidth()));
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

bool nsTextFrame::IsCurrentFontInflation(float aInflation) const {
  return fabsf(aInflation - GetFontSizeInflation()) < 1e-6;
}



 void
nsTextFrame::AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                               nsIFrame::InlineMinWidthData *aData)
{
  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  TextRunType trtype = (inflation == 1.0f) ? eNotInflated : eInflated;

  if (trtype == eInflated && !IsCurrentFontInflation(inflation)) {
    
    
    ClearTextRun(nullptr, nsTextFrame::eInflated);
  }

  nsTextFrame* f;
  gfxTextRun* lastTextRun = nullptr;
  
  
  for (f = this; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
    
    
    
    if (f == this || f->GetTextRun(trtype) != lastTextRun) {
      nsIFrame* lc;
      if (aData->lineContainer &&
          aData->lineContainer != (lc = FindLineContainer(f))) {
        NS_ASSERTION(f != this, "wrong InlineMinWidthData container"
                                " for first continuation");
        aData->line = nullptr;
        aData->lineContainer = lc;
      }

      
      f->AddInlineMinWidthForFlow(aRenderingContext, aData, trtype);
      lastTextRun = f->GetTextRun(trtype);
    }
  }
}



void
nsTextFrame::AddInlinePrefWidthForFlow(nsRenderingContext *aRenderingContext,
                                       nsIFrame::InlinePrefWidthData *aData,
                                       TextRunType aTextRunType)
{
  uint32_t flowEndInTextRun;
  gfxContext* ctx = aRenderingContext->ThebesContext();
  gfxSkipCharsIterator iter =
    EnsureTextRun(aTextRunType, ctx, aData->lineContainer,
                  aData->line, &flowEndInTextRun);
  gfxTextRun *textRun = GetTextRun(aTextRunType);
  if (!textRun)
    return;

  
  
  
  const nsStyleText* textStyle = StyleText();
  const nsTextFragment* frag = mContent->GetText();
  PropertyProvider provider(textRun, textStyle, frag, this,
                            iter, INT32_MAX, nullptr, 0, aTextRunType);

  bool collapseWhitespace = !textStyle->WhiteSpaceIsSignificant();
  bool preformatNewlines = textStyle->NewlineIsSignificant();
  bool preformatTabs = textStyle->WhiteSpaceIsSignificant();
  gfxFloat tabWidth = -1;
  uint32_t start =
    FindStartAfterSkippingWhitespace(&provider, aData, textStyle, &iter, flowEndInTextRun);

  
  
  
  uint32_t loopStart = (preformatNewlines || preformatTabs) ? start : flowEndInTextRun;
  for (uint32_t i = loopStart, lineStart = start; i <= flowEndInTextRun; ++i) {
    bool preformattedNewline = false;
    bool preformattedTab = false;
    if (i < flowEndInTextRun) {
      
      
      
      NS_ASSERTION(preformatNewlines || textStyle->NewlineIsDiscarded(),
                   "We can't be here unless newlines are hard breaks or are discarded");
      preformattedNewline = preformatNewlines && textRun->CharIsNewline(i);
      preformattedTab = preformatTabs && textRun->CharIsTab(i);
      if (!preformattedNewline && !preformattedTab) {
        
        continue;
      }
    }

    if (i > lineStart) {
      nscoord width =
        NSToCoordCeilClamped(textRun->GetAdvanceWidth(lineStart, i - lineStart, &provider));
      aData->currentLine = NSCoordSaturatingAdd(aData->currentLine, width);

      if (collapseWhitespace) {
        uint32_t trimStart = GetEndOfTrimmedText(frag, textStyle, lineStart, i, &iter);
        if (trimStart == start) {
          
          
          aData->trailingWhitespace += width;
        } else {
          
          aData->trailingWhitespace =
            NSToCoordCeilClamped(textRun->GetAdvanceWidth(trimStart, i - trimStart, &provider));
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
                         textRun, &tabWidth);
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
nsTextFrame::AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlinePrefWidthData *aData)
{
  float inflation = nsLayoutUtils::FontSizeInflationFor(this);
  TextRunType trtype = (inflation == 1.0f) ? eNotInflated : eInflated;

  if (trtype == eInflated && !IsCurrentFontInflation(inflation)) {
    
    
    ClearTextRun(nullptr, nsTextFrame::eInflated);
  }

  nsTextFrame* f;
  gfxTextRun* lastTextRun = nullptr;
  
  
  for (f = this; f; f = static_cast<nsTextFrame*>(f->GetNextContinuation())) {
    
    
    
    if (f == this || f->GetTextRun(trtype) != lastTextRun) {
      nsIFrame* lc;
      if (aData->lineContainer &&
          aData->lineContainer != (lc = FindLineContainer(f))) {
        NS_ASSERTION(f != this, "wrong InlinePrefWidthData container"
                                " for first continuation");
        aData->line = nullptr;
        aData->lineContainer = lc;
      }

      
      f->AddInlinePrefWidthForFlow(aRenderingContext, aData, trtype);
      lastTextRun = f->GetTextRun(trtype);
    }
  }
}

 nsSize
nsTextFrame::ComputeSize(nsRenderingContext *aRenderingContext,
                         nsSize aCBSize, nscoord aAvailableWidth,
                         nsSize aMargin, nsSize aBorder, nsSize aPadding,
                         uint32_t aFlags)
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
  if (StyleContext()->HasTextDecorationLines() ||
      (GetStateBits() & TEXT_HYPHEN_BREAK)) {
    
    return GetVisualOverflowRect();
  }

  gfxSkipCharsIterator iter =
    const_cast<nsTextFrame*>(this)->EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return nsRect(0, 0, 0, 0);

  PropertyProvider provider(const_cast<nsTextFrame*>(this), iter,
                            nsTextFrame::eInflated);
  
  provider.InitializeForDisplay(true);

  gfxTextRun::Metrics metrics =
        mTextRun->MeasureText(provider.GetStart().GetSkippedOffset(),
                              ComputeTransformedLength(provider),
                              gfxFont::TIGHT_HINTED_OUTLINE_EXTENTS,
                              aContext, &provider);
  
  
  return RoundOut(metrics.mBoundingBox) + nsPoint(0, mAscent);
}

static bool
HasSoftHyphenBefore(const nsTextFragment* aFrag, gfxTextRun* aTextRun,
                    int32_t aStartOffset, const gfxSkipCharsIterator& aIter)
{
  if (aIter.GetSkippedOffset() < aTextRun->GetLength() &&
      aTextRun->CanHyphenateBefore(aIter.GetSkippedOffset())) {
    return true;
  }
  if (!(aTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_SHY))
    return false;
  gfxSkipCharsIterator iter = aIter;
  while (iter.GetOriginalOffset() > aStartOffset) {
    iter.AdvanceOriginal(-1);
    if (!iter.IsOriginalCharSkipped())
      break;
    if (aFrag->CharAt(iter.GetOriginalOffset()) == CH_SHY)
      return true;
  }
  return false;
}

static void
RemoveInFlows(nsTextFrame* aFrame, nsTextFrame* aFirstToNotRemove)
{
  NS_PRECONDITION(aFrame != aFirstToNotRemove, "This will go very badly");
  
  
  
  
  
  

  
  
  
  
  NS_ASSERTION(aFirstToNotRemove->GetPrevContinuation() ==
               aFirstToNotRemove->GetPrevInFlow() &&
               aFirstToNotRemove->GetPrevInFlow() != nullptr,
               "aFirstToNotRemove should have a fluid prev continuation");
  NS_ASSERTION(aFrame->GetPrevContinuation() ==
               aFrame->GetPrevInFlow() &&
               aFrame->GetPrevInFlow() != nullptr,
               "aFrame should have a fluid prev continuation");
  
  nsIFrame* prevContinuation = aFrame->GetPrevContinuation();
  nsIFrame* lastRemoved = aFirstToNotRemove->GetPrevContinuation();

  
  
  
  
  
  
  aFrame->ClearTextRuns();
  if (aFrame != lastRemoved) {
    
    static_cast<nsTextFrame*>(lastRemoved)->ClearTextRuns();
  }

  prevContinuation->SetNextInFlow(aFirstToNotRemove);
  aFirstToNotRemove->SetPrevInFlow(prevContinuation);

  aFrame->SetPrevInFlow(nullptr);
  lastRemoved->SetNextInFlow(nullptr);

  nsIFrame* parent = aFrame->GetParent();
  nsBlockFrame* parentBlock = nsLayoutUtils::GetAsBlock(parent);
  if (parentBlock) {
    
    
    
    parentBlock->DoRemoveFrame(aFrame, nsBlockFrame::FRAMES_ARE_EMPTY);
  } else {
    
    
    parent->RemoveFrame(nsIFrame::kNoReflowPrincipalList, aFrame);
  }
}

void
nsTextFrame::SetLength(int32_t aLength, nsLineLayout* aLineLayout,
                       uint32_t aSetLengthFlags)
{
  mContentLengthHint = aLength;
  int32_t end = GetContentOffset() + aLength;
  nsTextFrame* f = static_cast<nsTextFrame*>(GetNextInFlow());
  if (!f)
    return;

  
  
  
  
  
  
  
  if (aLineLayout &&
      (end != f->mContentOffset || (f->GetStateBits() & NS_FRAME_IS_DIRTY))) {
    aLineLayout->SetDirtyNextLine();
  }

  if (end < f->mContentOffset) {
    
    if (aLineLayout &&
        StyleText()->WhiteSpaceIsSignificant() &&
        HasTerminalNewline() &&
        GetParent()->GetType() != nsGkAtoms::letterFrame &&
        (aSetLengthFlags & ALLOW_FRAME_CREATION_AND_DESTRUCTION)) {
      
      
      
      
      
      
      
      
      
      
      nsPresContext* presContext = PresContext();
      nsIFrame* newFrame;
      nsresult rv = presContext->PresShell()->FrameConstructor()->
        CreateContinuingFrame(presContext, this, GetParent(), &newFrame);
      if (NS_SUCCEEDED(rv)) {
        nsTextFrame* next = static_cast<nsTextFrame*>(newFrame);
        nsFrameList temp(next, next);
        GetParent()->InsertFrames(kNoReflowPrincipalList, this, temp);
        f = next;
      }
    }

    f->mContentOffset = end;
    if (f->GetTextRun(nsTextFrame::eInflated) != mTextRun) {
      ClearTextRuns();
      f->ClearTextRuns();
    }
    return;
  }
  
  
  
  
  

  
  
  nsTextFrame* framesToRemove = nullptr;
  while (f && f->mContentOffset < end) {
    f->mContentOffset = end;
    if (f->GetTextRun(nsTextFrame::eInflated) != mTextRun) {
      ClearTextRuns();
      f->ClearTextRuns();
    }
    nsTextFrame* next = static_cast<nsTextFrame*>(f->GetNextInFlow());
    
    
    
    
    if (next && next->mContentOffset <= end && f->GetNextSibling() == next &&
        (aSetLengthFlags & ALLOW_FRAME_CREATION_AND_DESTRUCTION)) {
      
      
      
      
      
      
      if (!framesToRemove) {
        
        framesToRemove = f;
      }

      
      
      
      if (f->StyleContext() == f->GetPrevContinuation()->StyleContext()) {
        f->AddStateBits(TEXT_STYLE_MATCHES_PREV_CONTINUATION);
      }
    } else if (framesToRemove) {
      RemoveInFlows(framesToRemove, f);
      framesToRemove = nullptr;
    }
    f = next;
  }
  NS_POSTCONDITION(!framesToRemove || (f && f->mContentOffset == end),
                   "How did we exit the loop if we null out framesToRemove if "
                   "!next || next->mContentOffset > end ?");
  if (framesToRemove) {
    
    
    RemoveInFlows(framesToRemove, f);
  }

#ifdef DEBUG
  f = this;
  int32_t iterations = 0;
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

bool
nsTextFrame::IsFloatingFirstLetterChild() const
{
  nsIFrame* frame = GetParent();
  return frame && frame->IsFloating() &&
         frame->GetType() == nsGkAtoms::letterFrame;
}

struct NewlineProperty {
  int32_t mStartOffset;
  
  int32_t mNewlineOffset;

  static void Destroy(void* aObject, nsIAtom* aPropertyName,
                      void* aPropertyValue, void* aData)
  {
    delete static_cast<NewlineProperty*>(aPropertyValue);
  }
};

NS_IMETHODIMP
nsTextFrame::Reflow(nsPresContext*           aPresContext,
                    nsHTMLReflowMetrics&     aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus)
{
  DO_GLOBAL_REFLOW_COUNT("nsTextFrame");
  DISPLAY_REFLOW(aPresContext, this, aReflowState, aMetrics, aStatus);

  
  
  
  if (!aReflowState.mLineLayout) {
    ClearMetrics(aMetrics);
    aStatus = NS_FRAME_COMPLETE;
    return NS_OK;
  }

  ReflowText(*aReflowState.mLineLayout, aReflowState.availableWidth,
             aReflowState.rendContext, aReflowState.mFlags.mBlinks,
             aMetrics, aStatus);

  NS_FRAME_SET_TRUNCATION(aStatus, aReflowState, aMetrics);
  return NS_OK;
}

#ifdef ACCESSIBILITY



class NS_STACK_CLASS ReflowTextA11yNotifier
{
public:
  ReflowTextA11yNotifier(nsPresContext* aPresContext, nsIContent* aContent) :
    mContent(aContent), mPresContext(aPresContext)
  {
  }
  ~ReflowTextA11yNotifier()
  {
    nsAccessibilityService* accService = nsIPresShell::AccService();
    if (accService) {
      accService->UpdateText(mPresContext->PresShell(), mContent);
    }
  }
private:
  ReflowTextA11yNotifier();
  ReflowTextA11yNotifier(const ReflowTextA11yNotifier&);
  ReflowTextA11yNotifier& operator =(const ReflowTextA11yNotifier&);

  nsIContent* mContent;
  nsPresContext* mPresContext;
};
#endif

void
nsTextFrame::ReflowText(nsLineLayout& aLineLayout, nscoord aAvailableWidth,
                        nsRenderingContext* aRenderingContext,
                        bool aShouldBlink,
                        nsHTMLReflowMetrics& aMetrics,
                        nsReflowStatus& aStatus)
{
#ifdef NOISY_REFLOW
  ListTag(stdout);
  printf(": BeginReflow: availableWidth=%d\n", aAvailableWidth);
#endif

  nsPresContext* presContext = PresContext();

#ifdef ACCESSIBILITY
  
  ReflowTextA11yNotifier(presContext, mContent);
#endif

  
  
  

  
  
  
  RemoveStateBits(TEXT_REFLOW_FLAGS | TEXT_WHITESPACE_FLAGS);

  
  
  
  
  int32_t maxContentLength = GetInFlowContentLength();

  
  if (!maxContentLength) {
    ClearMetrics(aMetrics);
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  if (aShouldBlink) {
    if (0 == (mState & TEXT_BLINK_ON)) {
      mState |= TEXT_BLINK_ON;
      nsBlinkTimer::AddBlinkFrame(presContext, this);
    }
  }
  else {
    if (0 != (mState & TEXT_BLINK_ON)) {
      mState &= ~TEXT_BLINK_ON;
      nsBlinkTimer::RemoveBlinkFrame(this);
    }
  }

#ifdef NOISY_BIDI
    printf("Reflowed textframe\n");
#endif

  const nsStyleText* textStyle = StyleText();

  bool atStartOfLine = aLineLayout.LineAtStart();
  if (atStartOfLine) {
    AddStateBits(TEXT_START_OF_LINE);
  }

  uint32_t flowEndInTextRun;
  nsIFrame* lineContainer = aLineLayout.GetLineContainerFrame();
  gfxContext* ctx = aRenderingContext->ThebesContext();
  const nsTextFragment* frag = mContent->GetText();

  
  
  
  int32_t length = maxContentLength;
  int32_t offset = GetContentOffset();

  
  int32_t newLineOffset = -1; 
  int32_t contentNewLineOffset = -1;
  
  NewlineProperty* cachedNewlineOffset = nullptr;
  if (textStyle->NewlineIsSignificant()) {
    cachedNewlineOffset =
      static_cast<NewlineProperty*>(mContent->GetProperty(nsGkAtoms::newline));
    if (cachedNewlineOffset && cachedNewlineOffset->mStartOffset <= offset &&
        (cachedNewlineOffset->mNewlineOffset == -1 ||
         cachedNewlineOffset->mNewlineOffset >= offset)) {
      contentNewLineOffset = cachedNewlineOffset->mNewlineOffset;
    } else {
      contentNewLineOffset = FindChar(frag, offset, 
                                      mContent->TextLength() - offset, '\n');
    }
    if (contentNewLineOffset < offset + length) {
      




      newLineOffset = contentNewLineOffset;
    }
    if (newLineOffset >= 0) {
      length = newLineOffset + 1 - offset;
    }
  }
  if ((atStartOfLine && !textStyle->WhiteSpaceIsSignificant()) ||
      (GetStateBits() & TEXT_FORCE_TRIM_WHITESPACE)) {
    
    
    int32_t skipLength = newLineOffset >= 0 ? length - 1 : length;
    int32_t whitespaceCount =
      GetTrimmableWhitespaceCount(frag, offset, skipLength, 1);
    if (whitespaceCount) {
      offset += whitespaceCount;
      length -= whitespaceCount;
      
      if (MOZ_UNLIKELY(offset > GetContentEnd())) {
        SetLength(offset - GetContentOffset(), &aLineLayout,
                  ALLOW_FRAME_CREATION_AND_DESTRUCTION);
      }
    }
  }

  bool completedFirstLetter = false;
  
  
  if (aLineLayout.GetInFirstLetter() || aLineLayout.GetInFirstLine()) {
    SetLength(maxContentLength, &aLineLayout,
              ALLOW_FRAME_CREATION_AND_DESTRUCTION);

    if (aLineLayout.GetInFirstLetter()) {
      
      
      
      ClearTextRuns();
      
      
      gfxSkipCharsIterator iter =
        EnsureTextRun(nsTextFrame::eInflated, ctx,
                      lineContainer, aLineLayout.GetLine(),
                      &flowEndInTextRun);

      if (mTextRun) {
        int32_t firstLetterLength = length;
        if (aLineLayout.GetFirstLetterStyleOK()) {
          completedFirstLetter =
            FindFirstLetterRange(frag, mTextRun, offset, iter, &firstLetterLength);
          if (newLineOffset >= 0) {
            
            firstLetterLength = std::min(firstLetterLength, length - 1);
            if (length == 1) {
              
              
              
              completedFirstLetter = true;
            }
          }
        } else {
          
          
          
          
          
          firstLetterLength = 0;
          completedFirstLetter = true;
        }
        length = firstLetterLength;
        if (length) {
          AddStateBits(TEXT_FIRST_LETTER);
        }
        
        
        
        SetLength(offset + length - GetContentOffset(), &aLineLayout,
                  ALLOW_FRAME_CREATION_AND_DESTRUCTION);
        
        ClearTextRuns();
      }
    } 
  }

  float fontSizeInflation = nsLayoutUtils::FontSizeInflationFor(this);

  if (!IsCurrentFontInflation(fontSizeInflation)) {
    
    
    ClearTextRun(nullptr, nsTextFrame::eInflated);
  }

  gfxSkipCharsIterator iter =
    EnsureTextRun(nsTextFrame::eInflated, ctx,
                  lineContainer, aLineLayout.GetLine(), &flowEndInTextRun);

  NS_ABORT_IF_FALSE(IsCurrentFontInflation(fontSizeInflation),
                    "EnsureTextRun should have set font size inflation");

  if (mTextRun && iter.GetOriginalEnd() < offset + length) {
    
    
    
    
    ClearTextRuns();
    iter = EnsureTextRun(nsTextFrame::eInflated, ctx,
                         lineContainer, aLineLayout.GetLine(),
                         &flowEndInTextRun);
  }

  if (!mTextRun) {
    ClearMetrics(aMetrics);
    aStatus = NS_FRAME_COMPLETE;
    return;
  }

  NS_ASSERTION(gfxSkipCharsIterator(iter).ConvertOriginalToSkipped(offset + length)
                    <= mTextRun->GetLength(),
               "Text run does not map enough text for our reflow");

  
  
  
  
  iter.SetOriginalOffset(offset);
  nscoord xOffsetForTabs = (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TAB) ?
    (aLineLayout.GetCurrentFrameXDistanceFromBlock() -
       lineContainer->GetUsedBorderAndPadding().left)
    : -1;
  PropertyProvider provider(mTextRun, textStyle, frag, this, iter, length,
      lineContainer, xOffsetForTabs, nsTextFrame::eInflated);

  uint32_t transformedOffset = provider.GetStart().GetSkippedOffset();

  
  gfxTextRun::Metrics textMetrics;
  gfxFont::BoundingBoxType boundingBoxType = IsFloatingFirstLetterChild() ?
                                               gfxFont::TIGHT_HINTED_OUTLINE_EXTENTS :
                                               gfxFont::LOOSE_INK_EXTENTS;
  NS_ASSERTION(!(NS_REFLOW_CALC_BOUNDING_METRICS & aMetrics.mFlags),
               "We shouldn't be passed NS_REFLOW_CALC_BOUNDING_METRICS anymore");

  int32_t limitLength = length;
  int32_t forceBreak = aLineLayout.GetForcedBreakPosition(mContent);
  bool forceBreakAfter = false;
  if (forceBreak >= offset + length) {
    forceBreakAfter = forceBreak == offset + length;
    
    forceBreak = -1;
  }
  if (forceBreak >= 0) {
    limitLength = forceBreak - offset;
    NS_ASSERTION(limitLength >= 0, "Weird break found!");
  }
  
  
  uint32_t transformedLength;
  if (offset + limitLength >= int32_t(frag->GetLength())) {
    NS_ASSERTION(offset + limitLength == int32_t(frag->GetLength()),
                 "Content offset/length out of bounds");
    NS_ASSERTION(flowEndInTextRun >= transformedOffset,
                 "Negative flow length?");
    transformedLength = flowEndInTextRun - transformedOffset;
  } else {
    
    
    gfxSkipCharsIterator iter(provider.GetStart());
    iter.SetOriginalOffset(offset + limitLength);
    transformedLength = iter.GetSkippedOffset() - transformedOffset;
  }
  uint32_t transformedLastBreak = 0;
  bool usedHyphenation;
  gfxFloat trimmedWidth = 0;
  gfxFloat availWidth = aAvailableWidth;
  bool canTrimTrailingWhitespace = !textStyle->WhiteSpaceIsSignificant() ||
                                   (GetStateBits() & TEXT_FORCE_TRIM_WHITESPACE);
  int32_t unusedOffset;  
  gfxBreakPriority breakPriority;
  aLineLayout.GetLastOptionalBreakPosition(&unusedOffset, &breakPriority);
  uint32_t transformedCharsFit =
    mTextRun->BreakAndMeasureText(transformedOffset, transformedLength,
                                  (GetStateBits() & TEXT_START_OF_LINE) != 0,
                                  availWidth,
                                  &provider, !aLineLayout.LineIsBreakable(),
                                  canTrimTrailingWhitespace ? &trimmedWidth : nullptr,
                                  &textMetrics, boundingBoxType, ctx,
                                  &usedHyphenation, &transformedLastBreak,
                                  textStyle->WordCanWrap(), &breakPriority);
  if (!length && !textMetrics.mAscent && !textMetrics.mDescent) {
    
    
    nsFontMetrics* fm = provider.GetFontMetrics();
    if (fm) {
      textMetrics.mAscent = gfxFloat(fm->MaxAscent());
      textMetrics.mDescent = gfxFloat(fm->MaxDescent());
    }
  }
  
  
  
  gfxSkipCharsIterator end(provider.GetEndHint());
  end.SetSkippedOffset(transformedOffset + transformedCharsFit);
  int32_t charsFit = end.GetOriginalOffset() - offset;
  if (offset + charsFit == newLineOffset) {
    
    
    
    
    ++charsFit;
  }
  
  
  
  int32_t lastBreak = -1;
  if (charsFit >= limitLength) {
    charsFit = limitLength;
    if (transformedLastBreak != UINT32_MAX) {
      
      
      lastBreak = end.ConvertSkippedToOriginal(transformedOffset + transformedLastBreak);
    }
    end.SetOriginalOffset(offset + charsFit);
    
    
    if ((forceBreak >= 0 || forceBreakAfter) &&
        HasSoftHyphenBefore(frag, mTextRun, offset, end)) {
      usedHyphenation = true;
    }
  }
  if (usedHyphenation) {
    
    AddHyphenToMetrics(this, mTextRun, &textMetrics, boundingBoxType, ctx);
    AddStateBits(TEXT_HYPHEN_BREAK | TEXT_HAS_NONCOLLAPSED_CHARACTERS);
  }

  gfxFloat trimmableWidth = 0;
  bool brokeText = forceBreak >= 0 || transformedCharsFit < transformedLength;
  if (canTrimTrailingWhitespace) {
    
    
    
    
    
    
    if (brokeText ||
        (GetStateBits() & TEXT_FORCE_TRIM_WHITESPACE)) {
      
      
      AddStateBits(TEXT_TRIMMED_TRAILING_WHITESPACE);
    } else if (!(GetStateBits() & TEXT_FORCE_TRIM_WHITESPACE)) {
      
      
      
      
      textMetrics.mAdvanceWidth += trimmedWidth;
      trimmableWidth = trimmedWidth;
      if (mTextRun->IsRightToLeft()) {
        
        
        textMetrics.mBoundingBox.MoveBy(gfxPoint(trimmedWidth, 0));
      }
    }
  }

  if (!brokeText && lastBreak >= 0) {
    
    
    NS_ASSERTION(textMetrics.mAdvanceWidth - trimmableWidth <= aAvailableWidth,
                 "If the text doesn't fit, and we have a break opportunity, why didn't MeasureText use it?");
    aLineLayout.NotifyOptionalBreakPosition(mContent, lastBreak, true, breakPriority);
  }

  int32_t contentLength = offset + charsFit - GetContentOffset();

  
  
  

  
  
  if (GetStateBits() & TEXT_FIRST_LETTER) {
    textMetrics.mAscent = std::max(gfxFloat(0.0), -textMetrics.mBoundingBox.Y());
    textMetrics.mDescent = std::max(gfxFloat(0.0), textMetrics.mBoundingBox.YMost());
  }

  
  
  aMetrics.width = NSToCoordCeil(std::max(gfxFloat(0.0), textMetrics.mAdvanceWidth));

  if (transformedCharsFit == 0 && !usedHyphenation) {
    aMetrics.ascent = 0;
    aMetrics.height = 0;
  } else if (boundingBoxType != gfxFont::LOOSE_INK_EXTENTS) {
    
    aMetrics.ascent = NSToCoordCeil(textMetrics.mAscent);
    aMetrics.height = aMetrics.ascent + NSToCoordCeil(textMetrics.mDescent);
  } else {
    
    
    
    nsFontMetrics* fm = provider.GetFontMetrics();
    nscoord fontAscent = fm->MaxAscent();
    nscoord fontDescent = fm->MaxDescent();
    aMetrics.ascent = std::max(NSToCoordCeil(textMetrics.mAscent), fontAscent);
    nscoord descent = std::max(NSToCoordCeil(textMetrics.mDescent), fontDescent);
    aMetrics.height = aMetrics.ascent + descent;
  }

  NS_ASSERTION(aMetrics.ascent >= 0, "Negative ascent???");
  NS_ASSERTION(aMetrics.height - aMetrics.ascent >= 0, "Negative descent???");

  mAscent = aMetrics.ascent;

  
  nsRect boundingBox = RoundOut(textMetrics.mBoundingBox) + nsPoint(0, mAscent);
  aMetrics.SetOverflowAreasToDesiredBounds();
  aMetrics.VisualOverflow().UnionRect(aMetrics.VisualOverflow(), boundingBox);

  
  
  
  UnionAdditionalOverflow(presContext, *aLineLayout.GetLineContainerRS(),
                          provider, &aMetrics.VisualOverflow(), false);

  
  
  

  
  
  
  
  
  if (transformedCharsFit > 0) {
    aLineLayout.SetTrimmableWidth(NSToCoordFloor(trimmableWidth));
    AddStateBits(TEXT_HAS_NONCOLLAPSED_CHARACTERS);
  }
  if (charsFit > 0 && charsFit == length &&
      textStyle->mHyphens != NS_STYLE_HYPHENS_NONE &&
      HasSoftHyphenBefore(frag, mTextRun, offset, end)) {
    
    aLineLayout.NotifyOptionalBreakPosition(mContent, offset + length,
        textMetrics.mAdvanceWidth + provider.GetHyphenWidth() <= availWidth,
                                           eNormalBreak);
  }
  bool breakAfter = forceBreakAfter;
  
  bool emptyTextAtStartOfLine = atStartOfLine && length == 0;
  if (!breakAfter && charsFit == length && !emptyTextAtStartOfLine &&
      transformedOffset + transformedLength == mTextRun->GetLength() &&
      (mTextRun->GetFlags() & nsTextFrameUtils::TEXT_HAS_TRAILING_BREAK)) {
    
    
    

    
    
    
    
    if (textMetrics.mAdvanceWidth - trimmableWidth > availWidth) {
      breakAfter = true;
    } else {
      aLineLayout.NotifyOptionalBreakPosition(mContent, offset + length,
                                              true, eNormalBreak);
    }
  }

  
  aStatus = contentLength == maxContentLength
    ? NS_FRAME_COMPLETE : NS_FRAME_NOT_COMPLETE;

  if (charsFit == 0 && length > 0 && !usedHyphenation) {
    
    aStatus = NS_INLINE_LINE_BREAK_BEFORE();
  } else if (contentLength > 0 && mContentOffset + contentLength - 1 == newLineOffset) {
    
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
    aLineLayout.SetLineEndsInBR(true);
  } else if (breakAfter) {
    aStatus = NS_INLINE_LINE_BREAK_AFTER(aStatus);
  }
  if (completedFirstLetter) {
    aLineLayout.SetFirstLetterStyleOK(false);
    aStatus |= NS_INLINE_BREAK_FIRST_LETTER_COMPLETE;
  }

  
  if (contentLength < maxContentLength &&
      textStyle->NewlineIsSignificant() &&
      (contentNewLineOffset < 0 ||
       mContentOffset + contentLength <= contentNewLineOffset)) {
    if (!cachedNewlineOffset) {
      cachedNewlineOffset = new NewlineProperty;
      if (NS_FAILED(mContent->SetProperty(nsGkAtoms::newline, cachedNewlineOffset,
                                          NewlineProperty::Destroy))) {
        delete cachedNewlineOffset;
        cachedNewlineOffset = nullptr;
      }
    }
    if (cachedNewlineOffset) {
      cachedNewlineOffset->mStartOffset = offset;
      cachedNewlineOffset->mNewlineOffset = contentNewLineOffset;
    }
  } else if (cachedNewlineOffset) {
    mContent->DeleteProperty(nsGkAtoms::newline);
  }

  
  if (!textStyle->WhiteSpaceIsSignificant() &&
      (lineContainer->StyleText()->mTextAlign == NS_STYLE_TEXT_ALIGN_JUSTIFY ||
       lineContainer->StyleText()->mTextAlignLast == NS_STYLE_TEXT_ALIGN_JUSTIFY) &&
      !lineContainer->IsSVGText()) {
    AddStateBits(TEXT_JUSTIFICATION_ENABLED);    
    
    int32_t numJustifiableCharacters =
      provider.ComputeJustifiableCharacters(offset, charsFit);

    NS_ASSERTION(numJustifiableCharacters <= charsFit,
                 "Bad justifiable character count");
    aLineLayout.SetTextJustificationWeights(numJustifiableCharacters,
        charsFit - numJustifiableCharacters);
  }

  SetLength(contentLength, &aLineLayout, ALLOW_FRAME_CREATION_AND_DESTRUCTION);

  InvalidateFrame();

#ifdef NOISY_REFLOW
  ListTag(stdout);
  printf(": desiredSize=%d,%d(b=%d) status=%x\n",
         aMetrics.width, aMetrics.height, aMetrics.ascent,
         aStatus);
#endif
}

 bool
nsTextFrame::CanContinueTextRun() const
{
  
  return true;
}

nsTextFrame::TrimOutput
nsTextFrame::TrimTrailingWhiteSpace(nsRenderingContext* aRC)
{
  TrimOutput result;
  result.mChanged = false;
  result.mLastCharIsJustifiable = false;
  result.mDeltaWidth = 0;

  AddStateBits(TEXT_END_OF_LINE);

  int32_t contentLength = GetContentLength();
  if (!contentLength)
    return result;

  gfxContext* ctx = aRC->ThebesContext();
  gfxSkipCharsIterator start =
    EnsureTextRun(nsTextFrame::eInflated, ctx);
  NS_ENSURE_TRUE(mTextRun, result);

  uint32_t trimmedStart = start.GetSkippedOffset();

  const nsTextFragment* frag = mContent->GetText();
  TrimmedOffsets trimmed = GetTrimmedOffsets(frag, true);
  gfxSkipCharsIterator trimmedEndIter = start;
  const nsStyleText* textStyle = StyleText();
  gfxFloat delta = 0;
  uint32_t trimmedEnd = trimmedEndIter.ConvertOriginalToSkipped(trimmed.GetEnd());
  
  if (GetStateBits() & TEXT_TRIMMED_TRAILING_WHITESPACE) {
    
    result.mLastCharIsJustifiable = true;
  } else if (trimmed.GetEnd() < GetContentEnd()) {
    gfxSkipCharsIterator end = trimmedEndIter;
    uint32_t endOffset = end.ConvertOriginalToSkipped(GetContentOffset() + contentLength);
    if (trimmedEnd < endOffset) {
      
      
      PropertyProvider provider(mTextRun, textStyle, frag, this, start, contentLength,
                                nullptr, 0, nsTextFrame::eInflated);
      delta = mTextRun->GetAdvanceWidth(trimmedEnd, endOffset - trimmedEnd, &provider);
      
      
      
      result.mLastCharIsJustifiable = true;
      result.mChanged = true;
    }
  }

  if (!result.mLastCharIsJustifiable &&
      (GetStateBits() & TEXT_JUSTIFICATION_ENABLED)) {
    
    PropertyProvider provider(mTextRun, textStyle, frag, this, start, contentLength,
                              nullptr, 0, nsTextFrame::eInflated);
    bool isCJK = IsChineseOrJapanese(this);
    gfxSkipCharsIterator justificationStart(start), justificationEnd(trimmedEndIter);
    provider.FindJustificationRange(&justificationStart, &justificationEnd);

    for (int32_t i = justificationEnd.GetOriginalOffset();
         i < trimmed.GetEnd(); ++i) {
      if (IsJustifiableCharacter(frag, i, isCJK)) {
        result.mLastCharIsJustifiable = true;
      }
    }
  }

  gfxFloat advanceDelta;
  mTextRun->SetLineBreaks(trimmedStart, trimmedEnd - trimmedStart,
                          (GetStateBits() & TEXT_START_OF_LINE) != 0, true,
                          &advanceDelta, ctx);
  if (advanceDelta != 0) {
    result.mChanged = true;
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

nsOverflowAreas
nsTextFrame::RecomputeOverflow(const nsHTMLReflowState& aBlockReflowState)
{
  nsRect bounds(nsPoint(0, 0), GetSize());
  nsOverflowAreas result(bounds, bounds);

  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  if (!mTextRun)
    return result;

  PropertyProvider provider(this, iter, nsTextFrame::eInflated);
  provider.InitializeForDisplay(true);

  gfxTextRun::Metrics textMetrics =
    mTextRun->MeasureText(provider.GetStart().GetSkippedOffset(),
                          ComputeTransformedLength(provider),
                          gfxFont::LOOSE_INK_EXTENTS, nullptr,
                          &provider);
  nsRect &vis = result.VisualOverflow();
  vis.UnionRect(vis, RoundOut(textMetrics.mBoundingBox) + nsPoint(0, mAscent));
  UnionAdditionalOverflow(PresContext(), aBlockReflowState, provider,
                          &vis, true);
  return result;
}
static PRUnichar TransformChar(const nsStyleText* aStyle, gfxTextRun* aTextRun,
                               uint32_t aSkippedOffset, PRUnichar aChar)
{
  if (aChar == '\n') {
    return aStyle->NewlineIsSignificant() || aStyle->NewlineIsDiscarded() ?
             aChar : ' ';
  }
  switch (aStyle->mTextTransform) {
  case NS_STYLE_TEXT_TRANSFORM_LOWERCASE:
    aChar = ToLowerCase(aChar);
    break;
  case NS_STYLE_TEXT_TRANSFORM_UPPERCASE:
    aChar = ToUpperCase(aChar);
    break;
  case NS_STYLE_TEXT_TRANSFORM_CAPITALIZE:
    if (aTextRun->CanBreakLineBefore(aSkippedOffset)) {
      aChar = ToTitleCase(aChar);
    }
    break;
  }

  return aChar;
}

nsresult nsTextFrame::GetRenderedText(nsAString* aAppendToString,
                                      gfxSkipChars* aSkipChars,
                                      gfxSkipCharsIterator* aSkipIter,
                                      uint32_t aSkippedStartOffset,
                                      uint32_t aSkippedMaxLength)
{
  
  gfxSkipCharsBuilder skipCharsBuilder;
  nsTextFrame* textFrame;
  const nsTextFragment* textFrag = mContent->GetText();
  uint32_t keptCharsLength = 0;
  uint32_t validCharsLength = 0;

  
  for (textFrame = this; textFrame;
       textFrame = static_cast<nsTextFrame*>(textFrame->GetNextContinuation())) {
    

    if (textFrame->GetStateBits() & NS_FRAME_IS_DIRTY) {
      
      break;
    }

    
    gfxSkipCharsIterator iter =
      textFrame->EnsureTextRun(nsTextFrame::eInflated);
    if (!textFrame->mTextRun)
      return NS_ERROR_FAILURE;

    
    
    
    
    TrimmedOffsets trimmedContentOffsets = textFrame->GetTrimmedOffsets(textFrag, false);
    int32_t startOfLineSkipChars = trimmedContentOffsets.mStart - textFrame->mContentOffset;
    if (startOfLineSkipChars > 0) {
      skipCharsBuilder.SkipChars(startOfLineSkipChars);
      iter.SetOriginalOffset(trimmedContentOffsets.mStart);
    }

    
    const nsStyleText* textStyle = textFrame->StyleText();
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
nsTextFrame::ToCString(nsCString& aBuf, int32_t* aTotalContentLength) const
{
  
  const nsTextFragment* frag = mContent->GetText();
  if (!frag) {
    return;
  }

  
  *aTotalContentLength = frag->GetLength();

  int32_t contentLength = GetContentLength();
  
  if (0 == contentLength) {
    return;
  }
  int32_t fragOffset = GetContentOffset();
  int32_t n = fragOffset + contentLength;
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

 bool
nsTextFrame::IsEmpty()
{
  NS_ASSERTION(!(mState & TEXT_IS_ONLY_WHITESPACE) ||
               !(mState & TEXT_ISNOT_ONLY_WHITESPACE),
               "Invalid state");
  
  
  const nsStyleText* textStyle = StyleText();
  if (textStyle->WhiteSpaceIsSignificant() &&
      textStyle->mWhiteSpace != NS_STYLE_WHITESPACE_PRE_DISCARD_NEWLINES) {
    
    return false;
  }

  if (mState & TEXT_ISNOT_ONLY_WHITESPACE) {
    return false;
  }

  if (mState & TEXT_IS_ONLY_WHITESPACE) {
    return true;
  }

  bool isEmpty =
    textStyle->mWhiteSpace == NS_STYLE_WHITESPACE_PRE_DISCARD_NEWLINES ?
      IsAllNewlines(mContent->GetText()) :
      IsAllWhitespace(mContent->GetText(),
                      textStyle->mWhiteSpace != NS_STYLE_WHITESPACE_PRE_LINE);
  mState |= (isEmpty ? TEXT_IS_ONLY_WHITESPACE : TEXT_ISNOT_ONLY_WHITESPACE);
  return isEmpty;
}

#ifdef DEBUG
NS_IMETHODIMP
nsTextFrame::GetFrameName(nsAString& aResult) const
{
  MakeFrameName(NS_LITERAL_STRING("Text"), aResult);
  int32_t totalContentLength;
  nsAutoCString tmp;
  ToCString(tmp, &totalContentLength);
  tmp.SetLength(std::min(tmp.Length(), 50u));
  aResult += NS_LITERAL_STRING("\"") + NS_ConvertASCIItoUTF16(tmp) + NS_LITERAL_STRING("\"");
  return NS_OK;
}

NS_IMETHODIMP_(nsFrameState)
nsTextFrame::GetDebugStateBits() const
{
  
  return nsFrame::GetDebugStateBits() &
    ~(TEXT_WHITESPACE_FLAGS | TEXT_REFLOW_FLAGS);
}

NS_IMETHODIMP
nsTextFrame::List(FILE* out, int32_t aIndent, uint32_t aFlags) const
{
  
  IndentBy(out, aIndent);
  ListTag(out);
  if (HasView()) {
    fprintf(out, " [view=%p]", static_cast<void*>(GetView()));
  }
  fprintf(out, " [run=%p]", static_cast<void*>(mTextRun));

  
  bool isComplete = uint32_t(GetContentEnd()) == GetContent()->TextLength();
  fprintf(out, "[%d,%d,%c] ", 
          GetContentOffset(), GetContentLength(),
          isComplete ? 'T':'F');
  
  if (GetNextSibling()) {
    fprintf(out, " next=%p", static_cast<void*>(GetNextSibling()));
  }
  nsIFrame* prevContinuation = GetPrevContinuation();
  if (nullptr != prevContinuation) {
    fprintf(out, " prev-continuation=%p", static_cast<void*>(prevContinuation));
  }
  if (nullptr != mNextContinuation) {
    fprintf(out, " next-continuation=%p", static_cast<void*>(mNextContinuation));
  }

  
  fprintf(out, " {%d,%d,%d,%d}", mRect.x, mRect.y, mRect.width, mRect.height);
  fprintf(out, " [state=%016llx]", (unsigned long long)mState);
  if (IsSelected()) {
    fprintf(out, " SELECTED");
  }
  fprintf(out, " [content=%p]", static_cast<void*>(mContent));
  if (HasOverflowAreas()) {
    nsRect overflowArea = GetVisualOverflowRect();
    fprintf(out, " [vis-overflow=%d,%d,%d,%d]",
            overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
    overflowArea = GetScrollableOverflowRect();
    fprintf(out, " [scr-overflow=%d,%d,%d,%d]",
            overflowArea.x, overflowArea.y,
            overflowArea.width, overflowArea.height);
  }
  fprintf(out, " sc=%p", static_cast<void*>(mStyleContext));
  nsIAtom* pseudoTag = mStyleContext->GetPseudo();
  if (pseudoTag) {
    nsAutoString atomString;
    pseudoTag->ToString(atomString);
    fprintf(out, " pst=%s",
            NS_LossyConvertUTF16toASCII(atomString).get());
  }
  fputs("\n", out);
  return NS_OK;
}
#endif

void
nsTextFrame::AdjustOffsetsForBidi(int32_t aStart, int32_t aEnd)
{
  AddStateBits(NS_FRAME_IS_BIDI);
  mContent->DeleteProperty(nsGkAtoms::flowlength);

  




  ClearTextRuns();

  nsTextFrame* prev = static_cast<nsTextFrame*>(GetPrevContinuation());
  if (prev) {
    
    
    int32_t prevOffset = prev->GetContentOffset();
    aStart = std::max(aStart, prevOffset);
    aEnd = std::max(aEnd, prevOffset);
    prev->ClearTextRuns();
  }

  mContentOffset = aStart;
  SetLength(aEnd - aStart, nullptr, 0);

  







  nsRefPtr<nsFrameSelection> frameSelection = GetFrameSelection();
  if (frameSelection) {
    frameSelection->UndefineCaretBidiLevel();
  }
}





bool
nsTextFrame::HasTerminalNewline() const
{
  return ::HasTerminalNewline(this);
}

bool
nsTextFrame::IsAtEndOfLine() const
{
  return (GetStateBits() & TEXT_END_OF_LINE) != 0;
}

nscoord
nsTextFrame::GetBaseline() const
{
  return mAscent;
}

bool
nsTextFrame::HasAnyNoncollapsedCharacters()
{
  gfxSkipCharsIterator iter = EnsureTextRun(nsTextFrame::eInflated);
  int32_t offset = GetContentOffset(),
          offsetEnd = GetContentEnd();
  int32_t skippedOffset = iter.ConvertOriginalToSkipped(offset);
  int32_t skippedOffsetEnd = iter.ConvertOriginalToSkipped(offsetEnd);
  return skippedOffset != skippedOffsetEnd;
}
