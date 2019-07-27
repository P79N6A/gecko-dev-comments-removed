




#ifndef nsTextFrame_h__
#define nsTextFrame_h__

#include "mozilla/Attributes.h"
#include "mozilla/gfx/2D.h"
#include "nsFrame.h"
#include "nsSplittableFrame.h"
#include "nsLineBox.h"
#include "gfxSkipChars.h"
#include "gfxTextRun.h"
#include "nsDisplayList.h"
#include "JustificationUtils.h"


#if defined(XP_WIN) && defined(DrawText)
#undef DrawText
#endif

class nsTextPaintStyle;
class PropertyProvider;
struct SelectionDetails;
class nsTextFragment;

typedef nsFrame nsTextFrameBase;

class nsDisplayTextGeometry;
class nsDisplayText;

class nsTextFrameTextRunCache {
public:
  static void Init();
  static void Shutdown();
};

class nsTextFrame : public nsTextFrameBase {
  typedef mozilla::gfx::DrawTarget DrawTarget;
  typedef mozilla::gfx::Rect Rect;

public:
  NS_DECL_QUERYFRAME_TARGET(nsTextFrame)
  NS_DECL_FRAMEARENA_HELPERS

  friend class nsContinuingTextFrame;
  friend class nsDisplayTextGeometry;
  friend class nsDisplayText;

  explicit nsTextFrame(nsStyleContext* aContext)
    : nsTextFrameBase(aContext)
  {
    NS_ASSERTION(mContentOffset == 0, "Bogus content offset");
  }
  
  
  NS_DECL_QUERYFRAME

  
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) override;

  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) override;

  virtual void DestroyFrom(nsIFrame* aDestructRoot) override;
  
  virtual nsresult GetCursor(const nsPoint& aPoint,
                             nsIFrame::Cursor& aCursor) override;
  
  virtual nsresult CharacterDataChanged(CharacterDataChangeInfo* aInfo) override;
                                  
  virtual nsIFrame* GetNextContinuation() const override {
    return mNextContinuation;
  }
  virtual void SetNextContinuation(nsIFrame* aNextContinuation) override {
    NS_ASSERTION (!aNextContinuation || GetType() == aNextContinuation->GetType(),
                  "setting a next continuation with incorrect type!");
    NS_ASSERTION (!nsSplittableFrame::IsInNextContinuationChain(aNextContinuation, this),
                  "creating a loop in continuation chain!");
    mNextContinuation = aNextContinuation;
    if (aNextContinuation)
      aNextContinuation->RemoveStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
  }
  virtual nsIFrame* GetNextInFlowVirtual() const override { return GetNextInFlow(); }
  nsIFrame* GetNextInFlow() const {
    return mNextContinuation && (mNextContinuation->GetStateBits() & NS_FRAME_IS_FLUID_CONTINUATION) ? 
      mNextContinuation : nullptr;
  }
  virtual void SetNextInFlow(nsIFrame* aNextInFlow) override {
    NS_ASSERTION (!aNextInFlow || GetType() == aNextInFlow->GetType(),
                  "setting a next in flow with incorrect type!");
    NS_ASSERTION (!nsSplittableFrame::IsInNextContinuationChain(aNextInFlow, this),
                  "creating a loop in continuation chain!");
    mNextContinuation = aNextInFlow;
    if (aNextInFlow)
      aNextInFlow->AddStateBits(NS_FRAME_IS_FLUID_CONTINUATION);
  }
  virtual nsIFrame* LastInFlow() const override;
  virtual nsIFrame* LastContinuation() const override;
  
  virtual nsSplittableType GetSplittableType() const override {
    return NS_FRAME_SPLITTABLE;
  }
  
  




  virtual nsIAtom* GetType() const override;
  
  virtual bool IsFrameOfType(uint32_t aFlags) const override
  {
    
    
    return nsFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced |
                                             nsIFrame::eLineParticipant));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) override;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) override;

#ifdef DEBUG_FRAME_DUMP
  void List(FILE* out = stderr, const char* aPrefix = "", uint32_t aFlags = 0) const override;
  virtual nsresult GetFrameName(nsAString& aResult) const override;
  void ToCString(nsCString& aBuf, int32_t* aTotalContentLength) const;
#endif

#ifdef DEBUG
  virtual nsFrameState GetDebugStateBits() const override;
