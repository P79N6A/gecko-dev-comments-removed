





































#include "nsPACMan.h"
#include "nsThreadUtils.h"
#include "nsIDNSService.h"
#include "nsIDNSListener.h"
#include "nsICancelable.h"
#include "nsIAuthPrompt.h"
#include "nsIPromptFactory.h"
#include "nsIHttpChannel.h"
#include "nsIPrefService.h"
#include "nsIPrefBranch.h"
#include "nsNetUtil.h"
#include "nsAutoPtr.h"
#include "nsCRT.h"
#include "prmon.h"
#include "nsIAsyncVerifyRedirectCallback.h"





static bool
HttpRequestSucceeded(nsIStreamLoader *loader)
{
  nsCOMPtr<nsIRequest> request;
  loader->GetRequest(getter_AddRefs(request));

  bool result = true;  

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
  if (httpChannel)
    httpChannel->GetRequestSucceeded(&result);

  return result;
}





class PendingPACQuery : public PRCList, public nsIDNSListener
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDNSLISTENER

  PendingPACQuery(nsPACMan *pacMan, nsIURI *uri, nsPACManCallback *callback)
    : mPACMan(pacMan)
    , mURI(uri)
    , mCallback(callback)
  {
    PR_INIT_CLIST(this);
  }

  nsresult Start(PRUint32 flags);
  void     Complete(nsresult status, const nsCString &pacString);

private:
  nsPACMan                  *mPACMan;  
  nsCOMPtr<nsIURI>           mURI;
  nsRefPtr<nsPACManCallback> mCallback;
  nsCOMPtr<nsICancelable>    mDNSRequest;
};


NS_IMPL_THREADSAFE_ISUPPORTS1(PendingPACQuery, nsIDNSListener)

nsresult
PendingPACQuery::Start(PRUint32 flags)
{
  if (mDNSRequest)
    return NS_OK;  

  nsresult rv;
  nsCOMPtr<nsIDNSService> dns = do_GetService(NS_DNSSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    NS_WARNING("unable to get the DNS service");
    return rv;
  }

  nsCAutoString host;
  rv = mURI->GetAsciiHost(host);
  if (NS_FAILED(rv))
    return rv;

  rv = dns->AsyncResolve(host, flags, this, NS_GetCurrentThread(),
                         getter_AddRefs(mDNSRequest));
  if (NS_FAILED(rv))
    NS_WARNING("DNS AsyncResolve failed");

  return rv;
}


void
PendingPACQuery::Complete(nsresult status, const nsCString &pacString)
{
  if (!mCallback)
    return;

  mCallback->OnQueryComplete(status, pacString);
  mCallback = nsnull;

  if (mDNSRequest) {
    mDNSRequest->Cancel(NS_ERROR_ABORT);
    mDNSRequest = nsnull;
  }
}

NS_IMETHODIMP
PendingPACQuery::OnLookupComplete(nsICancelable *request,
                                  nsIDNSRecord *record,
                                  nsresult status)
{
  
  
 
  mDNSRequest = nsnull;  

  
  if (!mCallback)
    return NS_OK;

  
  PR_REMOVE_LINK(this);

  nsCAutoString pacString;
  status = mPACMan->GetProxyForURI(mURI, pacString);
  Complete(status, pacString);

  NS_RELEASE_THIS();
  return NS_OK;
}



nsPACMan::nsPACMan()
  : mLoadPending(false)
  , mShutdown(false)
  , mScheduledReload(LL_MAXINT)
  , mLoadFailureCount(0)
{
  PR_INIT_CLIST(&mPendingQ);
}

nsPACMan::~nsPACMan()
{
  NS_ASSERTION(mLoader == nsnull, "pac man not shutdown properly");
  NS_ASSERTION(mPAC == nsnull, "pac man not shutdown properly");
  NS_ASSERTION(PR_CLIST_IS_EMPTY(&mPendingQ), "pac man not shutdown properly");
}

void
nsPACMan::Shutdown()
{
  CancelExistingLoad();
  ProcessPendingQ(NS_ERROR_ABORT);

  mPAC = nsnull;
  mShutdown = true;
}

