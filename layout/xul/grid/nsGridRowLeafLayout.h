











#ifndef nsGridRowLeafLayout_h___
#define nsGridRowLeafLayout_h___

#include "mozilla/Attributes.h"
#include "nsGridRowLayout.h"
#include "nsCOMPtr.h"






class nsGridRowLeafLayout final : public nsGridRowLayout
{
public:

  friend already_AddRefed<nsBoxLayout> NS_NewGridRowLeafLayout();

  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState) override;
  NS_IMETHOD Layout(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) override;
  virtual void CountRowsColumns(nsIFrame* aBox, int32_t& aRowCount, int32_t& aComputedColumnCount) override;
  virtual void DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState) override;
  virtual int32_t BuildRows(nsIFrame* aBox, nsGridRow* aRows) override;
  virtual Type GetType() override { return eRowLeaf; }

protected:

  virtual void PopulateBoxSizes(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState,
                                nsBoxSize*& aBoxSizes, nscoord& aMinSize,
                                nscoord& aMaxSize, int32_t& aFlexes) override;
  virtual void ComputeChildSizes(nsIFrame* aBox,
                                 nsBoxLayoutState& aState,
                                 nscoord& aGivenSize,
                                 nsBoxSize* aBoxSizes,
                                 nsComputedBoxSize*& aComputedBoxSizes) override;


  nsGridRowLeafLayout();
  virtual ~nsGridRowLeafLayout();
  

private:

}; 

#endif

