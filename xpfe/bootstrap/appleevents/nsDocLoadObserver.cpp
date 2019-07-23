





































#include "nsCOMPtr.h"
#include "nsError.h"
#include "nsIObserverService.h"
#include "nsIServiceManager.h"
#include "nsString.h"
#include "nsReadableUtils.h"
#include "nsAEUtils.h"
#include "nsAESpyglassSuiteHandler.h"

#include "nsDocLoadObserver.h"

nsDocLoadObserver::nsDocLoadObserver()
:	mRegistered(PR_FALSE)
{
}

nsDocLoadObserver::~nsDocLoadObserver()
{
}


NS_IMPL_ISUPPORTS1(nsDocLoadObserver, nsIObserver)

void nsDocLoadObserver::Register()
{
	if (!mRegistered)
	{
		nsresult rv;
		nsCOMPtr<nsIObserverService> anObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
		if (NS_SUCCEEDED(rv))
		{
			if (NS_SUCCEEDED(anObserverService->AddObserver(this, "EndDocumentLoad", PR_FALSE)))
			{
				mRegistered = PR_TRUE;
			}
		}
	}
}

void nsDocLoadObserver::Unregister()
{
	if (mRegistered)
	{
		nsresult rv;
		nsCOMPtr<nsIObserverService> anObserverService = do_GetService("@mozilla.org/observer-service;1", &rv);
		if (NS_SUCCEEDED(rv))
		{
			if (NS_SUCCEEDED(anObserverService->RemoveObserver(this, 
				"EndDocumentLoad")))
			{
				mRegistered = PR_FALSE;
			}
		}
	}
}

void nsDocLoadObserver::AddEchoRequester(OSType appSignature)
{
  
  mEchoRequesters.RemoveElement((void*)appSignature);
  
	mEchoRequesters.AppendElement((void*)appSignature);
	Register();     
}

void nsDocLoadObserver::RemoveEchoRequester(OSType appSignature)
{
	mEchoRequesters.RemoveElement((void*)appSignature);
	if (mEchoRequesters.Count() == 0)
	  Unregister();
}

NS_IMETHODIMP nsDocLoadObserver::Observe(nsISupports* ,
		const char* , const PRUnichar* someData)
{
	
	if (!someData)
		return NS_ERROR_NULL_POINTER;

	
	if (mEchoRequesters.Count() == 0)
		return NS_OK;

	
	StAEDesc from;
	const OSType mozz = 'MOZZ';
	if (noErr != ::AECreateDesc(typeType, &mozz, sizeof(mozz), &from))
		return NS_ERROR_UNEXPECTED;

	
	nsString urlText(someData);
	char* urlString = ToNewCString(urlText);

	StAEDesc url;
	OSErr err = ::AECreateDesc(typeChar, urlString, urlText.Length(), &url);
	nsMemory::Free(urlString);
	if (err != noErr)
		return NS_ERROR_UNEXPECTED;

	
	nsVoidArray requestersToRemove;
	
	PRInt32		numRequesters = mEchoRequesters.Count();
	
	
	for (PRInt32 i = 0; i < numRequesters; i ++)
	{
		
		StAEDesc targetAddress;
		const OSType target = (OSType)mEchoRequesters.ElementAt(i);
		if (noErr != ::AECreateDesc(typeApplSignature, &target, sizeof(target), &targetAddress))
			return NS_ERROR_UNEXPECTED;

		
		AppleEvent sendEvent;
		err = ::AECreateAppleEvent(AESpyglassSuiteHandler::kSpyglassSendSignature,
				AESpyglassSuiteHandler::kSendURLEchoEvent, 
				&targetAddress, kAutoGenerateReturnID, kAnyTransactionID, &sendEvent);
		if (noErr != err)
		{	
			requestersToRemove.AppendElement((void *)target);
			continue;
		}

		
		err = ::AEPutParamDesc(&sendEvent, keyOriginalAddressAttr, &from);
		NS_ASSERTION(noErr == err, "AEPutParamDesc");

		
		err = ::AEPutParamDesc(&sendEvent, keyDirectObject, &url);
		NS_ASSERTION(noErr == err, "AEPutParamDesc");
		
		
		AppleEvent reply;
		err = ::AESend(&sendEvent, &reply, kAENoReply, kAENormalPriority, 180, NULL, NULL);
		NS_ASSERTION(noErr == err, "AESend");
		::AEDisposeDesc(&sendEvent);
	}

	
	for (PRInt32 i = 0; i < requestersToRemove.Count(); i ++)
	{
		OSType	thisRequester = (OSType)requestersToRemove.ElementAt(i);
		mEchoRequesters.RemoveElement((void *)thisRequester);
	}
	
	return NS_OK;
}
