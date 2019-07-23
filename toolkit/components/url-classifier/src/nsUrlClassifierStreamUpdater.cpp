





































#include "nsCRT.h"
#include "nsIObserverService.h"
#include "nsIURI.h"
#include "nsIUrlClassifierDBService.h"
#include "nsStreamUtils.h"
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




class nsUrlClassifierStreamUpdater;

class TableUpdateListener : public nsIStreamListener
{
public:
  TableUpdateListener(nsIUrlClassifierCallback *aTableCallback,
                      nsIUrlClassifierCallback *aErrorCallback,
                      nsUrlClassifierStreamUpdater* aStreamUpdater);
  nsCOMPtr<nsIUrlClassifierDBService> mDBService;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER

private:
  ~TableUpdateListener() {}

  
  nsCOMPtr<nsIUrlClassifierCallback> mTableCallback;
  nsCOMPtr<nsIUrlClassifierCallback> mErrorCallback;

  
  nsUrlClassifierStreamUpdater *mStreamUpdater;
};

TableUpdateListener::TableUpdateListener(
                                nsIUrlClassifierCallback *aTableCallback,
                                nsIUrlClassifierCallback *aErrorCallback,
                                nsUrlClassifierStreamUpdater* aStreamUpdater)
{
  mTableCallback = aTableCallback;
  mErrorCallback = aErrorCallback;
  mStreamUpdater = aStreamUpdater;
}

NS_IMPL_ISUPPORTS2(TableUpdateListener, nsIStreamListener, nsIRequestObserver)

NS_IMETHODIMP
TableUpdateListener::OnStartRequest(nsIRequest *request, nsISupports* context)
{
  nsresult rv;
  if (!mDBService) {
    mDBService = do_GetService(NS_URLCLASSIFIERDBSERVICE_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
  NS_ENSURE_STATE(httpChannel);

  nsresult status;
  rv = httpChannel->GetStatus(&status);
  NS_ENSURE_SUCCESS(rv, rv);
  if (NS_ERROR_CONNECTION_REFUSED == status ||
      NS_ERROR_NET_TIMEOUT == status) {
    
    mErrorCallback->HandleEvent(nsCString());
    return NS_ERROR_ABORT;
  }

  return NS_OK;
}

NS_IMETHODIMP
TableUpdateListener::OnDataAvailable(nsIRequest *request,
                                     nsISupports* context,
                                     nsIInputStream *aIStream,
                                     PRUint32 aSourceOffset,
                                     PRUint32 aLength)
{
  if (!mDBService)
    return NS_ERROR_NOT_INITIALIZED;

  LOG(("OnDataAvailable (%d bytes)", aLength));

  
  nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface(request);
  NS_ENSURE_STATE(httpChannel);

  nsresult rv;
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
    mErrorCallback->HandleEvent(strStatus);
    return NS_ERROR_ABORT;
  }

  
  nsCString chunk;
  rv = NS_ConsumeStream(aIStream, aLength, chunk);
  NS_ENSURE_SUCCESS(rv, rv);

  

  rv = mDBService->Update(chunk);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
TableUpdateListener::OnStopRequest(nsIRequest *request, nsISupports* context,
                                   nsresult aStatus)
{
  if (!mDBService)
    return NS_ERROR_NOT_INITIALIZED;

  LOG(("OnStopRequest status: %d", aStatus));

  
  
  if (NS_SUCCEEDED(aStatus))
    mDBService->Finish(mTableCallback);
  else
    mDBService->CancelStream();

  mStreamUpdater->DownloadDone();

  return NS_OK;
}





nsUrlClassifierStreamUpdater::nsUrlClassifierStreamUpdater()
  : mIsUpdating(PR_FALSE), mInitialized(PR_FALSE), mUpdateUrl(nsnull),
   mListener(nsnull), mChannel(nsnull)
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierStreamUpdaterLog)
    gUrlClassifierStreamUpdaterLog = PR_NewLogModule("UrlClassifierStreamUpdater");
#endif

}

NS_IMPL_ISUPPORTS2(nsUrlClassifierStreamUpdater,
                   nsIUrlClassifierStreamUpdater,
                   nsIObserver)




void
nsUrlClassifierStreamUpdater::DownloadDone()
{
  mIsUpdating = PR_FALSE;
  mChannel = nsnull;
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
  nsresult rv = NS_NewURI(getter_AddRefs(mUpdateUrl), aUpdateUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierStreamUpdater::DownloadUpdates(
                                nsIUrlClassifierCallback *aTableCallback,
                                nsIUrlClassifierCallback *aErrorCallback,
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

  if (!mInitialized) {
    
    
    
    nsCOMPtr<nsIObserverService> observerService =
        do_GetService("@mozilla.org/observer-service;1");
    if (!observerService)
      return NS_ERROR_FAILURE;

    observerService->AddObserver(this, gQuitApplicationMessage, PR_FALSE);
    mInitialized = PR_TRUE;
  }

  
  nsresult rv;
  rv = NS_NewChannel(getter_AddRefs(mChannel), mUpdateUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  
  mListener = new TableUpdateListener(aTableCallback, aErrorCallback, this);

  
  rv = mChannel->AsyncOpen(mListener.get(), nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  mIsUpdating = PR_TRUE;
  *_retval = PR_TRUE;

  return NS_OK;
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
