










































#include "stdafx.h"
#include "nsIObserServ.h"
#include "QaUtils.h"




CnsIObserServ::CnsIObserServ()

{
	mRefCnt = 1 ;
}


CnsIObserServ::~CnsIObserServ()
{

}

ObserverElement ObserverTable[] = {
	{"profile-before-change", PR_TRUE},
    {"profile-after-change", PR_FALSE},
	{"profile-do-change", PR_TRUE},
	{"text/xml", PR_FALSE},
	{"application/xml", PR_TRUE},
	{"htmlparser", PR_FALSE},
	{"xmlparser", PR_TRUE},	
	{"memory-pressure", PR_FALSE},
	{"quit-application", PR_TRUE},
	{"login-failed", PR_FALSE}	
}; 


void CnsIObserServ::OnStartTests(UINT nMenuID)

{
	
	

	switch(nMenuID)
	{

	case ID_INTERFACES_NSIOBSERVERSERVICE_RUNALLTESTS :
		RunAllTests();
		break;

	case ID_INTERFACES_NSIOBSERVERSERVICE_ADDOBSERVERS :
		AddObserversTest(2);
		break;

	case ID_INTERFACES_NSIOBSERVERSERVICE_ENUMERATEOBSERVERS :
		QAOutput("Adding observers first", 2);
		AddObserversTest(1);		
		EnumerateObserversTest(1);
		break;

	case ID_INTERFACES_NSIOBSERVERSERVICE_NOTIFYOBSERVERS :
		NotifyObserversTest(1);
		break;

	case ID_INTERFACES_NSIOBSERVERSERVICE_REMOVEOBSERVERS :
		QAOutput("Adding observers first.", 2);
		AddObserversTest(1);
		RemoveObserversTest(1);
		break;

	default :
		AfxMessageBox("Menu handler not added for this menu item");
		break;
	}
}

void CnsIObserServ::RunAllTests()
{
	AddObserversTest(1);
	EnumerateObserversTest(1);
	NotifyObserversTest(1);
	RemoveObserversTest(1);
}

void CnsIObserServ::AddObserversTest(int displayType)
{
	int i;

	nsCOMPtr<nsIObserverService>observerService(do_GetService("@mozilla.org/observer-service;1",&rv));
	RvTestResult(rv, "nsIObserverService object test", displayType);
	RvTestResultDlg(rv, "nsIObserverService object test", true);

	QAOutput("\n nsIObserverService::AddObserversTest().");
	if (!observerService) 
	{
		QAOutput("Can't get nsIObserverService object. Tests fail.");
		return;
	}

	for (i=0; i<10; i++)
	{
		rv = observerService->AddObserver(this, ObserverTable[i].theTopic, 
									      ObserverTable[i].theOwnsWeak);
		FormatAndPrintOutput("The observer to be added = ", ObserverTable[i].theTopic, 1);	
		RvTestResult(rv, "AddObservers() test", displayType);
		RvTestResultDlg(rv, "AddObservers() test");
	}
}

void CnsIObserServ::RemoveObserversTest(int displayType)
{
	int i;

	nsCOMPtr<nsIObserverService>observerService(do_GetService("@mozilla.org/observer-service;1",&rv));

	QAOutput("\n nsIObserverService::RemoveObserversTest().");
	if (!observerService) 
	{
		QAOutput("Can't get nsIObserverService object. Tests fail.");
		return;
	}

	for (i=0; i<10; i++)
	{
		rv = observerService->RemoveObserver(this, ObserverTable[i].theTopic);
		FormatAndPrintOutput("The observer to be removed = ", ObserverTable[i].theTopic, 1);	
		RvTestResult(rv, "RemoveObservers() test", displayType);
		RvTestResultDlg(rv, "RemoveObservers() test");
	}
}


