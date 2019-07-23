










































#ifndef nsBlockFrame_h___
#define nsBlockFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsHTMLParts.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsLineBox.h"
#include "nsCSSPseudoElements.h"
#include "nsStyleSet.h"

enum LineReflowStatus {
  
  
  LINE_REFLOW_OK,
  
  
  LINE_REFLOW_STOP,
  
  
  LINE_REFLOW_REDO_NO_PULL,
  
  
  LINE_REFLOW_REDO_NEXT_BAND,
  
  
  LINE_REFLOW_TRUNCATED
};

class nsBlockReflowState;
class nsBulletFrame;
class nsLineBox;
class nsFirstLineFrame;
class nsILineIterator;
class nsIntervalSet;




#define NS_BLOCK_LIST_COUNT  (NS_CONTAINER_LIST_COUNT_INCL_OC + 4)




























#define NS_BLOCK_HAS_LINE_CURSOR            0x01000000
#define NS_BLOCK_HAS_OVERFLOW_LINES         0x02000000
#define NS_BLOCK_HAS_OVERFLOW_OUT_OF_FLOWS  0x04000000
#define NS_BLOCK_HAS_OVERFLOW_PLACEHOLDERS  0x08000000




#define NS_BLOCK_HAS_CLEAR_CHILDREN         0x10000000

#define nsBlockFrameSuper nsHTMLContainerFrame

#define NS_BLOCK_FRAME_CID \
 { 0xa6cf90df, 0x15b3, 0x11d2,{0x93, 0x2e, 0x00, 0x80, 0x5f, 0x8a, 0xdd, 0x32}}

extern const nsIID kBlockFrameCID;







 
class nsBlockFrame : public nsBlockFrameSuper
{
public:
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

  
  NS_IMETHOD  QueryInterface(const nsIID& aIID, void** aInstancePtr);

  
  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsIFrame*       aChildList);
  NS_IMETHOD  AppendFrames(nsIAtom*        aListName,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  InsertFrames(nsIAtom*        aListName,
                           nsIFrame*       aPrevFrame,
                           nsIFrame*       aFrameList);
  NS_IMETHOD  RemoveFrame(nsIAtom*        aListName,
                          nsIFrame*       aOldFrame);
  virtual nsIFrame* GetFirstChild(nsIAtom* aListName) const;
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
                                  PRBool aImmediate);
  virtual nsIAtom* GetType() const;
  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsContainerFrame::IsFrameOfType(aFlags &
             ~(nsIFrame::eCanContainOverflowContainers));
  }

#ifdef DEBUG
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
  NS_IMETHOD_(nsFrameState) GetDebugStateBits() const;
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  NS_IMETHOD VerifyTree() const;
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

  virtual void MarkIntrinsicWidthsDirty();
  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  NS_IMETHOD AttributeChanged(PRInt32         aNameSpaceID,
                              nsIAtom*        aAttribute,
                              PRInt32         aModType);

  virtual nsresult StealFrame(nsPresContext* aPresContext,
                              nsIFrame*      aChild,
                              PRBool         aForceNormal);

  virtual void DeleteNextInFlowChild(nsPresContext* aPresContext,
                                     nsIFrame*       aNextInFlow);

  




  PRBool CheckForCollapsedBottomMarginFromClearanceLine();

  



  nsIFrame* GetTopBlockChild(nsPresContext *aPresContext);

  
  
  line_iterator FindLineFor(nsIFrame* aFrame);

  static nsresult GetCurrentLine(nsBlockReflowState *aState, nsLineBox **aOutCurrentLine);

  
  
  nsresult SplitPlaceholder(nsBlockReflowState& aState, nsIFrame* aPlaceholder);

  void UndoSplitPlaceholders(nsBlockReflowState& aState,
                             nsIFrame*           aLastPlaceholder);
  
  PRBool HandleOverflowPlaceholdersForPulledFrame(
    nsBlockReflowState& aState, nsIFrame* aFrame);

  PRBool HandleOverflowPlaceholdersOnPulledLine(
    nsBlockReflowState& aState, nsLineBox* aLine);

  static PRBool BlockIsMarginRoot(nsIFrame* aBlock);
  static PRBool BlockNeedsSpaceManager(nsIFrame* aBlock);
  
