




#include "nsBaseChannel.h"
#include "nsChannelProperties.h"
#include "nsURLHelper.h"
#include "nsNetUtil.h"
#include "nsMimeTypes.h"
#include "nsIOService.h"
#include "nsIHttpEventSink.h"
#include "nsIHttpChannel.h"
#include "nsIChannelEventSink.h"
#include "nsIStreamConverterService.h"
#include "nsIContentSniffer.h"
#include "nsChannelClassifier.h"
#include "nsAsyncRedirectVerifyHelper.h"

static PLDHashOperator
CopyProperties(const nsAString &key, nsIVariant *data, void *closure)
{
  nsIWritablePropertyBag *bag =
      static_cast<nsIWritablePropertyBag *>(closure);

  bag->SetProperty(key, data);
  return PL_DHASH_NEXT;
}


class ScopedRequestSuspender {
public:
  ScopedRequestSuspender(nsIRequest *request)
    : mRequest(request) {
    if (mRequest && NS_FAILED(mRequest->Suspend())) {
      NS_WARNING("Couldn't suspend pump");
      mRequest = nullptr;
    }
  }
  ~ScopedRequestSuspender() {
    if (mRequest)
      mRequest->Resume();
  }
private:
  nsIRequest *mRequest;
};



#define SUSPEND_PUMP_FOR_SCOPE() \
  ScopedRequestSuspender pump_suspender__(mPump)




nsBaseChannel::nsBaseChannel()
  : mLoadFlags(LOAD_NORMAL)
  , mQueriedProgressSink(true)
  , mSynthProgressEvents(false)
  , mWasOpened(false)
  , mWaitingOnAsyncRedirect(false)
  , mStatus(NS_OK)
{
  mContentType.AssignLiteral(UNKNOWN_CONTENT_TYPE);
}

nsresult
nsBaseChannel::Redirect(nsIChannel *newChannel, uint32_t redirectFlags,
                        bool openNewChannel)
{
  SUSPEND_PUMP_FOR_SCOPE();

  

  newChannel->SetLoadGroup(mLoadGroup);
  newChannel->SetNotificationCallbacks(mCallbacks);
  newChannel->SetLoadFlags(mLoadFlags | LOAD_REPLACE);

  nsCOMPtr<nsIWritablePropertyBag> bag = ::do_QueryInterface(newChannel);
  if (bag)
    mPropertyHash.EnumerateRead(CopyProperties, bag.get());

  
  
  

  nsRefPtr<nsAsyncRedirectVerifyHelper> redirectCallbackHelper =
      new nsAsyncRedirectVerifyHelper();

  bool checkRedirectSynchronously = !openNewChannel;

  mRedirectChannel = newChannel;
  mRedirectFlags = redirectFlags;
  mOpenRedirectChannel = openNewChannel;
  nsresult rv = redirectCallbackHelper->Init(this, newChannel, redirectFlags,
                                             checkRedirectSynchronously);
  if (NS_FAILED(rv))
    return rv;

  if (checkRedirectSynchronously && NS_FAILED(mStatus))
    return mStatus;

  return NS_OK;
}

nsresult
nsBaseChannel::ContinueRedirect()
{
  
  
  
  if (!(mRedirectFlags & nsIChannelEventSink::REDIRECT_INTERNAL)) {
    nsCOMPtr<nsIHttpChannel> httpChannel = do_QueryInterface();
    if (httpChannel) {
      nsCOMPtr<nsIHttpEventSink> httpEventSink;
      GetCallback(httpEventSink);
      if (httpEventSink) {
        nsresult rv = httpEventSink->OnRedirect(httpChannel, mRedirectChannel);
        if (NS_FAILED(rv)) {
          return rv;
        }
      }
    }
  }

  
  mRedirectChannel->SetOriginalURI(OriginalURI());

  
  
  

  if (mOpenRedirectChannel) {
    nsresult rv = mRedirectChannel->AsyncOpen(mListener, mListenerContext);
    if (NS_FAILED(rv))
      return rv;
  }

  mRedirectChannel = nullptr;

  
  Cancel(NS_BINDING_REDIRECTED);
  mListener = nullptr;
  mListenerContext = nullptr;

  return NS_OK;
}

bool
nsBaseChannel::HasContentTypeHint() const
{
  NS_ASSERTION(!IsPending(), "HasContentTypeHint called too late");
  return !mContentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE);
}

void
nsBaseChannel::SetContentLength64(int64_t len)
{
  
  
  
  SetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH, len);
}

int64_t
nsBaseChannel::ContentLength64()
{
  int64_t len;
  nsresult rv = GetPropertyAsInt64(NS_CHANNEL_PROP_CONTENT_LENGTH, &len);
  return NS_SUCCEEDED(rv) ? len : -1;
}

