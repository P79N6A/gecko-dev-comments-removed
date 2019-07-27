


















#include "mozilla/Attributes.h"
#include "nsBoxFrame.h"

class nsProgressMeterFrame : public nsBoxFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewProgressMeterFrame(nsIPresShell* aPresShell, nsStyleContext* aContext);

  NS_IMETHOD DoLayout(nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;

  virtual nsresult AttributeChanged(int32_t aNameSpaceID,
                                    nsIAtom* aAttribute,
                                    int32_t aModType) MOZ_OVERRIDE;

#ifdef DEBUG_FRAME_DUMP
  virtual nsresult GetFrameName(nsAString& aResult) const MOZ_OVERRIDE;
#endif

protected:
  explicit nsProgressMeterFrame(nsStyleContext* aContext) :
    nsBoxFrame(aContext), mNeedsReflowCallback(true) {}
  virtual ~nsProgressMeterFrame();

  bool mNeedsReflowCallback;
}; 
