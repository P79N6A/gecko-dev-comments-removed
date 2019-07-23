











































#ifndef nsGridRowLeafLayout_h___
#define nsGridRowLeafLayout_h___

#include "nsGridRowLayout.h"
#include "nsCOMPtr.h"






class nsGridRowLeafLayout : public nsGridRowLayout
{
public:

  friend already_AddRefed<nsIBoxLayout> NS_NewGridRowLeafLayout();

  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState);
  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  virtual void DirtyRows(nsIBox* aBox, nsBoxLayoutState& aState);
  virtual PRInt32 BuildRows(nsIBox* aBox, nsGridRow* aRows);
  virtual Type GetType() { return eRowLeaf; }

protected:

  virtual void PopulateBoxSizes(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsBoxSize*& aBoxSizes, nscoord& aMinSize, nscoord& aMaxSize, PRInt32& aFlexes);
  virtual void ComputeChildSizes(nsIBox* aBox, 
                         nsBoxLayoutState& aState, 
                         nscoord& aGivenSize, 
                         nsBoxSize* aBoxSizes, 
                         nsComputedBoxSize*& aComputedBoxSizes);


  nsGridRowLeafLayout();
  virtual ~nsGridRowLeafLayout();
  

private:

}; 

#endif