void CnsIObserServ::NotifyObserversTest(int displayType)
{
	PRInt32 i;
	nsCOMPtr<nsIObserverService>observerService(do_GetService("@mozilla.org/observer-service;1",&rv));

	QAOutput("\n nsIObserverService::NotifyObserversTest().");

	if (!observerService) 
	{
		QAOutput("Can't get nsIObserverService object. Tests fail.");
		return;
	}

	for (i=0; i<10; i++)
	{
		FormatAndPrintOutput("The notified observer = ", ObserverTable[i].theTopic, 1);
		rv = observerService->NotifyObservers(nsnull, ObserverTable[i].theTopic, 0);
		RvTestResult(rv, "NotifyObservers() test", displayType);
		RvTestResultDlg(rv, "NotifyObservers() test");
	}
}


void CnsIObserServ::EnumerateObserversTest(int displayType)
{
	PRInt32 i=0;
	nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1",&rv));
	nsCOMPtr<nsISimpleEnumerator> simpleEnum;

	QAOutput("\n nsIObserverService::EnumerateObserversTest().");
	if (!observerService) 
	{
		QAOutput("Can't get nsIObserverService object. Tests fail.");
		return;
	}

	for (i=0; i<10; i++)
	{
		

		rv = observerService->EnumerateObservers(ObserverTable[i].theTopic, 
												 getter_AddRefs(simpleEnum));

		RvTestResult(rv, "EnumerateObserversTest() test", displayType);
		RvTestResultDlg(rv, "EnumerateObserversTest() test");
		if (!simpleEnum)
		{
			QAOutput("Didn't get SimpleEnumerator object. Tests fail.");
			return;
		}

		nsCOMPtr<nsIObserver> observer;
		PRBool theLoop = PR_TRUE;
		PRBool bLoop = PR_TRUE;

		while( NS_SUCCEEDED(simpleEnum->HasMoreElements(&theLoop)) && bLoop) 
		{
			simpleEnum->GetNext(getter_AddRefs(observer));

			if (!observer)
			{
				QAOutput("Didn't get the observer object. Tests fail.");
				return;
			}
			rv = observer->Observe(observer, ObserverTable[i].theTopic, 0);
			RvTestResult(rv, "nsIObserver() test", 1);	
			RvTestResultDlg(rv, "nsIObserver() test");
			
			
			if( this == reinterpret_cast<CnsIObserServ*>(reinterpret_cast<void*>(observer.get())))
			{
				QAOutput("observers match. Test passes.");
				bLoop = PR_FALSE;
			}
			else
				QAOutput("observers don't match. Test fails.");
		}
	}
}


NS_IMPL_THREADSAFE_ISUPPORTS2(CnsIObserServ,  nsIObserver,  nsISupportsWeakReference)


NS_IMETHODIMP CnsIObserServ::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *someData)
{
    nsresult rv = NS_OK;
    
    if (nsCRT::strcmp(aTopic, "profile-before-change") == 0)
    {
 		QAOutput("Observed 'profile-before-change'.");
    }
    else if (nsCRT::strcmp(aTopic, "profile-after-change") == 0)
    {
 		QAOutput("Observed 'profile-after-change'.");
    }
	else if (nsCRT::strcmp(aTopic, "profile-do-change") == 0)
    {
 		QAOutput("Observed 'profile-do-change'.");
    }
    else if (nsCRT::strcmp(aTopic, "text/xml") == 0)
    {
 		QAOutput("Observed 'text/xml'.");
    }
	else if (nsCRT::strcmp(aTopic, "application/xml") == 0)
    {
 		QAOutput("Observed 'application/xml'.");
    }
    else if (nsCRT::strcmp(aTopic, "htmlparser") == 0)
    {
 		QAOutput("Observed 'htmlparser'.");
    }
	else if (nsCRT::strcmp(aTopic, "xmlparser") == 0)
    {
 		QAOutput("Observed 'xmlparser'.");
    }
	else if (nsCRT::strcmp(aTopic, "memory-pressure") == 0)
    {
 		QAOutput("Observed 'memory-pressure'.");
    }
	else if (nsCRT::strcmp(aTopic, "quit-application") == 0)
    {
 		QAOutput("Observed 'quit-application'.");
    }
	else if (nsCRT::strcmp(aTopic, "login-failed") == 0)
    {
 		QAOutput("Observed 'login-failed'.");
    }

    return rv;
}
