




#ifndef nsFirstLetterFrame_h__
#define nsFirstLetterFrame_h__



#include "nsContainerFrame.h"

class nsFirstLetterFrame : public nsContainerFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsFirstLetterFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  nsFirstLetterFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList);
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  virtual nsIAtom* GetType() const;

  bool IsFloating() const { return GetStateBits() & NS_FRAME_OUT_OF_FLOW; }

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    if (!IsFloating())
      aFlags = aFlags & ~(nsIFrame::eLineParticipant);
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer));
  }

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual bool CanContinueTextRun() const;
  virtual nscoord GetBaseline() const;


  NS_IMETHOD GetChildFrameContainingOffset(int32_t inContentOffset,
                                           bool inHint,
                                           int32_t* outFrameContentOffset,
                                           nsIFrame **outChildFrame);

  nscoord GetFirstLetterBaseline() const { return mBaseline; }

  
  
  
  
  nsresult CreateContinuationForFloatingParent(nsPresContext* aPresContext,
                                               nsIFrame* aChild,
                                               nsIFrame** aContinuation,
                                               bool aIsFluid);

protected:
  nscoord mBaseline;

  virtual int GetSkipSides() const;

  void DrainOverflowFrames(nsPresContext* aPresContext);
};

#endif 
