





































#include "nsUrlClassifierHashCompleter.h"
#include "nsIChannel.h"
#include "nsICryptoHMAC.h"
#include "nsIHttpChannel.h"
#include "nsIKeyModule.h"
#include "nsIObserverService.h"
#include "nsIUploadChannel.h"
#include "nsNetUtil.h"
#include "nsStreamUtils.h"
#include "nsStringStream.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsUrlClassifierDBService.h"
#include "nsUrlClassifierUtils.h"
#include "prlog.h"
#include "prprf.h"


#if defined(PR_LOGGING)
static const PRLogModuleInfo *gUrlClassifierHashCompleterLog = nsnull;
#define LOG(args) PR_LOG(gUrlClassifierHashCompleterLog, PR_LOG_DEBUG, args)
#define LOG_ENABLED() PR_LOG_TEST(gUrlClassifierHashCompleterLog, 4)
#else
#define LOG(args)
#define LOG_ENABLED() (PR_FALSE)
#endif



static const PRUint32 gBackoffErrors = 2;

static const PRUint32 gBackoffTime = 5 * 60;

static const PRUint32 gBackoffInterval = 30 * 60;

static const PRUint32 gBackoffMax = 8 * 60 * 60;

NS_IMPL_ISUPPORTS3(nsUrlClassifierHashCompleterRequest,
                   nsIRequestObserver,
                   nsIStreamListener,
                   nsIObserver)

