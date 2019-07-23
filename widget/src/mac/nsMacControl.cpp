




































#include "nsIFontMetrics.h"
#include "nsIDeviceContext.h"
#include "nsFont.h"
#include "nsFontUtils.h"
#include "nsToolkit.h"
#include "nsGfxUtils.h"

#include "nsMacControl.h"
#include "nsColor.h"
#include "nsFontMetricsMac.h"

#include "nsIServiceManager.h"
#include "nsIPlatformCharset.h"

#include <Carbon/Carbon.h>

#if 0
void DumpControlState(ControlHandle inControl, const char* message)
{
  if (!message) message = "gdb called";
  
  CGrafPtr curPort;
  ::GetPort((GrafPtr*)&curPort);
  Rect portBounds;
  ::GetPortBounds(curPort, &portBounds);

  Rect controlBounds = {0, 0, 0, 0};
  if (inControl)
    ::GetControlBounds(inControl, &controlBounds);
    
  printf("%20s -- port %p bounds %d, %d, %d, %d, control bounds %d, %d, %d, %d\n", message, curPort,
    portBounds.left, portBounds.top, portBounds.right, portBounds.bottom,
    controlBounds.left, controlBounds.top, controlBounds.right, controlBounds.bottom);
}
#endif


nsIUnicodeEncoder * nsMacControl::mUnicodeEncoder = nsnull;
nsIUnicodeDecoder * nsMacControl::mUnicodeDecoder = nsnull;

#pragma mark -





nsMacControl::nsMacControl()
: mWidgetArmed(PR_FALSE)
, mMouseInButton(PR_FALSE)
, mValue(0)
, mMin(0)
, mMax(0)
, mControl(nsnull)
, mControlType(pushButProc)
, mControlEventHandler(nsnull)
, mWindowEventHandler(nsnull)
, mLastValue(0)
, mLastHilite(0)
{
  AcceptFocusOnClick(PR_FALSE);
}





NS_IMETHODIMP nsMacControl::Create(nsIWidget *aParent,
                      const nsRect &aRect,
                      EVENT_CALLBACK aHandleEventFunction,
                      nsIDeviceContext *aContext,
                      nsIAppShell *aAppShell,
                      nsIToolkit *aToolkit,
                      nsWidgetInitData *aInitData) 
{
	Inherited::Create(aParent, aRect, aHandleEventFunction,
						aContext, aAppShell, aToolkit, aInitData);
  
	
	nsresult		theResult = CreateOrReplaceMacControl(mControlType);

	mLastBounds = mBounds;

	return theResult;
}





nsMacControl::~nsMacControl()
{
	if (mControl)
	{
		Show(PR_FALSE);
		ClearControl();
		mControl = nsnull;
	}
}





NS_IMETHODIMP
nsMacControl::Destroy()
{
	if (mOnDestroyCalled)
		return NS_OK;

	
	
	
	
	Show(PR_FALSE);

	return Inherited::Destroy();
}

#pragma mark -




PRBool nsMacControl::OnPaint(nsPaintEvent &aEvent)
{
	if (mControl && mVisible)
	{
		
		Boolean		isVisible = ::IsControlVisible(mControl);
		::SetControlVisibility(mControl, false, false);

		
		if (mLabel != mLastLabel)
		{
			NSStringSetControlTitle(mControl,mLabel);
		}

		
		if (mBounds != mLastBounds)
		{
			nsRect ctlRect;
			GetRectForMacControl(ctlRect);
			Rect macRect;
			nsRectToMacRect(ctlRect, macRect);

			::SetControlBounds(mControl, &macRect);

			mLastBounds = mBounds;

#if 0
			
			
			
			
			
			
			
			
			
			nsRect bounds = mBounds;
			bounds.x = bounds. y = 0;
			nsRectToMacRect(bounds, macRect);
			::EraseRect(&macRect);
#endif
		}

		
		if (mValue != mLastValue)
		{
			mLastValue = mValue;
			::SetControl32BitValue(mControl, mValue);
		}

		
		SetupControlHiliteState();

		::SetControlVisibility(mControl, isVisible, false);

		
		::DrawOneControl(mControl);
	}
	return PR_FALSE;
}





