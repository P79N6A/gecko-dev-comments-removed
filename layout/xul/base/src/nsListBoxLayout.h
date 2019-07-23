






































#ifndef nsListBoxLayout_h___
#define nsListBoxLayout_h___

#include "nsGridRowGroupLayout.h"
#include "nsIFrame.h"

class nsBoxLayoutState;

class nsListBoxLayout : public nsGridRowGroupLayout
{
public:
  nsListBoxLayout(nsIPresShell* aShell);

  
  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState);
  NS_IMETHOD GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);

protected:
  NS_IMETHOD LayoutInternal(nsIBox* aBox, nsBoxLayoutState& aState);
};

#endif

