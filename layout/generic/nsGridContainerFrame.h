







#ifndef nsGridContainerFrame_h___
#define nsGridContainerFrame_h___

#include "nsContainerFrame.h"





nsContainerFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);

class nsGridContainerFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsGridContainerFrame)
  NS_DECL_QUERYFRAME

  
  void Reflow(nsPresContext*           aPresContext,
              nsHTMLReflowMetrics&     aDesiredSize,
              const nsHTMLReflowState& aReflowState,
              nsReflowStatus&          aStatus) MOZ_OVERRIDE;
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  friend nsContainerFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                                    nsStyleContext* aContext);
  explicit nsGridContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}

#ifdef DEBUG
  void SanityCheckAnonymousGridItems() const;
#endif 
};

#endif 
