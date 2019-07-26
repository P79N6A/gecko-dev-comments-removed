










#ifndef nsBlockFrame_h___
#define nsBlockFrame_h___

#include "nsContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsLineBox.h"
#include "nsCSSPseudoElements.h"
#include "nsStyleSet.h"
#include "nsFloatManager.h"

enum LineReflowStatus {
  
  
  LINE_REFLOW_OK,
  
  
  LINE_REFLOW_STOP,
  
  
  LINE_REFLOW_REDO_NO_PULL,
  
  
  
  LINE_REFLOW_REDO_MORE_FLOATS,
  
  
  LINE_REFLOW_REDO_NEXT_BAND,
  
  
  LINE_REFLOW_TRUNCATED
};

class nsBlockReflowState;
class nsBlockInFlowLineIterator;
class nsBulletFrame;
class nsLineBox;
class nsFirstLineFrame;
class nsIntervalSet;
































#define NS_BLOCK_NEEDS_BIDI_RESOLUTION      NS_FRAME_STATE_BIT(20)
#define NS_BLOCK_HAS_PUSHED_FLOATS          NS_FRAME_STATE_BIT(21)
#define NS_BLOCK_HAS_LINE_CURSOR            NS_FRAME_STATE_BIT(24)
#define NS_BLOCK_HAS_OVERFLOW_LINES         NS_FRAME_STATE_BIT(25)
#define NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS  NS_FRAME_STATE_BIT(26)




#define NS_BLOCK_HAS_CLEAR_CHILDREN         NS_FRAME_STATE_BIT(27)









#define NS_BLOCK_FRAME_INTRINSICS_INFLATED  NS_FRAME_STATE_BIT(62)

#define nsBlockFrameSuper nsContainerFrame





 
class nsBlockFrame : public nsBlockFrameSuper
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsBlockFrame)
  NS_DECL_FRAMEARENA_HELPERS

  typedef nsLineList::iterator                  line_iterator;
  typedef nsLineList::const_iterator            const_line_iterator;
  typedef nsLineList::reverse_iterator          reverse_line_iterator;
  typedef nsLineList::const_reverse_iterator    const_reverse_line_iterator;

  line_iterator begin_lines() { return mLines.begin(); }
  line_iterator end_lines() { return mLines.end(); }
  const_line_iterator begin_lines() const { return mLines.begin(); }
  const_line_iterator end_lines() const { return mLines.end(); }
  reverse_line_iterator rbegin_lines() { return mLines.rbegin(); }
  reverse_line_iterator rend_lines() { return mLines.rend(); }
  const_reverse_line_iterator rbegin_lines() const { return mLines.rbegin(); }
  const_reverse_line_iterator rend_lines() const { return mLines.rend(); }
  line_iterator line(nsLineBox* aList) { return mLines.begin(aList); }
  reverse_line_iterator rline(nsLineBox* aList) { return mLines.rbegin(aList); }

  friend nsIFrame* NS_NewBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, uint32_t aFlags);

  
  NS_DECL_QUERYFRAME

  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);
  NS_IMETHOD  AppendFrames(ChildListID     aListID,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  InsertFrames(ChildListID     aListID,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  RemoveFrame(ChildListID     aListID,
                          nsIFrame*       aOldFrame);
  virtual const nsFrameList& GetChildList(ChildListID aListID) const;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const;
  virtual nscoord GetBaseline() const MOZ_OVERRIDE;
  virtual nscoord GetCaretBaseline() const MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot);
  virtual nsSplittableType GetSplittableType() const;
  virtual bool IsFloatContainingBlock() const;
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
  virtual nsIAtom* GetType() const;
  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers |
               nsIFrame::eBlockFrame));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0);
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0);

