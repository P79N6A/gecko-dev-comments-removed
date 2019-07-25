









































#ifndef nsViewportFrame_h___
#define nsViewportFrame_h___

#include "nsContainerFrame.h"
#include "nsGkAtoms.h"

class nsPresContext;






class ViewportFrame : public nsContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  typedef nsContainerFrame Super;

  ViewportFrame(nsStyleContext* aContext)
    : nsContainerFrame(aContext)
  {}
  virtual ~ViewportFrame() { } 

  virtual void DestroyFrom(nsIFrame* aDestructRoot);

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        asPrevInFlow);

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

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext);
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus);

  




  virtual nsIAtom* GetType() const;
  
  virtual PRBool IsContainingBlock() const;

  virtual void InvalidateInternal(const nsRect& aDamageRect,
                                  nscoord aX, nscoord aY, nsIFrame* aForChild,
                                  PRUint32 aFlags);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

private:
  virtual nsIAtom* GetAbsoluteListName() const { return nsGkAtoms::fixedList; }

protected:
  nsPoint AdjustReflowStateForScrollbars(nsHTMLReflowState* aReflowState) const;
};


#endif 
