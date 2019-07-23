




































#ifndef nsFirstLetterFrame_h__
#define nsFirstLetterFrame_h__



#include "nsHTMLContainerFrame.h"

#define nsFirstLetterFrameSuper nsHTMLContainerFrame

class nsFirstLetterFrame : public nsFirstLetterFrameSuper {
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsFirstLetterFrame(nsStyleContext* aContext) : nsHTMLContainerFrame(aContext) {}

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        aPrevInFlow);
  NS_IMETHOD SetInitialChildList(nsIAtom*        aListName,
                                 nsFrameList&    aChildList);
#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif
  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    if (!GetStyleDisplay()->IsFloating())
      aFlags = aFlags & ~(nsIFrame::eLineParticipant);
    return nsFirstLetterFrameSuper::IsFrameOfType(aFlags &
      ~(nsIFrame::eBidiInlineContainer));
  }

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual void AddInlineMinWidth(nsIRenderingContext *aRenderingContext,
                                 InlineMinWidthData *aData);
  virtual void AddInlinePrefWidth(nsIRenderingContext *aRenderingContext,
                                  InlinePrefWidthData *aData);
  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  virtual PRBool CanContinueTextRun() const;


  NS_IMETHOD GetChildFrameContainingOffset(PRInt32 inContentOffset,
                                           PRBool inHint,
                                           PRInt32* outFrameContentOffset,
                                           nsIFrame **outChildFrame);

  nscoord GetFirstLetterBaseline() const { return mBaseline; }

protected:
  nscoord mBaseline;

  virtual PRIntn GetSkipSides() const;

  void DrainOverflowFrames(nsPresContext* aPresContext);
};

#endif 