#endif
  
  virtual ContentOffsets CalcContentOffsetsFromFramePoint(nsPoint aPoint) override;
  ContentOffsets GetCharacterOffsetAtFramePoint(const nsPoint &aPoint);

  








  void SetSelectedRange(uint32_t aStart, uint32_t aEnd, bool aSelected,
                        SelectionType aType);

  virtual FrameSearchResult PeekOffsetNoAmount(bool aForward, int32_t* aOffset) override;
  virtual FrameSearchResult PeekOffsetCharacter(bool aForward, int32_t* aOffset,
                                     bool aRespectClusters = true) override;
  virtual FrameSearchResult PeekOffsetWord(bool aForward, bool aWordSelectEatSpace, bool aIsKeyboardSelect,
                                int32_t* aOffset, PeekWordState* aState) override;

  virtual nsresult CheckVisibility(nsPresContext* aContext, int32_t aStartIndex, int32_t aEndIndex, bool aRecurse, bool *aFinished, bool *_retval) override;
  
  
  enum { ALLOW_FRAME_CREATION_AND_DESTRUCTION = 0x01 };

  
  void SetLength(int32_t aLength, nsLineLayout* aLineLayout,
                 uint32_t aSetLengthFlags = 0);
  
  virtual nsresult GetOffsets(int32_t &start, int32_t &end)const override;
  
  virtual void AdjustOffsetsForBidi(int32_t start, int32_t end) override;
  
  virtual nsresult GetPointFromOffset(int32_t  inOffset,
                                      nsPoint* outPoint) override;
  
  virtual nsresult GetChildFrameContainingOffset(int32_t inContentOffset,
                                                 bool    inHint,
                                                 int32_t* outFrameContentOffset,
                                                 nsIFrame** outChildFrame) override;
  
  virtual bool IsVisibleInSelection(nsISelection* aSelection) override;
  
  virtual bool IsEmpty() override;
  virtual bool IsSelfEmpty() override { return IsEmpty(); }
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const override;
  
  virtual bool HasSignificantTerminalNewline() const override;

  



  bool IsAtEndOfLine() const;
  
  



  bool HasNoncollapsedCharacters() const {
    return (GetStateBits() & TEXT_HAS_NONCOLLAPSED_CHARACTERS) != 0;
  }
  
