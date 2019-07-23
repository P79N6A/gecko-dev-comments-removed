











































#ifndef nsGridRowGroupLayout_h___
#define nsGridRowGroupLayout_h___

#include "nsGridRowLayout.h"




class nsGridRowGroupLayout : public nsGridRowLayout
{
public:

  friend nsresult NS_NewGridRowGroupLayout(nsIPresShell* aPresShell, nsIBoxLayout** aNewLayout);

  virtual nsGridRowGroupLayout* CastToRowGroupLayout() { return this; }
  NS_IMETHOD GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  NS_IMETHOD GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState, nsSize& aSize);
  virtual void CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  virtual void DirtyRows(nsIBox* aBox, nsBoxLayoutState& aState);
  virtual PRInt32 BuildRows(nsIBox* aBox, nsGridRow* aRows);
  virtual nsMargin GetTotalMargin(nsIBox* aBox, PRBool aIsHorizontal);
  virtual PRInt32 GetRowCount() { return mRowCount; }
  virtual Type GetType() { return eRowGroup; }

protected:
  nsGridRowGroupLayout(nsIPresShell* aShell);
  virtual ~nsGridRowGroupLayout();

  virtual void ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState);
  static void AddWidth(nsSize& aSize, nscoord aSize2, PRBool aIsHorizontal);

private:
  nsGridRow* mRowColumn;
  PRInt32 mRowCount;
};

#endif

