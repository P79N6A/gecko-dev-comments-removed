





































#include "nsMemory.h"

#include "nsAESpyglassSuiteHandler.h"
#include "nsCommandLineServiceMac.h"
#include "nsDocLoadObserver.h"





AESpyglassSuiteHandler::AESpyglassSuiteHandler()
:   mDocObserver(nsnull)
{
}





AESpyglassSuiteHandler::~AESpyglassSuiteHandler()
{
    NS_IF_RELEASE(mDocObserver);
}






void AESpyglassSuiteHandler::HandleSpyglassSuiteEvent(const AppleEvent *appleEvent, AppleEvent *reply)
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
			case kOpenURLEvent:
				HandleOpenURLEvent(appleEvent, reply);
				break;
			
			case kRegisterURLEchoEvent:
				HandleRegisterURLEchoEvent(appleEvent, reply);
				break;
				
			case kUnregisterURLEchoEvent:
				HandleUnregisterURLEchoEvent(appleEvent, reply);
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






void AESpyglassSuiteHandler::HandleOpenURLEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	StAEDesc		directParameter;
	FSSpec		saveToFile;
	Boolean		gotSaveToFile = false;
	SInt32		targetWindowID = -1;
	SInt32		openFlags = 0;
	OSErr		err;
	
	
	err = ::AEGetKeyDesc(appleEvent, keyDirectObject, typeWildCard, &directParameter);
	ThrowIfOSErr(err);

	
	StAEDesc		targetFileDesc;
	err = ::AEGetKeyDesc(appleEvent, kParamSaveToFileDest, typeFSS, &targetFileDesc);
	if (err != errAEDescNotFound)
	{
		targetFileDesc.GetFileSpec(saveToFile);
		gotSaveToFile = true;
	}
	
#if 0
	
	StAEDesc		openInWindowDesc;
	err = ::AEGetKeyDesc(appleEvent, kParamOpenInWindow, typeLongInteger, &openInWindowDesc);
	if (err != errAEDescNotFound)
		targetWindowID = openInWindowDesc.GetLong();

	
	StAEDesc		openFlagsDesc;
	err = ::AEGetKeyDesc(appleEvent, kParamOpenFlags, typeLongInteger, &openFlagsDesc);
	if (err != errAEDescNotFound)
		openFlags = openFlagsDesc.GetLong();

        
#endif
	
	long		dataSize = directParameter.GetDataSize();
	char*	urlString = (char *)nsMemory::Alloc(dataSize + 1);
	ThrowIfNil(urlString);
	
	directParameter.GetCString(urlString, dataSize + 1);
	
	nsMacCommandLine&  cmdLine = nsMacCommandLine::GetMacCommandLine();
	cmdLine.DispatchURLToNewBrowser(urlString);
	
	nsMemory::Free(urlString);	
}







void AESpyglassSuiteHandler::HandleRegisterURLEchoEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	
	StAEDesc directParameter;
	OSErr err = ::AEGetKeyDesc(appleEvent, keyDirectObject, typeType, &directParameter);
	ThrowIfOSErr(err);

	if (typeType == directParameter.descriptorType)
	{
	    if (mDocObserver == nsnull) {
    		mDocObserver = new nsDocLoadObserver;
    		ThrowIfNil(mDocObserver);
    		NS_ADDREF(mDocObserver);        
    	}
    	OSType requester;
    	if (AEGetDescData(&directParameter, &requester, sizeof(requester)) == noErr)
    		mDocObserver->AddEchoRequester(requester);
	}
}





void AESpyglassSuiteHandler::HandleUnregisterURLEchoEvent(const AppleEvent *appleEvent, AppleEvent *reply)
{
	
	StAEDesc directParameter;
	OSErr err = ::AEGetKeyDesc(appleEvent, keyDirectObject, typeType, &directParameter);
	ThrowIfOSErr(err);

	if (typeType == directParameter.descriptorType)
	{
	    if (mDocObserver)
		mDocObserver->RemoveEchoRequester(**(OSType**)directParameter.dataHandle);
	}
}
