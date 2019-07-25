











#ifndef nsGridRowLeafLayout_h___
#define nsGridRowLeafLayout_h___

#include "nsGridRowLayout.h"
#include "nsCOMPtr.h"






class nsGridRowLeafLayout : public nsGridRowLayout
{
public:

  friend already_AddRefed<nsBoxLayout> NS_NewGridRowLeafLayout();

  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState);
  NS_IMETHOD Layout(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void CountRowsColumns(nsIFrame* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  virtual void DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState);
  virtual PRInt32 BuildRows(nsIFrame* aBox, nsGridRow* aRows);
  virtual Type GetType() { return eRowLeaf; }

protected:

  virtual void PopulateBoxSizes(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState,
                                nsBoxSize*& aBoxSizes, nscoord& aMinSize,
                                nscoord& aMaxSize, PRInt32& aFlexes);
  virtual void ComputeChildSizes(nsIFrame* aBox,
                                 nsBoxLayoutState& aState,
                                 nscoord& aGivenSize,
                                 nsBoxSize* aBoxSizes,
                                 nsComputedBoxSize*& aComputedBoxSizes);


  nsGridRowLeafLayout();
  virtual ~nsGridRowLeafLayout();
  

private:

}; 

#endif