PRBool  nsMacControl::DispatchMouseEvent(nsMouseEvent &aEvent)
{
	PRBool eatEvent = PR_FALSE;
	switch (aEvent.message)
	{
		case NS_MOUSE_DOUBLECLICK:
		case NS_MOUSE_BUTTON_DOWN:
			if (aEvent.button == nsMouseEvent::eLeftButton && mEnabled)
			{
				mMouseInButton = PR_TRUE;
				mWidgetArmed = PR_TRUE;
				Invalidate(PR_TRUE);
			}
			break;

		case NS_MOUSE_BUTTON_UP:
			if (aEvent.button == nsMouseEvent::eLeftButton)
			{
			
			if (!mWidgetArmed)
				eatEvent = PR_TRUE;
			
			
			if (!mMouseInButton)
				eatEvent = PR_TRUE;
			mWidgetArmed = PR_FALSE;
			if (mMouseInButton)
				Invalidate(PR_TRUE);
			}
			break;

		case NS_MOUSE_EXIT:
			mMouseInButton = PR_FALSE;
			if (mWidgetArmed)
				Invalidate(PR_TRUE);
			break;

		case NS_MOUSE_ENTER:
			mMouseInButton = PR_TRUE;
			if (mWidgetArmed)
				Invalidate(PR_TRUE);
			break;
	}
	if (eatEvent)
		return PR_TRUE;
	return (Inherited::DispatchMouseEvent(aEvent));
}





void  nsMacControl::ControlChanged(PRInt32 aNewValue)
{
	if (aNewValue != mValue)
	{
		mValue = aNewValue;
		mLastValue = mValue;	

		nsGUIEvent guiEvent(PR_TRUE, NS_CONTROL_CHANGE, this);
 		guiEvent.time	 	= PR_IntervalNow();
		Inherited::DispatchWindowEvent(guiEvent);
	}
}


#pragma mark -




NS_IMETHODIMP nsMacControl::Enable(PRBool bState)
{
  PRBool priorState = mEnabled;
  Inherited::Enable(bState);
  if ( priorState != bState )
    Invalidate(PR_FALSE);
  return NS_OK;
}




NS_IMETHODIMP nsMacControl::Show(PRBool bState)
{
  Inherited::Show(bState);
  if (mControl)
  {
  	::SetControlVisibility(mControl, bState, false);		
  }
  return NS_OK;
}






NS_IMETHODIMP nsMacControl::SetFont(const nsFont &aFont)
{
	Inherited::SetFont(aFont);	

	SetupMacControlFont();
	
 	return NS_OK;
}

#pragma mark -







void nsMacControl::GetRectForMacControl(nsRect &outRect)
{
		outRect = mBounds;
		outRect.x = outRect.y = 0;
}






ControlPartCode nsMacControl::GetControlHiliteState()
{
  
  PRInt16 curHilite = kControlInactivePart;

  
  
  PRBool isPopup = PR_FALSE;
  nsCOMPtr<nsIWidget> windowWidget;
  nsToolkit::GetTopWidget(mWindowPtr, getter_AddRefs(windowWidget));
  if (windowWidget) {
    nsWindowType windowType;
    if (NS_SUCCEEDED(windowWidget->GetWindowType(windowType)) &&
        windowType == eWindowType_popup) {
      isPopup = PR_TRUE;
    }
  }

  if (mEnabled && (isPopup || ::IsWindowActive(mWindowPtr)))
    if (mWidgetArmed && mMouseInButton)
      curHilite = kControlLabelPart;
    else
      curHilite = kControlNoPart;

  return curHilite;
}

void
nsMacControl::SetupControlHiliteState()
{
  PRInt16 curHilite = GetControlHiliteState();
  if (curHilite != mLastHilite) {
    mLastHilite = curHilite;
    ::HiliteControl(mControl, curHilite);
  }
}






nsresult nsMacControl::CreateOrReplaceMacControl(short inControlType)
{
  nsresult rv = NS_ERROR_NULL_POINTER;
  nsRect controlRect;
  GetRectForMacControl(controlRect);
  Rect macRect;
  nsRectToMacRect(controlRect, macRect);

  ClearControl();

  if (mWindowPtr) {
    mControl = ::NewControl(mWindowPtr, &macRect, "\p", PR_FALSE,
                            mValue, mMin, mMax, inControlType, nsnull);

    if (mControl) {
      InstallEventHandlerOnControl();
      SetupControlHiliteState();

      
      
      if (mFontMetrics)
        SetupMacControlFont();

      if (mVisible)
        ::ShowControl(mControl);

      rv = NS_OK;
    }
  }

  return rv;
}





void nsMacControl::ClearControl()
{
	RemoveEventHandlerFromControl();
	if (mControl)
	{
		::DisposeControl(mControl);
		mControl = nsnull;
	}
}





