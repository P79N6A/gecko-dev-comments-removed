




































#ifndef nsIBoxLayout_h___
#define nsIBoxLayout_h___

#include "nsISupports.h"
#include "nsIFrame.h"

class nsPresContext;
class nsBoxLayout;
class nsBoxLayoutState;
class nsIRenderingContext;
struct nsRect;


#define NS_IBOX_LAYOUT_IID \
{ 0xd0f7955e, 0x7cae, 0x4213, \
  { 0x9e, 0x08, 0xad, 0x1d, 0x51, 0x2f, 0x39, 0x6f } }

class nsIBoxLayout : public nsISupports {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBOX_LAYOUT_IID)

  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState)=0;

  NS_IMETHOD GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)=0;
  NS_IMETHOD GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)=0;
  NS_IMETHOD GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize)=0;
  NS_IMETHOD GetFlex(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nscoord& aFlex)=0;
  NS_IMETHOD GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent)=0;
  NS_IMETHOD IsCollapsed(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, PRBool& aCollapsed)=0;

  NS_IMETHOD ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aPrevBox, nsIBox* aChildList)=0;
  NS_IMETHOD ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  NS_IMETHOD ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  NS_IMETHOD ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  NS_IMETHOD IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aState)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIBoxLayout, NS_IBOX_LAYOUT_IID)

#endif
