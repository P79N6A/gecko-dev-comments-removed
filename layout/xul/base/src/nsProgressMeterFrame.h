


















































#include "nsBoxFrame.h"

class nsPresContext;

class nsProgressMeterFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewProgressMeterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState);

  NS_IMETHOD AttributeChanged(PRInt32 aNameSpaceID,
                              nsIAtom* aAttribute,
                              PRInt32 aModType);

#ifdef NS_DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const;
#endif

protected:
  nsProgressMeterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext) :
    nsBoxFrame(aPresShell, aContext), mNeedsReflowCallback(PR_TRUE) {}
  virtual ~nsProgressMeterFrame();

  PRBool mNeedsReflowCallback;
}; 
