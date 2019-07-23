



































#ifndef nsGfxCheckboxControlFrame_h___
#define nsGfxCheckboxControlFrame_h___

#include "nsFormControlFrame.h"
#include "nsICheckboxControlFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif

class nsGfxCheckboxControlFrame : public nsFormControlFrame,
                                  public nsICheckboxControlFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsGfxCheckboxControlFrame(nsStyleContext* aContext);
  virtual ~nsGfxCheckboxControlFrame();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const {
    return MakeFrameName(NS_LITERAL_STRING("CheckboxControl"), aResult);
  }
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists);

#ifdef ACCESSIBILITY
  NS_IMETHOD GetAccessible(nsIAccessible** aAccessible);
#endif

  
  NS_IMETHOD OnChecked(nsPresContext* aPresContext, PRBool aChecked);

  NS_DECL_QUERYFRAME

protected:

  PRBool IsChecked();
  PRBool IsIndeterminate();
};

#endif

