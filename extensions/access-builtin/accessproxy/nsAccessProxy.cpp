





































#include "nsCOMPtr.h"
#include "nsMemory.h"
#include "nsIServiceManager.h"
#include "nsIObserverService.h"
#include "nsIGenericFactory.h"
#include "nsIWebProgress.h"
#include "nsCURILoader.h"
#include "nsIDocShell.h"
#include "nsIDOMWindow.h"
#include "nsIDOMWindowInternal.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMNSEvent.h"
#include "nsIPrefBranch.h"
#include "nsIPrefService.h"

#include "nsIRegistry.h"
#include "nsString.h"

#include "nsIDOMNode.h"
#include "nsIPresShell.h"
#include "nsIDOMDocument.h"
#include "nsIDocument.h"
#include "nsISelection.h"
#include "nsISelectionController.h"
#include "nsICaret.h"


#include "nsAccessProxy.h"







NS_IMPL_ISUPPORTS4(nsAccessProxy, nsIObserver, nsISupportsWeakReference, nsIWebProgressListener, nsIDOMEventListener)

nsAccessProxy* nsAccessProxy::mInstance = nsnull;

nsAccessProxy::nsAccessProxy()
{
}

nsAccessProxy::~nsAccessProxy()
{
}

nsAccessProxy *nsAccessProxy::GetInstance()
{
  if (mInstance == nsnull) {
    mInstance = new nsAccessProxy();
    
    NS_IF_ADDREF(mInstance);
  }

  NS_IF_ADDREF(mInstance);
  return mInstance;
}

void nsAccessProxy::ReleaseInstance()
{
  NS_IF_RELEASE(nsAccessProxy::mInstance);
}


NS_IMETHODIMP nsAccessProxy::HandleEvent(nsIDOMEvent* aEvent)
{
  nsresult rv;

  
  nsAutoString eventNameStr;
  rv=aEvent->GetType(eventNameStr);
  if (NS_FAILED(rv))
    return rv;
  
  #ifdef NS_DEBUG_ACCESS_BUILTIN
  printf("\n==== %s event occurred ====\n",NS_ConvertUTF16toUTF8(eventNameStr).get());
  #endif

  
  nsCOMPtr<nsIDOMEventTarget> targetNode;

  nsCOMPtr<nsIDOMNSEvent> nsevent(do_QueryInterface(aEvent));

  if (nsevent) {
    rv = nsevent->GetOriginalTarget(getter_AddRefs(targetNode));

    if (NS_FAILED(rv))
      return rv;
  }

  if (!targetNode)
    return NS_ERROR_NULL_POINTER;
  nsCOMPtr<nsIDOMNode> domNode = do_QueryInterface(targetNode);
  if (!domNode)
    return NS_OK;

  
  nsCOMPtr<nsIDOMDocument> domDoc;
  nsIPresShell *presShell = nsnull;
  nsCOMPtr<nsIDocument> doc;
  domNode->GetOwnerDocument(getter_AddRefs(domDoc));
  if (domDoc) {
    doc = do_QueryInterface(domDoc);
    if (doc) {
      presShell = doc->GetPrimaryShell();
    }
  }
  
  























  return NS_OK;
}



NS_IMETHODIMP nsAccessProxy::Observe(nsISupports *aSubject, const char *aTopic, const PRUnichar *aData) 
{
  static PRBool accessProxyInstalled;

  nsresult rv = NS_OK;
  nsDependentCString aTopicString(aTopic);

  if (accessProxyInstalled && aTopicString.EqualsLiteral(NS_XPCOM_SHUTDOWN_OBSERVER_ID))
    return Release();

  if (!accessProxyInstalled && aTopicString.EqualsLiteral(APPSTARTUP_CATEGORY)) {
    accessProxyInstalled = PR_TRUE; 
    nsCOMPtr<nsIWebProgress> progress(do_GetService(NS_DOCUMENTLOADER_SERVICE_CONTRACTID));
    rv = NS_ERROR_FAILURE;
    if (progress) {
      rv = progress->AddProgressListener(static_cast<nsIWebProgressListener*>(this),
                                         nsIWebProgress::NOTIFY_STATE_DOCUMENT);
      if (NS_SUCCEEDED(rv))
        AddRef();
    }
     
    if (NS_SUCCEEDED(rv)) {
      nsCOMPtr<nsIObserverService> observerService(do_GetService("@mozilla.org/observer-service;1", &rv));
      if (NS_SUCCEEDED(rv)) 
        rv = observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);
    }
  }
  return rv;
}


NS_IMETHODIMP nsAccessProxy::OnStateChange(nsIWebProgress *aWebProgress,
  nsIRequest *aRequest, PRUint32 aStateFlags, nsresult aStatus)
{















  if ((aStateFlags & (STATE_STOP|STATE_START)) && (aStateFlags & STATE_IS_DOCUMENT)) {
    
    
    nsCOMPtr<nsIPrefBranch> prefBranch(do_GetService(NS_PREFSERVICE_CONTRACTID));
    nsXPIDLCString textToSpeechEngine, brailleDisplayEngine;
    if (prefBranch) {
      prefBranch->GetCharPref("accessibility.usetexttospeech", getter_Copies(textToSpeechEngine));
      prefBranch->GetCharPref("accessibility.usebrailledisplay", getter_Copies(brailleDisplayEngine));
    }

    if ((textToSpeechEngine && *textToSpeechEngine) || (brailleDisplayEngine && *brailleDisplayEngine)) {  
      
      nsCOMPtr<nsIDOMWindow> domWindow;
      aWebProgress->GetDOMWindow(getter_AddRefs(domWindow));

      if (domWindow) {
        nsCOMPtr<nsIDOMEventTarget> eventTarget = do_QueryInterface(domWindow);
        nsCOMPtr<nsIDOMWindowInternal> windowInternal = do_QueryInterface(domWindow);
        nsCOMPtr<nsIDOMWindowInternal> opener;
        if (windowInternal)
          windowInternal->GetOpener(getter_AddRefs(opener));
        if (eventTarget && opener) {
          eventTarget->AddEventListener(NS_LITERAL_STRING("keyup"), this, PR_FALSE);
          eventTarget->AddEventListener(NS_LITERAL_STRING("keypress"), this, PR_FALSE);
          eventTarget->AddEventListener(NS_LITERAL_STRING("focus"), this, PR_FALSE);
          eventTarget->AddEventListener(NS_LITERAL_STRING("load"), this, PR_FALSE);
          eventTarget->AddEventListener(NS_LITERAL_STRING("click"), this, PR_FALSE); 
        }
      }
    }
  }

  return NS_OK;
}


NS_IMETHODIMP nsAccessProxy::OnProgressChange(nsIWebProgress *aWebProgress,
  nsIRequest *aRequest, PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress,
  PRInt32 aCurTotalProgress, PRInt32 aMaxTotalProgress)
{
  
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}


NS_IMETHODIMP nsAccessProxy::OnLocationChange(nsIWebProgress *aWebProgress,
  nsIRequest *aRequest, nsIURI *location)
{
  
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}


NS_IMETHODIMP nsAccessProxy::OnStatusChange(nsIWebProgress *aWebProgress,
  nsIRequest *aRequest, nsresult aStatus, const PRUnichar *aMessage)
{
  
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}


NS_IMETHODIMP nsAccessProxy::OnSecurityChange(nsIWebProgress *aWebProgress,
  nsIRequest *aRequest, PRUint32 state)
{
  
  NS_NOTREACHED("notification excluded in AddProgressListener(...)");
  return NS_OK;
}

