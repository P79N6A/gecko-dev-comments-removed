






































#ifdef _WINDOWS
  #include "stdafx.h"
#endif
#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"

#include "TestEmbed.h"
#include "BrowserView.h"
#include "BrowserFrm.h"

#include "QaUtils.h"
#include "Tests.h"













class CBrowserView;

NS_IMETHODIMP CBrowserImpl::OnProgressChange(nsIWebProgress *progress, nsIRequest *request,
                                                  PRInt32 curSelfProgress, PRInt32 maxSelfProgress,
                                                  PRInt32 curTotalProgress, PRInt32 maxTotalProgress)
{
	
	nsCString stringMsg;

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	QAOutput("Entering nsIWebProgLstnr::OnProgressChange().");

	PRInt32 nProgress = curTotalProgress;
	PRInt32 nProgressMax = maxTotalProgress;

	RequestName(request, stringMsg);

	if (nProgressMax == 0)
		nProgressMax = LONG_MAX;

	FormatAndPrintOutput("OnProgressChange(): curSelfProgress value = ", curSelfProgress, 1); 
	FormatAndPrintOutput("OnProgressChange(): maxSelfProgress value = ", maxSelfProgress, 1);
	FormatAndPrintOutput("OnProgressChange(): curTotalProgress value = ", nProgress, 1); 
	FormatAndPrintOutput("OnProgressChange(): maxTotalProgress value = ", nProgressMax, 1);

	if (curSelfProgress == maxSelfProgress && maxSelfProgress != -1)
	{
		QAOutput("nsIWebProgLstnr::OnProgressChange(): Self progress complete!", 1);

		
		WebProgDOMWindowTest(progress, "OnProgressChange()", 1);
	}

	if (nProgress > nProgressMax && nProgressMax != -1)
	{
		nProgress = nProgressMax; 

		QAOutput("nsIWebProgLstnr::OnProgressChange(): Progress Update complete!", 1);
	}
	WebProgIsDocLoadingTest(progress, "OnProgressChange()", 1);
	m_pBrowserFrameGlue->UpdateProgress(nProgress, nProgressMax);

	QAOutput("Exiting nsIWebProgLstnr::OnProgressChange().\r\n");
  
	return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnStateChange(nsIWebProgress *progress, nsIRequest *request,
                                          PRUint32 progressStateFlags, nsresult status)
{
	char theDocType[100];
	char theStateType[100];
	int displayMode = 1;
	nsCString stringMsg;

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	QAOutput("Entering nsIWebProgLstnr::OnStateChange().");

	RequestName(request, stringMsg);	

	if (progressStateFlags & STATE_IS_DOCUMENT)		
	{
		displayMode = 1;
		strcpy(theDocType, "DOCUMENT");
		if (progressStateFlags & STATE_START)
		{ 
			
			strcpy(theStateType, "STATE_START");
			displayMode = 2;

			if(m_pBrowserFrameGlue)
				m_pBrowserFrameGlue->UpdateBusyState(PR_TRUE);
		}

		else if (progressStateFlags & STATE_REDIRECTING)
			strcpy(theStateType, "STATE_REDIRECTING");

		else if (progressStateFlags & STATE_TRANSFERRING) {
			strcpy(theStateType, "STATE_TRANSFERRING");	
		}

		else if (progressStateFlags & STATE_NEGOTIATING)
			strcpy(theStateType, "STATE_NEGOTIATING");				

		else if (progressStateFlags & STATE_STOP)
		{
			

			strcpy(theStateType, "STATE_STOP");
			displayMode = 2;

			m_pBrowserFrameGlue->UpdateBusyState(PR_FALSE);
			m_pBrowserFrameGlue->UpdateProgress(0, 100);       
			m_pBrowserFrameGlue->UpdateStatusBarText(nsnull);  

		
			WebProgDOMWindowTest(progress, "OnStateChange()", 1);
		}

		onStateChangeString(theStateType, theDocType, stringMsg, status, displayMode);

	}		

	if (progressStateFlags & STATE_IS_REQUEST)		
	{
		displayMode = 1;
		strcpy(theDocType, "REQUEST");
		if (progressStateFlags & STATE_START)
			strcpy(theStateType, "STATE_START");
		else if (progressStateFlags & STATE_REDIRECTING)
			strcpy(theStateType, "STATE_REDIRECTING");

		else if (progressStateFlags & STATE_TRANSFERRING)
			strcpy(theStateType, "STATE_TRANSFERRING");		

		else if (progressStateFlags & STATE_NEGOTIATING)
			strcpy(theStateType, "STATE_NEGOTIATING");				

		else if (progressStateFlags & STATE_STOP)
			strcpy(theStateType, "STATE_STOP");
		
		onStateChangeString(theStateType, theDocType, stringMsg, status, displayMode);
	}

	if (progressStateFlags & STATE_IS_NETWORK)		
	{
		displayMode = 1;
		strcpy(theDocType, "NETWORK");
		if (progressStateFlags & STATE_START)
			strcpy(theStateType, "STATE_START");
		else if (progressStateFlags & STATE_REDIRECTING)
			strcpy(theStateType, "STATE_REDIRECTING");

		else if (progressStateFlags & STATE_TRANSFERRING)
			strcpy(theStateType, "STATE_TRANSFERRING");		

		else if (progressStateFlags & STATE_NEGOTIATING)
			strcpy(theStateType, "STATE_NEGOTIATING");				

		else if (progressStateFlags & STATE_STOP)
			strcpy(theStateType, "STATE_STOP");
		
		onStateChangeString(theStateType, theDocType, stringMsg, status, displayMode);

	}
	if (progressStateFlags & STATE_IS_WINDOW)		
	{
		displayMode = 1;
		strcpy(theDocType, "WINDOW");
		if (progressStateFlags & STATE_START)
			strcpy(theStateType, "STATE_START");
		else if (progressStateFlags & STATE_REDIRECTING)
			strcpy(theStateType, "STATE_REDIRECTING");

		else if (progressStateFlags & STATE_TRANSFERRING)
			strcpy(theStateType, "STATE_TRANSFERRING");		

		else if (progressStateFlags & STATE_NEGOTIATING)
			strcpy(theStateType, "STATE_NEGOTIATING");				

		else if (progressStateFlags & STATE_STOP)
			strcpy(theStateType, "STATE_STOP");
		
		onStateChangeString(theStateType, theDocType, stringMsg, status, displayMode);

	}
	
	WebProgIsDocLoadingTest(progress, "OnStateChange()", 1);

	QAOutput("Exiting nsIWebProgLstnr::OnStateChange().\r\n");

    return NS_OK;
}


NS_IMETHODIMP CBrowserImpl::OnLocationChange(nsIWebProgress* aWebProgress,
                                                 nsIRequest* aRequest,
                                                 nsIURI *location)
{
	nsCString stringMsg;

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	QAOutput("Entering nsIWebProgLstnr::OnLocationChange().");

	
	GetTheURI(location, 1);
 
	RequestName(aRequest, stringMsg);  

	PRBool isSubFrameLoad = PR_FALSE; 
	if (aWebProgress) {
		nsCOMPtr<nsIDOMWindow>  domWindow;
		nsCOMPtr<nsIDOMWindow>  topDomWindow;
		aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));
		if (domWindow) { 
			domWindow->GetTop(getter_AddRefs(topDomWindow));
		}
		if (domWindow != topDomWindow)
			isSubFrameLoad = PR_TRUE;
		WebProgIsDocLoadingTest(aWebProgress, "OnLocationChange()", 1);
	}

	if (!isSubFrameLoad) 
	  m_pBrowserFrameGlue->UpdateCurrentURI(location);

  	QAOutput("Exiting nsIWebProgLstnr::OnLocationChange().\r\n");
    return NS_OK;
}

