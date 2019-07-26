











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
  virtual nsGrid* GetGrid(nsIFrame* aBox, int32_t* aIndex, nsGridRowLayout* aRequestor=nullptr);
  virtual nsIGridPart* GetParentGridPart(nsIFrame* aBox, nsIFrame** aParentBox);
  virtual void ChildrenInserted(nsIFrame* aBox, nsBoxLayoutState& aState,
                                nsIFrame* aPrevBox,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenAppended(nsIFrame* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenRemoved(nsIFrame* aBox, nsBoxLayoutState& aState, nsIFrame* aChildList);
  virtual void ChildrenSet(nsIFrame* aBox, nsBoxLayoutState& aState, nsIFrame* aChildList);
  virtual nsMargin GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal);

  virtual nsIGridPart* AsGridPart() { return this; }

protected:
  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState)=0;

  nsGridRowLayout();
};

#endif

