






































#ifndef nsInlineFrame_h___
#define nsInlineFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsAbsoluteContainingBlock.h"
#include "nsLineLayout.h"

class nsAnonymousBlockFrame;

#define nsInlineFrameSuper nsHTMLContainerFrame







#define NS_INLINE_FRAME_HARD_TEXT_OFFSETS            0x00100000






#define NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET     0x00200000

#define NS_INLINE_FRAME_BIDI_VISUAL_IS_LEFT_MOST     0x00400000

#define NS_INLINE_FRAME_BIDI_VISUAL_IS_RIGHT_MOST    0x00800000







class nsInlineFrame : public nsInlineFrameSuper
{
public:
  NS_DECL_QUERYFRAME_TARGET(nsInlineFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewInlineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  
  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

#ifdef ACCESSIBILITY
  NS_IMETHODIMP GetAccessible(nsIAccessible** aAccessible);
#endif

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsInlineFrameSuper::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer | nsIFrame::eLineParticipant));
  }

  virtual PRBool IsEmpty();
  virtual PRBool IsSelfEmpty();

  virtual PRBool PeekOffsetCharacter(PRBool aForward, PRInt32* aOffset);
  
  
  virtual void AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  virtual PRBool CanContinueTextRun() const;

  virtual void PullOverflowsFromPrevInFlow();

  


  PRBool IsLeftMost() const {
    
    
    return (GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET)
             ? !!(GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_IS_LEFT_MOST)
             : (!GetPrevInFlow());
  }

  


  PRBool IsRightMost() const {
    
    
    return (GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET)
             ? !!(GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_IS_RIGHT_MOST)
             : (!GetNextInFlow());
  }

protected:
  
  struct InlineReflowState {
    nsIFrame* mPrevFrame;
    nsInlineFrame* mNextInFlow;
    nsIFrame*      mLineContainer;
    nsLineLayout*  mLineLayout;
    PRPackedBool mSetParentPointer;  
                                     

    InlineReflowState()  {
      mPrevFrame = nsnull;
      mNextInFlow = nsnull;
      mLineContainer = nsnull;
      mLineLayout = nsnull;
      mSetParentPointer = PR_FALSE;
    }
  };

  nsInlineFrame(nsStyleContext* aContext) : nsInlineFrameSuper(aContext) {}

  virtual PRIntn GetSkipSides() const;

  nsresult ReflowFrames(nsPresContext* aPresContext,
                        const nsHTMLReflowState& aReflowState,
                        InlineReflowState& rs,
                        nsHTMLReflowMetrics& aMetrics,
                        nsReflowStatus& aStatus);

  nsresult ReflowInlineFrame(nsPresContext* aPresContext,
                             const nsHTMLReflowState& aReflowState,
                             InlineReflowState& rs,
                             nsIFrame* aFrame,
                             nsReflowStatus& aStatus);

  





  void ReparentFloatsForInlineChild(nsIFrame* aOurBlock, nsIFrame* aFrame,
                                    PRBool aReparentSiblings);

  virtual nsIFrame* PullOneFrame(nsPresContext* aPresContext,
                                 InlineReflowState& rs,
                                 PRBool* aIsComplete);

  virtual void PushFrames(nsPresContext* aPresContext,
                          nsIFrame* aFromChild,
                          nsIFrame* aPrevSibling,
                          InlineReflowState& aState);

};







class nsFirstLineFrame : public nsInlineFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewFirstLineFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  virtual nsIAtom* GetType() const;
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  virtual void PullOverflowsFromPrevInFlow();

protected:
  nsFirstLineFrame(nsStyleContext* aContext) : nsInlineFrame(aContext) {}

  virtual nsIFrame* PullOneFrame(nsPresContext* aPresContext,
                                 InlineReflowState& rs,
                                 PRBool* aIsComplete);
};







class nsPositionedInlineFrame : public nsInlineFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsPositionedInlineFrame(nsStyleContext* aContext)
    : nsInlineFrame(aContext)
    , mAbsoluteContainer(nsGkAtoms::absoluteList)
  {}

  virtual ~nsPositionedInlineFrame() { } 

  virtual void Destroy();

  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);
  NS_IMETHOD AppendFrames(nsIAtom*        aListName,
                          nsFrameList&    aFrameList);
  NS_IMETHOD InsertFrames(nsIAtom*        aListName,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList);
  NS_IMETHOD RemoveFrame(nsIAtom*        aListName,
                         nsIFrame*       aOldFrame);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;

  virtual nsFrameList GetChildList(nsIAtom* aListName) const;

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  
  virtual nsIAtom* GetType() const;

  virtual PRBool NeedsView() { return PR_TRUE; }

protected:
  nsAbsoluteContainingBlock mAbsoluteContainer;
};

#endif 
