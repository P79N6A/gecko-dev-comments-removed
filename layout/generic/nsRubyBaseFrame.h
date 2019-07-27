







#ifndef nsRubyBaseFrame_h___
#define nsRubyBaseFrame_h___

#include "nsRubyContentFrame.h"

typedef nsRubyContentFrame nsRubyBaseFrameSuper;





nsContainerFrame* NS_NewRubyBaseFrame(nsIPresShell* aPresShell,
                                      nsStyleContext* aContext);

class nsRubyBaseFrame final : public nsRubyBaseFrameSuper
{
public:
  NS_DECL_FRAMEARENA_HELPERS
  NS_DECL_QUERYFRAME_TARGET(nsRubyBaseFrame)
  NS_DECL_QUERYFRAME

  
  virtual nsIAtom* GetType() const override;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override;
#endif

protected:
  friend nsContainerFrame* NS_NewRubyBaseFrame(nsIPresShell* aPresShell,
                                               nsStyleContext* aContext);
  explicit nsRubyBaseFrame(nsStyleContext* aContext)
    : nsRubyBaseFrameSuper(aContext) {}
};

#endif 
