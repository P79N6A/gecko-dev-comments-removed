






































#include "nsPrintProgress.h"

#include "nsIBaseWindow.h"
#include "nsISupportsArray.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIComponentManager.h"

NS_IMPL_THREADSAFE_ADDREF(nsPrintProgress)
NS_IMPL_THREADSAFE_RELEASE(nsPrintProgress)

NS_INTERFACE_MAP_BEGIN(nsPrintProgress)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIPrintStatusFeedback)
   NS_INTERFACE_MAP_ENTRY(nsIPrintProgress)
   NS_INTERFACE_MAP_ENTRY(nsIPrintStatusFeedback)
   NS_INTERFACE_MAP_ENTRY(nsIWebProgressListener)
NS_INTERFACE_MAP_END_THREADSAFE


nsPrintProgress::nsPrintProgress(nsIPrintSettings* aPrintSettings)
{
  m_closeProgress = PR_FALSE;
  m_processCanceled = PR_FALSE;
  m_pendingStateFlags = -1;
  m_pendingStateValue = 0;
  m_PrintSetting = aPrintSettings;
}

nsPrintProgress::~nsPrintProgress()
{
  (void)ReleaseListeners();
}


NS_IMETHODIMP nsPrintProgress::OpenProgressDialog(nsIDOMWindowInternal *parent,
                                                  const char *dialogURL,
                                                  nsISupports *parameters, 
                                                  nsIObserver *openDialogObserver,
                                                  PRBool *notifyOnOpen)
{
  *notifyOnOpen = PR_TRUE;
  m_observer = openDialogObserver;
  nsresult rv = NS_ERROR_FAILURE;
  
  if (m_dialog)
    return NS_ERROR_ALREADY_INITIALIZED;
  
  if (!dialogURL || !*dialogURL)
    return NS_ERROR_INVALID_ARG;

  if (parent)
  {
    
    nsCOMPtr<nsISupportsArray> array;
    rv = NS_NewISupportsArray(getter_AddRefs(array));
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupportsInterfacePointer> ifptr =
      do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    
    ifptr->SetData(NS_STATIC_CAST(nsIPrintProgress*, this));
    ifptr->SetDataIID(&NS_GET_IID(nsIPrintProgress));

    array->AppendElement(ifptr);

    array->AppendElement(parameters);

    
    nsCOMPtr<nsIDOMWindow> newWindow;
    rv = parent->OpenDialog(NS_ConvertASCIItoUTF16(dialogURL),
                            NS_LITERAL_STRING("_blank"),
                            NS_LITERAL_STRING("chrome,titlebar,dependent,centerscreen"),
                            array, getter_AddRefs(newWindow));
  }

  return rv;
}


NS_IMETHODIMP nsPrintProgress::CloseProgressDialog(PRBool forceClose)
{
  m_closeProgress = PR_TRUE;
  return OnStateChange(nsnull, nsnull, nsIWebProgressListener::STATE_STOP, forceClose);
}


NS_IMETHODIMP nsPrintProgress::GetPrompter(nsIPrompt **_retval)
{
  NS_ENSURE_ARG_POINTER(_retval);
  *_retval = nsnull;

  if (! m_closeProgress && m_dialog)
    return m_dialog->GetPrompter(_retval);
    
  return NS_ERROR_FAILURE;
}


NS_IMETHODIMP nsPrintProgress::GetProcessCanceledByUser(PRBool *aProcessCanceledByUser)
{
  NS_ENSURE_ARG_POINTER(aProcessCanceledByUser);
  *aProcessCanceledByUser = m_processCanceled;
  return NS_OK;
}
NS_IMETHODIMP nsPrintProgress::SetProcessCanceledByUser(PRBool aProcessCanceledByUser)
{
  if(m_PrintSetting)
    m_PrintSetting->SetIsCancelled(PR_TRUE);
  m_processCanceled = aProcessCanceledByUser;
  OnStateChange(nsnull, nsnull, nsIWebProgressListener::STATE_STOP, PR_FALSE);
  return NS_OK;
}


NS_IMETHODIMP nsPrintProgress::RegisterListener(nsIWebProgressListener * listener)
{
  nsresult rv = NS_OK;
  
  if (!listener) 
    return NS_OK;
  
  if (!m_listenerList)
    rv = NS_NewISupportsArray(getter_AddRefs(m_listenerList));
  
  if (NS_SUCCEEDED(rv) && m_listenerList)
  {
    m_listenerList->AppendElement(listener);
    if (m_closeProgress || m_processCanceled)
      listener->OnStateChange(nsnull, nsnull, nsIWebProgressListener::STATE_STOP, 0);
    else
    {
      listener->OnStatusChange(nsnull, nsnull, 0, m_pendingStatus.get());
      if (m_pendingStateFlags != -1)
        listener->OnStateChange(nsnull, nsnull, m_pendingStateFlags, m_pendingStateValue);
    }
  }
    
  return NS_OK;
}


