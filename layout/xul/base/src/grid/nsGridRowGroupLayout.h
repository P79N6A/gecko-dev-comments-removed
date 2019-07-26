











#ifndef nsGridRowGroupLayout_h___
#define nsGridRowGroupLayout_h___

#include "nsGridRowLayout.h"




class nsGridRowGroupLayout : public nsGridRowLayout
{
public:

  friend already_AddRefed<nsBoxLayout> NS_NewGridRowGroupLayout();

  virtual nsGridRowGroupLayout* CastToRowGroupLayout() { return this; }
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState);
  virtual void CountRowsColumns(nsIFrame* aBox, PRInt32& aRowCount, PRInt32& aComputedColumnCount);
  virtual void DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState);
  virtual PRInt32 BuildRows(nsIFrame* aBox, nsGridRow* aRows);
  virtual nsMargin GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal);
  virtual PRInt32 GetRowCount() { return mRowCount; }
  virtual Type GetType() { return eRowGroup; }

protected:
  nsGridRowGroupLayout();
  virtual ~nsGridRowGroupLayout();

  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState);
  static void AddWidth(nsSize& aSize, nscoord aSize2, bool aIsHorizontal);

private:
  nsGridRow* mRowColumn;
  PRInt32 mRowCount;
};

#endif

