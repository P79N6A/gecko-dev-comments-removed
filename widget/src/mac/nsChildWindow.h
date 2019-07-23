



































#ifndef ChildWindow_h__
#define ChildWindow_h__

#include "nsWindow.h"


class nsChildWindow : public nsWindow
{
private:
	typedef nsWindow Inherited;

public:
	nsChildWindow();
	virtual ~nsChildWindow();

    virtual nsresult        StandardCreate(nsIWidget *aParent,
		                            const nsRect &aRect,
		                            EVENT_CALLBACK aHandleEventFunction,
		                            nsIDeviceContext *aContext,
		                            nsIAppShell *aAppShell,
		                            nsIToolkit *aToolkit,
		                            nsWidgetInitData *aInitData,
		                            nsNativeWidget aNativeParent = nsnull);

	virtual void				CalcWindowRegions();

protected:
	PRPackedBool				mClipChildren;
	PRPackedBool				mClipSiblings;
};


#endif 
