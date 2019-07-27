











#ifndef nsGridRowGroupLayout_h___
#define nsGridRowGroupLayout_h___

#include "mozilla/Attributes.h"
#include "nsGridRowLayout.h"




class nsGridRowGroupLayout : public nsGridRowLayout
{
public:

  friend already_AddRefed<nsBoxLayout> NS_NewGridRowGroupLayout();

  virtual nsGridRowGroupLayout* CastToRowGroupLayout() override { return this; }
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual void CountRowsColumns(nsIFrame* aBox, int32_t& aRowCount, int32_t& aComputedColumnCount) override;
  virtual void DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState) override;
  virtual int32_t BuildRows(nsIFrame* aBox, nsGridRow* aRows) override;
  virtual nsMargin GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal) override;
  virtual int32_t GetRowCount() override { return mRowCount; }
  virtual Type GetType() override { return eRowGroup; }

protected:
  nsGridRowGroupLayout();
  virtual ~nsGridRowGroupLayout();

  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState) override;
  static void AddWidth(nsSize& aSize, nscoord aSize2, bool aIsHorizontal);

private:
  int32_t mRowCount;
};

#endif

