
















































#ifndef nsTextFrame_h__
#define nsTextFrame_h__

#include "nsFrame.h"
#include "nsSplittableFrame.h"
#include "nsLineBox.h"
#include "gfxFont.h"
#include "gfxSkipChars.h"
#include "gfxContext.h"
#include "nsDisplayList.h"

class nsTextPaintStyle;
class PropertyProvider;



#define TEXT_HAS_NONCOLLAPSED_CHARACTERS NS_FRAME_STATE_BIT(31)

class nsTextFrame : public nsFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend class nsContinuingTextFrame;

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

  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  
  NS_IMETHOD GetCursor(const nsPoint& aPoint,
                       nsIFrame::Cursor& aCursor);
  
  NS_IMETHOD CharacterDataChanged(CharacterDataChangeInfo* aInfo);
                                  
  virtual void DidSetStyleContext(nsStyleContext* aOldStyleContext);
  
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
  ContentOffsets GetCharacterOffsetAtFramePoint(const nsPoint &aPoint);

  








  virtual void SetSelected(PRBool        aSelected,
                           SelectionType aType);
  void SetSelectedRange(PRUint32 aStart,
                        PRUint32 aEnd,
                        PRBool aSelected,
                        SelectionType aType);

  virtual PRBool PeekOffsetNoAmount(PRBool aForward, PRInt32* aOffset);
  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset,
                                     PRBool aRespectClusters = PR_TRUE);
  virtual PRBool PeekOffsetWord(PRBool aForward, PRBool aWordSelectEatSpace, PRBool aIsKeyboardSelect,
                                PRInt32* aOffset, PeekWordState* aState);

  NS_IMETHOD CheckVisibility(nsPresContext* aContext, PRInt32 aStartIndex, PRInt32 aEndIndex, PRBool aRecurse, PRBool *aFinished, PRBool *_retval);
  
  
  enum { ALLOW_FRAME_CREATION_AND_DESTRUCTION = 0x01 };

  
  void SetLength(PRInt32 aLength, nsLineLayout* aLineLayout,
                 PRUint32 aSetLengthFlags = 0);
  
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
  virtual nscoord GetBaseline() const;
  
  



  virtual PRBool HasTerminalNewline() const;

  



  PRBool IsAtEndOfLine() const;
  
  



  PRBool HasNoncollapsedCharacters() const {
    return (GetStateBits() & TEXT_HAS_NONCOLLAPSED_CHARACTERS) != 0;
  }
  
