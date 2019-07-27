







#ifndef nsRubyTextFrame_h___
#define nsRubyTextFrame_h___

#include "nsContainerFrame.h"





nsContainerFrame* NS_NewRubyTextFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

class nsRubyTextFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyTextFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  friend nsContainerFrame* NS_NewRubyTextFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);
  nsRubyTextFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}
};

#endif 
