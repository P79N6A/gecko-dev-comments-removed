









#ifndef nsViewportFrame_h___
#define nsViewportFrame_h___

#include "mozilla/Attributes.h"
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

  NS_IMETHOD Init(nsIContent*      aContent,
                  nsIFrame*        aParent,
                  nsIFrame*        asPrevInFlow) MOZ_OVERRIDE;

  NS_IMETHOD SetInitialChildList(ChildListID     aListID,
                                 nsFrameList&    aChildList) MOZ_OVERRIDE;

  NS_IMETHOD AppendFrames(ChildListID     aListID,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;

  NS_IMETHOD InsertFrames(ChildListID     aListID,
                          nsIFrame*       aPrevFrame,
                          nsFrameList&    aFrameList) MOZ_OVERRIDE;

  NS_IMETHOD RemoveFrame(ChildListID     aListID,
                         nsIFrame*       aOldFrame) MOZ_OVERRIDE;

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nscoord GetMinWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  virtual nscoord GetPrefWidth(nsRenderingContext *aRenderingContext) MOZ_OVERRIDE;
  NS_IMETHOD Reflow(nsPresContext*          aPresContext,
                    nsHTMLReflowMetrics&     aDesiredSize,
                    const nsHTMLReflowState& aReflowState,
                    nsReflowStatus&          aStatus) MOZ_OVERRIDE;

  




  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

private:
  virtual mozilla::layout::FrameChildListID GetAbsoluteListID() const { return kFixedList; }

protected:
  nsPoint AdjustReflowStateForScrollbars(nsHTMLReflowState* aReflowState) const;
};


#endif 
