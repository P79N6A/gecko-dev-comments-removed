










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
class nsFirstLineFrame;

























typedef nsContainerFrame nsBlockFrameSuper;





 
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

  friend nsBlockFrame* NS_NewBlockFrame(nsIPresShell* aPresShell,
                                        nsStyleContext* aContext,
                                        nsFrameState aFlags);

  
  NS_DECL_QUERYFRAME

  
  virtual void Init(nsIContent*       aContent,
                    nsContainerFrame* aParent,
                    nsIFrame*         aPrevInFlow) MOZ_OVERRIDE;
  virtual void SetInitialChildList(ChildListID     aListID,
                                   nsFrameList&    aChildList) MOZ_OVERRIDE;
  virtual void AppendFrames(ChildListID     aListID,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void InsertFrames(ChildListID     aListID,
                            nsIFrame*       aPrevFrame,
                            nsFrameList&    aFrameList) MOZ_OVERRIDE;
  virtual void RemoveFrame(ChildListID     aListID,
                           nsIFrame*       aOldFrame) MOZ_OVERRIDE;
  virtual const nsFrameList& GetChildList(ChildListID aListID) const MOZ_OVERRIDE;
  virtual void GetChildLists(nsTArray<ChildList>* aLists) const MOZ_OVERRIDE;
  virtual nscoord GetLogicalBaseline(mozilla::WritingMode aWritingMode) const MOZ_OVERRIDE;
  virtual nscoord GetCaretBaseline() const MOZ_OVERRIDE;
  virtual void DestroyFrom(nsIFrame* aDestructRoot) MOZ_OVERRIDE;
  virtual nsSplittableType GetSplittableType() const MOZ_OVERRIDE;
  virtual bool IsFloatContainingBlock() const MOZ_OVERRIDE;
  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers |
               nsIFrame::eBlockFrame));
  }

  virtual void InvalidateFrame(uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;
  virtual void InvalidateFrameWithRect(const nsRect& aRect, uint32_t aDisplayItemKey = 0) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  void List(FILE* out = stderr, const char* aPrefix = "", uint32_t aFlags = 0) const MOZ_OVERRIDE;
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

#ifdef DEBUG
  virtual nsFrameState GetDebugStateBits() const MOZ_OVERRIDE;
#endif

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  
  
  
  
  
  
  
  
  
  
  

  
  void ClearLineCursor();
  
  
  
  
  
  nsLineBox* GetFirstLineContaining(nscoord y);
  
  
  
  void SetupLineCursor();

  virtual void ChildIsDirty(nsIFrame* aChild) MOZ_OVERRIDE;
  virtual bool IsVisibleInSelection(nsISelection* aSelection) MOZ_OVERRIDE;

  virtual bool IsEmpty() MOZ_OVERRIDE;
  virtual bool CachedIsEmpty() MOZ_OVERRIDE;
  virtual bool IsSelfEmpty() MOZ_OVERRIDE;

  
  
  
  bool BulletIsEmpty() const;

  


  void GetSpokenBulletText(nsAString& aText) const;

  


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

  virtual void MarkIntrinsicISizesDirty() MOZ_OVERRIDE;
private:
  void CheckIntrinsicCacheAgainstShrinkWrapState();
public:
  virtual nscoord GetMinISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefISize(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;

  virtual nsRect ComputeTightBounds(gfxContext* aContext) const MOZ_OVERRIDE;

  virtual nsresult GetPrefWidthTightBounds(nsRenderingContext* aContext,
                                           nscoord* aX,
                                           nscoord* aXMost) MOZ_OVERRIDE;

  
















  void ComputeFinalBSize(const nsHTMLReflowState&      aReflowState,
                         nsReflowStatus*               aStatus,
                         nscoord                       aContentBSize,
                         const mozilla::LogicalMargin& aBorderPadding,
                         mozilla::LogicalSize&         aFinalSize,
                         nscoord                       aConsumed);

  virtual void Reflow(nsPresContext*           aPresContext,
                      nsHTMLReflowMetrics&     aDesiredSize,
                      const nsHTMLReflowState& aReflowState,
                      nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t         aNameSpaceID,
                                    nsIAtom*        aAttribute,
                                    int32_t         aModType) MOZ_OVERRIDE;

  



  virtual bool DrainSelfOverflowList() MOZ_OVERRIDE;

  virtual nsresult StealFrame(nsIFrame* aChild,
                              bool      aForceNormal = false) MOZ_OVERRIDE;

  virtual void DeleteNextInFlowChild(nsIFrame* aNextInFlow,
                                     bool      aDeletingEmptyFrames) MOZ_OVERRIDE;

  





  virtual const nsStyleText* StyleTextForLineLayout();

  




  bool CheckForCollapsedBEndMarginFromClearanceLine();

  static nsresult GetCurrentLine(nsBlockReflowState *aState, nsLineBox **aOutCurrentLine);

  


  void IsMarginRoot(bool* aBStartMarginRoot, bool* aBEndMarginRoot);

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
  explicit nsBlockFrame(nsStyleContext* aContext)
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
  already_AddRefed<nsStyleContext> GetFirstLetterStyle(nsPresContext* aPresContext)
  {
    return aPresContext->StyleSet()->
      ProbePseudoElementStyle(mContent->AsElement(),
                              nsCSSPseudoElements::ePseudo_firstLetter,
                              mStyleContext);
  }
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
  


  void RemoveFrameFromLine(nsIFrame* aChild, nsLineList::iterator aLine,
                           nsFrameList& aFrameList, nsLineList& aLineList);

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

  void ComputeFinalSize(const nsHTMLReflowState& aReflowState,
                        nsBlockReflowState&      aState,
                        nsHTMLReflowMetrics&     aMetrics,
                        nscoord*                 aBottomEdgeOfChildren);

  void ComputeOverflowAreas(const nsRect&         aBounds,
                            const nsStyleDisplay* aDisplay,
                            nscoord               aBottomEdgeOfChildren,
                            nsOverflowAreas&      aOverflowAreas);

  







  void AddFrames(nsFrameList& aFrameList, nsIFrame* aPrevSibling);

  


  nsresult ResolveBidi();

  







  bool IsVisualFormControl(nsPresContext* aPresContext);

public:
  









  enum {
    REMOVE_FIXED_CONTINUATIONS = 0x02,
    FRAMES_ARE_EMPTY           = 0x04
  };
  void DoRemoveFrame(nsIFrame* aDeletedFrame, uint32_t aFlags);

  void ReparentFloats(nsIFrame* aFirstFrame, nsBlockFrame* aOldParent,
                      bool aReparentSiblings);

  virtual bool UpdateOverflow() MOZ_OVERRIDE;

  




  static void RecoverFloatsFor(nsIFrame*       aFrame,
                               nsFloatManager& aFloatManager);

  





  bool HasPushedFloatsFromPrevContinuation() const {
    if (!mFloats.IsEmpty()) {
      
      
      if (mFloats.FirstChild()->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT) {
        return true;
      }
    }

#ifdef DEBUG
    
    
    for (nsFrameList::Enumerator e(mFloats); !e.AtEnd(); e.Next()) {
      nsIFrame* f = e.get();
      NS_ASSERTION(!(f->GetStateBits() & NS_FRAME_IS_PUSHED_FLOAT),
        "pushed floats must be at the beginning of the float list");
    }
#endif
    return false;
  }

protected:

  



  bool DrainOverflowLines();

  



  bool MaybeHasFloats() const {
    if (!mFloats.IsEmpty()) {
      return true;
    }
    
    
    nsFrameList* list = GetPushedFloats();
    if (list && !list->IsEmpty()) {
      return true;
    }
    
    
    return GetStateBits() & NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS;
  }

  


  void DrainPushedFloats(nsBlockReflowState& aState);

  


  void RecoverFloats(nsFloatManager& aFloatManager);

  

  void ReflowPushedFloats(nsBlockReflowState& aState,
                          nsOverflowAreas&    aOverflowAreas,
                          nsReflowStatus&     aStatus);

  

  uint8_t FindTrailingClear();

  


  void RemoveFloat(nsIFrame* aFloat);
  


  void RemoveFloatFromFloatCache(nsIFrame* aFloat);

  void CollectFloats(nsIFrame* aFrame, nsFrameList& aList,
                     bool aCollectFromSiblings) {
    if (MaybeHasFloats()) {
      DoCollectFloats(aFrame, aList, aCollectFromSiblings);
    }
  }
  void DoCollectFloats(nsIFrame* aFrame, nsFrameList& aList,
                       bool aCollectFromSiblings);

  
  static void DoRemoveOutOfFlowFrame(nsIFrame* aFrame);

  


  void PrepareResizeReflow(nsBlockReflowState& aState);

  
  void ReflowDirtyLines(nsBlockReflowState& aState);

  
  void MarkLineDirtyForInterrupt(nsLineBox* aLine);

  
  
  






  void ReflowLine(nsBlockReflowState& aState,
                  line_iterator aLine,
                  bool* aKeepReflowGoing);

  
  
  
  bool PlaceLine(nsBlockReflowState& aState,
                   nsLineLayout&       aLineLayout,
                   line_iterator       aLine,
                   nsFloatManager::SavedState* aFloatStateBeforeLine,
                   nsRect&             aFloatAvailableSpace, 
                   nscoord&            aAvailableSpaceHeight, 
                   bool*             aKeepReflowGoing);

  



  void LazyMarkLinesDirty();

  







  void MarkLineDirty(line_iterator aLine, const nsLineList* aLineList);

  
  bool IsLastLine(nsBlockReflowState& aState,
                  line_iterator aLine);

  void DeleteLine(nsBlockReflowState& aState,
                  nsLineList::iterator aLine,
                  nsLineList::iterator aLineEnd);

  
  

  bool ShouldApplyBStartMargin(nsBlockReflowState& aState,
                               nsLineBox* aLine,
                               nsIFrame* aChildFrame);

  void ReflowBlockFrame(nsBlockReflowState& aState,
                        line_iterator aLine,
                        bool* aKeepGoing);

  void ReflowInlineFrames(nsBlockReflowState& aState,
                          line_iterator aLine,
                          bool* aKeepLineGoing);

  void DoReflowInlineFrames(nsBlockReflowState& aState,
                            nsLineLayout& aLineLayout,
                            line_iterator aLine,
                            nsFlowAreaRect& aFloatAvailableSpace,
                            nscoord& aAvailableSpaceHeight,
                            nsFloatManager::SavedState*
                            aFloatStateBeforeLine,
                            bool* aKeepReflowGoing,
                            LineReflowStatus* aLineReflowStatus,
                            bool aAllowPullUp);

  void ReflowInlineFrame(nsBlockReflowState& aState,
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
  
  
  
  
  void ReflowFloat(nsBlockReflowState& aState,
                   const nsRect&       aAdjustedAvailableSpace,
                   nsIFrame*           aFloat,
                   nsMargin&           aFloatMargin,
                   nsMargin&           aFloatOffsets,
                   
                   
                   
                   bool                aFloatPushedDown,
                   nsReflowStatus&     aReflowStatus);

  
  

  







  bool CreateContinuationFor(nsBlockReflowState& aState,
                             nsLineBox*          aLine,
                             nsIFrame*           aFrame);

  




  void PushTruncatedLine(nsBlockReflowState& aState,
                         line_iterator       aLine,
                         bool*               aKeepReflowGoing);

  void SplitLine(nsBlockReflowState& aState,
                 nsLineLayout& aLineLayout,
                 line_iterator aLine,
                 nsIFrame* aFrame,
                 LineReflowStatus* aLineReflowStatus);

  




  nsIFrame* PullFrame(nsBlockReflowState& aState,
                      line_iterator       aLine);

  











  nsIFrame* PullFrameFrom(nsLineBox*           aLine,
                          nsBlockFrame*        aFromContainer,
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

  

  virtual nsILineIterator* GetLineIterator() MOZ_OVERRIDE;

public:
  bool HasOverflowLines() const {
    return 0 != (GetStateBits() & NS_BLOCK_HAS_OVERFLOW_LINES);
  }
  FrameLines* GetOverflowLines() const;
protected:
  FrameLines* RemoveOverflowLines();
  void SetOverflowLines(FrameLines* aOverflowLines);
  void DestroyOverflowLines();

  





  struct nsAutoOOFFrameList {
    nsFrameList mList;

    explicit nsAutoOOFFrameList(nsBlockFrame* aBlock)
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
  explicit AutoNoisyIndenter(bool aDoIndent) : mIndented(aDoIndent) {
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
