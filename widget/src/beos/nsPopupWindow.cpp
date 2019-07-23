


































 
#include "nsPopupWindow.h"

nsPopupWindow::nsPopupWindow() : nsWindow()
{
}







nsresult nsPopupWindow::StandardWindowCreate(nsIWidget *aParent,
                                        const nsRect &aRect,
                                        EVENT_CALLBACK aHandleEventFunction,
                                        nsIDeviceContext *aContext,
                                        nsIAppShell *aAppShell,
                                        nsIToolkit *aToolkit,
                                        nsWidgetInitData *aInitData,
                                        nsNativeWidget aNativeParent)
{
	NS_ASSERTION(aInitData->mWindowType == eWindowType_popup,
		"The windowtype is not handled by this class.");

	NS_ASSERTION(!aParent, "Popups should not be hooked into nsIWidget hierarchy");
	
	mIsTopWidgetWindow = PR_FALSE;
	
	BaseCreate(aParent, aRect, aHandleEventFunction, aContext, aAppShell,
		aToolkit, aInitData);

	mListenForResizes = aNativeParent ? PR_TRUE : aInitData->mListenForResizes;
		
	
	
	
	nsToolkit* toolkit = (nsToolkit *)mToolkit;
	if (toolkit && !toolkit->IsGuiThread())
	{
		uint32 args[7];
		args[0] = (uint32)aParent;
		args[1] = (uint32)&aRect;
		args[2] = (uint32)aHandleEventFunction;
		args[3] = (uint32)aContext;
		args[4] = (uint32)aAppShell;
		args[5] = (uint32)aToolkit;
		args[6] = (uint32)aInitData;

		if (nsnull != aParent)
		{
			
			MethodInfo info(this, this, nsSwitchToUIThread::CREATE, 7, args);
			toolkit->CallMethod(&info);
		}
		else
		{
			
			MethodInfo info(this, this, nsSwitchToUIThread::CREATE_NATIVE, 7, args);
			toolkit->CallMethod(&info);
		}
		return NS_OK;
	}

	mParent = aParent;
	
	
	mWindowParent = (nsWindow *)aParent;
	SetBounds(aRect);

	
	
	
	uint32 flags = B_NOT_RESIZABLE | B_NOT_MINIMIZABLE | B_NOT_ZOOMABLE
		| B_NOT_CLOSABLE | B_ASYNCHRONOUS_CONTROLS | B_AVOID_FOCUS
		| B_NO_WORKSPACE_ACTIVATION;
	window_look look = B_NO_BORDER_WINDOW_LOOK;

	
	
	if ( !(eBorderStyle_default == mBorderStyle || eBorderStyle_all & mBorderStyle))
	{
		if (eBorderStyle_border & mBorderStyle)
			look = B_MODAL_WINDOW_LOOK;

		if (eBorderStyle_resizeh & mBorderStyle)
		{
			
			look = B_MODAL_WINDOW_LOOK;
			flags &= !B_NOT_RESIZABLE;
		}

		
		if (eBorderStyle_title & mBorderStyle || eBorderStyle_menu & mBorderStyle)
			look = B_TITLED_WINDOW_LOOK;

		if (eBorderStyle_minimize & mBorderStyle)
			flags &= !B_NOT_MINIMIZABLE;

		if (eBorderStyle_maximize & mBorderStyle)
			flags &= !B_NOT_ZOOMABLE;

		if (eBorderStyle_close & mBorderStyle)
			flags &= !B_NOT_CLOSABLE;
	}
	
	if (aNativeParent)
	{
		
		
		
		if (((BView *)aNativeParent)->Window() &&
			((BView *)aNativeParent)->Window()->IsFloating())
		{
			mBWindowFeel = B_FLOATING_ALL_WINDOW_FEEL;
		}
	}


	nsWindowBeOS * w = new nsWindowBeOS(this, 
		BRect(aRect.x, aRect.y, aRect.x + aRect.width - 1, aRect.y + aRect.height - 1),
		"", look, mBWindowFeel, flags);
	if (!w)
		return NS_ERROR_OUT_OF_MEMORY;

	mView = new nsViewBeOS(this, w->Bounds(), "popup view",
		B_FOLLOW_ALL, B_WILL_DRAW);

	if (!mView)
		return NS_ERROR_OUT_OF_MEMORY;

	w->AddChild(mView);
	
	w->Run();	
	DispatchStandardEvent(NS_CREATE);
	return NS_OK;
}

NS_METHOD nsPopupWindow::Show(PRBool bState)
{
	
	if (bState == PR_TRUE && mView && mView->Window() != NULL )
		mView->Window()->SetWorkspaces(B_CURRENT_WORKSPACE);
		
	return nsWindow::Show(bState);
}