nsresult
nsPACMan::GetProxyForURI(nsIURI *uri, nsACString &result)
{
  NS_ENSURE_STATE(!mShutdown);

  if (IsPACURI(uri)) {
    result.Truncate();
    return NS_OK;
  }

  MaybeReloadPAC();

  if (IsLoading())
    return NS_ERROR_IN_PROGRESS;
  if (!mPAC)
    return NS_ERROR_NOT_AVAILABLE;

  nsCAutoString spec, host;
  uri->GetAsciiSpec(spec);
  uri->GetAsciiHost(host);

  return mPAC->GetProxyForURI(spec, host, result);
}

nsresult
nsPACMan::AsyncGetProxyForURI(nsIURI *uri, nsPACManCallback *callback)
{
  NS_ENSURE_STATE(!mShutdown);

  MaybeReloadPAC();

  PendingPACQuery *query = new PendingPACQuery(this, uri, callback);
  if (!query)
    return NS_ERROR_OUT_OF_MEMORY;
  NS_ADDREF(query);
  PR_APPEND_LINK(query, &mPendingQ);

  
  
  
  
  
  int isPACURI = IsPACURI(uri);

  if (IsLoading() && !isPACURI)
    return NS_OK;

  nsresult rv = query->Start(isPACURI ? 0 : nsIDNSService::RESOLVE_SPECULATE);
  if (rv == NS_ERROR_DNS_LOOKUP_QUEUE_FULL && !isPACURI) {
    query->OnLookupComplete(NULL, NULL, NS_OK);
    rv = NS_OK;
  } else if (NS_FAILED(rv)) {
    NS_WARNING("failed to start PAC query");
    PR_REMOVE_LINK(query);
    NS_RELEASE(query);
  }

  return rv;
}

nsresult
nsPACMan::LoadPACFromURI(nsIURI *pacURI)
{
  NS_ENSURE_STATE(!mShutdown);
  NS_ENSURE_ARG(pacURI || mPACURI);

  nsCOMPtr<nsIStreamLoader> loader =
      do_CreateInstance(NS_STREAMLOADER_CONTRACTID);
  NS_ENSURE_STATE(loader);

  
  
  
  
  

  if (!mLoadPending) {
    nsCOMPtr<nsIRunnable> event =
      NS_NewRunnableMethod(this, &nsPACMan::StartLoading);
    nsresult rv;
    if (NS_FAILED(rv = NS_DispatchToCurrentThread(event)))
      return rv;
    mLoadPending = true;
  }

  CancelExistingLoad();

  mLoader = loader;
  if (pacURI) {
    mPACURI = pacURI;
    mLoadFailureCount = 0;  
  }
  mScheduledReload = LL_MAXINT;
  mPAC = nsnull;
  return NS_OK;
}

void
nsPACMan::StartLoading()
{
  mLoadPending = false;

  
  if (!mLoader) {
    ProcessPendingQ(NS_ERROR_ABORT);
    return;
  }

  if (NS_SUCCEEDED(mLoader->Init(this))) {
    
    nsCOMPtr<nsIIOService> ios = do_GetIOService();
    if (ios) {
      nsCOMPtr<nsIChannel> channel;

      
      ios->NewChannelFromURI(mPACURI, getter_AddRefs(channel));

      if (channel) {
        channel->SetLoadFlags(nsIRequest::LOAD_BYPASS_CACHE);
        channel->SetNotificationCallbacks(this);
        if (NS_SUCCEEDED(channel->AsyncOpen(mLoader, nsnull)))
          return;
      }
    }
  }

  CancelExistingLoad();
  ProcessPendingQ(NS_ERROR_UNEXPECTED);
}

void
nsPACMan::MaybeReloadPAC()
{
  if (!mPACURI)
    return;

  if (PR_Now() > mScheduledReload)
    LoadPACFromURI(nsnull);
}

void
nsPACMan::OnLoadFailure()
{
  PRInt32 minInterval = 5;    
  PRInt32 maxInterval = 300;  

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  if (prefs) {
    prefs->GetIntPref("network.proxy.autoconfig_retry_interval_min",
                      &minInterval);
    prefs->GetIntPref("network.proxy.autoconfig_retry_interval_max",
                      &maxInterval);
  }

  PRInt32 interval = minInterval << mLoadFailureCount++;  
  if (!interval || interval > maxInterval)
    interval = maxInterval;

#ifdef DEBUG
  printf("PAC load failure: will retry in %d seconds\n", interval);
#endif

  mScheduledReload = PR_Now() + PRInt64(interval) * PR_USEC_PER_SEC;
}

