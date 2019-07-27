











#ifndef nsGridRowLeafLayout_h___
#define nsGridRowLeafLayout_h___

#include "mozilla/Attributes.h"
#include "nsGridRowLayout.h"
#include "nsCOMPtr.h"






class nsGridRowLeafLayout MOZ_FINAL : public nsGridRowLayout
{
public:

  friend already_AddRefed<nsBoxLayout> NS_NewGridRowLeafLayout();

  virtual nsSize GetPrefSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMinSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual nsSize GetMaxSize(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState) MOZ_OVERRIDE;
  NS_IMETHOD Layout(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState) MOZ_OVERRIDE;
  virtual void CountRowsColumns(nsIFrame* aBox, int32_t& aRowCount, int32_t& aComputedColumnCount) MOZ_OVERRIDE;
  virtual void DirtyRows(nsIFrame* aBox, nsBoxLayoutState& aState) MOZ_OVERRIDE;
  virtual int32_t BuildRows(nsIFrame* aBox, nsGridRow* aRows) MOZ_OVERRIDE;
  virtual Type GetType() MOZ_OVERRIDE { return eRowLeaf; }

protected:

  virtual void PopulateBoxSizes(nsIFrame* aBox, nsBoxLayoutState& aBoxLayoutState,
                                nsBoxSize*& aBoxSizes, nscoord& aMinSize,
                                nscoord& aMaxSize, int32_t& aFlexes) MOZ_OVERRIDE;
  virtual void ComputeChildSizes(nsIFrame* aBox,
                                 nsBoxLayoutState& aState,
                                 nscoord& aGivenSize,
                                 nsBoxSize* aBoxSizes,
                                 nsComputedBoxSize*& aComputedBoxSizes) MOZ_OVERRIDE;


  nsGridRowLeafLayout();
  virtual ~nsGridRowLeafLayout();
  

private:

}; 

#endif