NS_IMETHODIMP nsPrintProgress::UnregisterListener(nsIWebProgressListener *listener)
{
  if (m_listenerList && listener)
    m_listenerList->RemoveElement(listener);
  
  return NS_OK;
}


NS_IMETHODIMP nsPrintProgress::DoneIniting()
{
  if (m_observer) {
    m_observer->Observe(nsnull, nsnull, nsnull);
  }
  return NS_OK;
}


NS_IMETHODIMP nsPrintProgress::OnStateChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{
  nsresult rv = NS_OK;

  m_pendingStateFlags = aStateFlags;
  m_pendingStateValue = aStatus;
  
  if (m_listenerList)
  {
    PRUint32 count;
    PRInt32 i;

    rv = m_listenerList->Count(&count);
    NS_ASSERTION(NS_SUCCEEDED(rv), "m_listenerList->Count() failed");
    if (NS_FAILED(rv))
      return rv;
  
    nsCOMPtr<nsISupports> aSupports;
    nsCOMPtr<nsIWebProgressListener> aProgressListener;
    for (i = count - 1; i >= 0; i --)
    {
      m_listenerList->GetElementAt(i, getter_AddRefs(aSupports));
      aProgressListener = do_QueryInterface(aSupports);
      if (aProgressListener)
        aProgressListener->OnStateChange(aWebProgress, aRequest, aStateFlags, aStatus);
    }
  }
  
  return rv;
}


NS_IMETHODIMP nsPrintProgress::OnProgressChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress, PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
  nsresult rv = NS_OK;

  if (m_listenerList)
  {
    PRUint32 count;
    PRInt32 i;

    rv = m_listenerList->Count(&count);
    NS_ASSERTION(NS_SUCCEEDED(rv), "m_listenerList->Count() failed");
    if (NS_FAILED(rv))
      return rv;
  
    nsCOMPtr<nsISupports> aSupports;
    nsCOMPtr<nsIWebProgressListener> aProgressListener;
    for (i = count - 1; i >= 0; i --)
    {
      m_listenerList->GetElementAt(i, getter_AddRefs(aSupports));
      aProgressListener = do_QueryInterface(aSupports);
      if (aProgressListener)
        aProgressListener->OnProgressChange(aWebProgress, aRequest, aCurSelfProgress, aMaxSelfProgress, aCurTotalProgress, aMaxTotalProgress);
    }
  }
  
  return rv;
}


NS_IMETHODIMP nsPrintProgress::OnLocationChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsIURI *location)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsPrintProgress::OnStatusChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
  nsresult rv = NS_OK;

  if (aMessage && *aMessage)
  m_pendingStatus = aMessage;
  if (m_listenerList)
  {
    PRUint32 count;
    PRInt32 i;

    rv = m_listenerList->Count(&count);
    NS_ASSERTION(NS_SUCCEEDED(rv), "m_listenerList->Count() failed");
    if (NS_FAILED(rv))
      return rv;
  
    nsCOMPtr<nsISupports> aSupports;
    nsCOMPtr<nsIWebProgressListener> aProgressListener;
    for (i = count - 1; i >= 0; i --)
    {
      m_listenerList->GetElementAt(i, getter_AddRefs(aSupports));
      aProgressListener = do_QueryInterface(aSupports);
      if (aProgressListener)
        aProgressListener->OnStatusChange(aWebProgress, aRequest, aStatus, aMessage);
    }
  }
  
  return rv;
}


NS_IMETHODIMP nsPrintProgress::OnSecurityChange(nsIWebProgress *aWebProgress, nsIRequest *aRequest, PRUint32 state)
{
    return NS_OK;
}

nsresult nsPrintProgress::ReleaseListeners()
{
  nsresult rv = NS_OK;

  if (m_listenerList)
  {
    PRUint32 count;
    PRInt32 i;

    rv = m_listenerList->Count(&count);
    NS_ASSERTION(NS_SUCCEEDED(rv), "m_listenerList->Count() failed");
    if (NS_SUCCEEDED(rv))    
      for (i = count - 1; i >= 0; i --)
        m_listenerList->RemoveElementAt(i);
  }
  
  return rv;
}

NS_IMETHODIMP nsPrintProgress::ShowStatusString(const PRUnichar *status)
{
  return OnStatusChange(nsnull, nsnull, NS_OK, status);
}


NS_IMETHODIMP nsPrintProgress::StartMeteors()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsPrintProgress::StopMeteors()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsPrintProgress::ShowProgress(PRInt32 percent)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsPrintProgress::SetDocShell(nsIDocShell *shell, nsIDOMWindowInternal *window)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP nsPrintProgress::CloseWindow()
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

