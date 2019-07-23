






































#ifndef nsIScrollPositionListener_h___
#define nsIScrollPositionListener_h___

#include "nsCoord.h"





class nsIScrollPositionListener {
public:

	virtual void ScrollPositionWillChange(nscoord aX, nscoord aY) = 0;
	virtual void ScrollPositionDidChange(nscoord aX, nscoord aY) = 0;
};

#endif 

