






































#ifndef nsHTMLCanvasFrame_h___
#define nsHTMLCanvasFrame_h___

#include "nsSplittableFrame.h"
#include "nsString.h"
#include "nsAString.h"
#include "nsPresContext.h"
#include "nsIIOService.h"

nsIFrame* NS_NewHTMLCanvasFrame (nsIPresShell* aPresShell, nsStyleContext* aContext);

class nsHTMLCanvasFrame : public nsSplittableFrame
{
public:
  nsHTMLCanvasFrame(nsStyleContext* aContext) : nsSplittableFrame(aContext) {}

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  void PaintCanvas(nsIRenderingContext& aRenderingContext,
                   const nsRect& aDirtyRect, nsPoint aPt);
                              
  
  nsIntSize GetCanvasSize();

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
  virtual nsSize GetIntrinsicRatio();

  virtual nsSize ComputeSize(nsIRenderingContext *aRenderingContext,
                             nsSize aCBSize, nscoord aAvailableWidth,
                             nsSize aMargin, nsSize aBorder, nsSize aPadding,
                             PRBool aShrinkWrap);

  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);
  
  nsRect GetInnerArea() const;

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  virtual nsIAtom* GetType() const;

  virtual PRBool IsFrameOfType(PRUint32 aFlags) const
  {
    return nsSplittableFrame::IsFrameOfType(aFlags & ~(nsIFrame::eReplaced));
  }

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
  NS_IMETHOD List(FILE* out, PRInt32 aIndent) const;
#endif

protected:
  virtual ~nsHTMLCanvasFrame();

  nscoord GetContinuationOffset(nscoord* aWidth = 0) const;

  nsMargin mBorderPadding;
};

#endif 
