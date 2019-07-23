






































#ifndef nsIScrollPositionListener_h___
#define nsIScrollPositionListener_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsTArray.h"
#include "nsIWidget.h"


class nsIScrollableView;


#define NS_ISCROLLPOSITIONLISTENER_IID \
  { 0x9654a477, 0x49a7, 0x4aea, \
    { 0xb7, 0xe3, 0x90, 0xe5, 0xe5, 0xd4, 0x28, 0xcd } }





class nsIScrollPositionListener : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLPOSITIONLISTENER_IID)

	NS_IMETHOD ScrollPositionWillChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY) = 0;
	
	
	
	virtual void ViewPositionDidChange(nsIScrollableView* aScrollable,
	                                   nsTArray<nsIWidget::Configuration>* aConfigurations) = 0;
	NS_IMETHOD ScrollPositionDidChange(nsIScrollableView* aScrollable, nscoord aX, nscoord aY) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollPositionListener,
                              NS_ISCROLLPOSITIONLISTENER_IID)

#endif 

