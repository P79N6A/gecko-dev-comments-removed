












#ifndef nsGridRowGroupFrame_h___
#define nsGridRowGroupFrame_h___

#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"







class nsGridRowGroupFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
      return MakeFrameName(NS_LITERAL_STRING("nsGridRowGroup"), aResult);
  }
#endif

  nsGridRowGroupFrame(nsStyleContext* aContext,
                      nsBoxLayout* aLayoutManager):
    nsBoxFrame(aContext, false, aLayoutManager) {}

  virtual nscoord GetFlex(nsBoxLayoutState& aBoxLayoutState) override;

}; 



#endif

