






























#ifdef _WINDOWS
  #include "stdafx.h"
#endif
#include "BrowserImpl.h"
#include "IBrowserFrameGlue.h"













NS_IMETHODIMP CBrowserImpl::OnProgressChange(nsIWebProgress *progress,
                                             nsIRequest *request,
                                             PRInt32 curSelfProgress,
                                             PRInt32 maxSelfProgress,
                                             PRInt32 curTotalProgress,
                                             PRInt32 maxTotalProgress)
{
  if(! m_pBrowserFrameGlue) {
    
    return NS_OK;
  }

  PRInt32 nProgress = curTotalProgress;
  PRInt32 nProgressMax = maxTotalProgress;

  if (nProgressMax == 0)
    nProgressMax = LONG_MAX;

  if (nProgress > nProgressMax)
    nProgress = nProgressMax; 

  m_pBrowserFrameGlue->UpdateProgress(nProgress, nProgressMax);

  return NS_OK;
}

NS_IMETHODIMP CBrowserImpl::OnStateChange(nsIWebProgress *progress,
                                          nsIRequest *request,
                                          PRUint32 progressStateFlags,
                                          nsresult status)
{
  if(! m_pBrowserFrameGlue) {
    
    return NS_OK;
  }

  if ((progressStateFlags & STATE_START) && (progressStateFlags & STATE_IS_DOCUMENT))
  {
    
    m_pBrowserFrameGlue->UpdateBusyState(PR_TRUE);
  }

  if ((progressStateFlags & STATE_STOP) && (progressStateFlags & STATE_IS_DOCUMENT))
  {
    

    m_pBrowserFrameGlue->UpdateBusyState(PR_FALSE);
    m_pBrowserFrameGlue->UpdateProgress(0, 100);       
    m_pBrowserFrameGlue->UpdateStatusBarText(nsnull);  
  }

  return NS_OK;
}


NS_IMETHODIMP CBrowserImpl::OnLocationChange(nsIWebProgress* aWebProgress,
                                                 nsIRequest* aRequest,
                                                 nsIURI *location)
{
  if(! m_pBrowserFrameGlue) {
    
    return NS_OK;
  }

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

  }

  if (!isSubFrameLoad) 
    m_pBrowserFrameGlue->UpdateCurrentURI(location);

    return NS_OK;
}

NS_IMETHODIMP 
CBrowserImpl::OnStatusChange(nsIWebProgress* aWebProgress,
                                 nsIRequest* aRequest,
                                 nsresult aStatus,
                                 const PRUnichar* aMessage)
{
  if(m_pBrowserFrameGlue)
    m_pBrowserFrameGlue->UpdateStatusBarText(aMessage);

  return NS_OK;
}



NS_IMETHODIMP 
CBrowserImpl::OnSecurityChange(nsIWebProgress *aWebProgress, 
                                    nsIRequest *aRequest, 
                                    PRUint32 state)
{
  m_pBrowserFrameGlue->UpdateSecurityStatus(state);

  return NS_OK;
}
