




































#include "nsCURILoader.h"
#include "nsDocNavStartProgressListener.h"
#include "nsIChannel.h"
#include "nsINestedURI.h"
#include "nsIRequest.h"
#include "nsITimer.h"
#include "nsIURI.h"
#include "nsIWebProgress.h"
#include "nsComponentManagerUtils.h"
#include "nsServiceManagerUtils.h"
#include "nsStringAPI.h"
#include "prlog.h"

NS_IMPL_ISUPPORTS4(nsDocNavStartProgressListener,
                   nsIDocNavStartProgressListener,
                   nsIWebProgressListener,
                   nsIObserver,
                   nsISupportsWeakReference)


#if defined(PR_LOGGING)
static const PRLogModuleInfo *gDocNavStartProgressListenerLog = nsnull;
#define LOG(args) PR_LOG(gDocNavStartProgressListenerLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif

nsDocNavStartProgressListener::nsDocNavStartProgressListener() :
  mEnabled(PR_FALSE), mDelay(0), mRequests(nsnull), mTimers(nsnull)
{
#if defined(PR_LOGGING)
  if (!gDocNavStartProgressListenerLog)
    gDocNavStartProgressListenerLog = PR_NewLogModule("DocNavStart");
#endif
        
}


nsDocNavStartProgressListener::~nsDocNavStartProgressListener()
{
  
  mRequests.Clear();

  
  PRUint32 length = mTimers.Count();

  for (PRUint32 i = 0; i < length; ++i) {
    mTimers[i]->Cancel();
  }

  mTimers.Clear();
  
  mCallback = nsnull;
}



