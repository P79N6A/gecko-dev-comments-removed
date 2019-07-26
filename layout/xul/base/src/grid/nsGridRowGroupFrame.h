












#ifndef nsGridRowGroupFrame_h___
#define nsGridRowGroupFrame_h___

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"







class nsGridRowGroupFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE
  {
      return MakeFrameName(NS_LITERAL_STRING("nsGridRowGroup"), aResult);
  }
#endif

  nsGridRowGroupFrame(nsIPresShell* aPresShell,
                      nsStyleContext* aContext,
                      nsBoxLayout* aLayoutManager):
    nsBoxFrame(aPresShell, aContext, false, aLayoutManager) {}

  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

}; 



#endif