#ifdef DEBUG
  NS_IMETHOD List(FILE* out, int32_t aIndent, uint32_t aFlags = 0) const;
  NS_IMETHOD_(nsFrameState) GetDebugStateBits() const;
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  
  
  
  
  
  
  
  
  
  
  

  
  void ClearLineCursor();
  
  
  
  
  
  nsLineBox* GetFirstLineContaining(nscoord y);
  
  
  
  void SetupLineCursor();

  virtual void ChildIsDirty(nsIFrame* aChild);
  virtual bool IsVisibleInSelection(nsISelection* aSelection);

  virtual bool IsEmpty();
  virtual bool CachedIsEmpty();
  virtual bool IsSelfEmpty();

  
  
  
  bool BulletIsEmpty() const;

  


  void GetBulletText(nsAString& aText) const;

  


  bool HasBullet() const {
    return HasOutsideBullet() || HasInsideBullet();
  }

  


  bool HasInsideBullet() const {
    return 0 != (mState & NS_BLOCK_FRAME_HAS_INSIDE_BULLET);
  }

  


  bool HasOutsideBullet() const {
    return 0 != (mState & NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET);
  }

  


  nsBulletFrame* GetBullet() const {
    nsBulletFrame* outside = GetOutsideBullet();
    return outside ? outside : GetInsideBullet();
  }

  virtual void MarkIntrinsicWidthsDirty();
private:
  void CheckIntrinsicCacheAgainstShrinkWrapState();
public:
  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);

  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;
  
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD AttributeChanged(int32_t         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              int32_t         aModType);

  



  virtual bool DrainSelfOverflowList();

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              bool           aForceNormal = false);

  virtual void DeleteNextInFlowChild(nsPresContext* aPresContext,
                                     nsIFrame*      aNextInFlow,
                                     bool           aDeletingEmptyFrames);

  




  bool CheckForCollapsedBottomMarginFromClearanceLine();

  static nsresult GetCurrentLine(nsBlockReflowState *aState, nsLineBox **aOutCurrentLine);

  


  void IsMarginRoot(bool* aTopMarginRoot, bool* aBottomMarginRoot);

  static bool BlockNeedsFloatManager(nsIFrame* aBlock);

  




  static bool BlockCanIntersectFloats(nsIFrame* aFrame);

  





  struct ReplacedElementWidthToClear {
    nscoord marginLeft, borderBoxWidth, marginRight;
    nscoord MarginBoxWidth() const
      { return marginLeft + borderBoxWidth + marginRight; }
  };
  static ReplacedElementWidthToClear
    WidthToClearPastFloats(nsBlockReflowState& aState,
                           const nsRect& aFloatAvailableSpace,
                           nsIFrame* aFrame);

  






  nsresult SplitFloat(nsBlockReflowState& aState,
                      nsIFrame*           aFloat,
                      nsReflowStatus      aFloatStatus);

  



  static nsBlockFrame* GetNearestAncestorBlock(nsIFrame* aCandidate);
  
  struct FrameLines {
    nsLineList mLines;
    nsFrameList mFrames;
  };

protected:
  nsBlockFrame(nsStyleContext* aContext)
    : nsContainerFrame(aContext)
    , mMinWidth(NS_INTRINSIC_WIDTH_UNKNOWN)
    , mPrefWidth(NS_INTRINSIC_WIDTH_UNKNOWN)
  {
#ifdef DEBUG
  InitDebugFlags();
#endif
  }
  virtual ~nsBlockFrame();

#ifdef DEBUG
#ifdef _IMPL_NS_LAYOUT
  already_AddRefed<nsStyleContext> GetFirstLetterStyle(nsPresContext* aPresContext)
  {
    return aPresContext->StyleSet()->
      ProbePseudoElementStyle(mContent->AsElement(),
                              nsCSSPseudoElements::ePseudo_firstLetter,
                              mStyleContext);
  }
