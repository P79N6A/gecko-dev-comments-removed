




#ifndef nsFirstLetterFrame_h__
#define nsFirstLetterFrame_h__



#include "mozilla/Attributes.h"
#include "nsContainerFrame.h"

class nsFirstLetterFrame : public nsContainerFrame {
public:
  NS_DECL_QUERYFRAME_TARGET(nsFirstLetterFrame)
  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

  nsFirstLetterFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow) MOZ_OVERRIDE;
  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;
#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

  bool IsFloating() const { return GetStateBits() & NS_FRAME_OUT_OF_FLOW; }

  virtual bool IsFrameOfType(uint32_t aFlags) const
  {
    if (!IsFloating())
      aFlags = aFlags & ~(nsIFrame::eLineParticipant);
    return nsContainerFrame::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer));
  }

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual void AddInlineMinWidth(nsRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData) MOZ_OVERRIDE;
  virtual void AddInlinePrefWidth(nsRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData) MOZ_OVERRIDE;
  virtual nsSize ComputeSize(nsRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             uint32_t aFlags) MOZ_OVERRIDE;
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  virtual bool CanContinueTextRun() const MOZ_OVERRIDE;
  virtual nscoord GetBaseline() const MOZ_OVERRIDE;


  NS_IMETHOD GetChildFrameContainingOffset(int32_t inContentOffset,
                                           bool inHint,
                                           int32_t* outFrameContentOffset,
                                           nsIFrame **outChildFrame) MOZ_OVERRIDE;

  nscoord GetFirstLetterBaseline() const { return mBaseline; }

  
  
  
  
  nsresult CreateContinuationForFloatingParent(nsPresContext* aPresContext,
                                               nsIFrame* aChild,
                                               nsIFrame** aContinuation,
                                               bool aIsFluid);

protected:
  nscoord mBaseline;

  void DrainOverflowFrames(nsPresContext* aPresContext);
};

#endif 
