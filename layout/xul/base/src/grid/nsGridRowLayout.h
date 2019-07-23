











































#ifndef nsGridRowLayout_h___
#define nsGridRowLayout_h___

#include "nsSprocketLayout.h"
#include "nsIGridPart.h"
class nsGridRowGroupLayout;
class nsGridLayout2;
class nsBoxLayoutState;
class nsIPresShell;
class nsGrid;








class nsGridRowLayout : public nsSprocketLayout,
                        public nsIGridPart
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsGridRowGroupLayout* CastToRowGroupLayout() { return nsnull; }
  virtual nsGridLayout2* CastToGridLayout() { return nsnull; }
  virtual nsGrid* GetGrid(nsIBox* aBox, PRInt32* aIndex, nsGridRowLayout* aRequestor=nsnull);
  virtual void GetParentGridPart(nsIBox* aBox, nsIBox** aParentBox, nsIGridPart** aParentGridRow);
  virtual void ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState,
                                nsIBox* aPrevBox,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren);
  virtual void ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  virtual void ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  virtual nsMargin GetTotalMargin(nsIBox* aBox, PRBool aIsHorizontal);

protected:
  virtual void ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState)=0;

  nsGridRowLayout();
};

#endif

