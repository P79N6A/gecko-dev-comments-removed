









































#ifndef nsViewportFrame_h___
#define nsViewportFrame_h___

#include "nsContainerFrame.h"
#include "nsGkAtoms.h"
#include "nsPresContext.h"
#include "nsIPresShell.h"
#include "nsAbsoluteContainingBlock.h"







class ViewportFrame : public nsContainerFrame {
public:
  NS_DECL_FRAMEARENA_HELPERS

  typedef nsContainerFrame Super;

  ViewportFrame(nsStyleContext* aContext)
    : nsContainerFrame(aContext)
    , mFixedContainer(nsGkAtoms::fixedList)
  {}
  virtual ~ViewportFrame() { } 

  virtual void Destroy();

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

  virtual nsIAtom* GetAdditionalChildListName(PRInt32 aIndex) const;

  virtual nsFrameList GetChildList(nsIAtom* aListName) const;

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

  virtual nscoord GetMinWidth(nsIRenderingContext *aRenderingContext);
  virtual nscoord GetPrefWidth(nsIRenderingContext *aRenderingContext);
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

protected:
  nsPoint AdjustReflowStateForScrollbars(nsHTMLReflowState* aReflowState) const;

protected:
  
  
  nsAbsoluteContainingBlock mFixedContainer;
};


#endif 
