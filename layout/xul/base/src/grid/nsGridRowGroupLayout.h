











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
  virtual void CountRowsColumns(nsIFrame* aBox, int32_t& aRowCount, int32_t& aComputedColumnCount);
  virtual void DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState);
  virtual int32_t BuildRows(nsIFrame* aBox, nsGridRow* aRows);
  virtual nsMargin GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal);
  virtual int32_t GetRowCount() { return mRowCount; }
  virtual Type GetType() { return eRowGroup; }

protected:
  nsGridRowGroupLayout();
  virtual ~nsGridRowGroupLayout();

  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState);
  static void AddWidth(nsSize& aSize, nscoord aSize2, bool aIsHorizontal);

private:
  nsGridRow* mRowColumn;
  int32_t mRowCount;
};

#endif

