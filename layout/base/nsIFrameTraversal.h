



































#ifndef NSIFRAMETRAVERSAL_H
#define NSIFRAMETRAVERSAL_H

#include "nsISupports.h"
#include "nsIEnumerator.h"
#include "nsIFrame.h"

enum nsIteratorType {
  eLeaf,
  ePreOrder,
  ePostOrder
};


#define NS_IFRAMETRAVERSAL_IID \
{ 0x9d469828, 0x9bf2, 0x4151, { 0xa3, 0x85, 0x05, 0xf3, 0x02, 0x19, 0x22, 0x1b } }

class nsIFrameTraversal : public nsISupports
{
public:
  NS_DECLARE_STATIC_IID_ACCESSOR(NS_IFRAMETRAVERSAL_IID)

  














  NS_IMETHOD NewFrameTraversal(nsIBidirectionalEnumerator **aEnumerator,
                               nsPresContext* aPresContext,
                               nsIFrame *aStart,
                               PRInt32 aType,
                               PRBool aVisual,
                               PRBool aLockInScrollView,
                               PRBool aFollowOOFs) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIFrameTraversal, NS_IFRAMETRAVERSAL_IID)

#endif 
