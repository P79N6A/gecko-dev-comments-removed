


















#include "nsBoxFrame.h"

class nsProgressMeterFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewProgressMeterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  NS_IMETHOD AttributeChanged(int32_t aNameSpaceID,
                              nsIAtom* aAttribute,
                              int32_t aModType);

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

protected:
  nsProgressMeterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext) :
    nsBoxFrame(aPresShell, aContext), mNeedsReflowCallback(true) {}
  virtual ~nsProgressMeterFrame();

  bool mNeedsReflowCallback;
}; 
