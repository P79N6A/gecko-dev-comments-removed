




































#ifndef nsMacControl_h__
#define nsMacControl_h__

#include "nsIServiceManager.h"
#include "nsICharsetConverterManager.h"
#include "nsChildWindow.h"
#include <Controls.h>

class nsMacControl : public nsChildWindow
{
private:
	typedef nsChildWindow Inherited;

public:
						nsMacControl();
	virtual				~nsMacControl();

	NS_IMETHOD 			Create(nsIWidget *aParent,
				              const nsRect &aRect,
				              EVENT_CALLBACK aHandleEventFunction,
				              nsIDeviceContext *aContext = nsnull,
				              nsIAppShell *aAppShell = nsnull,
				              nsIToolkit *aToolkit = nsnull,
				              nsWidgetInitData *aInitData = nsnull);
	NS_IMETHOD			Destroy();

	virtual void		SetControlType(short type)	{mControlType = type;}
	short				GetControlType()			{return mControlType;}

	
	virtual PRBool		OnPaint(nsPaintEvent & aEvent);
	virtual PRBool		DispatchMouseEvent(nsMouseEvent &aEvent);
	static pascal OSStatus	ControlEventHandler(EventHandlerCallRef aHandlerCallRef, EventRef aEvent, void* aUserData);
	static pascal OSStatus	WindowEventHandler(EventHandlerCallRef aHandlerCallRef, EventRef aEvent, void* aUserData);
    
	
	NS_IMETHOD			Enable(PRBool bState);
	NS_IMETHOD			Show(PRBool aState);
	NS_IMETHODIMP		SetFont(const nsFont &aFont);

	
	
	static void 		StringToStr255(const nsAString& aText, Str255& aStr255);
	static void 		Str255ToString(const Str255& aStr255, nsString& aText);

protected:
	
	nsresult			CreateOrReplaceMacControl(short inControlType);

 	void				ClearControl();

	virtual void		GetRectForMacControl(nsRect &outRect);
	virtual ControlPartCode	GetControlHiliteState();
	void			SetupControlHiliteState();

	void				SetupMacControlFont();
	void				ControlChanged(PRInt32 aNewValue);
	void				NSStringSetControlTitle(ControlHandle theControl, nsString title);
	void				SetupMacControlFontForScript(short theScript);
	static void			GetFileSystemCharset(nsCString & fileSystemCharset);

	OSStatus			InstallEventHandlerOnControl();
	void				RemoveEventHandlerFromControl();

	PRBool				IsQDStateOK();

	nsString			mLabel;
	PRBool				mWidgetArmed;
	PRBool				mMouseInButton;

	PRInt32				mValue;
	PRInt32				mMin;
	PRInt32				mMax;
	ControlHandle			mControl;
	short				mControlType;
	EventHandlerRef			mControlEventHandler;
	EventHandlerRef			mWindowEventHandler;

	nsString			mLastLabel;
	nsRect				mLastBounds;
	PRInt32				mLastValue;
	PRInt16				mLastHilite;

	static nsIUnicodeEncoder*     mUnicodeEncoder;
	static nsIUnicodeDecoder*     mUnicodeDecoder;
};

#endif 
