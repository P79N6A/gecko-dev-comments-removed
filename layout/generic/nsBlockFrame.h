










































#ifndef nsBlockFrame_h___
#define nsBlockFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsAbsoluteContainingBlock.h"
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




#define NS_BLOCK_LIST_COUNT  (NS_CONTAINER_LIST_COUNT_INCL_OC + 4)






























#define NS_BLOCK_NEEDS_BIDI_RESOLUTION      0x00100000 
#define NS_BLOCK_HAS_LINE_CURSOR            0x01000000
#define NS_BLOCK_HAS_OVERFLOW_LINES         0x02000000
#define NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS  0x04000000




#define NS_BLOCK_HAS_CLEAR_CHILDREN         0x08000000

#define nsBlockFrameSuper nsHTMLContainerFrame







 
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

  friend nsIFrame* NS_NewBlockFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, PRUint32 aFlags);

  
  NS_DECL_QUERYFRAME

  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);
  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsFrameList&    aFrameList);
  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);
  virtual nsFrameList GetChildList(nsIAtom* aListName) const;
  virtual nsIFrame* GetLastChild(nsIAtom* aListName) const;
  virtual nscoord GetBaseline() const;
  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;
  virtual void Destroy();
  virtual nsSplittableType GetSplittableType() const;
  virtual PRBool IsContainingBlock() const;
  virtual PRBool IsFloatContainingBlock() const;
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);
  virtual nsIAtom* GetType() const;
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers |
               nsIFrame::eBlockFrame));
  }

#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD_(nsFrameState) GetDebugStateBits() const;
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  
  
  
  
  
  
  
  
  
  

  
  void ClearLineCursor();
  
  
  
  
  
  nsLineBox* GetFirstLineContaining(nscoord y);
  
  
  
  void SetupLineCursor();

  virtual void ChildIsDirty(nsIFrame* aChild);
  virtual PRBool IsVisibleInSelection(nsISelection* aSelection);

  virtual PRBool IsEmpty();
  virtual PRBool CachedIsEmpty();
  virtual PRBool IsSelfEmpty();

  
  
  
  PRBool BulletIsEmpty() const;

  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;
  
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              PRBool         aForceNormal = PR_FALSE);

  virtual void DeleteNextInFlowChild(nsPresContext* aPresContext,
                                     nsIFrame*      aNextInFlow,
                                     PRBool         aDeletingEmptyFrames);

  




  PRBool CheckForCollapsedBottomMarginFromClearanceLine();

  



  nsIFrame* GetTopBlockChild(nsPresContext *aPresContext);

  static nsresult GetCurrentLine(nsBlockReflowState *aState, nsLineBox **aOutCurrentLine);

  static PRBool BlockIsMarginRoot(nsIFrame* aBlock);
  static PRBool BlockNeedsFloatManager(nsIFrame* aBlock);

  




  static PRBool BlockCanIntersectFloats(nsIFrame* aFrame);

  





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
  
protected:
  nsBlockFrame(nsStyleContext* aContext)
    : nsHTMLContainerFrame(aContext)
    , mMinWidth(NS_INTRINSIC_WIDTH_UNKNOWN)
    , mPrefWidth(NS_INTRINSIC_WIDTH_UNKNOWN)
    , mAbsoluteContainer(nsGkAtoms::absoluteList)
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
      ProbePseudoStyleFor(mContent,
                          nsCSSPseudoElements::firstLetter, mStyleContext);
  }
#endif

  



  virtual void PaintTextDecorationLine(gfxContext* aCtx,
                                       const nsPoint& aPt,
                                       nsLineBox* aLine,
                                       nscolor aColor,
                                       gfxFloat aOffset,
                                       gfxFloat aAscent,
                                       gfxFloat aSize,
                                       const PRUint8 aDecoration);

  void TryAllLines(nsLineList::iterator* aIterator,
                   nsLineList::iterator* aStartIterator,
                   nsLineList::iterator* aEndIterator,
                   PRBool* aInOverflowLines);

  void SetFlags(PRUint32 aFlags) {
    mState &= ~NS_BLOCK_FLAGS_MASK;
    mState |= aFlags;
  }

  PRBool HaveOutsideBullet() const {
#if defined(DEBUG) && !defined(DEBUG_rods)
    if(mState & NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET) {
      NS_ASSERTION(mBullet,"NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET flag set and no mBullet");
    }
#endif
    return 0 != (mState & NS_BLOCK_FRAME_HAS_OUTSIDE_BULLET);
  }

  


  void SlideLine(nsBlockReflowState& aState,
                 nsLineBox* aLine, nscoord aDY);

  virtual PRIntn GetSkipSides() const;

  virtual void ComputeFinalSize(const nsHTMLReflowState& aReflowState,
                                nsBlockReflowState&      aState,
                                nsHTMLReflowMetrics&     aMetrics,
                                nscoord*                 aBottomEdgeOfChildren);

  void ComputeCombinedArea(const nsHTMLReflowState& aReflowState,
                           nsHTMLReflowMetrics&     aMetrics,
                           nscoord                  aBottomEdgeOfChildren);

  





  virtual nsresult AddFrames(const nsFrameList& aFrameList,
                             nsIFrame* aPrevSibling);

