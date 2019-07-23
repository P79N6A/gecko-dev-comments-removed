


































 
#ifndef nsPopupWindow_h__
#define nsPopupWindow_h__

#include "nsWindow.h"

class nsPopupWindow : public nsWindow 
{
public:
	nsPopupWindow();

	NS_IMETHOD              Show(PRBool bState);

protected:
	virtual nsresult        StandardWindowCreate(nsIWidget *aParent,
	                                             const nsRect &aRect,
	                                             EVENT_CALLBACK aHandleEventFunction,
	                                             nsIDeviceContext *aContext,
	                                             nsIAppShell *aAppShell,
	                                             nsIToolkit *aToolkit,
	                                             nsWidgetInitData *aInitData,
	                                             nsNativeWidget aNativeParent = nsnull);

};
#endif 
