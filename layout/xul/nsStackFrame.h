












#ifndef nsStackFrame_h___
#define nsStackFrame_h___

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsStackFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewStackFrame(nsIPresShell* aPresShell,
                                    nsStyleContext* aContext);

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
    return MakeFrameName(NS_LITERAL_STRING("Stack"), aResult);
  }
#endif

  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) override;

protected:
  explicit nsStackFrame(nsStyleContext* aContext);
}; 



#endif