#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif
  
  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aMetrics,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);
  virtual PRBool CanContinueTextRun() const;
  
  
  
  struct TrimOutput {
    
    
    PRPackedBool mChanged;
    
    
    
    PRPackedBool mLastCharIsJustifiable;
    
    nscoord      mDeltaWidth;
  };
  TrimOutput TrimTrailingWhiteSpace(nsRenderingContext* aRC);
  virtual nsresult GetRenderedText(nsAString* aString = nsnull,
                                   gfxSkipChars* aSkipChars = nsnull,
                                   gfxSkipCharsIterator* aSkipIter = nsnull,
                                   PRUint32 aSkippedStartOffset = 0,
                                   PRUint32 aSkippedMaxLength = PR_UINT32_MAX);

  nsOverflowAreas RecomputeOverflow();

  void AddInlineMinWidthForFlow(nsRenderingContext *aRenderingContext,
                                nsIFrame::InlineMinWidthData *aData);
  void AddInlinePrefWidthForFlow(nsRenderingContext *aRenderingContext,
                                 InlinePrefWidthData *aData);

  gfxFloat GetSnappedBaselineY(gfxContext* aContext, gfxFloat aY);

  







  bool MeasureCharClippedText(gfxContext* aCtx,
                              nscoord aLeftEdge, nscoord aRightEdge,
                              nscoord* aSnappedLeftEdge,
                              nscoord* aSnappedRightEdge);
  





  bool MeasureCharClippedText(gfxContext* aCtx,
                              PropertyProvider& aProvider,
                              nscoord aLeftEdge, nscoord aRightEdge,
                              PRUint32* aStartOffset, PRUint32* aMaxLength,
                              nscoord* aSnappedLeftEdge,
                              nscoord* aSnappedRightEdge);
  
  
  void PaintText(nsRenderingContext* aRenderingContext, nsPoint aPt,
                 const nsRect& aDirtyRect, const nsCharClipDisplayItem& aItem);
  
  void PaintTextDecorations(gfxContext* aCtx, const gfxRect& aDirtyRect,
                            const gfxPoint& aFramePt,
                            const gfxPoint& aTextBaselinePt,
                            nsTextPaintStyle& aTextStyle,
                            PropertyProvider& aProvider,
                            const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                            const nscolor* aOverrideColor = nsnull);
  
  
  
  PRBool PaintTextWithSelection(gfxContext* aCtx,
                                const gfxPoint& aFramePt,
                                const gfxPoint& aTextBaselinePt,
                                const gfxRect& aDirtyRect,
                                PropertyProvider& aProvider,
                                PRUint32 aContentOffset,
                                PRUint32 aContentLength,
                                nsTextPaintStyle& aTextPaintStyle,
                                const nsCharClipDisplayItem::ClipEdges& aClipEdges);
  
  
  
  void PaintTextWithSelectionColors(gfxContext* aCtx,
                                    const gfxPoint& aFramePt,
                                    const gfxPoint& aTextBaselinePt,
                                    const gfxRect& aDirtyRect,
                                    PropertyProvider& aProvider,
                                    PRUint32 aContentOffset,
                                    PRUint32 aContentLength,
                                    nsTextPaintStyle& aTextPaintStyle,
                                    SelectionDetails* aDetails,
                                    SelectionType* aAllTypes);
  
  void PaintTextSelectionDecorations(gfxContext* aCtx,
                                     const gfxPoint& aFramePt,
                                     const gfxPoint& aTextBaselinePt,
                                     const gfxRect& aDirtyRect,
                                     PropertyProvider& aProvider,
                                     PRUint32 aContentOffset,
                                     PRUint32 aContentLength,
                                     nsTextPaintStyle& aTextPaintStyle,
                                     SelectionDetails* aDetails,
                                     SelectionType aSelectionType);

  virtual nscolor GetCaretColorAt(PRInt32 aOffset);

  PRInt16 GetSelectionStatus(PRInt16* aSelectionFlags);

#ifdef DEBUG
  void ToCString(nsCString& aBuf, PRInt32* aTotalContentLength) const;
#endif

  PRInt32 GetContentOffset() const { return mContentOffset; }
  PRInt32 GetContentLength() const
  {
    NS_ASSERTION(GetContentEnd() - mContentOffset >= 0, "negative length");
    return GetContentEnd() - mContentOffset;
  }
  PRInt32 GetContentEnd() const;
  
  
  
  PRInt32 GetContentLengthHint() const { return mContentLengthHint; }

  
  
  
  
  PRInt32 GetInFlowContentLength();

  











  gfxSkipCharsIterator EnsureTextRun(gfxContext* aReferenceContext = nsnull,
                                     nsIFrame* aLineContainer = nsnull,
                                     const nsLineList::iterator* aLine = nsnull,
                                     PRUint32* aFlowEndInTextRun = nsnull);

  gfxTextRun* GetTextRun() { return mTextRun; }
  void SetTextRun(gfxTextRun* aTextRun) { mTextRun = aTextRun; }
  




  void ClearTextRun(nsTextFrame* aStartContinuation);

  
  
  
  struct TrimmedOffsets {
    PRInt32 mStart;
    PRInt32 mLength;
    PRInt32 GetEnd() { return mStart + mLength; }
  };
  TrimmedOffsets GetTrimmedOffsets(const nsTextFragment* aFrag,
                                   PRBool aTrimAfter);

  
  void ReflowText(nsLineLayout& aLineLayout, nscoord aAvailableWidth,
                  nsRenderingContext* aRenderingContext, PRBool aShouldBlink,
                  nsHTMLReflowMetrics& aMetrics, nsReflowStatus& aStatus);

