




































#ifndef nsGfxRadioControlFrame_h___
#define nsGfxRadioControlFrame_h___

#include "nsFormControlFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif



class nsGfxRadioControlFrame : public nsFormControlFrame
{
public:
  nsGfxRadioControlFrame(nsStyleContext* aContext);
  ~nsGfxRadioControlFrame();

  NS_DECL_FRAMEARENA_HELPERS

#ifdef ACCESSIBILITY
  virtual already_AddRefed<nsAccessible> CreateAccessible();
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);
};

#endif
