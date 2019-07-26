








#ifndef nsGridContainerFrame_h___
#define nsGridContainerFrame_h___

#include "nsContainerFrame.h"

nsIFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                   nsStyleContext* aContext);

typedef nsContainerFrame nsGridContainerFrameSuper;

class nsGridContainerFrame : public nsGridContainerFrameSuper {

  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsGridContainerFrame)
  NS_DECL_QUERYFRAME

  
  friend nsIFrame* NS_NewGridContainerFrame(nsIPresShell* aPresShell,
                                            nsStyleContext* aContext);

public:
  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  
  nsGridContainerFrame(nsStyleContext* aContext) : nsGridContainerFrameSuper(aContext) {}
  virtual ~nsGridContainerFrame();
};

#endif 