void nsMacControl::SetupMacControlFont()
{
	NS_PRECONDITION(mFontMetrics != nsnull, "No font metrics in SetupMacControlFont");
	NS_PRECONDITION(mContext != nsnull, "No context metrics in SetupMacControlFont");
	
	TextStyle		theStyle;
	
	if (theStyle.tsSize < 9)
		theStyle.tsSize = 9;
	
	ControlFontStyleRec fontStyleRec;
	fontStyleRec.flags = (kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask);
	fontStyleRec.font = theStyle.tsFont;
	fontStyleRec.size = theStyle.tsSize;
	fontStyleRec.style = theStyle.tsFace;
	::SetControlFontStyle(mControl, &fontStyleRec);
}

#pragma mark -






void nsMacControl::StringToStr255(const nsAString& aText, Str255& aStr255)
{
	nsresult rv = NS_OK;
	nsAString::const_iterator begin;
	const PRUnichar *text = aText.BeginReading(begin).get();

	
	if (nsnull == mUnicodeEncoder) {
		nsCAutoString fileSystemCharset;
		GetFileSystemCharset(fileSystemCharset);

		nsCOMPtr<nsICharsetConverterManager> ccm = 
		         do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv); 
		if (NS_SUCCEEDED(rv)) {
			rv = ccm->GetUnicodeEncoderRaw(fileSystemCharset.get(), &mUnicodeEncoder);
            if (NS_SUCCEEDED(rv)) {
              rv = mUnicodeEncoder->SetOutputErrorBehavior(nsIUnicodeEncoder::kOnError_Replace, nsnull, (PRUnichar)'?');
            }
		}
	}

	
	if (NS_SUCCEEDED(rv)) {
		PRInt32 inLength = aText.Length();
		PRInt32 outLength = 255;
		rv = mUnicodeEncoder->Convert(text, &inLength, (char *) &aStr255[1], &outLength);
		if (NS_SUCCEEDED(rv))
			aStr255[0] = outLength;
	}

	if (NS_FAILED(rv)) {

		NS_LossyConvertUTF16toASCII buffer(Substring(aText,0,254));
		PRInt32 len = buffer.Length();
		memcpy(&aStr255[1], buffer.get(), len);
		aStr255[0] = len;
	}
}






void nsMacControl::Str255ToString(const Str255& aStr255, nsString& aText)
{
	nsresult rv = NS_OK;
	
	
	if (nsnull == mUnicodeDecoder) {
		nsCAutoString fileSystemCharset;
		GetFileSystemCharset(fileSystemCharset);

		nsCOMPtr<nsICharsetConverterManager> ccm = 
		         do_GetService(NS_CHARSETCONVERTERMANAGER_CONTRACTID, &rv); 
		if (NS_SUCCEEDED(rv)) {
			rv = ccm->GetUnicodeDecoderRaw(fileSystemCharset.get(), &mUnicodeDecoder);
		}
	}
  
	
	if (NS_SUCCEEDED(rv)) {
		PRUnichar buffer[512];
		PRInt32 inLength = aStr255[0];
		PRInt32 outLength = 512;
		rv = mUnicodeDecoder->Convert((char *) &aStr255[1], &inLength, buffer, &outLength);
		if (NS_SUCCEEDED(rv)) {
			aText.Assign(buffer, outLength);
		}
	}
	
	if (NS_FAILED(rv)) {

		aText.AssignWithConversion((char *) &aStr255[1], aStr255[0]);
	}
}






void nsMacControl::NSStringSetControlTitle(ControlHandle theControl, nsString title)
{	
  
  CFStringRef str = CFStringCreateWithCharacters(NULL, (const UniChar*)title.get(), title.Length());
  SetControlTitleWithCFString(theControl, str);
  CFRelease(str);
}




void nsMacControl::SetupMacControlFontForScript(short theScript)
{
	short					themeFontID;
	Str255					themeFontName;
	SInt16					themeFontSize;
	Style					themeFontStyle;
	TextStyle				theStyle;
	OSErr					err;

	NS_PRECONDITION(mFontMetrics != nsnull, "No font metrics in SetupMacControlFont");
	NS_PRECONDITION(mContext != nsnull, "No context metrics in SetupMacControlFont");

	
	
	
	err = ::GetThemeFont(kThemeSystemFont,theScript,themeFontName,&themeFontSize,&themeFontStyle);
	NS_ASSERTION(err==noErr,"nsMenu::NSStringNewMenu: GetThemeFont failed.");
	::GetFNum(themeFontName,&themeFontID);
	
	
	if (theStyle.tsSize < 9)
		theStyle.tsSize = 9;
	
	ControlFontStyleRec fontStyleRec;
	fontStyleRec.flags = (kControlUseFontMask | kControlUseFaceMask | kControlUseSizeMask);
	fontStyleRec.font = themeFontID;
	fontStyleRec.size = theStyle.tsSize;
	fontStyleRec.style = theStyle.tsFace;
	::SetControlFontStyle(mControl, &fontStyleRec);
}




