







#ifndef nsRubyBaseFrame_h___
#define nsRubyBaseFrame_h___

#include "nsInlineFrame.h"

typedef nsInlineFrame nsRubyBaseFrameSuper;





nsContainerFrame* NS_NewRubyBaseFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

class nsRubyBaseFrame MOZ_FINAL : public nsRubyBaseFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyBaseFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const MOZ_OVERRIDE;
  virtual bool IsFrameOfType(uint32_t aFlags) const MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  friend nsContainerFrame* NS_NewRubyBaseFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);
  explicit nsRubyBaseFrame(nsStyleContext* aContext)
    : nsRubyBaseFrameSuper(aContext) {}
};

#endif 