#endif
#endif

  NS_DECLARE_FRAME_PROPERTY(LineCursorProperty, nullptr)
  nsLineBox* GetLineCursor() {
    return (GetStateBits() & NS_BLOCK_HAS_LINE_CURSOR) ?
      static_cast<nsLineBox*>(Properties().Get(LineCursorProperty())) : nullptr;
  }

  nsLineBox* NewLineBox(nsIFrame* aFrame, bool aIsBlock) {
    return NS_NewLineBox(PresContext()->PresShell(), aFrame, aIsBlock);
  }
  nsLineBox* NewLineBox(nsLineBox* aFromLine, nsIFrame* aFrame, int32_t aCount) {
    return NS_NewLineBox(PresContext()->PresShell(), aFromLine, aFrame, aCount);
  }
  void FreeLineBox(nsLineBox* aLine) {
    if (aLine == GetLineCursor()) {
      ClearLineCursor();
    }
    aLine->Destroy(PresContext()->PresShell());
  }

  void TryAllLines(nsLineList::iterator* aIterator,
                   nsLineList::iterator* aStartIterator,
                   nsLineList::iterator* aEndIterator,
                   bool*        aInOverflowLines,
                   FrameLines** aOverflowLines);

  void SetFlags(nsFrameState aFlags) {
    mState &= ~NS_BLOCK_FLAGS_MASK;
    mState |= aFlags;
  }

  


  void SlideLine(nsBlockReflowState& aState,
                 nsLineBox* aLine, nscoord aDY);

  virtual int GetSkipSides() const MOZ_OVERRIDE;

  virtual void ComputeFinalSize(const nsHTMLReflowState& aReflowState,
                                nsBlockReflowState&      aState,
                                nsHTMLReflowMetrics&     aMetrics,
                                nscoord*                 aBottomEdgeOfChildren);

  void ComputeOverflowAreas(const nsRect&         aBounds,
                            const nsStyleDisplay* aDisplay,
                            nscoord               aBottomEdgeOfChildren,
                            nsOverflowAreas&      aOverflowAreas);

  






  virtual nsresult AddFrames(nsFrameList& aFrameList, nsIFrame* aPrevSibling);

#ifdef IBMBIDI
  


  nsresult ResolveBidi();

  







  bool IsVisualFormControl(nsPresContext* aPresContext);
#endif

public:
  









  enum {
    REMOVE_FIXED_CONTINUATIONS = 0x02,
    FRAMES_ARE_EMPTY           = 0x04
  };
  nsresult DoRemoveFrame(nsIFrame* aDeletedFrame, uint32_t aFlags);

  void ReparentFloats(nsIFrame* aFirstFrame,
                      nsBlockFrame* aOldParent, bool aFromOverflow,
                      bool aReparentSiblings);

  virtual bool UpdateOverflow();

  




  static void RecoverFloatsFor(nsIFrame*       aFrame,
                               nsFloatManager& aFloatManager);

