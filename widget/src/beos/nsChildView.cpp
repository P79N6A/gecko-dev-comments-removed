



































#include "nsChildView.h"

nsChildView::nsChildView() : nsWindow()
{
}


nsresult nsChildView::Create(nsIWidget *aParent,
                             nsNativeWidget aNativeParent,
                             const nsRect &aRect,
                             EVENT_CALLBACK aHandleEventFunction,
                             nsIDeviceContext *aContext,
                             nsIAppShell *aAppShell,
                             nsIToolkit *aToolkit,
                             nsWidgetInitData *aInitData)
{

	NS_ASSERTION(aInitData->mWindowType == eWindowType_child
		|| aInitData->mWindowType == eWindowType_plugin, 
		"The windowtype is not handled by this class." );

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
	
	
	mWindowParent = (nsWindow *) aParent;
	SetBounds(aRect);

 	
 	
	BView *parent= (BView *)
		(aParent ? aParent->GetNativeData(NS_NATIVE_WIDGET) :  aNativeParent);
	
	
	
	
	NS_PRECONDITION(parent, "Childviews without parents don't get added to anything.");
	
	if (!parent)
		return NS_ERROR_FAILURE;
				
	mView = new nsViewBeOS(this, 
		BRect(aRect.x, aRect.y, aRect.x + aRect.width - 1, aRect.y + aRect.height - 1)
		, "Child view", 0, B_WILL_DRAW);
#if defined(BeIME)
	mView->SetFlags(mView->Flags() | B_INPUT_METHOD_AWARE);
#endif	
	bool mustUnlock = parent->Parent() && parent->LockLooper();
 	parent->AddChild(mView);
	if (mustUnlock)
		parent->UnlockLooper();
	DispatchStandardEvent(NS_CREATE);
	return NS_OK;
}







NS_METHOD nsChildView::Show(PRBool bState)
{
	if (!mEnabled)
		return NS_OK;

	if (!mView || !mView->LockLooper())
		return NS_OK;

	
	
	
	
	
	if (PR_FALSE == bState)
	{
		if (!mView->IsHidden())
			mView->Hide();
	}
	else
	{
		if (mView->IsHidden())
			mView->Show();              
	}
	
	mView->UnlockLooper();
	mIsVisible = bState;	
	return NS_OK;
}


NS_METHOD nsChildView::SetTitle(const nsAString& aTitle)
{
	if (mView)
		mView->SetName(NS_ConvertUTF16toUTF8(aTitle).get());
	return NS_OK;
}