nsresult
nsBaseChannel::PushStreamConverter(const char *fromType,
                                   const char *toType,
                                   bool invalidatesContentLength,
                                   nsIStreamListener **result)
{
  NS_ASSERTION(mListener, "no listener");

  nsresult rv;
  nsCOMPtr<nsIStreamConverterService> scs =
      do_GetService(NS_STREAMCONVERTERSERVICE_CONTRACTID, &rv);
  if (NS_FAILED(rv))
    return rv;

  nsCOMPtr<nsIStreamListener> converter;
  rv = scs->AsyncConvertData(fromType, toType, mListener, mListenerContext,
                             getter_AddRefs(converter));
  if (NS_SUCCEEDED(rv)) {
    mListener = converter;
    if (invalidatesContentLength)
      SetContentLength64(-1);
    if (result) {
      *result = nullptr;
      converter.swap(*result);
    }
  }
  return rv;
}

nsresult
nsBaseChannel::BeginPumpingData()
{
  nsCOMPtr<nsIInputStream> stream;
  nsCOMPtr<nsIChannel> channel;
  nsresult rv = OpenContentStream(true, getter_AddRefs(stream),
                                  getter_AddRefs(channel));
  if (NS_FAILED(rv))
    return rv;

  NS_ASSERTION(!stream || !channel, "Got both a channel and a stream?");

  if (channel) {
      rv = NS_DispatchToCurrentThread(new RedirectRunnable(this, channel));
      if (NS_SUCCEEDED(rv))
          mWaitingOnAsyncRedirect = true;
      return rv;
  }

  
  
  
  
  
 
  rv = nsInputStreamPump::Create(getter_AddRefs(mPump), stream, -1, -1, 0, 0,
                                 true);
  if (NS_SUCCEEDED(rv))
    rv = mPump->AsyncRead(this, nullptr);

  return rv;
}

void
nsBaseChannel::HandleAsyncRedirect(nsIChannel* newChannel)
{
  NS_ASSERTION(!mPump, "Shouldn't have gotten here");

  nsresult rv = mStatus;
  if (NS_SUCCEEDED(mStatus)) {
    rv = Redirect(newChannel,
                  nsIChannelEventSink::REDIRECT_TEMPORARY,
                  true);
    if (NS_SUCCEEDED(rv)) {
      
      return;
    }
  }

  ContinueHandleAsyncRedirect(rv);
}

void
nsBaseChannel::ContinueHandleAsyncRedirect(nsresult result)
{
  mWaitingOnAsyncRedirect = false;

  if (NS_FAILED(result))
    Cancel(result);

  if (NS_FAILED(result) && mListener) {
    
    mListener->OnStartRequest(this, mListenerContext);
    mListener->OnStopRequest(this, mListenerContext, mStatus);
    mListener = nullptr;
    mListenerContext = nullptr;
  }

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nullptr, mStatus);

  
  mCallbacks = nullptr;
  CallbacksChanged();
}

void
nsBaseChannel::ClassifyURI()
{
  nsresult rv;

  if (mLoadFlags & LOAD_CLASSIFY_URI) {
    nsRefPtr<nsChannelClassifier> classifier = new nsChannelClassifier();
    if (classifier) {
      rv = classifier->Start(this);
      if (NS_FAILED(rv)) {
        Cancel(rv);
      }
    } else {
      Cancel(NS_ERROR_OUT_OF_MEMORY);
    }
  }
}




NS_IMPL_ISUPPORTS_INHERITED7(nsBaseChannel,
                             nsHashPropertyBag,
                             nsIRequest,
                             nsIChannel,
                             nsIInterfaceRequestor,
                             nsITransportEventSink,
                             nsIRequestObserver,
                             nsIStreamListener,
                             nsIAsyncVerifyRedirectCallback)




NS_IMETHODIMP
nsBaseChannel::GetName(nsACString &result)
{
  if (!mURI) {
    result.Truncate();
    return NS_OK;
  }
  return mURI->GetSpec(result);
}

