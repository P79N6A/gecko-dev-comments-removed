


































 
#ifndef nsChildView_h__
#define nsChildView_h__

#include "nsWindow.h"

class nsChildView : public nsWindow 
{
public:
	nsChildView();
	virtual PRBool          IsChild() { return(PR_TRUE); };
	
	NS_IMETHOD              Show(PRBool bState);
	NS_IMETHOD              SetTitle(const nsAString& aTitle);

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
