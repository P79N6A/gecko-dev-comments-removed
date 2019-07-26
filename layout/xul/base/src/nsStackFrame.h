












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

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
    return MakeFrameName(NS_LITERAL_STRING("Stack"), aResult);
  }
#endif

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists) MOZ_OVERRIDE;

protected:
  nsStackFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);
}; 



#endif

