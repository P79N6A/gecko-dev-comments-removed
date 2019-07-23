











































#ifndef nsGridRowLeafLayout_h___
#define nsGridRowLeafLayout_h___

#include "nsGridRowLayout.h"
#include "nsCOMPtr.h"






class nsGridRowLeafLayout : public nsGridRowLayout
{
public:

  friend nsresult NS_NewGridRowLeafLayout(nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout);

  NS_IMETHOD GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  virtual void ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState);
  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  virtual void DirtyRows(nsIBox* aBox, nsBoxLayoutState& aState);
  virtual PRInt32 BuildRows(nsIBox* aBox, nsGridRow* aRows);
  virtual Type GetType() { return eRowLeaf; }

protected:

  virtual void PopulateBoxSizes(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsBoxSize*& aBoxSizes, nsComputedBoxSize*& aComputedBoxSizes, nscoord& aMinSize, nscoord& aMaxSize, PRInt32& aFlexes);
  virtual void ComputeChildSizes(nsIBox* aBox, 
                         nsBoxLayoutState& aState, 
                         nscoord& aGivenSize, 
                         nsBoxSize* aBoxSizes, 
                         nsComputedBoxSize*& aComputedBoxSizes);


  nsGridRowLeafLayout(nsIPresShell* aShell);
  virtual ~nsGridRowLeafLayout();
  

private:

}; 

#endif

