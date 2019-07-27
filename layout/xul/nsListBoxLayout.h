




#ifndef nsListBoxLayout_h___
#define nsListBoxLayout_h___

#include "mozilla/Attributes.h"
#include "nsGridRowGroupLayout.h"

class nsIFrame;
typedef class nsIFrame nsIFrame;
class nsBoxLayoutState;

class nsListBoxLayout : public nsGridRowGroupLayout
{
public:
  nsListBoxLayout();

  
  NS_IMETHOD Layout(nsIFrame* aBox, nsBoxLayoutState& aState) override;
  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;

protected:
  NS_IMETHOD LayoutInternal(nsIFrame* aBox, nsBoxLayoutState& aState);
};

#endif