NS_IMETHODIMP 
CBrowserImpl::OnStatusChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage)
{
	nsCString stringMsg;

	if(! m_pBrowserFrameGlue)
		return NS_ERROR_FAILURE;

	QAOutput("Entering nsIWebProgLstnr::OnStatusChange().");

	RequestName(aRequest, stringMsg);

			
	FormatAndPrintOutput("OnStatusChange(): Status id = ", aStatus, 1);

			
	WebProgDOMWindowTest(aWebProgress, "OnStatusChange(): web prog DOM window test", 1);
			
	WebProgIsDocLoadingTest(aWebProgress, "OnStatusChange()", 1);

	m_pBrowserFrameGlue->UpdateStatusBarText(aMessage);

	QAOutput("Exiting nsIWebProgLstnr::OnStatusChange().\r\n");

    return NS_OK;
}


NS_IMETHODIMP 
CBrowserImpl::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
	nsCString stringMsg;

	QAOutput("Entering nsIWebProgLstnr::OnSecurityChange().");

	RequestName(aRequest, stringMsg);

	if (state & STATE_IS_SECURE)
	{
		QAOutput("OnSecurityChange():STATE_IS_SECURE. All docs & subdocs are https.");
		if ((state & 0xFFFF0000) == STATE_SECURE_HIGH)
			QAOutput("OnSecurityChange(): STATE_SECURE_HIGH state");
		else if (state & STATE_SECURE_MED)
			QAOutput("OnSecurityChange(): STATE_SECURE_MED state");
		else if (state & STATE_SECURE_LOW)
			QAOutput("OnSecurityChange(): STATE_SECURE_LOW state");
	}
	else if (state & STATE_IS_BROKEN)
		QAOutput("OnSecurityChange():STATE_IS_BROKEN. Mixed: some docs are https.");
	else if ((state & 0xFFFF) ==  STATE_IS_INSECURE)
		QAOutput("OnSecurityChange():STATE_IS_INSECURE. Nothing is https.");

				
	WebProgDOMWindowTest(aWebProgress, "OnSecurityChange()", 1);
				
	WebProgIsDocLoadingTest(aWebProgress, "OnSecurityChange()", 1);

	QAOutput("Exiting nsIWebProgLstnr::OnSecurityChange().\r\n");

    return NS_OK;
}
