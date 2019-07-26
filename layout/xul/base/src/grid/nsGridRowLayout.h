











#ifndef nsGridRowLayout_h___
#define nsGridRowLayout_h___

#include "nsSprocketLayout.h"
#include "nsIGridPart.h"
class nsGridRowGroupLayout;
class nsGridLayout2;
class nsBoxLayoutState;
class nsGrid;








class nsGridRowLayout : public nsSprocketLayout,
                        public nsIGridPart
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsGridRowGroupLayout* CastToRowGroupLayout() { return nullptr; }
  virtual nsGridLayout2* CastToGridLayout() { return nullptr; }
  virtual nsGrid* GetGrid(nsIBox* aBox, PRInt32* aIndex, nsGridRowLayout* aRequestor=nullptr);
  virtual nsIGridPart* GetParentGridPart(nsIBox* aBox, nsIBox** aParentBox);
  virtual void ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState,
                                nsIBox* aPrevBox,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  virtual void ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  virtual nsMargin GetTotalMargin(nsIBox* aBox, bool aIsHorizontal);

  virtual nsIGridPart* AsGridPart() { return this; }

protected:
  virtual void ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState)=0;

  nsGridRowLayout();
};

#endif

