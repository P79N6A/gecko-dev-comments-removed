




































#ifndef nsGfxRadioControlFrame_h___
#define nsGfxRadioControlFrame_h___

#include "nsFormControlFrame.h"
#include "nsIRadioControlFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif



class nsGfxRadioControlFrame : public nsFormControlFrame,
                               public nsIRadioControlFrame

{
public:
  nsGfxRadioControlFrame(nsStyleContext* aContext);
  ~nsGfxRadioControlFrame();

  NS_DECL_QUERYFRAME
  NS_DECL_FRAMEARENA_HELPERS

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif
  NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked);

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
};

#endif
