




































#ifndef nsBoxLayout_h___
#define nsBoxLayout_h___

#include "nsIBoxLayout.h"

class nsBoxLayout : public nsIBoxLayout {

public:

  nsBoxLayout();
  virtual ~nsBoxLayout() {};

  NS_DECL_ISUPPORTS

  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState);

  NS_IMETHOD GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetFlex(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nscoord& aFlex);
  NS_IMETHOD GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nscoord& aAscent);
  NS_IMETHOD IsCollapsed(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, PRBool& aCollapsed);
  NS_IMETHOD ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aPrevBox, nsIBox* aChildList);
  NS_IMETHOD ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  NS_IMETHOD ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  NS_IMETHOD ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  NS_IMETHOD IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aState);

  virtual void GetParentLayout(nsIBox* aBox, nsIBoxLayout** aParent);
  virtual void AddBorderAndPadding(nsIBox* aBox, nsSize& aSize);
  virtual void AddMargin(nsIBox* aChild, nsSize& aSize);
  virtual void AddMargin(nsSize& aSize, const nsMargin& aMargin);

  static void AddLargestSize(nsSize& aSize, const nsSize& aToAdd);
  static void AddSmallestSize(nsSize& aSize, const nsSize& aToAdd);
};

#endif

