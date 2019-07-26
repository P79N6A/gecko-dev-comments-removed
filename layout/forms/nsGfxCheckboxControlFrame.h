



#ifndef nsGfxCheckboxControlFrame_h___
#define nsGfxCheckboxControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsFormControlFrame.h"

#ifdef ACCESSIBILITY
class nsIAccessible;
#endif

class nsGfxCheckboxControlFrame : public nsFormControlFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  nsGfxCheckboxControlFrame(nsStyleContext* aContext);
  virtual ~nsGfxCheckboxControlFrame();

#ifdef DEBUG
  NS_IMETHOD GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("CheckboxControl"), aResult);
  }
#endif

  NS_IMETHOD BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                              const nsRect&           aDirtyRect,
                              const nsDisplayListSet& aLists) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual already_AddRefed<Accessible> CreateAccessible() MOZ_OVERRIDE;
#endif

protected:

  bool IsChecked();
  bool IsIndeterminate();
};

#endif

