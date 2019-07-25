




#ifndef nsListBoxLayout_h___
#define nsListBoxLayout_h___

#include "nsGridRowGroupLayout.h"

class nsIFrame;
typedef class nsIFrame nsIFrame;
class nsBoxLayoutState;

class nsListBoxLayout : public nsGridRowGroupLayout
{
public:
  nsListBoxLayout();

  
  NS_IMETHOD Layout(nsIFrame* aBox, nsBoxLayoutState& aState);
  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);

protected:
  NS_IMETHOD LayoutInternal(nsIFrame* aBox, nsBoxLayoutState& aState);
};

#endif

