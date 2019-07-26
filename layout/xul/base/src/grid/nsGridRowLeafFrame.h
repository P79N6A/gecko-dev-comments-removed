












#ifndef nsGridRowLeafFrame_h___
#define nsGridRowLeafFrame_h___

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"








class nsGridRowLeafFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewGridRowLeafFrame(nsIPresShell* aPresShell,
                                          nsStyleContext* aContext);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
      return MakeFrameName(NS_LITERAL_STRING("nsGridRowLeaf"), aResult);
  }
#endif

  nsGridRowLeafFrame(nsIPresShell* aPresShell,
                     nsStyleContext* aContext,
                     bool aIsRoot,
                     nsBoxLayout* aLayoutManager):
    nsBoxFrame(aPresShell, aContext, aIsRoot, aLayoutManager) {}

  NS_IMETHOD GetBorderAndPadding(nsMargin& aBorderAndPadding);

}; 



#endif

