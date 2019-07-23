






































#ifndef nsListBoxLayout_h___
#define nsListBoxLayout_h___

#include "nsGridRowGroupLayout.h"
#include "nsIFrame.h"

class nsBoxLayoutState;

class nsListBoxLayout : public nsGridRowGroupLayout
{
public:
  nsListBoxLayout();

  
  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState);
  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);

protected:
  NS_IMETHOD LayoutInternal(nsIBox* aBox, nsBoxLayoutState& aState);
};

#endif

