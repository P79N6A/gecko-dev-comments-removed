












































#ifndef nsStackFrame_h___
#define nsStackFrame_h___

#include "nsBoxFrame.h"

class nsStackFrame : public nsBoxFrame
{
public:

  friend nsIFrame* NS_NewStackFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, nsIBoxLayout* aLayout = nsnull);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
    return MakeFrameName(NS_LITERAL_STRING("Stack"), aResult);
  }
#endif

  NS_IMETHOD BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                         const nsRect&           aDirtyRect,
                                         const nsDisplayListSet& aLists);

protected:
  nsStackFrame(nsIPresShell* aPresShell, nsStyleContext* aContext, nsIBoxLayout* aLayout = nsnull);
}; 



#endif

