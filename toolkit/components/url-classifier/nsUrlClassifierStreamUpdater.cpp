




#include "nsCRT.h"
#include "nsIHttpChannel.h"
#include "nsIObserverService.h"
#include "nsIStringStream.h"
#include "nsIUploadChannel.h"
#include "nsIURI.h"
#include "nsIUrlClassifierDBService.h"
#include "nsStreamUtils.h"
#include "nsStringStream.h"
#include "nsToolkitCompsCID.h"
#include "nsUrlClassifierStreamUpdater.h"
#include "prlog.h"

static const char* gQuitApplicationMessage = "quit-application";


#if defined(PR_LOGGING)
static const PRLogModuleInfo *gUrlClassifierStreamUpdaterLog = nullptr;
#define LOG(args) PR_LOG(gUrlClassifierStreamUpdaterLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif






nsUrlClassifierStreamUpdater::nsUrlClassifierStreamUpdater()
  : mIsUpdating(false), mInitialized(false), mDownloadError(false),
    mBeganStream(false), mUpdateUrl(nullptr), mChannel(nullptr)
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierStreamUpdaterLog)
    gUrlClassifierStreamUpdaterLog = PR_NewLogModule("UrlClassifierStreamUpdater");
#endif

}

NS_IMPL_THREADSAFE_ISUPPORTS9(nsUrlClassifierStreamUpdater,
                              nsIUrlClassifierStreamUpdater,
                              nsIUrlClassifierUpdateObserver,
                              nsIRequestObserver,
                              nsIStreamListener,
                              nsIObserver,
                              nsIBadCertListener2,
                              nsISSLErrorListener,
                              nsIInterfaceRequestor,
                              nsITimerCallback)