void nsMacControl::GetFileSystemCharset(nsCString & fileSystemCharset)
{
  static nsCAutoString aCharset;
  nsresult rv;

  if (aCharset.IsEmpty()) {
    nsCOMPtr <nsIPlatformCharset> platformCharset = do_GetService(NS_PLATFORMCHARSET_CONTRACTID, &rv);
	  if (NS_SUCCEEDED(rv)) 
		  rv = platformCharset->GetCharset(kPlatformCharsetSel_FileName, aCharset);

    NS_ASSERTION(NS_SUCCEEDED(rv), "error getting platform charset");
	  if (NS_FAILED(rv)) 
		  aCharset.AssignLiteral("x-mac-roman");
  }
  fileSystemCharset = aCharset;
}






OSStatus nsMacControl::InstallEventHandlerOnControl()
{
  const EventTypeSpec kControlEventList[] = {
    
    
    
    { kEventClassControl, kEventControlDraw },
  };

  static EventHandlerUPP sControlEventHandlerUPP;
  if (!sControlEventHandlerUPP)
    sControlEventHandlerUPP = ::NewEventHandlerUPP(ControlEventHandler);

  OSStatus err =
   ::InstallControlEventHandler(mControl,
                                sControlEventHandlerUPP,
                                GetEventTypeCount(kControlEventList),
                                kControlEventList,
                                (void*)this,
                                &mControlEventHandler);
  NS_ENSURE_TRUE(err == noErr, err);

  const EventTypeSpec kWindowEventList[] = {
    { kEventClassWindow, kEventWindowActivated },
    { kEventClassWindow, kEventWindowDeactivated },
  };

  static EventHandlerUPP sWindowEventHandlerUPP;
  if (!sWindowEventHandlerUPP)
    sWindowEventHandlerUPP = ::NewEventHandlerUPP(WindowEventHandler);

  err = ::InstallWindowEventHandler(mWindowPtr,
                                    sWindowEventHandlerUPP,
                                    GetEventTypeCount(kWindowEventList),
                                    kWindowEventList,
                                    (void*)this,
                                    &mWindowEventHandler);
  return err;
}





void nsMacControl::RemoveEventHandlerFromControl()
{
  if (mControlEventHandler) {
    ::RemoveEventHandler(mControlEventHandler);
    mControlEventHandler = nsnull;
  }

  if (mWindowEventHandler) {
    ::RemoveEventHandler(mWindowEventHandler);
    mWindowEventHandler = nsnull;
  }
}







pascal OSStatus
nsMacControl::ControlEventHandler(EventHandlerCallRef aHandlerCallRef,
                                  EventRef            aEvent,
                                  void*               aUserData)
{
  nsMacControl* self = NS_STATIC_CAST(nsMacControl*, aUserData);

  PRBool wasDrawing = self->IsDrawing();

  if (wasDrawing) {
    if (!self->IsQDStateOK()) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      
      return noErr;
    }
  }
  else {
    self->StartDraw();
  }

  OSStatus err = ::CallNextEventHandler(aHandlerCallRef, aEvent);

  if (!wasDrawing) {
    self->EndDraw();
  }

  return err;
}








PRBool nsMacControl::IsQDStateOK()
{
  CGrafPtr controlPort = ::GetWindowPort(mWindowPtr);
  CGrafPtr currentPort;
  ::GetPort(&currentPort);

  if (controlPort != currentPort) {
    return PR_FALSE;
  }

  nsRect controlBounds;
  GetBounds(controlBounds);
  LocalToWindowCoordinate(controlBounds);
  Rect currentBounds;
  ::GetPortBounds(currentPort, &currentBounds);

  if (-controlBounds.x != currentBounds.left ||
      -controlBounds.y != currentBounds.top) {
    return PR_FALSE;
  }

  return PR_TRUE;
}









pascal OSStatus
nsMacControl::WindowEventHandler(EventHandlerCallRef aHandlerCallRef,
                                 EventRef            aEvent,
                                 void*               aUserData)
{
  nsMacControl* self = NS_STATIC_CAST(nsMacControl*, aUserData);

  
  
  if (self->mVisible && self->ContainerHierarchyIsVisible())
    self->SetupControlHiliteState();

  return ::CallNextEventHandler(aHandlerCallRef, aEvent);
}