nsresult
nsDocNavStartProgressListener::AttachListeners()
{
  nsresult rv;
  nsCOMPtr<nsIWebProgress> webProgressService = do_GetService(
      NS_DOCUMENTLOADER_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return webProgressService->AddProgressListener(this,
      nsIWebProgress::NOTIFY_LOCATION);
}




nsresult
nsDocNavStartProgressListener::DetachListeners()
{
  nsresult rv;
  nsCOMPtr<nsIWebProgress> webProgressService = do_GetService(
      NS_DOCUMENTLOADER_SERVICE_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  return webProgressService->RemoveProgressListener(this);
}


nsresult
nsDocNavStartProgressListener::GetRequestUri(nsIRequest* aReq, nsIURI** uri)
{
  nsCOMPtr<nsIChannel> channel;
  nsresult rv;
  channel = do_QueryInterface(aReq, &rv);
  if (NS_FAILED(rv))
    return rv;

  rv = channel->GetURI(uri);
  if (NS_FAILED(rv))
    return rv;
  return NS_OK;
}






NS_IMETHODIMP
nsDocNavStartProgressListener::GetGlobalProgressListenerEnabled(PRBool* aEnabled)
{
  *aEnabled = mEnabled;
  return NS_OK;
}




NS_IMETHODIMP
nsDocNavStartProgressListener::SetGlobalProgressListenerEnabled(PRBool aEnabled)
{
  if (aEnabled && ! mEnabled) {
    
    mEnabled = PR_TRUE;
    return AttachListeners();
  } else if (! aEnabled && mEnabled) {
    
    mEnabled = PR_FALSE;
    return DetachListeners();
  }
  return NS_OK; 
}

NS_IMETHODIMP
nsDocNavStartProgressListener::GetDelay(PRUint32* aDelay)
{
  *aDelay = mDelay;
  return NS_OK;
}

NS_IMETHODIMP
nsDocNavStartProgressListener::SetDelay(PRUint32 aDelay)
{
  mDelay = aDelay;
  return NS_OK;
}



NS_IMETHODIMP
nsDocNavStartProgressListener::GetCallback(
    nsIDocNavStartProgressCallback** aCallback)
{
  NS_ENSURE_ARG_POINTER(aCallback);
  *aCallback = mCallback;
  NS_IF_ADDREF(*aCallback);
  return NS_OK;
}




NS_IMETHODIMP
nsDocNavStartProgressListener::SetCallback(
    nsIDocNavStartProgressCallback* aCallback)
{
  mCallback = aCallback;
  return NS_OK;
}

NS_IMETHODIMP
nsDocNavStartProgressListener::IsSpurious(nsIURI* aURI, PRBool* isSpurious)
{
  nsCAutoString scheme;
  nsresult rv = aURI->GetScheme(scheme);
  NS_ENSURE_SUCCESS(rv, rv);

  *isSpurious = scheme.Equals("about") ||
                scheme.Equals("chrome") ||
                scheme.Equals("file") ||
                scheme.Equals("javascript");

  if (!*isSpurious) {
    
    nsCOMPtr<nsINestedURI> nestedURI = do_QueryInterface(aURI);
    if (nestedURI) {
      nsCOMPtr<nsIURI> innerURI;
      rv = nestedURI->GetInnerURI(getter_AddRefs(innerURI));
      NS_ENSURE_SUCCESS(rv, rv);
      return IsSpurious(innerURI, isSpurious);
    }
  }

  return NS_OK;
}






NS_IMETHODIMP
nsDocNavStartProgressListener::OnStateChange(nsIWebProgress *aWebProgress,
                                            nsIRequest *aRequest,
                                            PRUint32 aStateFlags,
                                            nsresult aStatus)
{
  return NS_OK;
}




NS_IMETHODIMP
nsDocNavStartProgressListener::OnProgressChange(nsIWebProgress *aWebProgress,
                                               nsIRequest *aRequest,
                                               PRInt32 aCurSelfProgress, PRInt32 aMaxSelfProgress,
                                               PRInt32 aCurTotalProgress,
                                               PRInt32 aMaxTotalProgress)
{
  return NS_OK;
}




NS_IMETHODIMP
nsDocNavStartProgressListener::OnLocationChange(nsIWebProgress *aWebProgress,
                                               nsIRequest *aRequest,
                                               nsIURI *aLocation)
{
  nsresult rv;
  nsCAutoString uriString;
  nsCOMPtr<nsIURI> uri;

  
  rv = GetRequestUri(aRequest, getter_AddRefs(uri));
  if (NS_FAILED(rv))
    return NS_OK;
  rv = uri->GetAsciiSpec(uriString);
  if (NS_FAILED(rv))
    return NS_OK;

  LOG(("Firing OnLocationChange for %s", uriString.get()));

  
  
  nsCOMPtr<nsITimer> timer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  NS_ENSURE_TRUE(timer, rv);

  rv = timer->Init(this, mDelay, nsITimer::TYPE_ONE_SHOT);
  NS_ENSURE_SUCCESS(rv, rv);

  mRequests.AppendObject(aRequest);
  mTimers.AppendObject(timer);

  return NS_OK;
}




NS_IMETHODIMP
nsDocNavStartProgressListener::OnStatusChange(nsIWebProgress *aWebProgress,
                                             nsIRequest *aRequest,
                                             nsresult aStatus,
                                             const PRUnichar *aMessage)
{
  return NS_OK;
}




NS_IMETHODIMP
nsDocNavStartProgressListener::OnSecurityChange(nsIWebProgress *aWebProgress,
                                               nsIRequest *aRequest,
                                               PRUint32 aState)
{
  return NS_OK;
}



NS_IMETHODIMP
nsDocNavStartProgressListener::Observe(nsISupports *subject, const char *topic,
                                       const PRUnichar *data)
{
  if (strcmp(topic, NS_TIMER_CALLBACK_TOPIC) == 0) {
    
#ifdef DEBUG
    PRUint32 length = mRequests.Count();
    NS_ASSERTION(length > 0, "timer callback with empty request queue?");
    length = mTimers.Count();
    NS_ASSERTION(length > 0, "timer callback with empty timer queue?");
#endif

    nsIRequest* request = mRequests[0];

    if (mCallback) {
      PRBool isSpurious;

      nsCOMPtr<nsIURI> uri;
      nsresult rv = GetRequestUri(request, getter_AddRefs(uri));
      NS_ENSURE_SUCCESS(rv, rv);
      
      rv = IsSpurious(uri, &isSpurious);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!isSpurious) {
        nsCString uriString;
        rv = uri->GetAsciiSpec(uriString);
        NS_ENSURE_SUCCESS(rv, rv);
        
        
        PRInt32 pos = uriString.FindChar('#');
        if (pos > -1) {
          uriString.SetLength(pos);
        }
        
        LOG(("Firing DocNavStart for %s", uriString.get()));
        mCallback->OnDocNavStart(request, uriString);
      }
    }

    mRequests.RemoveObjectAt(0);
    mTimers.RemoveObjectAt(0);
  }
  return NS_OK;
}
