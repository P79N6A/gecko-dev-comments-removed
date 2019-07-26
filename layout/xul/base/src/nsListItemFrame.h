




#include "mozilla/Attributes.h"
#include "nsGridRowLeafFrame.h"

nsIFrame* NS_NewListItemFrame(nsIPresShell* aPresShell,
                              nsStyleContext *aContext);

class nsListItemFrame : public nsGridRowLeafFrame
{
public:
  NS_DECL_FRAMEARENA_HELPERS

  friend nsIFrame* NS_NewListItemFrame(nsIPresShell* aPresShell,
                                       nsStyleContext *aContext);

  
  
  virtual void BuildDisplayListForChildren(nsDisplayListBuilder*   aBuilder,
                                           const nsRect&           aDirtyRect,
                                           const nsDisplayListSet& aLists) MOZ_OVERRIDE;

  virtual nsSize GetPrefSize(nsBoxLayoutState& aState) MOZ_OVERRIDE;

protected:
  nsListItemFrame(nsIPresShell* aPresShell,
                  nsStyleContext *aContext,
                  bool aIsRoot = false,
                  nsBoxLayout* aLayoutManager = nullptr);
  virtual ~nsListItemFrame();

}; 