NS_IMETHODIMP
nsBaseChannel::IsPending(bool *result)
{
  *result = IsPending();
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetStatus(nsresult *status)
{
  if (mPump && NS_SUCCEEDED(mStatus)) {
    mPump->GetStatus(status);
  } else {
    *status = mStatus;
  }
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::Cancel(nsresult status)
{
  
  if (NS_FAILED(mStatus))
    return NS_OK;

  mStatus = status;

  if (mPump)
    mPump->Cancel(status);

  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::Suspend()
{
  NS_ENSURE_TRUE(mPump, NS_ERROR_NOT_INITIALIZED);
  return mPump->Suspend();
}

NS_IMETHODIMP
nsBaseChannel::Resume()
{
  NS_ENSURE_TRUE(mPump, NS_ERROR_NOT_INITIALIZED);
  return mPump->Resume();
}

NS_IMETHODIMP
nsBaseChannel::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
  *aLoadFlags = mLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetLoadFlags(nsLoadFlags aLoadFlags)
{
  mLoadFlags = aLoadFlags;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
  NS_IF_ADDREF(*aLoadGroup = mLoadGroup);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
  mLoadGroup = aLoadGroup;
  CallbacksChanged();
  return NS_OK;
}




NS_IMETHODIMP
nsBaseChannel::GetOriginalURI(nsIURI **aURI)
{
  *aURI = OriginalURI();
  NS_ADDREF(*aURI);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetOriginalURI(nsIURI *aURI)
{
  NS_ENSURE_ARG_POINTER(aURI);
  mOriginalURI = aURI;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetURI(nsIURI **aURI)
{
  NS_IF_ADDREF(*aURI = mURI);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetOwner(nsISupports **aOwner)
{
  NS_IF_ADDREF(*aOwner = mOwner);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetOwner(nsISupports *aOwner)
{
  mOwner = aOwner;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks)
{
  NS_IF_ADDREF(*aCallbacks = mCallbacks);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks)
{
  mCallbacks = aCallbacks;
  CallbacksChanged();
  return NS_OK;
}

NS_IMETHODIMP 
nsBaseChannel::GetSecurityInfo(nsISupports **aSecurityInfo)
{
  NS_IF_ADDREF(*aSecurityInfo = mSecurityInfo);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetContentType(nsACString &aContentType)
{
  aContentType = mContentType;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetContentType(const nsACString &aContentType)
{
  
  bool dummy;
  net_ParseContentType(aContentType, mContentType, mContentCharset, &dummy);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetContentCharset(nsACString &aContentCharset)
{
  aContentCharset = mContentCharset;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetContentCharset(const nsACString &aContentCharset)
{
  mContentCharset = aContentCharset;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetContentDisposition(uint32_t *aContentDisposition)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsBaseChannel::GetContentDispositionFilename(nsAString &aContentDispositionFilename)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsBaseChannel::GetContentDispositionHeader(nsACString &aContentDispositionHeader)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsBaseChannel::GetContentLength(int32_t *aContentLength)
{
  int64_t len = ContentLength64();
  if (len > PR_INT32_MAX || len < 0)
    *aContentLength = -1;
  else
    *aContentLength = (int32_t) len;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetContentLength(int32_t aContentLength)
{
  SetContentLength64(aContentLength);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::Open(nsIInputStream **result)
{
  NS_ENSURE_TRUE(mURI, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!mPump, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_IN_PROGRESS);

  nsCOMPtr<nsIChannel> chan;
  nsresult rv = OpenContentStream(false, result, getter_AddRefs(chan));
  NS_ASSERTION(!chan || !*result, "Got both a channel and a stream?");
  if (NS_SUCCEEDED(rv) && chan) {
      rv = Redirect(chan, nsIChannelEventSink::REDIRECT_INTERNAL, false);
      if (NS_FAILED(rv))
          return rv;
      rv = chan->Open(result);
  } else if (rv == NS_ERROR_NOT_IMPLEMENTED)
    return NS_ImplementChannelOpen(this, result);

  if (NS_SUCCEEDED(rv)) {
    mWasOpened = true;
    ClassifyURI();
  }

  return rv;
}

NS_IMETHODIMP
nsBaseChannel::AsyncOpen(nsIStreamListener *listener, nsISupports *ctxt)
{
  NS_ENSURE_TRUE(mURI, NS_ERROR_NOT_INITIALIZED);
  NS_ENSURE_TRUE(!mPump, NS_ERROR_IN_PROGRESS);
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);
  NS_ENSURE_ARG(listener);

  
  nsresult rv = NS_CheckPortSafety(mURI);
  if (NS_FAILED(rv)) {
    mCallbacks = nullptr;
    return rv;
  }

  
  
  
  
  
  mListener = listener;
  mListenerContext = ctxt;

  
  
  rv = BeginPumpingData();
  if (NS_FAILED(rv)) {
    mPump = nullptr;
    mListener = nullptr;
    mListenerContext = nullptr;
    mCallbacks = nullptr;
    return rv;
  }

  

  mWasOpened = true;

  SUSPEND_PUMP_FOR_SCOPE();

  if (mLoadGroup)
    mLoadGroup->AddRequest(this, nullptr);

  ClassifyURI();

  return NS_OK;
}




NS_IMETHODIMP
nsBaseChannel::OnTransportStatus(nsITransport *transport, nsresult status,
                                 uint64_t progress, uint64_t progressMax)
{
  

  if (!mPump || NS_FAILED(mStatus) || HasLoadFlag(LOAD_BACKGROUND))
    return NS_OK;

  SUSPEND_PUMP_FOR_SCOPE();

  
  if (!mProgressSink) {
    if (mQueriedProgressSink)
      return NS_OK;
    GetCallback(mProgressSink);
    mQueriedProgressSink = true;
    if (!mProgressSink)
      return NS_OK;
  }

  nsAutoString statusArg;
  if (GetStatusArg(status, statusArg))
    mProgressSink->OnStatus(this, mListenerContext, status, statusArg.get());

  if (progress)
    mProgressSink->OnProgress(this, mListenerContext, progress, progressMax);

  return NS_OK;
}




NS_IMETHODIMP
nsBaseChannel::GetInterface(const nsIID &iid, void **result)
{
  NS_QueryNotificationCallbacks(mCallbacks, mLoadGroup, iid, result);
  return *result ? NS_OK : NS_ERROR_NO_INTERFACE; 
}




static void
CallTypeSniffers(void *aClosure, const uint8_t *aData, uint32_t aCount)
{
  nsIChannel *chan = static_cast<nsIChannel*>(aClosure);

  const nsCOMArray<nsIContentSniffer>& sniffers =
    gIOService->GetContentSniffers();
  uint32_t length = sniffers.Count();
  for (uint32_t i = 0; i < length; ++i) {
    nsAutoCString newType;
    nsresult rv =
      sniffers[i]->GetMIMETypeFromContent(chan, aData, aCount, newType);
    if (NS_SUCCEEDED(rv) && !newType.IsEmpty()) {
      chan->SetContentType(newType);
      break;
    }
  }
}

static void
CallUnknownTypeSniffer(void *aClosure, const uint8_t *aData, uint32_t aCount)
{
  nsIChannel *chan = static_cast<nsIChannel*>(aClosure);

  nsCOMPtr<nsIContentSniffer> sniffer =
    do_CreateInstance(NS_GENERIC_CONTENT_SNIFFER);
  if (!sniffer)
    return;

  nsAutoCString detected;
  nsresult rv = sniffer->GetMIMETypeFromContent(chan, aData, aCount, detected);
  if (NS_SUCCEEDED(rv))
    chan->SetContentType(detected);
}

NS_IMETHODIMP
nsBaseChannel::OnStartRequest(nsIRequest *request, nsISupports *ctxt)
{
  
  
  
  
  bool shouldSniff = mContentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE) ||
            ((mLoadFlags & LOAD_TREAT_APPLICATION_OCTET_STREAM_AS_UNKNOWN) &&
            mContentType.EqualsLiteral(APPLICATION_OCTET_STREAM));

  if (NS_SUCCEEDED(mStatus) && shouldSniff) {
    mPump->PeekStream(CallUnknownTypeSniffer, static_cast<nsIChannel*>(this));
  }

  
  if ((mLoadFlags & LOAD_CALL_CONTENT_SNIFFERS) &&
      gIOService->GetContentSniffers().Count() != 0)
    mPump->PeekStream(CallTypeSniffers, static_cast<nsIChannel*>(this));

  SUSPEND_PUMP_FOR_SCOPE();

  return mListener->OnStartRequest(this, mListenerContext);
}

NS_IMETHODIMP
nsBaseChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                             nsresult status)
{
  
  
  if (NS_SUCCEEDED(mStatus))
    mStatus = status;

  
  mPump = nullptr;

  mListener->OnStopRequest(this, mListenerContext, mStatus);
  mListener = nullptr;
  mListenerContext = nullptr;

  
  

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nullptr, mStatus);

  
  mCallbacks = nullptr;
  CallbacksChanged();

  return NS_OK;
}




NS_IMETHODIMP
nsBaseChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                               nsIInputStream *stream, uint32_t offset,
                               uint32_t count)
{
  SUSPEND_PUMP_FOR_SCOPE();

  nsresult rv = mListener->OnDataAvailable(this, mListenerContext, stream,
                                           offset, count);
  if (mSynthProgressEvents && NS_SUCCEEDED(rv)) {
    uint64_t prog = uint64_t(offset) + count;
    uint64_t progMax = ContentLength64();
    OnTransportStatus(nullptr, NS_NET_STATUS_READING, prog, progMax);
  }

  return rv;
}

NS_IMETHODIMP
nsBaseChannel::OnRedirectVerifyCallback(nsresult result)
{
  if (NS_SUCCEEDED(result))
    result = ContinueRedirect();

  if (NS_FAILED(result) && !mWaitingOnAsyncRedirect) {
    if (NS_SUCCEEDED(mStatus))
      mStatus = result;
    return NS_OK;
  }

  if (mWaitingOnAsyncRedirect)
    ContinueHandleAsyncRedirect(result);

  return NS_OK;
}
