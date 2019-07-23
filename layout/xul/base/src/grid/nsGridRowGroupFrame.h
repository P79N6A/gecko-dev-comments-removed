












































#ifndef nsGridRowGroupFrame_h___
#define nsGridRowGroupFrame_h___

#include "nsBoxFrame.h"







class nsGridRowGroupFrame : public nsBoxFrame
{
public:

  friend nsIFrame* NS_NewGridRowGroupFrame(nsIPresShell* aPresShell,
                                           nsStyleContext* aContext,
                                           nsIBoxLayout* aLayoutManager);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const
  {
      return MakeFrameName(NS_LITERAL_STRING("nsGridRowGroup"), aResult);
  }
#endif

  nsGridRowGroupFrame(nsIPresShell* aPresShell,
                      nsStyleContext* aContext,
                      PRBool aIsRoot,
                      nsIBoxLayout* aLayoutManager):
    nsBoxFrame(aPresShell, aContext, aIsRoot, aLayoutManager) {}

  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState);

}; 



#endif

