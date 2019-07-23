






































#ifndef nsIScrollPositionListener_h___
#define nsIScrollPositionListener_h___

#include "nsISupports.h"
#include "nsCoord.h"
#include "nsTArray.h"
#include "nsIWidget.h"


#define NS_ISCROLLPOSITIONLISTENER_IID \
{ 0x20b82adf, 0x1f5c, 0x44f7, \
  { 0x9b, 0x74, 0xc0, 0xa3, 0x14, 0xd8, 0xcf, 0x91 } }





class nsIScrollPositionListener : public nsISupports {
public:
	NS_DECLARE_STATIC_IID_ACCESSOR(NS_ISCROLLPOSITIONLISTENER_IID)

	NS_IMETHOD ScrollPositionWillChange(nscoord aX, nscoord aY) = 0;
	
	
	
	virtual void ViewPositionDidChange(nsTArray<nsIWidget::Configuration>* aConfigurations) = 0;
	NS_IMETHOD ScrollPositionDidChange(nscoord aX, nscoord aY) = 0;
};

NS_DEFINE_STATIC_IID_ACCESSOR(nsIScrollPositionListener,
                              NS_ISCROLLPOSITIONLISTENER_IID)

#endif 

