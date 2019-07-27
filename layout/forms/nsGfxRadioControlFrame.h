




#ifndef nsGfxRadioControlFrame_h___
#define nsGfxRadioControlFrame_h___

#include "mozilla/Attributes.h"
#include "nsFormControlFrame.h"




class nsGfxRadioControlFrame : public nsFormControlFrame
{
public:
  explicit nsGfxRadioControlFrame(nsStyleContext* aContext);
  ~nsGfxRadioControlFrame();

  NS_DECL_FRAMEARENA_HELPERS

#ifdef ACCESSIBILITY
  virtual mozilla::a11y::AccType AccessibleType() MOZ_OVERRIDE;
#endif

  virtual void BuildDisplayList(nsDisplayListBuilder*   aBuilder,
                                const nsRect&           aDirtyRect,
                                const nsDisplayListSet& aLists) MOZ_OVERRIDE;
};

#endif