#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() override;
#endif

  float GetFontSizeInflation() const;
  bool IsCurrentFontInflation(float aInflation) const;
  bool HasFontSizeInflation() const {
    return (GetStateBits() & TEXT_HAS_FONT_INFLATION) != 0;
  }
  void SetFontSizeInflation(float aInflation);

  virtual void MarkIntrinsicISizesDirty() override;
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) override;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) override;
  virtual void AddInlineMinISize(nsRenderingContext *aRenderingContext,
                                 InlineMinISizeData *aData) override;
  virtual void AddInlinePrefISize(nsRenderingContext *aRenderingContext,
                                  InlinePrefISizeData *aData) override;
  virtual mozilla::LogicalSize
  ComputeSize(nsRenderingContext *aRenderingContext,
              mozilla::WritingMode aWritingMode,
              const mozilla::LogicalSize& aCBSize,
              nscoord aAvailableISize,
              const mozilla::LogicalSize& aMargin,
              const mozilla::LogicalSize& aBorder,
              const mozilla::LogicalSize& aPadding,
              ComputeSizeFlags aFlags) override;
  virtual nsRect ComputeTightBounds(gfxContext* aContext) const override;
  virtual nsresult GetPrefWidthTightBounds(nsRenderingContext* aContext,
                                           nscoord* aX,
                                           nscoord* aXMost) override;
  virtual void Reflow(nsPresContext* aPresContext,
                      nsHTMLReflowMetrics& aMetrics,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus& aStatus) override;
  virtual bool CanContinueTextRun() const override;
  
  
  
  struct TrimOutput {
    
    
    bool mChanged;
    
    nscoord      mDeltaWidth;
  };
  TrimOutput TrimTrailingWhiteSpace(nsRenderingContext* aRC);
  virtual nsresult GetRenderedText(nsAString* aString = nullptr,
                                   gfxSkipChars* aSkipChars = nullptr,
                                   gfxSkipCharsIterator* aSkipIter = nullptr,
                                   uint32_t aSkippedStartOffset = 0,
                                   uint32_t aSkippedMaxLength = UINT32_MAX) override;

  nsOverflowAreas RecomputeOverflow(nsIFrame* aBlockFrame);

  enum TextRunType {
    
    
    
    eInflated,
    
    
    eNotInflated
  };

  void AddInlineMinISizeForFlow(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlineMinISizeData *aData,
                                TextRunType aTextRunType);
  void AddInlinePrefISizeForFlow(nsRenderingContext *aRenderingContext,
                                 InlinePrefISizeData *aData,
                                 TextRunType aTextRunType);

  







  bool MeasureCharClippedText(nscoord aVisIStartEdge, nscoord aVisIEndEdge,
                              nscoord* aSnappedStartEdge,
                              nscoord* aSnappedEndEdge);
  





  bool MeasureCharClippedText(PropertyProvider& aProvider,
                              nscoord aVisIStartEdge, nscoord aVisIEndEdge,
                              uint32_t* aStartOffset, uint32_t* aMaxLength,
                              nscoord* aSnappedStartEdge,
                              nscoord* aSnappedEndEdge);

  




















  struct DrawPathCallbacks : gfxTextRunDrawCallbacks
  {
    


    explicit DrawPathCallbacks(bool aShouldPaintSVGGlyphs = false)
      : gfxTextRunDrawCallbacks(aShouldPaintSVGGlyphs)
    {
    }

    



    virtual void NotifySelectionBackgroundNeedsFill(const Rect& aBackgroundRect,
                                                    nscolor aColor,
                                                    DrawTarget& aDrawTarget) { }

    



    virtual void PaintDecorationLine(Rect aPath, nscolor aColor) { }

    




    virtual void PaintSelectionDecorationLine(Rect aPath, nscolor aColor) { }

    



    virtual void NotifyBeforeText(nscolor aColor) { }

    



    virtual void NotifyAfterText() { }

    



    virtual void NotifyBeforeSelectionDecorationLine(nscolor aColor) { }

    



    virtual void NotifySelectionDecorationLinePathEmitted() { }
  };

  
  
  
  
  void PaintText(nsRenderingContext* aRenderingContext, nsPoint aPt,
                 const nsRect& aDirtyRect, const nsCharClipDisplayItem& aItem,
                 gfxTextContextPaint* aContextPaint = nullptr,
                 DrawPathCallbacks* aCallbacks = nullptr);
  
  
  
  bool PaintTextWithSelection(gfxContext* aCtx,
                              const gfxPoint& aFramePt,
                              const gfxPoint& aTextBaselinePt,
                              const gfxRect& aDirtyRect,
                              PropertyProvider& aProvider,
                              uint32_t aContentOffset,
                              uint32_t aContentLength,
                              nsTextPaintStyle& aTextPaintStyle,
                              const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                              gfxTextContextPaint* aContextPaint,
                              DrawPathCallbacks* aCallbacks);
  
  
  
  
  
  bool PaintTextWithSelectionColors(gfxContext* aCtx,
                                    const gfxPoint& aFramePt,
                                    const gfxPoint& aTextBaselinePt,
                                    const gfxRect& aDirtyRect,
                                    PropertyProvider& aProvider,
                                    uint32_t aContentOffset,
                                    uint32_t aContentLength,
                                    nsTextPaintStyle& aTextPaintStyle,
                                    SelectionDetails* aDetails,
                                    SelectionType* aAllTypes,
                             const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                                    DrawPathCallbacks* aCallbacks);
  
  void PaintTextSelectionDecorations(gfxContext* aCtx,
                                     const gfxPoint& aFramePt,
                                     const gfxPoint& aTextBaselinePt,
                                     const gfxRect& aDirtyRect,
                                     PropertyProvider& aProvider,
                                     uint32_t aContentOffset,
                                     uint32_t aContentLength,
                                     nsTextPaintStyle& aTextPaintStyle,
                                     SelectionDetails* aDetails,
                                     SelectionType aSelectionType,
                                     DrawPathCallbacks* aCallbacks);

  virtual nscolor GetCaretColorAt(int32_t aOffset) override;

  int16_t GetSelectionStatus(int16_t* aSelectionFlags);

  int32_t GetContentOffset() const { return mContentOffset; }
  int32_t GetContentLength() const
  {
    NS_ASSERTION(GetContentEnd() - mContentOffset >= 0, "negative length");
    return GetContentEnd() - mContentOffset;
  }
  int32_t GetContentEnd() const;
  
  
  
  int32_t GetContentLengthHint() const { return mContentLengthHint; }

  
  
  
  
  int32_t GetInFlowContentLength();

  














  gfxSkipCharsIterator EnsureTextRun(TextRunType aWhichTextRun,
                                     gfxContext* aReferenceContext = nullptr,
                                     nsIFrame* aLineContainer = nullptr,
                                     const nsLineList::iterator* aLine = nullptr,
                                     uint32_t* aFlowEndInTextRun = nullptr);

  gfxTextRun* GetTextRun(TextRunType aWhichTextRun) {
    if (aWhichTextRun == eInflated || !HasFontSizeInflation())
      return mTextRun;
    return GetUninflatedTextRun();
  }
  gfxTextRun* GetUninflatedTextRun();
  void SetTextRun(gfxTextRun* aTextRun, TextRunType aWhichTextRun,
                  float aInflation);
  bool IsInTextRunUserData() const {
    return GetStateBits() &
      (TEXT_IN_TEXTRUN_USER_DATA | TEXT_IN_UNINFLATED_TEXTRUN_USER_DATA);
  }
  





  bool RemoveTextRun(gfxTextRun* aTextRun);
  






  void ClearTextRun(nsTextFrame* aStartContinuation,
                    TextRunType aWhichTextRun);

  void ClearTextRuns() {
    ClearTextRun(nullptr, nsTextFrame::eInflated);
    if (HasFontSizeInflation()) {
      ClearTextRun(nullptr, nsTextFrame::eNotInflated);
    }
  }

  


  void DisconnectTextRuns();

  
  
  
  struct TrimmedOffsets {
    int32_t mStart;
    int32_t mLength;
    int32_t GetEnd() const { return mStart + mLength; }
  };
  TrimmedOffsets GetTrimmedOffsets(const nsTextFragment* aFrag,
                                   bool aTrimAfter, bool aPostReflow = true);

  
  void ReflowText(nsLineLayout& aLineLayout, nscoord aAvailableWidth,
                  nsRenderingContext* aRenderingContext,
                  nsHTMLReflowMetrics& aMetrics, nsReflowStatus& aStatus);

  bool IsFloatingFirstLetterChild() const;

  virtual bool UpdateOverflow() override;

  void AssignJustificationGaps(const mozilla::JustificationAssignment& aAssign);
  mozilla::JustificationAssignment GetJustificationAssignment() const;