void
nsPACMan::CancelExistingLoad()
{
  if (mLoader) {
    nsCOMPtr<nsIRequest> request;
    mLoader->GetRequest(getter_AddRefs(request));
    if (request)
      request->Cancel(NS_ERROR_ABORT);
    mLoader = nsnull;
  }
}

void
nsPACMan::ProcessPendingQ(nsresult status)
{
  
  PRCList *node = PR_LIST_HEAD(&mPendingQ);
  while (node != &mPendingQ) {
    PendingPACQuery *query = static_cast<PendingPACQuery *>(node);
    node = PR_NEXT_LINK(node);
    if (NS_SUCCEEDED(status)) {
      
      
      status = query->Start(nsIDNSService::RESOLVE_SPECULATE);
    }
    if (status == NS_ERROR_DNS_LOOKUP_QUEUE_FULL) {
      query->OnLookupComplete(NULL, NULL, NS_OK);
      status = NS_OK;
    } else if (NS_FAILED(status)) {
      
      PR_REMOVE_LINK(query);
      query->Complete(status, EmptyCString());
      NS_RELEASE(query);
    }
  }
}

NS_IMPL_ISUPPORTS3(nsPACMan, nsIStreamLoaderObserver, nsIInterfaceRequestor,
                   nsIChannelEventSink)

NS_IMETHODIMP
nsPACMan::OnStreamComplete(nsIStreamLoader *loader,
                           nsISupports *context,
                           nsresult status,
                           PRUint32 dataLen,
                           const PRUint8 *data)
{
  if (mLoader != loader) {
    
    
    
    
    if (status == NS_ERROR_ABORT)
      return NS_OK;
  }

  mLoader = nsnull;

  if (NS_SUCCEEDED(status) && HttpRequestSucceeded(loader)) {
    
    nsCAutoString pacURI;
    {
      nsCOMPtr<nsIRequest> request;
      loader->GetRequest(getter_AddRefs(request));
      nsCOMPtr<nsIChannel> channel = do_QueryInterface(request);
      if (channel) {
        nsCOMPtr<nsIURI> uri;
        channel->GetURI(getter_AddRefs(uri));
        if (uri)
          uri->GetAsciiSpec(pacURI);
      }
    }

    if (!mPAC) {
      mPAC = do_CreateInstance(NS_PROXYAUTOCONFIG_CONTRACTID, &status);
      if (!mPAC)
        NS_WARNING("failed to instantiate PAC component");
    }
    if (NS_SUCCEEDED(status)) {
      
      
      
      const char *text = (const char *) data;
      status = mPAC->Init(pacURI, NS_ConvertASCIItoUTF16(text, dataLen));
    }

    
    
    mLoadFailureCount = 0;
  } else {
    
    
    OnLoadFailure();
  }

  
  if (mPAC && NS_FAILED(status))
    mPAC = nsnull;

  ProcessPendingQ(status);
  return NS_OK;
}

NS_IMETHODIMP
nsPACMan::GetInterface(const nsIID &iid, void **result)
{
  
  if (iid.Equals(NS_GET_IID(nsIAuthPrompt))) {
    nsCOMPtr<nsIPromptFactory> promptFac = do_GetService("@mozilla.org/prompter;1");
    NS_ENSURE_TRUE(promptFac, NS_ERROR_FAILURE);
    return promptFac->GetPrompt(nsnull, iid, reinterpret_cast<void**>(result));
  }

  
  if (iid.Equals(NS_GET_IID(nsIChannelEventSink))) {
    NS_ADDREF_THIS();
    *result = static_cast<nsIChannelEventSink *>(this);
    return NS_OK;
  }

  return NS_ERROR_NO_INTERFACE;
}

NS_IMETHODIMP
nsPACMan::AsyncOnChannelRedirect(nsIChannel *oldChannel, nsIChannel *newChannel,
                                 PRUint32 flags,
                                 nsIAsyncVerifyRedirectCallback *callback)
{
  nsresult rv = NS_OK;
  if (NS_FAILED((rv = newChannel->GetURI(getter_AddRefs(mPACURI)))))
      return rv;

  callback->OnRedirectVerifyCallback(NS_OK);
  return NS_OK;
}
