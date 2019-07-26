







#ifndef nsGridContainerFrame_h___
#define nsGridContainerFrame_h___

#include "nsContainerFrame.h"





nsIFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                   nsStyleContext* aContext);

class nsGridContainerFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsGridContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  friend nsIFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                            nsStyleContext* aContext);
  nsGridContainerFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}
};

#endif 
