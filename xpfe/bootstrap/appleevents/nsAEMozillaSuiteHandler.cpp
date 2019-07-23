






































#include "nsAEMozillaSuiteHandler.h"






AEMozillaSuiteHandler::AEMozillaSuiteHandler()
{
}






AEMozillaSuiteHandler::~AEMozillaSuiteHandler()
{
}






void AEMozillaSuiteHandler::HandleMozillaSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
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
			case kDoJavaScriptEvent:
				
				ThrowOSErr(errAEEventNotHandled);
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

