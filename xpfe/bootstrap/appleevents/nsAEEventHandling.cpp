






































#include "nsAEEventHandling.h"

#include "nsAECoreClass.h"







OSErr CreateAEHandlerClasses(Boolean suspendFirstEvent)
{
	OSErr	err = noErr;
	
	
	if (AECoreClass::sAECoreHandler)
		return noErr;
	
	try
	{
		AECoreClass::sAECoreHandler = new AECoreClass(suspendFirstEvent);
	}
	catch(OSErr catchErr)
	{
		err = catchErr;
	}
	catch( ... )
	{
		err = paramErr;
	}
	
	return err;
}







OSErr ResumeAEHandling(AppleEvent *appleEvent, AppleEvent *reply, Boolean dispatchEvent)
{
	OSErr	err = noErr;
	
	if (!AECoreClass::sAECoreHandler)
		return paramErr;
	
	try
	{
		AECoreClass::sAECoreHandler->ResumeEventHandling(appleEvent, reply, dispatchEvent);
	}
	catch(OSErr catchErr)
	{
		err = catchErr;
	}
	catch( ... )
	{
		err = paramErr;
	}

	return err;
}







OSErr GetSuspendedEvent(AppleEvent *theEvent, AppleEvent *reply)
{
	OSErr	err = noErr;
	
	theEvent->descriptorType = typeNull;
	theEvent->dataHandle = nil;
	
	reply->descriptorType = typeNull;
	reply->dataHandle = nil;
	
	if (!AECoreClass::sAECoreHandler)
		return paramErr;
	
	try
	{
		AECoreClass::sAECoreHandler->GetSuspendedEvent(theEvent, reply);
	}
	catch(OSErr catchErr)
	{
		err = catchErr;
	}
	catch( ... )
	{
		err = paramErr;
	}
	
	return err;
}






OSErr ShutdownAEHandlerClasses(void)
{
	if (!AECoreClass::sAECoreHandler)
		return noErr;
	
	try
	{
		delete AECoreClass::sAECoreHandler;
	}
	catch(...)
	{
	}
	
	AECoreClass::sAECoreHandler = nil;
	return noErr;
}

