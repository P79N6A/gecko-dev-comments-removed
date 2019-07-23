






































#ifndef nsIScrollPositionListener_h___
#define nsIScrollPositionListener_h___

#include "nsISupports.h"
#include "nsCoord.h"


class nsIScrollableView;



#define NS_ISCROLLPOSITIONLISTENER_IID \
{ 0xf8dfc500, 0x6ad1, 0x11d3, { 0x83, 0x60, 0xa3, 0xf3, 0x73, 0xff, 0x79, 0xfc } }





class nsIScrollPositionListener : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLPOSITIONLISTENER_IID)

	NS_IMETHOD ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY) = 0;
	NS_IMETHOD ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollPositionListener,
                              NS_ISCROLLPOSITIONLISTENER_IID)

#endif 

