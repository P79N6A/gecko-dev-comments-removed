











#ifndef nsGridRowLayout_h___
#define nsGridRowLayout_h___

#include "mozilla/Attributes.h"
#include "nsSprocketLayout.h"
#include "nsIGridPart.h"
class nsGridRowGroupLayout;
class nsGridLayout2;
class nsBoxLayoutState;
class nsGrid;








class nsGridRowLayout : public nsSprocketLayout,
                        public nsIGridPart
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  virtual nsGridRowGroupLayout* CastToRowGroupLayout() override { return nullptr; }
  virtual nsGridLayout2* CastToGridLayout() override { return nullptr; }
  virtual nsGrid* GetGrid(nsIFrame* aBox, int32_t* aIndex, nsGridRowLayout* aRequestor=nullptr) override;
  virtual nsIGridPart* GetParentGridPart(nsIFrame* aBox, nsIFrame** aParentBox) override;
  virtual void ChildrenInserted(nsIFrame* aBox, nsBoxLayoutState& aState,
                                nsIFrame* aPrevBox,
                                const nsFrameList::Slice& aNewChildren) override;
  virtual void ChildrenAppended(nsIFrame* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren) override;
  virtual void ChildrenRemoved(nsIFrame* aBox, nsBoxLayoutState& aState, nsIFrame* aChildList) override;
  virtual void ChildrenSet(nsIFrame* aBox, nsBoxLayoutState& aState, nsIFrame* aChildList) override;
  virtual nsMargin GetTotalMargin(nsIFrame* aBox, bool aIsHorizontal) override;

  virtual nsIGridPart* AsGridPart() override { return this; }

protected:
  virtual void ChildAddedOrRemoved(nsIFrame* aBox, nsBoxLayoutState& aState)=0;

  nsGridRowLayout();
  virtual ~nsGridRowLayout();
};

#endif