protected:
  virtual ~nsTextFrame();

  nsIFrame*   mNextContinuation;
  
  
  
  
  
  
  
  
  PRInt32     mContentOffset;
  
  
  
  
  PRInt32     mContentLengthHint;
  nscoord     mAscent;
  gfxTextRun* mTextRun;

  
  
  
  SelectionDetails* GetSelectionDetails();
  
  void UnionTextDecorationOverflow(nsPresContext* aPresContext,
                                   PropertyProvider& aProvider,
                                   nsRect* aVisualOverflowRect);

  void DrawText(gfxContext* aCtx,
                const gfxPoint& aTextBaselinePt,
                PRUint32 aOffset,
                PRUint32 aLength,
                const gfxRect* aDirtyRect,
                PropertyProvider* aProvider,
                gfxFloat& aAdvanceWidth,
                PRBool aDrawSoftHyphen);

  void PaintOneShadow(PRUint32 aOffset,
                      PRUint32 aLength,
                      nsCSSShadowItem* aShadowDetails,
                      PropertyProvider* aProvider,
                      const nsRect& aDirtyRect,
                      const gfxPoint& aFramePt,
                      const gfxPoint& aTextBaselinePt,
                      gfxContext* aCtx,
                      const nscolor& aForegroundColor,
                      const nsCharClipDisplayItem::ClipEdges& aClipEdges,
                      nscoord aLeftSideOffset);

  struct TextDecorations {
    PRUint8 mDecorations;
    PRUint8 mOverStyle;
    PRUint8 mUnderStyle;
    PRUint8 mStrikeStyle;
    nscolor mOverColor;
    nscolor mUnderColor;
    nscolor mStrikeColor;

    TextDecorations() :
      mDecorations(0), mOverStyle(NS_STYLE_TEXT_DECORATION_STYLE_SOLID),
      mUnderStyle(NS_STYLE_TEXT_DECORATION_STYLE_SOLID),
      mStrikeStyle(NS_STYLE_TEXT_DECORATION_STYLE_SOLID),
      mOverColor(NS_RGB(0, 0, 0)), mUnderColor(NS_RGB(0, 0, 0)),
      mStrikeColor(NS_RGB(0, 0, 0))
    { }

    PRBool HasDecorationlines() {
      return HasUnderline() || HasOverline() || HasStrikeout();
    }
    PRBool HasUnderline() {
      return (mDecorations & NS_STYLE_TEXT_DECORATION_LINE_UNDERLINE) &&
             mUnderStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE;
    }
    PRBool HasOverline() {
      return (mDecorations & NS_STYLE_TEXT_DECORATION_LINE_OVERLINE) &&
             mOverStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE;
    }
    PRBool HasStrikeout() {
      return (mDecorations & NS_STYLE_TEXT_DECORATION_LINE_LINE_THROUGH) &&
             mStrikeStyle != NS_STYLE_TEXT_DECORATION_STYLE_NONE;
    }
  };
  TextDecorations GetTextDecorations(nsPresContext* aPresContext);

  
  
  PRBool CombineSelectionUnderlineRect(nsPresContext* aPresContext,
                                       nsRect& aRect);

  PRBool IsFloatingFirstLetterChild();

  ContentOffsets GetCharacterOffsetAtFramePointInternal(const nsPoint &aPoint,
                   PRBool aForInsertionPoint);

  void ClearFrameOffsetCache();

  virtual PRBool HasAnyNoncollapsedCharacters();

  void ClearMetrics(nsHTMLReflowMetrics& aMetrics);
};

#endif
