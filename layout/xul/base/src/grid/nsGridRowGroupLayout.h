











































#ifndef nsGridRowGroupLayout_h___
#define nsGridRowGroupLayout_h___

#include "nsGridRowLayout.h"




class nsGridRowGroupLayout : public nsGridRowLayout
{
public:

  friend already_AddRefed<nsBoxLayout> NS_NewGridRowGroupLayout();

  virtual nsGridRowGroupLayout* CastToRowGroupLayout() { return this; }
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void CountRowsColumns(nsIBox* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  virtual void DirtyRows(nsIBox* aBox, nsBoxLayoutState& aState);
  virtual PRInt32 BuildRows(nsIBox* aBox, nsGridRow* aRows);
  virtual nsMargin GetTotalMargin(nsIBox* aBox, bool aIsHorizontal);
  virtual PRInt32 GetRowCount() { return mRowCount; }
  virtual Type GetType() { return eRowGroup; }

protected:
  nsGridRowGroupLayout();
  virtual ~nsGridRowGroupLayout();

  virtual void ChildAddedOrRemoved(nsIBox* aBox, nsBoxLayoutState& aState);
  static void AddWidth(nsSize& aSize, nscoord aSize2, bool aIsHorizontal);

private:
  nsGridRow* mRowColumn;
  PRInt32 mRowCount;
};

#endif