protected:
  nsBlockFrame(nsStyleContext* aContext)
    : nsHTMLContainerFrame(aContext)
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
      ProbePseudoStyleFor(mContent,
                          nsCSSPseudoElements::firstLetter, mStyleContext);
  }
#endif

  



  virtual void PaintTextDecorationLine(nsIRenderingContext& aRenderingContext,
                                       nsPoint aPt,
                                       nsLineBox* aLine,
                                       nscolor aColor,
                                       nscoord aOffset,
                                       nscoord aAscent,
                                       nscoord aSize);

  void TryAllLines(nsLineList::iterator* aIterator,
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
                                nsHTMLReflowMetrics&     aMetrics);

  void ComputeCombinedArea(const nsHTMLReflowState& aReflowState,
                           nsHTMLReflowMetrics& aMetrics);

  





  nsresult AddFrames(nsIFrame* aFrameList,
                     nsIFrame* aPrevSibling);

#ifdef IBMBIDI
  


  nsresult ResolveBidi();

  







  PRBool IsVisualFormControl(nsPresContext* aPresContext);
#endif

public:
  









  nsresult DoRemoveFrame(nsIFrame* aDeletedFrame, PRBool aDestroyFrames = PR_TRUE, 
                         PRBool aRemoveOnlyFluidContinuations = PR_TRUE);

  void ReparentFloats(nsIFrame* aFirstFrame,
                      nsBlockFrame* aOldParent, PRBool aFromOverflow,
                      PRBool aReparentSiblings);

protected:

  



  PRBool DrainOverflowLines(nsBlockReflowState& aState);

  



  line_iterator RemoveFloat(nsIFrame* aFloat);

  void CollectFloats(nsIFrame* aFrame, nsFrameList& aList, nsIFrame** aTail,
                     PRBool aFromOverflow, PRBool aCollectFromSiblings);
  
  static void DoRemoveOutOfFlowFrame(nsIFrame* aFrame);

  


  nsresult PrepareResizeReflow(nsBlockReflowState& aState);

  
  nsresult ReflowDirtyLines(nsBlockReflowState& aState);

  
  
  






  nsresult ReflowLine(nsBlockReflowState& aState,
                      line_iterator aLine,
                      PRBool* aKeepReflowGoing);

  
  PRBool PlaceLine(nsBlockReflowState& aState,
                   nsLineLayout&       aLineLayout,
                   line_iterator       aLine,
                   PRBool*             aKeepReflowGoing);

  



  nsresult MarkLineDirty(line_iterator aLine);

  
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
                                PRBool* aKeepReflowGoing,
                                LineReflowStatus* aLineReflowStatus,
                                PRBool aAllowPullUp);

  nsresult ReflowInlineFrame(nsBlockReflowState& aState,
                             nsLineLayout& aLineLayout,
                             line_iterator aLine,
                             nsIFrame* aFrame,
                             LineReflowStatus* aLineReflowStatus);

  
  
  nsresult ReflowFloat(nsBlockReflowState& aState,
                       nsPlaceholderFrame* aPlaceholder,
                       nsMargin&           aFloatMargin,
                       nsReflowStatus&     aReflowStatus);

  
  

  virtual nsresult CreateContinuationFor(nsBlockReflowState& aState,
                                         nsLineBox*          aLine,
                                         nsIFrame*           aFrame,
                                         PRBool&             aMadeNewFrame);

  
  
  void PushTruncatedPlaceholderLine(nsBlockReflowState& aState,
                                    line_iterator       aLine,
                                    nsIFrame*           aLastPlaceholder,
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

  PRBool RenumberListsInBlock(nsPresContext* aPresContext,
                              nsBlockFrame* aContainerFrame,
                              PRInt32* aOrdinal,
                              PRInt32 aDepth);

  PRBool RenumberListsFor(nsPresContext* aPresContext, nsIFrame* aKid, PRInt32* aOrdinal, PRInt32 aDepth);

  static PRBool FrameStartsCounterScope(nsIFrame* aFrame);

  void ReflowBullet(nsBlockReflowState& aState,
                    nsHTMLReflowMetrics& aMetrics);

  

public:
  nsLineList* GetOverflowLines() const;
protected:
  nsLineList* RemoveOverflowLines();
  nsresult SetOverflowLines(nsLineList* aOverflowLines);

  nsFrameList* GetOverflowPlaceholders() const;

  





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
  static PRBool gNoisySpaceManager;
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

#endif 

