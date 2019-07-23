





































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
static const PRLogModuleInfo *gUrlClassifierStreamUpdaterLog = nsnull;
#define LOG(args) PR_LOG(gUrlClassifierStreamUpdaterLog, PR_LOG_DEBUG, args)
#else
#define LOG(args)
#endif






nsUrlClassifierStreamUpdater::nsUrlClassifierStreamUpdater()
  : mIsUpdating(PR_FALSE), mInitialized(PR_FALSE), mUpdateUrl(nsnull),
    mChannel(nsnull)
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierStreamUpdaterLog)
    gUrlClassifierStreamUpdaterLog = PR_NewLogModule("UrlClassifierStreamUpdater");
#endif

}

NS_IMPL_ISUPPORTS5(nsUrlClassifierStreamUpdater,
                   nsIUrlClassifierStreamUpdater,
                   nsIUrlClassifierUpdateObserver,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIObserver)




void
nsUrlClassifierStreamUpdater::DownloadDone()
{
  LOG(("nsUrlClassifierStreamUpdater::DownloadDone [this=%p]", this));
  mIsUpdating = PR_FALSE;

  mPendingUpdateUrls.Clear();
  mSuccessCallback = nsnull;
  mUpdateErrorCallback = nsnull;
  mDownloadErrorCallback = nsnull;
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
                                          const nsACString & aRequestBody)
{
  nsresult rv;
  rv = NS_NewChannel(getter_AddRefs(mChannel), aUpdateUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  if (!aRequestBody.IsEmpty()) {
    rv = AddRequestBody(aRequestBody);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  
  rv = mChannel->AsyncOpen(this, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsUrlClassifierStreamUpdater::FetchUpdate(const nsACString & aUpdateUrl,
                                          const nsACString & aRequestBody)
{
  nsCOMPtr<nsIURI> uri;
  nsresult rv = NS_NewURI(getter_AddRefs(uri), aUpdateUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  return FetchUpdate(uri, aRequestBody);
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::DownloadUpdates(
                                const nsACString &aRequestBody,
                                nsIUrlClassifierCallback *aSuccessCallback,
                                nsIUrlClassifierCallback *aUpdateErrorCallback,
                                nsIUrlClassifierCallback *aDownloadErrorCallback,
                                PRBool *_retval)
{
  if (mIsUpdating) {
    LOG(("already updating, skipping update"));
    *_retval = PR_FALSE;
    return NS_OK;
  }

  if (!mUpdateUrl) {
    NS_ERROR("updateUrl not set");
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsresult rv;

  if (!mInitialized) {
    
    
    
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");
    if (!observerService)
      return NS_ERROR_FAILURE;

    observerService->AddObserver(this, gQuitApplicationMessage, PR_FALSE);

    mDBService = do_GetService(NS_URLCLASSIFIERDBSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    mInitialized = PR_TRUE;
  }

  mSuccessCallback = aSuccessCallback;
  mUpdateErrorCallback = aUpdateErrorCallback;
  mDownloadErrorCallback = aDownloadErrorCallback;

  mIsUpdating = PR_TRUE;
  *_retval = PR_TRUE;

  mDBService->BeginUpdate(this);

  return FetchUpdate(mUpdateUrl, aRequestBody);
}




NS_IMETHODIMP
nsUrlClassifierStreamUpdater::UpdateUrlRequested(const nsACString &aUrl)
{
  LOG(("Queuing requested update from %s\n", PromiseFlatCString(aUrl).get()));

  
  if (StringBeginsWith(aUrl, NS_LITERAL_CSTRING("data:"))) {
    mPendingUpdateUrls.AppendElement(aUrl);
  } else {
    mPendingUpdateUrls.AppendElement(NS_LITERAL_CSTRING("http://") + aUrl);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::StreamFinished()
{
  nsresult rv;

  
  if (mPendingUpdateUrls.Length() > 0) {
    rv = FetchUpdate(mPendingUpdateUrls[0], NS_LITERAL_CSTRING(""));
    if (NS_FAILED(rv)) {
      LOG(("Error fetching update url: %s\n", mPendingUpdateUrls[0].get()));
      mDBService->CancelUpdate();
      return rv;
    }

    mPendingUpdateUrls.RemoveElementAt(0);
  } else {
    mDBService->FinishUpdate();
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::UpdateSuccess(PRUint32 requestedTimeout)
{
  LOG(("nsUrlClassifierStreamUpdater::UpdateSuccess [this=%p]", this));
  NS_ASSERTION(mPendingUpdateUrls.Length() == 0,
               "Didn't fetch all update URLs.");

  
  nsCOMPtr<nsIUrlClassifierCallback> successCallback = mSuccessCallback;
  DownloadDone();

  nsCAutoString strTimeout;
  strTimeout.AppendInt(requestedTimeout);
  if (successCallback) {
    successCallback->HandleEvent(strTimeout);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::UpdateError(PRUint32 result)
{
  LOG(("nsUrlClassifierStreamUpdater::UpdateSuccess [this=%p]", this));

  
  nsCOMPtr<nsIUrlClassifierCallback> errorCallback = mUpdateErrorCallback;
  DownloadDone();

  nsCAutoString strResult;
  strResult.AppendInt(result);
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
nsUrlClassifierStreamUpdater::OnStartRequest(nsIRequest *request, nsISupports* context)
{
  nsresult rv;

  rv = mDBService->BeginStream();
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
  if (httpChannel) {
    nsresult status;
    rv = httpChannel->GetStatus(&status);
    NS_ENSURE_SUCCESS(rv, rv);

    LOG(("OnStartRequest (status %x)", status));

    if (NS_ERROR_CONNECTION_REFUSED == status ||
        NS_ERROR_NET_TIMEOUT == status) {
      
      mDownloadErrorCallback->HandleEvent(nsCString());
      return NS_ERROR_ABORT;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::OnDataAvailable(nsIRequest *request,
                                              nsISupports* context,
                                              nsIInputStream *aIStream,
                                              PRUint32 aSourceOffset,
                                              PRUint32 aLength)
{
  if (!mDBService)
    return NS_ERROR_NOT_INITIALIZED;

  LOG(("OnDataAvailable (%d bytes)", aLength));

  nsresult rv;

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
  if (httpChannel) {
    PRBool succeeded = PR_FALSE;
    rv = httpChannel->GetRequestSucceeded(&succeeded);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!succeeded) {
      
      LOG(("HTTP request returned failure code."));

      PRUint32 status;
      rv = httpChannel->GetResponseStatus(&status);
      NS_ENSURE_SUCCESS(rv, rv);

      nsCAutoString strStatus;
      strStatus.AppendInt(status);
      mDownloadErrorCallback->HandleEvent(strStatus);
      return NS_ERROR_ABORT;
    }
  }

  
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

  
  
  if (NS_SUCCEEDED(aStatus))
    rv = mDBService->FinishStream();
  else
    rv = mDBService->CancelUpdate();

  mChannel = nsnull;

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
      mIsUpdating = PR_FALSE;
      mChannel = nsnull;
    }
  }
  return NS_OK;
}
