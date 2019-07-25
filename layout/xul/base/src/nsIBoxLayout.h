




































#ifndef nsIBoxLayout_h___
#define nsIBoxLayout_h___

#include "nsISupports.h"
#include "nsIFrame.h"

class nsBoxLayout;
class nsBoxLayoutState;
class nsRenderingContext;
struct nsRect;


#define NS_IBOX_LAYOUT_IID \
{ 0x6a529924, 0xc73d, 0x4fae, \
 { 0xaf, 0x7a, 0x0e, 0x80, 0x84, 0xe7, 0x01, 0xd5 } }

class nsIBoxLayout : public nsISupports {

public:

  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IBOX_LAYOUT_IID)

  NS_IMETHOD Layout(nsIBox* aBox, nsBoxLayoutState& aState)=0;

  virtual nsSize GetPrefSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;
  virtual nsSize GetMinSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;
  virtual nsSize GetMaxSize(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;
  virtual nscoord GetAscent(nsIBox* aBox, nsBoxLayoutState& aBoxLayoutState)=0;

  
  
  virtual void ChildrenInserted(nsIBox* aBox, nsBoxLayoutState& aState,
                                nsIBox* aPrevBox,
                                const nsFrameList::Slice& aNewChildren)=0;
  virtual void ChildrenAppended(nsIBox* aBox, nsBoxLayoutState& aState,
                                const nsFrameList::Slice& aNewChildren)=0;
  virtual void ChildrenRemoved(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  virtual void ChildrenSet(nsIBox* aBox, nsBoxLayoutState& aState, nsIBox* aChildList)=0;
  virtual void IntrinsicWidthsDirty(nsIBox* aBox, nsBoxLayoutState& aState)=0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIBoxLayout, NS_IBOX_LAYOUT_IID)

#endif