protected:

  



  bool DrainOverflowLines();

  


  void DrainPushedFloats(nsBlockReflowState& aState);

  


  void RecoverFloats(nsFloatManager& aFloatManager);

  

  nsresult ReflowPushedFloats(nsBlockReflowState& aState,
                              nsOverflowAreas&    aOverflowAreas,
                              nsReflowStatus&     aStatus);

  

  uint8_t FindTrailingClear();

  



  line_iterator RemoveFloat(nsIFrame* aFloat);

  void CollectFloats(nsIFrame* aFrame, nsFrameList& aList,
                     bool aFromOverflow, bool aCollectFromSiblings);
  
  static void DoRemoveOutOfFlowFrame(nsIFrame* aFrame);

  


  nsresult PrepareResizeReflow(nsBlockReflowState& aState);

  
  nsresult ReflowDirtyLines(nsBlockReflowState& aState);

  
  void MarkLineDirtyForInterrupt(nsLineBox* aLine);

  
  
  






  nsresult ReflowLine(nsBlockReflowState& aState,
                      line_iterator aLine,
                      bool* aKeepReflowGoing);

  
  
  
  bool PlaceLine(nsBlockReflowState& aState,
                   nsLineLayout&       aLineLayout,
                   line_iterator       aLine,
                   nsFloatManager::SavedState* aFloatStateBeforeLine,
                   nsRect&             aFloatAvailableSpace, 
                   nscoord&            aAvailableSpaceHeight, 
                   bool*             aKeepReflowGoing);

  







  void MarkLineDirty(line_iterator aLine, const nsLineList* aLineList);

  
  bool IsLastLine(nsBlockReflowState& aState,
                  line_iterator aLine);

  void DeleteLine(nsBlockReflowState& aState,
                  nsLineList::iterator aLine,
                  nsLineList::iterator aLineEnd);

  
  

  bool ShouldApplyTopMargin(nsBlockReflowState& aState,
                              nsLineBox* aLine);

  nsresult ReflowBlockFrame(nsBlockReflowState& aState,
                            line_iterator aLine,
                            bool* aKeepGoing);

  nsresult ReflowInlineFrames(nsBlockReflowState& aState,
                              line_iterator aLine,
                              bool* aKeepLineGoing);

  nsresult DoReflowInlineFrames(nsBlockReflowState& aState,
                                nsLineLayout& aLineLayout,
                                line_iterator aLine,
                                nsFlowAreaRect& aFloatAvailableSpace,
                                nscoord& aAvailableSpaceHeight,
                                nsFloatManager::SavedState*
                                  aFloatStateBeforeLine,
                                bool* aKeepReflowGoing,
                                LineReflowStatus* aLineReflowStatus,
                                bool aAllowPullUp);

  nsresult ReflowInlineFrame(nsBlockReflowState& aState,
                             nsLineLayout& aLineLayout,
                             line_iterator aLine,
                             nsIFrame* aFrame,
                             LineReflowStatus* aLineReflowStatus);

  
  nsRect AdjustFloatAvailableSpace(nsBlockReflowState& aState,
                                   const nsRect&       aFloatAvailableSpace,
                                   nsIFrame*           aFloatFrame);
  
  nscoord ComputeFloatWidth(nsBlockReflowState& aState,
                            const nsRect&       aFloatAvailableSpace,
                            nsIFrame*           aFloat);
  
  
  
  
  nsresult ReflowFloat(nsBlockReflowState& aState,
                       const nsRect&       aAdjustedAvailableSpace,
                       nsIFrame*           aFloat,
                       nsMargin&           aFloatMargin,
                       
                       
                       
                       bool                aFloatPushedDown,
                       nsReflowStatus&     aReflowStatus);

  
  

  








  virtual nsresult CreateContinuationFor(nsBlockReflowState& aState,
                                         nsLineBox*          aLine,
                                         nsIFrame*           aFrame,
                                         bool&             aMadeNewFrame);

  




  void PushTruncatedLine(nsBlockReflowState& aState,
                         line_iterator       aLine,
                         bool*               aKeepReflowGoing);

  nsresult SplitLine(nsBlockReflowState& aState,
                     nsLineLayout& aLineLayout,
                     line_iterator aLine,
                     nsIFrame* aFrame,
                     LineReflowStatus* aLineReflowStatus);

  




  nsIFrame* PullFrame(nsBlockReflowState& aState,
                      line_iterator       aLine);

  











  nsIFrame* PullFrameFrom(nsBlockReflowState&  aState,
                          nsLineBox*           aLine,
                          nsBlockFrame*        aFromContainer,
                          bool                 aFromOverflowLine,
                          nsFrameList&         aFromFrameList,
                          nsLineList::iterator aFromLine);

  




  void PushLines(nsBlockReflowState& aState,
                 nsLineList::iterator aLineBefore);

  void PropagateFloatDamage(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            nscoord aDeltaY);

  void CheckFloats(nsBlockReflowState& aState);

  
  

  
  
  
  
  bool RenumberLists(nsPresContext* aPresContext);

  static bool RenumberListsInBlock(nsPresContext* aPresContext,
                                   nsBlockFrame* aBlockFrame,
                                   int32_t* aOrdinal,
                                   int32_t aDepth,
                                   int32_t aIncrement);

  static bool RenumberListsFor(nsPresContext* aPresContext, nsIFrame* aKid,
                               int32_t* aOrdinal, int32_t aDepth,
                               int32_t aIncrement);

  static bool FrameStartsCounterScope(nsIFrame* aFrame);

  void ReflowBullet(nsIFrame* aBulletFrame,
                    nsBlockReflowState& aState,
                    nsHTMLReflowMetrics& aMetrics,
                    nscoord aLineTop);

  

  virtual nsILineIterator* GetLineIterator();

public:
  bool HasOverflowLines() const {
    return 0 != (GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES);
  }
  FrameLines* GetOverflowLines() const;
