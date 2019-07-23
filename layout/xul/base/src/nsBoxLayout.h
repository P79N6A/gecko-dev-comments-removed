




































#ifndef nsBoxLayout_h___
#define nsBoxLayout_h___

#include "nsIBoxLayout.h"

class nsBoxLayout : public nsIBoxLayout {

public:

  nsBoxLayout();
  virtual ~nsBoxLayout() {}

  NS_DECL_ISUPPORTS

  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState);

  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nscoord GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState,
                                nsIBox* aPrevBox,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  virtual void ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  virtual void IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aState);

  virtual void GetParentLayout(nsIBox* aBox, nsIBoxLayout** aParent);
  virtual void AddBorderAndPadding(nsIBox* aBox, nsSize& aSize);
  virtual void AddMargin(nsIBox* aChild, nsSize& aSize);
  virtual void AddMargin(nsSize& aSize, const nsMargin& aMargin);

  static void AddLargestSize(nsSize& aSize, const nsSize& aToAdd);
  static void AddSmallestSize(nsSize& aSize, const nsSize& aToAdd);
};

#endif

