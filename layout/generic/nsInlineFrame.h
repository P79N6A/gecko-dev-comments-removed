






































#ifndef nsInlineFrame_h___
#define nsInlineFrame_h___

#include "nsHTMLContainerFrame.h"
#include "nsLineLayout.h"

class nsAnonymousBlockFrame;

#define nsInlineFrameSuper nsHTMLContainerFrame






#define NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET     NS_FRAME_STATE_BIT(21)

#define NS_INLINE_FRAME_BIDI_VISUAL_IS_LEFT_MOST     NS_FRAME_STATE_BIT(22)

#define NS_INLINE_FRAME_BIDI_VISUAL_IS_RIGHT_MOST    NS_FRAME_STATE_BIT(23)







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
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  virtual nsIAtom* GetType() const;

  virtual bool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsInlineFrameSuper::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer | nsIFrame::eLineParticipant));
  }

  virtual bool IsEmpty();
  virtual bool IsSelfEmpty();

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  virtual bool PeekOffsetCharacter(bool aForward, PRInt32* aOffset,
                                     bool aRespectClusters = true);
  
  
  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             bool aShrinkWrap);
  virtual nsRect ComputeTightBounds(gfxContext* aContext) const;
  NS_IMETHOD Reflow(nsPresContext* aPresContext,
                    nsHTMLReflowMetrics& aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus& aStatus);

  virtual bool CanContinueTextRun() const;

  virtual void PullOverflowsFromPrevInFlow();
  virtual nscoord GetBaseline() const;

  


  bool IsLeftMost() const {
    
    
    return (GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_STATE_IS_SET)
             ? !!(GetStateBits() & NS_INLINE_FRAME_BIDI_VISUAL_IS_LEFT_MOST)
             : (!GetPrevInFlow());
  }

  


  bool IsRightMost() const {
    
    
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
    bool mSetParentPointer;  
                                     

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
                                    bool aReparentSiblings);

  virtual nsIFrame* PullOneFrame(nsPresContext* aPresContext,
                                 InlineReflowState& rs,
                                 bool* aIsComplete);

  virtual void PushFrames(nsPresContext* aPresContext,
                          nsIFrame* aFromChild,
                          nsIFrame* aPrevSibling,
                          InlineReflowState& aState);

  nscoord mBaseline;
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
                                 bool* aIsComplete);
};

#endif 
