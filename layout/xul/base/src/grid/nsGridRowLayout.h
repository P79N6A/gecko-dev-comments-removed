











































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
  NS_IMETHOD ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aPrevBox, nsIBox* aChildList);
  NS_IMETHOD ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  NS_IMETHOD ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  NS_IMETHOD ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList);
  virtual nsMargin GetTotalMargin(nsIBox* aBox, PRBool aIsHorizontal);

protected:
  virtual void ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState)=0;

  nsGridRowLayout(nsIPresShell* aShell);
};

#endif