#ifdef IBMBIDI
  


  nsresult ResolveBidi();

  







  PRBool IsVisualFormControl(nsPresContext* aPresContext);
#endif

public:
  









  enum {
    REMOVE_FIXED_CONTINUATIONS = 0x02,
    FRAMES_ARE_EMPTY           = 0x04
  };
  nsresult DoRemoveFrame(nsIFrame* aDeletedFrame, PRUint32 aFlags);

  void ReparentFloats(nsIFrame* aFirstFrame,
                      nsBlockFrame* aOldParent, PRBool aFromOverflow,
                      PRBool aReparentSiblings);

  




  static void RecoverFloatsFor(nsIFrame*       aFrame,
                               nsFloatManager& aFloatManager);

protected:

  



  PRBool DrainOverflowLines(nsBlockReflowState& aState);

  


  void DrainFloatContinuations(nsBlockReflowState& aState);

  


  void RecoverFloats(nsFloatManager& aFloatManager);

  

  nsresult ReflowFloatContinuations(nsBlockReflowState& aState,
                                    nsRect&             aBounds,
                                    nsReflowStatus&     aStatus);

  

  PRUint8 FindTrailingClear();

  



  line_iterator RemoveFloat(nsIFrame* aFloat);

  void CollectFloats(nsIFrame* aFrame, nsFrameList& aList, nsIFrame** aTail,
                     PRBool aFromOverflow, PRBool aCollectFromSiblings);
  
  static void DoRemoveOutOfFlowFrame(nsIFrame* aFrame);

  


  nsresult PrepareResizeReflow(nsBlockReflowState& aState);

  
  nsresult ReflowDirtyLines(nsBlockReflowState& aState);

  
  void MarkLineDirtyForInterrupt(nsLineBox* aLine);

  
  
  






  nsresult ReflowLine(nsBlockReflowState& aState,
                      line_iterator aLine,
                      PRBool* aKeepReflowGoing);

  
  
  
  PRBool PlaceLine(nsBlockReflowState& aState,
                   nsLineLayout&       aLineLayout,
                   line_iterator       aLine,
                   nsFloatManager::SavedState* aFloatStateBeforeLine,
                   nsRect&             aFloatAvailableSpace, 
                   nscoord&            aAvailableSpaceHeight, 
                   PRBool*             aKeepReflowGoing);

  








  nsresult MarkLineDirty(line_iterator aLine,
                         const nsLineList* aLineList = nsnull);

  
  PRBool ShouldJustifyLine(nsBlockReflowState& aState,
                           line_iterator aLine);

  void DeleteLine(nsBlockReflowState& aState,
                  nsLineList::iterator aLine,
                  nsLineList::iterator aLineEnd);

  
  

  PRBool ShouldApplyTopMargin(nsBlockReflowState& aState,
                              nsLineBox* aLine);

  nsresult ReflowBlockFrame(nsBlockReflowState& aState,
                            line_iterator aLine,
                            PRBool* aKeepGoing);

  nsresult ReflowInlineFrames(nsBlockReflowState& aState,
                              line_iterator aLine,
                              PRBool* aKeepLineGoing);

  nsresult DoReflowInlineFrames(nsBlockReflowState& aState,
                                nsLineLayout& aLineLayout,
                                line_iterator aLine,
                                nsFlowAreaRect& aFloatAvailableSpace,
                                nscoord& aAvailableSpaceHeight,
                                nsFloatManager::SavedState*
                                  aFloatStateBeforeLine,
                                PRBool* aKeepReflowGoing,
                                LineReflowStatus* aLineReflowStatus,
                                PRBool aAllowPullUp);

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
                       const nsRect&       aFloatAvailableSpace,
                       nsIFrame*           aFloat,
                       nsMargin&           aFloatMargin,
                       nsReflowStatus&     aReflowStatus);

  
  

  








  virtual nsresult CreateContinuationFor(nsBlockReflowState& aState,
                                         nsLineBox*          aLine,
                                         nsIFrame*           aFrame,
                                         PRBool&             aMadeNewFrame);

  
  
  void PushTruncatedPlaceholderLine(nsBlockReflowState& aState,
                                    line_iterator       aLine,
                                    PRBool&             aKeepReflowGoing);

  nsresult SplitLine(nsBlockReflowState& aState,
                     nsLineLayout& aLineLayout,
                     line_iterator aLine,
                     nsIFrame* aFrame,
                     LineReflowStatus* aLineReflowStatus);

  nsresult PullFrame(nsBlockReflowState& aState,
                     line_iterator aLine,
                     nsIFrame*& aFrameResult);

  PRBool PullFrameFrom(nsBlockReflowState& aState,
                       nsLineBox* aLine,
                       nsBlockFrame* aFromContainer,
                       PRBool aFromOverflowLine,
                       nsLineList::iterator aFromLine,
                       nsIFrame*& aFrameResult);

  void PushLines(nsBlockReflowState& aState,
                 nsLineList::iterator aLineBefore);

  void PropagateFloatDamage(nsBlockReflowState& aState,
                            nsLineBox* aLine,
                            nscoord aDeltaY);

  void CheckFloats(nsBlockReflowState& aState);

  
  

  
  
  
  
  PRBool RenumberLists(nsPresContext* aPresContext);

  static PRBool RenumberListsInBlock(nsPresContext* aPresContext,
                                     nsBlockFrame* aBlockFrame,
                                     PRInt32* aOrdinal,
                                     PRInt32 aDepth);

  static PRBool RenumberListsFor(nsPresContext* aPresContext, nsIFrame* aKid, PRInt32* aOrdinal, PRInt32 aDepth);

  static PRBool FrameStartsCounterScope(nsIFrame* aFrame);

  void ReflowBullet(nsBlockReflowState& aState,
                    nsHTMLReflowMetrics& aMetrics,
                    nscoord aLineTop);

  

  virtual nsILineIterator* GetLineIterator();

