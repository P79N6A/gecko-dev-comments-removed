












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

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const override
  {
      return MakeFrameName(NS_LITERAL_STRING("nsGridRowLeaf"), aResult);
  }
#endif

  nsGridRowLeafFrame(nsStyleContext* aContext,
                     bool aIsRoot,
                     nsBoxLayout* aLayoutManager):
    nsBoxFrame(aContext, aIsRoot, aLayoutManager) {}

  virtual nsresult GetBorderAndPadding(nsMargin& aBorderAndPadding) override;

}; 



#endif