void
nsUrlClassifierStreamUpdater::DownloadDone()
{
  LOG(("nsUrlClassifierStreamUpdater::DownloadDone [this=%p]", this));
  mIsUpdating = false;

  mPendingUpdates.Clear();
  mDownloadError = false;
  mSuccessCallback = nullptr;
  mUpdateErrorCallback = nullptr;
  mDownloadErrorCallback = nullptr;
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::GetUpdateUrl(nsACString & aUpdateUrl)
{
  if (mUpdateUrl) {
    mUpdateUrl->GetSpec(aUpdateUrl);
  } else {
    aUpdateUrl.Truncate();
  }
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::SetUpdateUrl(const nsACString & aUpdateUrl)
{
  LOG(("Update URL is %s\n", PromiseFlatCString(aUpdateUrl).get()));

  nsresult rv = NS_NewURI(getter_AddRefs(mUpdateUrl), aUpdateUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsUrlClassifierStreamUpdater::FetchUpdate(nsIURI *aUpdateUrl,
                                          const nsACString & aRequestBody,
                                          const nsACString & aStreamTable,
                                          const nsACString & aServerMAC)
{
  nsresult rv;
  uint32_t loadFlags = nsIChannel::INHIBIT_CACHING |
                       nsIChannel::LOAD_BYPASS_CACHE;
  rv = NS_NewChannel(getter_AddRefs(mChannel), aUpdateUrl, nullptr, nullptr, this,
                     loadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  mBeganStream = false;

  if (!aRequestBody.IsEmpty()) {
    rv = AddRequestBody(aRequestBody);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  
  bool match;
  if ((NS_SUCCEEDED(aUpdateUrl->SchemeIs("file", &match)) && match) ||
      (NS_SUCCEEDED(aUpdateUrl->SchemeIs("data", &match)) && match)) {
    mChannel->SetContentType(NS_LITERAL_CSTRING("application/vnd.google.safebrowsing-update"));
  }

  
  rv = mChannel->AsyncOpen(this, nullptr);
  NS_ENSURE_SUCCESS(rv, rv);

  mStreamTable = aStreamTable;
  mServerMAC = aServerMAC;

  return NS_OK;
}

nsresult
nsUrlClassifierStreamUpdater::FetchUpdate(const nsACString & aUpdateUrl,
                                          const nsACString & aRequestBody,
                                          const nsACString & aStreamTable,
                                          const nsACString & aServerMAC)
{
  LOG(("(pre) Fetching update from %s\n", PromiseFlatCString(aUpdateUrl).get()));

  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUpdateUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString urlSpec;
  uri->GetAsciiSpec(urlSpec);

  LOG(("(post) Fetching update from %s\n", urlSpec.get()));

  return FetchUpdate(uri, aRequestBody, aStreamTable, aServerMAC);
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::DownloadUpdates(
                                const nsACString &aRequestTables,
                                const nsACString &aRequestBody,
                                const nsACString &aClientKey,
                                nsIUrlClassifierCallback *aSuccessCallback,
                                nsIUrlClassifierCallback *aUpdateErrorCallback,
                                nsIUrlClassifierCallback *aDownloadErrorCallback,
                                bool *_retval)
{
  NS_ENSURE_ARG(aSuccessCallback);
  NS_ENSURE_ARG(aUpdateErrorCallback);
  NS_ENSURE_ARG(aDownloadErrorCallback);

  if (mIsUpdating) {
    LOG(("already updating, skipping update"));
    *_retval = false;
    return NS_OK;
  }

  if (!mUpdateUrl) {
    NS_ERROR("updateUrl not set");
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv;

  if (!mInitialized) {
    
    
    
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService)
      return NS_ERROR_FAILURE;

    observerService->AddObserver(this, gQuitApplicationMessage, false);

    mDBService = do_GetService(NS_URLCLASSIFIERDBSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mInitialized = true;
  }

  rv = mDBService->BeginUpdate(this, aRequestTables, aClientKey);
  if (rv == NS_ERROR_NOT_AVAILABLE) {
    LOG(("already updating, skipping update"));
    *_retval = false;
    return NS_OK;
  } else if (NS_FAILED(rv)) {
    return rv;
  }

  mSuccessCallback = aSuccessCallback;
  mUpdateErrorCallback = aUpdateErrorCallback;
  mDownloadErrorCallback = aDownloadErrorCallback;

  mIsUpdating = true;
  *_retval = true;

  nsCAutoString urlSpec;
  mUpdateUrl->GetAsciiSpec(urlSpec);

  LOG(("FetchUpdate: %s", urlSpec.get()));
  

  return FetchUpdate(mUpdateUrl, aRequestBody, EmptyCString(), EmptyCString());
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::UpdateUrlRequested(const nsACString &aUrl,
                                                 const nsACString &aTable,
                                                 const nsACString &aServerMAC)
{
  LOG(("Queuing requested update from %s\n", PromiseFlatCString(aUrl).get()));

  PendingUpdate *update = mPendingUpdates.AppendElement();
  if (!update)
    return NS_ERROR_OUT_OF_MEMORY;

  
  if (StringBeginsWith(aUrl, NS_LITERAL_CSTRING("data:")) ||
      StringBeginsWith(aUrl, NS_LITERAL_CSTRING("file:"))) {
    update->mUrl = aUrl;
  } else {
    update->mUrl = NS_LITERAL_CSTRING("http://") + aUrl;
  }
  update->mTable = aTable;
  update->mServerMAC = aServerMAC;

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::RekeyRequested()
{
  nsCOMPtr<nsIObserverService> observerService =
    mozilla::services::GetObserverService();

  if (!observerService)
    return NS_ERROR_FAILURE;

  return observerService->NotifyObservers(static_cast<nsIUrlClassifierStreamUpdater*>(this),
                                          "url-classifier-rekey-requested",
                                          nullptr);
}

nsresult
nsUrlClassifierStreamUpdater::FetchNext()
{
  if (mPendingUpdates.Length() == 0) {
    return NS_OK;
  }

  PendingUpdate &update = mPendingUpdates[0];
  LOG(("Fetching update url: %s\n", update.mUrl.get()));
  nsresult rv = FetchUpdate(update.mUrl, EmptyCString(),
                            update.mTable, update.mServerMAC);
  if (NS_FAILED(rv)) {
    LOG(("Error fetching update url: %s\n", update.mUrl.get()));
    
    
    mDownloadErrorCallback->HandleEvent(EmptyCString());
    mDownloadError = true;
    mDBService->FinishUpdate();
    return rv;
  }

  mPendingUpdates.RemoveElementAt(0);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::StreamFinished(nsresult status,
                                             uint32_t requestedDelay)
{
  LOG(("nsUrlClassifierStreamUpdater::StreamFinished [%x, %d]", status, requestedDelay));
  if (NS_FAILED(status) || mPendingUpdates.Length() == 0) {
    
    mDBService->FinishUpdate();
    return NS_OK;
  }

  
  nsresult rv;
  mTimer = do_CreateInstance("@mozilla.org/timer;1", &rv);
  if (NS_SUCCEEDED(rv)) {
    rv = mTimer->InitWithCallback(this, requestedDelay,
                                  nsITimer::TYPE_ONE_SHOT);
  }

  if (NS_FAILED(rv)) {
    NS_WARNING("Unable to initialize timer, fetching next safebrowsing item immediately");
    return FetchNext();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::UpdateSuccess(uint32_t requestedTimeout)
{
  LOG(("nsUrlClassifierStreamUpdater::UpdateSuccess [this=%p]", this));
  if (mPendingUpdates.Length() != 0) {
    NS_WARNING("Didn't fetch all safebrowsing update redirects");
  }

  
  nsCOMPtr<nsIUrlClassifierCallback> successCallback = mDownloadError ? nullptr : mSuccessCallback.get();
  DownloadDone();

  nsCAutoString strTimeout;
  strTimeout.AppendInt(requestedTimeout);
  if (successCallback) {
    successCallback->HandleEvent(strTimeout);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::UpdateError(nsresult result)
{
  LOG(("nsUrlClassifierStreamUpdater::UpdateError [this=%p]", this));

  
  nsCOMPtr<nsIUrlClassifierCallback> errorCallback = mDownloadError ? nullptr : mUpdateErrorCallback.get();

  DownloadDone();

  nsCAutoString strResult;
  strResult.AppendInt(static_cast<uint32_t>(result));
  if (errorCallback) {
    errorCallback->HandleEvent(strResult);
  }

  return NS_OK;
}

nsresult
nsUrlClassifierStreamUpdater::AddRequestBody(const nsACString &aRequestBody)
{
  nsresult rv;
  nsCOMPtr<nsIStringInputStream> strStream =
    do_CreateInstance(NS_STRINGINPUTSTREAM_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = strStream->SetData(aRequestBody.BeginReading(),
                          aRequestBody.Length());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIUploadChannel> uploadChannel = do_QueryInterface(mChannel, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = uploadChannel->SetUploadStream(strStream,
                                      NS_LITERAL_CSTRING("text/plain"),
                                      -1);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(mChannel, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = httpChannel->SetRequestMethod(NS_LITERAL_CSTRING("POST"));
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}





NS_IMETHODIMP
nsUrlClassifierStreamUpdater::OnStartRequest(nsIRequest *request,
                                             nsISupports* context)
{
  nsresult rv;
  bool downloadError = false;
  nsCAutoString strStatus;
  nsresult status = NS_OK;

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
  if (httpChannel) {
    rv = httpChannel->GetStatus(&status);
    NS_ENSURE_SUCCESS(rv, rv);

    if (NS_ERROR_CONNECTION_REFUSED == status ||
        NS_ERROR_NET_TIMEOUT == status) {
      
      downloadError = true;
    }

    if (NS_SUCCEEDED(status)) {
      bool succeeded = false;
      rv = httpChannel->GetRequestSucceeded(&succeeded);
      NS_ENSURE_SUCCESS(rv, rv);

      if (!succeeded) {
        
        LOG(("HTTP request returned failure code."));

        uint32_t requestStatus;
        rv = httpChannel->GetResponseStatus(&requestStatus);
        NS_ENSURE_SUCCESS(rv, rv);

        strStatus.AppendInt(requestStatus);
        downloadError = true;
      }
    }
  }

  if (downloadError) {
    mDownloadErrorCallback->HandleEvent(strStatus);
    mDownloadError = true;
    status = NS_ERROR_ABORT;
  } else if (NS_SUCCEEDED(status)) {
    mBeganStream = true;
    rv = mDBService->BeginStream(mStreamTable, mServerMAC);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mStreamTable.Truncate();
  mServerMAC.Truncate();

  return status;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::OnDataAvailable(nsIRequest *request,
                                              nsISupports* context,
                                              nsIInputStream *aIStream,
                                              uint32_t aSourceOffset,
                                              uint32_t aLength)
{
  if (!mDBService)
    return NS_ERROR_NOT_INITIALIZED;

  LOG(("OnDataAvailable (%d bytes)", aLength));

  nsresult rv;

  
  nsCString chunk;
  rv = NS_ConsumeStream(aIStream, aLength, chunk);
  NS_ENSURE_SUCCESS(rv, rv);

  

  rv = mDBService->UpdateStream(chunk);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::OnStopRequest(nsIRequest *request, nsISupports* context,
                                            nsresult aStatus)
{
  if (!mDBService)
    return NS_ERROR_NOT_INITIALIZED;

  LOG(("OnStopRequest (status %x)", aStatus));

  nsresult rv;

  if (NS_SUCCEEDED(aStatus)) {
    
    rv = mDBService->FinishStream();
  } else if (mBeganStream) {
    
    
    rv = mDBService->CancelUpdate();
  } else {
    
    
    
    rv = mDBService->FinishUpdate();
  }

  mChannel = nullptr;

  return rv;
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::Observe(nsISupports *aSubject, const char *aTopic,
                                      const PRUnichar *aData)
{
  if (nsCRT::strcmp(aTopic, gQuitApplicationMessage) == 0) {
    if (mIsUpdating && mChannel) {
      LOG(("Cancel download"));
      nsresult rv;
      rv = mChannel->Cancel(NS_ERROR_ABORT);
      NS_ENSURE_SUCCESS(rv, rv);
      mIsUpdating = false;
      mChannel = nullptr;
    }
    if (mTimer) {
      mTimer->Cancel();
      mTimer = nullptr;
    }
  }
  return NS_OK;
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::NotifyCertProblem(nsIInterfaceRequestor *socketInfo, 
                                                nsISSLStatus *status, 
                                                const nsACString &targetSite, 
                                                bool *_retval)
{
  *_retval = true;
  return NS_OK;
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::NotifySSLError(nsIInterfaceRequestor *socketInfo, 
                                             int32_t error, 
                                             const nsACString &targetSite, 
                                             bool *_retval)
{
  *_retval = true;
  return NS_OK;
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::GetInterface(const nsIID & eventSinkIID, void* *_retval)
{
  return QueryInterface(eventSinkIID, _retval);
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::Notify(nsITimer *timer)
{
  LOG(("nsUrlClassifierStreamUpdater::Notify [%p]", this));

  mTimer = nullptr;

  
  FetchNext();

  return NS_OK;
}

