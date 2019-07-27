



#ifndef nsGfxCheckboxControlFrame_h___
#define nsGfxCheckboxControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsFormControlFrame.h"

class nsGfxCheckboxControlFrame : public nsFormControlFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  explicit nsGfxCheckboxControlFrame(nsStyleContext* aContext);
  virtual ~nsGfxCheckboxControlFrame();

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE {
    return MakeFrameName(NS_LITERAL_STRING("CheckboxControl"), aResult);
  }
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

protected:

  bool IsChecked();
  bool IsIndeterminate();
};

#endif