protected:
  virtual ~nsTextFrame();

  nsIFrame*   mNextContinuation;
  
  
  
  
  
  
  
  
  int32_t     mContentOffset;
  
  
  
  
  int32_t     mContentLengthHint;
  nscoord     mAscent;
  gfxTextRun* mTextRun;

  



  virtual bool IsFrameSelected() const override;

  
  
  
  SelectionDetails* GetSelectionDetails();

  void UnionAdditionalOverflow(nsPresContext* aPresContext,
                               nsIFrame* aBlock,
                               PropertyProvider& aProvider,
                               nsRect* aVisualOverflowRect,
                               bool aIncludeTextDecorations);

  void PaintOneShadow(uint32_t aOffset,
                      uint32_t aLength,
                      nsCSSShadowItem* aShadowDetails,
                      PropertyProvider* aProvider,
                      const nsRect& aDirtyRect,
                      const gfxPoint& aFramePt,
                      const gfxPoint& aTextBaselinePt,
                      gfxContext* aCtx,
                      const nscolor& aForegroundColor,
                      const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                      nscoord aLeftSideOffset,
                      gfxRect& aBoundingBox,
                      uint32_t aBlurFlags);

  void PaintShadows(nsCSSShadowArray* aShadow,
                    uint32_t aOffset, uint32_t aLength,
                    const nsRect& aDirtyRect,
                    const gfxPoint& aFramePt,
                    const gfxPoint& aTextBaselinePt,
                    nscoord aLeftEdgeOffset,
                    PropertyProvider& aProvider,
                    nscolor aForegroundColor,
                    const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                    gfxContext* aCtx);

  struct LineDecoration {
    nsIFrame* mFrame;

    
    
    nscoord mBaselineOffset;

    nscolor mColor;
    uint8_t mStyle;

    LineDecoration(nsIFrame *const aFrame,
                   const nscoord aOff,
                   const nscolor aColor,
                   const uint8_t aStyle)
      : mFrame(aFrame),
        mBaselineOffset(aOff),
        mColor(aColor),
        mStyle(aStyle)
    {}

    LineDecoration(const LineDecoration& aOther)
      : mFrame(aOther.mFrame),
        mBaselineOffset(aOther.mBaselineOffset),
        mColor(aOther.mColor),
        mStyle(aOther.mStyle)
    {}

    bool operator==(const LineDecoration& aOther) const {
      return mFrame == aOther.mFrame &&
             mStyle == aOther.mStyle &&
             mColor == aOther.mColor &&
             mBaselineOffset == aOther.mBaselineOffset;
    }

    bool operator!=(const LineDecoration& aOther) const {
      return !(*this == aOther);
    }
  };
  struct TextDecorations {
    nsAutoTArray<LineDecoration, 1> mOverlines, mUnderlines, mStrikes;

    TextDecorations() { }

    bool HasDecorationLines() const {
      return HasUnderline() || HasOverline() || HasStrikeout();
    }
    bool HasUnderline() const {
      return !mUnderlines.IsEmpty();
    }
    bool HasOverline() const {
      return !mOverlines.IsEmpty();
    }
    bool HasStrikeout() const {
      return !mStrikes.IsEmpty();
    }
    bool operator==(const TextDecorations& aOther) const {
      return mOverlines == aOther.mOverlines &&
             mUnderlines == aOther.mUnderlines &&
             mStrikes == aOther.mStrikes;
    }
    
    bool operator!=(const TextDecorations& aOther) const {
      return !(*this == aOther);
    }

  };
  enum TextDecorationColorResolution {
    eResolvedColors,
    eUnresolvedColors
  };
  void GetTextDecorations(nsPresContext* aPresContext,
                          TextDecorationColorResolution aColorResolution,
                          TextDecorations& aDecorations);

  void DrawTextRun(gfxContext* const aCtx,
                   const gfxPoint& aTextBaselinePt,
                   uint32_t aOffset,
                   uint32_t aLength,
                   PropertyProvider& aProvider,
                   nscolor aTextColor,
                   gfxFloat& aAdvanceWidth,
                   bool aDrawSoftHyphen,
                   gfxTextContextPaint* aContextPaint,
                   DrawPathCallbacks* aCallbacks);

  void DrawTextRunAndDecorations(gfxContext* const aCtx,
                                 const gfxRect& aDirtyRect,
                                 const gfxPoint& aFramePt,
                                 const gfxPoint& aTextBaselinePt,
                                 uint32_t aOffset,
                                 uint32_t aLength,
                                 PropertyProvider& aProvider,
                                 const nsTextPaintStyle& aTextStyle,
                                 nscolor aTextColor,
                             const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                                 gfxFloat& aAdvanceWidth,
                                 bool aDrawSoftHyphen,
                                 const TextDecorations& aDecorations,
                                 const nscolor* const aDecorationOverrideColor,
                                 gfxTextContextPaint* aContextPaint,
                                 DrawPathCallbacks* aCallbacks);

  void DrawText(gfxContext* const aCtx,
                const gfxRect& aDirtyRect,
                const gfxPoint& aFramePt,
                const gfxPoint& aTextBaselinePt,
                uint32_t aOffset,
                uint32_t aLength,
                PropertyProvider& aProvider,
                const nsTextPaintStyle& aTextStyle,
                nscolor aTextColor,
                const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                gfxFloat& aAdvanceWidth,
                bool aDrawSoftHyphen,
                const nscolor* const aDecorationOverrideColor = nullptr,
                gfxTextContextPaint* aContextPaint = nullptr,
                DrawPathCallbacks* aCallbacks = nullptr);

  
  
  bool CombineSelectionUnderlineRect(nsPresContext* aPresContext,
                                       nsRect& aRect);

  ContentOffsets GetCharacterOffsetAtFramePointInternal(nsPoint aPoint,
                   bool aForInsertionPoint);

  void ClearFrameOffsetCache();

  virtual bool HasAnyNoncollapsedCharacters() override;

  void ClearMetrics(nsHTMLReflowMetrics& aMetrics);

  NS_DECLARE_FRAME_PROPERTY(JustificationAssignment, nullptr)
};

#endif
