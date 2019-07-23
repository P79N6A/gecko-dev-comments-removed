




































#ifndef nsIBoxLayout_h___
#define nsIBoxLayout_h___

#include "nsISupports.h"
#include "nsIFrame.h"

class nsPresContext;
class nsBoxLayout;
class nsBoxLayoutState;
class nsIRenderingContext;
struct nsRect;


#define NS_IBOX_LAYOUT_IID \
{ 0xc9bf9fe7, 0xa2f4, 0x4f38, \
    {0xbb, 0xed 0x11, 0xa0, 0x56, 0x33, 0xd6, 0x76} }

class nsIBoxLayout : public nsISupports {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBOX_LAYOUT_IID)

  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState)=0;

  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;
  virtual nscoord GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;

  virtual void ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aPrevBox, nsIBox* aChildList)=0;
  virtual void ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  virtual void ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  virtual void ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  virtual void IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aState)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIBoxLayout, NS_IBOX_LAYOUT_IID)

#endif
