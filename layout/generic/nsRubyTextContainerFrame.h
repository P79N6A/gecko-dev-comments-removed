







#ifndef nsRubyTextContainerFrame_h___
#define nsRubyTextContainerFrame_h___

#include "nsBlockFrame.h"





nsContainerFrame* NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);




class nsRubyTextContainerFrame MOZ_FINAL : public nsBlockFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyTextContainerFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  friend nsContainerFrame*
    NS_NewRubyTextContainerFrame(nsIPresShell* aPresShell,
                                 nsStyleContext* aContext);
  nsRubyTextContainerFrame(nsStyleContext* aContext) : nsBlockFrame(aContext) {}
};

#endif 
