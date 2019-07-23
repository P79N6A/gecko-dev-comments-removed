






































#ifndef nsIScrollPositionListener_h___
#define nsIScrollPositionListener_h___

#include "nsISupports.h"
#include "nsCoord.h"


class nsIScrollableView;




#define NS_ISCROLLPOSITIONLISTENER_IID \
    { 0x98a0c040, 0x09cf, 0x408b, \
      { 0xb5, 0x5f, 0x32, 0x1b, 0x4f, 0x8d, 0x9d, 0x67 } }





class nsIScrollPositionListener : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLPOSITIONLISTENER_IID)

	NS_IMETHOD ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY) = 0;
	virtual void ViewPositionDidChange(nsIScrollableView* aScrollable) = 0;
	NS_IMETHOD ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollPositionListener,
                              NS_ISCROLLPOSITIONLISTENER_IID)

#endif 