nsresult
nsUrlClassifierHashCompleterRequest::Begin()
{
  LOG(("nsUrlClassifierHashCompleterRequest::Begin [%p]", this));

  if (PR_IntervalNow() < mCompleter->GetNextRequestTime()) {
    NS_WARNING("Gethash server backed off, failing gethash request.");
    NotifyFailure(NS_ERROR_ABORT);
    return NS_ERROR_ABORT;
  }

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_FALSE);

  nsresult rv = OpenChannel();
  if (NS_FAILED(rv)) {
    NotifyFailure(rv);
    return rv;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierHashCompleterRequest::Add(const nsACString& partialHash,
                                         nsIUrlClassifierHashCompleterCallback *c)
{
  LOG(("nsUrlClassifierHashCompleterRequest::Add [%p]", this));
  Request *request = mRequests.AppendElement();
  if (!request)
    return NS_ERROR_OUT_OF_MEMORY;

  request->partialHash = partialHash;
  request->callback = c;

  return NS_OK;
}


nsresult
nsUrlClassifierHashCompleterRequest::OpenChannel()
{
  LOG(("nsUrlClassifierHashCompleterRequest::OpenChannel [%p]", this));
  nsresult rv;

  PRUint32 loadFlags = nsIChannel::INHIBIT_CACHING |
                       nsIChannel::LOAD_BYPASS_CACHE;
  rv = NS_NewChannel(getter_AddRefs(mChannel), mURI, nsnull, nsnull, nsnull,
                     loadFlags);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString requestBody;
  rv = BuildRequest(requestBody);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddRequestBody(requestBody);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = mChannel->AsyncOpen(this, nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

nsresult
nsUrlClassifierHashCompleterRequest::BuildRequest(nsCAutoString &aRequestBody)
{
  LOG(("nsUrlClassifierHashCompleterRequest::BuildRequest [%p]", this));

  nsCAutoString body;
  for (PRUint32 i = 0; i < mRequests.Length(); i++) {
    Request &request = mRequests[i];
    body.Append(request.partialHash);
  }

  aRequestBody.AppendInt(PARTIAL_LENGTH);
  aRequestBody.Append(':');
  aRequestBody.AppendInt(body.Length());
  aRequestBody.Append('\n');
  aRequestBody.Append(body);

  return NS_OK;
}

nsresult
nsUrlClassifierHashCompleterRequest::AddRequestBody(const nsACString &aRequestBody)
{
  LOG(("nsUrlClassifierHashCompleterRequest::AddRequestBody [%p]", this));

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

void
nsUrlClassifierHashCompleterRequest::RescheduleItems()
{
  
  
  for (PRUint32 i = 0; i < mRequests.Length(); i++) {
    Request &request = mRequests[i];
    nsresult rv = mCompleter->Complete(request.partialHash, request.callback);
    if (NS_FAILED(rv)) {
      
      
      request.callback->CompletionFinished(rv);
    }
  }

  mRescheduled = PR_TRUE;
}





nsresult
nsUrlClassifierHashCompleterRequest::HandleMAC(nsACString::const_iterator& begin,
                                               const nsACString::const_iterator& end)
{
  mVerified = PR_FALSE;

  
  nsACString::const_iterator iter = begin;
  if (!FindCharInReadable('\n', iter, end)) {
    return NS_ERROR_FAILURE;
  }

  nsCAutoString serverMAC(Substring(begin, iter++));
  begin = iter;

  if (serverMAC.EqualsLiteral("e:pleaserekey")) {
    LOG(("Rekey requested"));

    
    RescheduleItems();

    
    return mCompleter->RekeyRequested();
  }

  nsUrlClassifierUtils::UnUrlsafeBase64(serverMAC);

  nsresult rv;

  nsCOMPtr<nsIKeyObjectFactory> keyObjectFactory(
    do_GetService("@mozilla.org/security/keyobjectfactory;1", &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsIKeyObject> keyObject;
  rv = keyObjectFactory->KeyFromString(nsIKeyObject::HMAC, mClientKey,
      getter_AddRefs(keyObject));
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsICryptoHMAC> hmac =
    do_CreateInstance(NS_CRYPTO_HMAC_CONTRACTID, &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = hmac->Init(nsICryptoHMAC::SHA1, keyObject);
  NS_ENSURE_SUCCESS(rv, rv);

  const nsCSubstring &remaining = Substring(begin, end);
  rv = hmac->Update(reinterpret_cast<const PRUint8*>(remaining.BeginReading()),
                    remaining.Length());
  NS_ENSURE_SUCCESS(rv, rv);

  nsCAutoString clientMAC;
  rv = hmac->Finish(PR_TRUE, clientMAC);
  NS_ENSURE_SUCCESS(rv, rv);

  if (clientMAC != serverMAC) {
    NS_WARNING("Invalid MAC in gethash response.");
    return NS_ERROR_FAILURE;
  }

  mVerified = PR_TRUE;

  return NS_OK;
}

nsresult
nsUrlClassifierHashCompleterRequest::HandleItem(const nsACString& item,
                                                const nsACString& tableName,
                                                PRUint32 chunkId)
{
  
  
  for (PRUint32 i = 0; i < mRequests.Length(); i++) {
    Request &request = mRequests[i];
    if (StringBeginsWith(item, request.partialHash)) {
      Response *response = request.responses.AppendElement();
      if (!response)
        return NS_ERROR_OUT_OF_MEMORY;
      response->completeHash = item;
      response->tableName = tableName;
      response->chunkId = chunkId;
    }
  }

  return NS_OK;
}





nsresult
nsUrlClassifierHashCompleterRequest::HandleTable(nsACString::const_iterator& begin,
                                                 const nsACString::const_iterator& end)
{
  nsACString::const_iterator iter;
  iter = begin;
  if (!FindCharInReadable(':', iter, end)) {
    
    NS_WARNING("Received badly-formatted gethash response.");
    return NS_ERROR_FAILURE;
  }

  const nsCSubstring& tableName = Substring(begin, iter);
  iter++;
  begin = iter;

  if (!FindCharInReadable('\n', iter, end)) {
    
    NS_WARNING("Received badly-formatted gethash response.");
    return NS_ERROR_FAILURE;
  }

  const nsCSubstring& remaining = Substring(begin, iter);
  iter++;
  begin = iter;

  PRUint32 chunkId;
  PRInt32 size;
  if (PR_sscanf(PromiseFlatCString(remaining).get(),
                "%u:%d", &chunkId, &size) != 2) {
    NS_WARNING("Received badly-formatted gethash response.");
    return NS_ERROR_FAILURE;
  }

  if (size % COMPLETE_LENGTH != 0) {
    NS_WARNING("Unexpected gethash response length");
    return NS_ERROR_FAILURE;
  }

  

  if (begin.size_forward() < size) {
    NS_WARNING("Response does not match the expected response length.");
    return NS_ERROR_FAILURE;
  }

  for (PRInt32 i = 0; i < (size / COMPLETE_LENGTH); i++) {
    
    iter.advance(COMPLETE_LENGTH);

    nsresult rv = HandleItem(Substring(begin, iter), tableName, chunkId);
    NS_ENSURE_SUCCESS(rv, rv);

    begin = iter;
  }

  

  return NS_OK;
}

nsresult
nsUrlClassifierHashCompleterRequest::HandleResponse()
{
  if (mResponse.IsEmpty()) {
    
    return NS_OK;
  }

  nsCString::const_iterator begin, end;
  mResponse.BeginReading(begin);
  mResponse.EndReading(end);

  nsresult rv;

  
  if (!mClientKey.IsEmpty()) {
    rv = HandleMAC(begin, end);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mRescheduled) {
      
      
      return NS_OK;
    }
  }

  while (begin != end) {
    rv = HandleTable(begin, end);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  return NS_OK;
}

void
nsUrlClassifierHashCompleterRequest::NotifySuccess()
{
  LOG(("nsUrlClassifierHashCompleterRequest::NotifySuccess [%p]", this));

  for (PRUint32 i = 0; i < mRequests.Length(); i++) {
    Request &request = mRequests[i];

    for (PRUint32 j = 0; j < request.responses.Length(); j++) {
      Response &response = request.responses[j];
      request.callback->Completion(response.completeHash,
                                   response.tableName,
                                   response.chunkId,
                                   mVerified);
    }

    request.callback->CompletionFinished(NS_OK);
  }
}

void
nsUrlClassifierHashCompleterRequest::NotifyFailure(nsresult status)
{
  LOG(("nsUrlClassifierHashCompleterRequest::NotifyFailure [%p]", this));

  for (PRUint32 i = 0; i < mRequests.Length(); i++) {
    Request &request = mRequests[i];
    request.callback->CompletionFinished(status);
  }
}

NS_IMETHODIMP
nsUrlClassifierHashCompleterRequest::OnStartRequest(nsIRequest *request,
                                                    nsISupports *context)
{
  LOG(("nsUrlClassifierHashCompleter::OnStartRequest [%p]", this));
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierHashCompleterRequest::OnDataAvailable(nsIRequest *request,
                                                     nsISupports *context,
                                                     nsIInputStream *stream,
                                                     PRUint32 sourceOffset,
                                                     PRUint32 length)
{
  LOG(("nsUrlClassifierHashCompleter::OnDataAvailable [%p]", this));

  if (mShuttingDown)
    return NS_ERROR_ABORT;

  nsCAutoString piece;
  nsresult rv = NS_ConsumeStream(stream, length, piece);
  NS_ENSURE_SUCCESS(rv, rv);

  mResponse.Append(piece);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierHashCompleterRequest::OnStopRequest(nsIRequest *request,
                                                   nsISupports *context,
                                                   nsresult status)
{
  LOG(("nsUrlClassifierHashCompleter::OnStopRequest [%p, status=%d]",
       this, status));

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->RemoveObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID);

  if (mShuttingDown)
    return NS_ERROR_ABORT;

  if (NS_SUCCEEDED(status)) {
    nsCOMPtr<nsIHttpChannel> channel = do_QueryInterface(request);
    if (channel) {
      PRBool success;
      status = channel->GetRequestSucceeded(&success);
      if (NS_SUCCEEDED(status) && !success) {
        status = NS_ERROR_ABORT;
      }
    }
  }

  mCompleter->NoteServerResponse(NS_SUCCEEDED(status));

  if (NS_SUCCEEDED(status))
    status = HandleResponse();

  
  if (!mRescheduled) {
    if (NS_SUCCEEDED(status))
      NotifySuccess();
    else
      NotifyFailure(status);
  }

  mChannel = nsnull;

  return NS_OK;
}


NS_IMETHODIMP
nsUrlClassifierHashCompleterRequest::Observe(nsISupports *subject,
                                             const char *topic,
                                             const PRUnichar *data)
{
  if (!strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    mShuttingDown = PR_TRUE;
    if (mChannel)
      mChannel->Cancel(NS_ERROR_ABORT);
  }

  return NS_OK;
}

NS_IMPL_ISUPPORTS4(nsUrlClassifierHashCompleter,
                   nsIUrlClassifierHashCompleter,
                   nsIRunnable,
                   nsIObserver,
                   nsISupportsWeakReference)

nsresult
nsUrlClassifierHashCompleter::Init()
{
#if defined(PR_LOGGING)
  if (!gUrlClassifierHashCompleterLog)
    gUrlClassifierHashCompleterLog = PR_NewLogModule("UrlClassifierHashCompleter");
#endif

  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1");
  if (observerService)
    observerService->AddObserver(this, NS_XPCOM_SHUTDOWN_OBSERVER_ID, PR_TRUE);

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierHashCompleter::Complete(const nsACString &partialHash,
                                       nsIUrlClassifierHashCompleterCallback *c)
{
  LOG(("nsUrlClassifierHashCompleter::Complete [%p]", this));

  if (mShuttingDown)
    return NS_ERROR_NOT_INITIALIZED;

  
  
  if (!mRequest) {
    mRequest = new nsUrlClassifierHashCompleterRequest(this);
    if (!mRequest) {
      return NS_ERROR_OUT_OF_MEMORY;
    }

    
    
    if (!mGethashUrl.IsEmpty()) {
      
      NS_DispatchToCurrentThread(this);
    }
  }

  return mRequest->Add(partialHash, c);
}

NS_IMETHODIMP
nsUrlClassifierHashCompleter::SetGethashUrl(const nsACString &url)
{
  mGethashUrl = url;

  if (mRequest) {
    
    NS_DispatchToCurrentThread(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierHashCompleter::GetGethashUrl(nsACString &url)
{
  url = mGethashUrl;
  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierHashCompleter::SetKeys(const nsACString &clientKey,
                                      const nsACString &wrappedKey)
{
  LOG(("nsUrlClassifierHashCompleter::SetKeys [%p]", this));

  NS_ASSERTION(clientKey.IsEmpty() == wrappedKey.IsEmpty(),
               "Must either have both a client key and a wrapped key or neither.");

  if (clientKey.IsEmpty()) {
    mClientKey.Truncate();
    mWrappedKey.Truncate();
    return NS_OK;
  }

  nsresult rv = nsUrlClassifierUtils::DecodeClientKey(clientKey, mClientKey);
  NS_ENSURE_SUCCESS(rv, rv);
  mWrappedKey = wrappedKey;

  return NS_OK;
}

NS_IMETHODIMP
nsUrlClassifierHashCompleter::Run()
{
  LOG(("nsUrlClassifierHashCompleter::Run [%p]\n", this));

  if (mShuttingDown) {
    mRequest = nsnull;
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (!mRequest)
    return NS_OK;

  NS_ASSERTION(!mGethashUrl.IsEmpty(),
               "Request dispatched without a gethash url specified.");

  nsCOMPtr<nsIURI> uri;
  nsresult rv;
  if (mClientKey.IsEmpty()) {
    rv = NS_NewURI(getter_AddRefs(uri), mGethashUrl);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    mRequest->SetClientKey(mClientKey);

    nsCAutoString requestURL(mGethashUrl);
    requestURL.Append("&wrkey=");
    requestURL.Append(mWrappedKey);
    rv = NS_NewURI(getter_AddRefs(uri), requestURL);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  mRequest->SetURI(uri);

  
  rv = mRequest->Begin();
  mRequest = nsnull;
  return rv;
}

NS_IMETHODIMP
nsUrlClassifierHashCompleter::Observe(nsISupports *subject, const char *topic,
                                      const PRUnichar *data)
{
  if (!strcmp(topic, NS_XPCOM_SHUTDOWN_OBSERVER_ID)) {
    mShuttingDown = PR_TRUE;
  }

  return NS_OK;
}

nsresult
nsUrlClassifierHashCompleter::RekeyRequested()
{
  
  SetKeys(EmptyCString(), EmptyCString());

  
  
  
  nsresult rv;
  nsCOMPtr<nsIObserverService> observerService =
    do_GetService("@mozilla.org/observer-service;1", &rv);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = observerService->NotifyObservers(static_cast<nsIUrlClassifierHashCompleter*>(this),
                                        "url-classifier-rekey-requested",
                                        nsnull);
  NS_ENSURE_SUCCESS(rv, rv);

  return NS_OK;
}

void
nsUrlClassifierHashCompleter::NoteServerResponse(PRBool success)
{
  LOG(("nsUrlClassifierHashCompleter::NoteServerResponse [%p, %d]",
       this, success));

  if (success) {
    mBackoff = PR_FALSE;
    mNextRequestTime = 0;
    mBackoffTime = 0;
    return;
  }

  PRIntervalTime now = PR_IntervalNow();

  
  mErrorTimes.AppendElement(now);
  if (mErrorTimes.Length() > gBackoffErrors) {
    mErrorTimes.RemoveElementAt(0);
  }

  if (mBackoff) {
    mBackoffTime *= 2;
    LOG(("Doubled backoff time to %d seconds", mBackoffTime));
  } else if (mErrorTimes.Length() == gBackoffErrors &&
             PR_IntervalToSeconds(now - mErrorTimes[0]) <= gBackoffTime) {
    mBackoff = PR_TRUE;
    mBackoffTime = gBackoffInterval;
    LOG(("Starting backoff, backoff time is %d seconds", mBackoffTime));
  }

  if (mBackoff) {
    mBackoffTime = PR_MIN(mBackoffTime, gBackoffMax);
    LOG(("Using %d for backoff time", mBackoffTime));
    mNextRequestTime = now + PR_SecondsToInterval(mBackoffTime);
  }
}