public:
  nsLineList* GetOverflowLines() const;
protected:
  nsLineList* RemoveOverflowLines();
  nsresult SetOverflowLines(nsLineList* aOverflowLines);

  





  struct nsAutoOOFFrameList {
    nsFrameList mList;

    nsAutoOOFFrameList(nsBlockFrame* aBlock) :
      mList(aBlock->GetOverflowOutOfFlows().FirstChild()),
      aOldHead(mList.FirstChild()), mBlock(aBlock) {}
    ~nsAutoOOFFrameList() {
      if (mList.FirstChild() != aOldHead) {
        mBlock->SetOverflowOutOfFlows(mList);
      }
    }
  protected:
    nsIFrame* aOldHead;
    nsBlockFrame* mBlock;
  };
  friend struct nsAutoOOFFrameList;

  nsFrameList GetOverflowOutOfFlows() const;
  void SetOverflowOutOfFlows(const nsFrameList& aList);

  nsIFrame* LastChild();

#ifdef NS_DEBUG
  void VerifyLines(PRBool aFinalCheckOK);
  void VerifyOverflowSituation();
  PRInt32 GetDepth() const;
#endif

  nscoord mMinWidth, mPrefWidth;

  nsLineList mLines;

  
  nsFrameList mFloats;

  
  
  nsBulletFrame* mBullet;

  friend class nsBlockReflowState;
  friend class nsBlockInFlowLineIterator;

private:
  nsAbsoluteContainingBlock mAbsoluteContainer;


#ifdef DEBUG
public:
  static PRBool gLamePaintMetrics;
  static PRBool gLameReflowMetrics;
  static PRBool gNoisy;
  static PRBool gNoisyDamageRepair;
  static PRBool gNoisyIntrinsic;
  static PRBool gNoisyReflow;
  static PRBool gReallyNoisyReflow;
  static PRBool gNoisyFloatManager;
  static PRBool gVerifyLines;
  static PRBool gDisableResizeOpt;

  static PRInt32 gNoiseIndent;

  static const char* kReflowCommandType[];

protected:
  static void InitDebugFlags();
#endif
};

#ifdef DEBUG
class AutoNoisyIndenter {
public:
  AutoNoisyIndenter(PRBool aDoIndent) : mIndented(aDoIndent) {
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
  PRBool mIndented;
};
#endif




class nsBlockInFlowLineIterator {
public:
  typedef nsBlockFrame::line_iterator line_iterator;
  nsBlockInFlowLineIterator(nsBlockFrame* aFrame, line_iterator aLine, PRBool aInOverflow);
  



  nsBlockInFlowLineIterator(nsBlockFrame* aFrame, PRBool* aFoundValidLine);
  





  nsBlockInFlowLineIterator(nsBlockFrame* aFrame, nsIFrame* aFindFrame,
                            PRBool* aFoundValidLine);

  line_iterator GetLine() { return mLine; }
  PRBool IsLastLineInList();
  nsBlockFrame* GetContainer() { return mFrame; }
  PRBool GetInOverflow() { return mInOverflowLines != nsnull; }

  



  nsLineList* GetLineList() { return mInOverflowLines; }

  


  line_iterator End();

  



  PRBool Next();
  



  PRBool Prev();

private:
  nsBlockFrame* mFrame;
  line_iterator mLine;
  nsLineList*   mInOverflowLines;

  



  PRBool FindValidLine();
};

#endif 