protected:
  FrameLines* RemoveOverflowLines();
  void SetOverflowLines(FrameLines* aOverflowLines);
  void DestroyOverflowLines();

  
  
  
  
  
  nscoord GetEffectiveComputedHeight(const nsHTMLReflowState& aReflowState) const;

  





  struct nsAutoOOFFrameList {
    nsFrameList mList;

    nsAutoOOFFrameList(nsBlockFrame* aBlock)
      : mPropValue(aBlock->GetOverflowOutOfFlows())
      , mBlock(aBlock) {
      if (mPropValue) {
        mList = *mPropValue;
      }
    }
    ~nsAutoOOFFrameList() {
      mBlock->SetOverflowOutOfFlows(mList, mPropValue);
    }
  protected:
    nsFrameList* const mPropValue;
    nsBlockFrame* const mBlock;
  };
  friend struct nsAutoOOFFrameList;

  nsFrameList* GetOverflowOutOfFlows() const;
  void SetOverflowOutOfFlows(const nsFrameList& aList, nsFrameList* aPropValue);

  


  nsBulletFrame* GetInsideBullet() const;

  


  nsBulletFrame* GetOutsideBullet() const;

  


  nsFrameList* GetOutsideBulletList() const;

  


  bool HasPushedFloats() const {
    return 0 != (GetStateBits() & NS_BLOCK_HAS_PUSHED_FLOATS);
  }

  
  
  
  nsFrameList* GetPushedFloats() const;
  
  
  nsFrameList* EnsurePushedFloats();
  
  nsFrameList* RemovePushedFloats();

#ifdef DEBUG
  void VerifyLines(bool aFinalCheckOK);
  void VerifyOverflowSituation();
  int32_t GetDepth() const;
#endif

  nscoord mMinWidth, mPrefWidth;

  nsLineList mLines;

  
  
  nsFrameList mFloats;

  friend class nsBlockReflowState;
  friend class nsBlockInFlowLineIterator;

#ifdef DEBUG
public:
  static bool gLamePaintMetrics;
  static bool gLameReflowMetrics;
  static bool gNoisy;
  static bool gNoisyDamageRepair;
  static bool gNoisyIntrinsic;
  static bool gNoisyReflow;
  static bool gReallyNoisyReflow;
  static bool gNoisyFloatManager;
  static bool gVerifyLines;
  static bool gDisableResizeOpt;

  static int32_t gNoiseIndent;

  static const char* kReflowCommandType[];

protected:
  static void InitDebugFlags();
#endif
};

#ifdef DEBUG
class AutoNoisyIndenter {
public:
  AutoNoisyIndenter(bool aDoIndent) : mIndented(aDoIndent) {
    if (mIndented) {
      nsBlockFrame::gNoiseIndent++;
    }
  }
  ~AutoNoisyIndenter() {
    if (mIndented) {
      nsBlockFrame::gNoiseIndent--;
    }
  }
private:
  bool mIndented;
};
#endif




class nsBlockInFlowLineIterator {
public:
  typedef nsBlockFrame::line_iterator line_iterator;
  



  nsBlockInFlowLineIterator(nsBlockFrame* aFrame, line_iterator aLine);
  





  nsBlockInFlowLineIterator(nsBlockFrame* aFrame, bool* aFoundValidLine);
  







  nsBlockInFlowLineIterator(nsBlockFrame* aFrame, nsIFrame* aFindFrame,
                            bool* aFoundValidLine);

  line_iterator GetLine() { return mLine; }
  bool IsLastLineInList();
  nsBlockFrame* GetContainer() { return mFrame; }
  bool GetInOverflow() { return mLineList != &mFrame->mLines; }

  



  nsLineList* GetLineList() { return mLineList; }

  


  line_iterator End();

  



  bool Next();
  



  bool Prev();

private:
  friend class nsBlockFrame;
  
  nsBlockInFlowLineIterator(nsBlockFrame* aFrame, line_iterator aLine, bool aInOverflow);

  nsBlockFrame* mFrame;
  line_iterator mLine;
  nsLineList*   mLineList;  

  



  bool FindValidLine();
};

#endif 
