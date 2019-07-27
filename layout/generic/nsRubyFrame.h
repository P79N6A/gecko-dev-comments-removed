







#ifndef nsRubyFrame_h___
#define nsRubyFrame_h___

#include "nsContainerFrame.h"





nsContainerFrame* NS_NewRubyFrame(nsIPresShell* aPresShell,
                                  nsStyleContext* aContext);

class nsRubyFrame MOZ_FINAL : public nsContainerFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  friend nsContainerFrame* NS_NewRubyFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext);
  nsRubyFrame(nsStyleContext* aContext) : nsContainerFrame(aContext) {}
};

#endif 
