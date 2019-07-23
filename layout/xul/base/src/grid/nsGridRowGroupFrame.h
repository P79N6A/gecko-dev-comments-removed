












































#ifndef nsGridRowGroupFrame_h___
#define nsGridRowGroupFrame_h___

#include "nsBoxFrame.h"







class nsGridRowGroupFrame : public nsBoxFrame
{
public:

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("nsGridRowGroup"), aResult);
  }
#endif

  nsGridRowGroupFrame(nsIPresShell* aPresShell,
                      nsStyleContext* aContext,
                      nsIBoxLayout* aLayoutManager):
    nsBoxFrame(aPresShell, aContext, PR_FALSE, aLayoutManager) {}

  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState);

}; 



#endif

