






































#include "nsMemory.h"

#include "nsWindowUtils.h"
#include "nsAETokens.h"

#include "nsAEGetURLSuiteHandler.h"
#include "nsCommandLineServiceMac.h"

#include "nsCOMPtr.h"
#include "nsIServiceManager.h"
#include "nsIWindowMediator.h"
#include "nsIXULWindow.h"

#include "nsIURI.h"
#include "nsNetUtil.h"

using namespace nsWindowUtils;






AEGetURLSuiteHandler::AEGetURLSuiteHandler()
{
}





AEGetURLSuiteHandler::~AEGetURLSuiteHandler()
{
}






void AEGetURLSuiteHandler::HandleGetURLSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	OSErr		err = noErr;
	
	AEEventID		eventID;
	OSType		typeCode;
	Size			actualSize 	= 0L;
	
	
	err = AEGetAttributePtr(appleEvent, 	keyEventIDAttr, 
									typeType, 
									&typeCode, 
									(Ptr)&eventID, 
									sizeof(eventID), 
									&actualSize);
	ThrowIfOSErr(err);
	
	try
	{
		switch (eventID)
		{
			case kGetURLEvent:
				HandleGetURLEvent(appleEvent, reply);
				break;
				
			default:
				ThrowOSErr(errAEEventNotHandled);
				break;
		}
	}
	catch (OSErr catchErr)
	{
		PutReplyErrorNumber(reply, catchErr);
		throw;
	}
	catch ( ... )
	{
		PutReplyErrorNumber(reply, paramErr);
		throw;
	}
}






void AEGetURLSuiteHandler::HandleGetURLEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	StAEDesc		directParameter;
	WindowPtr		targetWindow = NULL;
	OSErr				err;
	
	
	err = ::AEGetKeyDesc(appleEvent, keyDirectObject, typeWildCard, &directParameter);
	ThrowIfOSErr(err);

	
	long		dataSize = directParameter.GetDataSize();
	char*	urlString = (char *)nsMemory::Alloc(dataSize + 1);
	ThrowIfNil(urlString);	
	directParameter.GetCString(urlString, dataSize + 1);

	
        
        CFURLRef url = ::CFURLCreateWithBytes(nsnull, (UInt8*)urlString,
                                              dataSize,
                                              kCFStringEncodingUTF8,
                                              nsnull);
        if (!url)
          return;

        CFStringRef scheme = ::CFURLCopyScheme(url);
        ::CFRelease(url);
        if (!scheme)
          return;

        CFComparisonResult isChrome = ::CFStringCompare(scheme, CFSTR("chrome"),
                                                        kCFCompareCaseInsensitive);
        ::CFRelease(scheme);

        if (isChrome == kCFCompareEqualTo)
          return;

	
	StAEDesc		openInWindowDesc;
	err = ::AEGetKeyDesc(appleEvent, kInsideWindowParameter, typeObjectSpecifier, &openInWindowDesc);
	if (err != errAEDescNotFound)
	{
		
		StAEDesc		tokenDesc;
		err = ::AEResolve(&openInWindowDesc, kAEIDoMinimum, &tokenDesc);
		ThrowIfOSErr(err);
		
		ConstAETokenDesc	tokenContainer(&tokenDesc);
		targetWindow = tokenContainer.GetWindowPtr();		
	}

  
  
  
  if ( targetWindow )
    LoadURLInWindow(targetWindow, urlString);
  else {
    nsCOMPtr<nsIXULWindow> xulwindow;
    nsCOMPtr<nsIWindowMediator> mediator (
                                do_GetService(NS_WINDOWMEDIATOR_CONTRACTID) );
    if ( mediator ) {
      nsCOMPtr<nsISimpleEnumerator> windowEnum;
      mediator->GetZOrderXULWindowEnumerator(
                  NS_LITERAL_STRING("navigator:browser").get(),
                  PR_TRUE, getter_AddRefs(windowEnum));
      if ( windowEnum ) {
        nsCOMPtr<nsISupports> windowSupports;
        windowEnum->GetNext(getter_AddRefs(windowSupports));
        xulwindow = do_QueryInterface(windowSupports);
      }
    }
    if ( xulwindow )
      LoadURLInXULWindow(xulwindow, urlString);
    else
      LoadURLInWindow(nsnull, urlString);
  }

	nsMemory::Free(urlString);	
}

