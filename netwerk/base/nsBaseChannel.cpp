





#include "nsBaseChannel.h"
#include "nsContentUtils.h"
#include "nsURLHelper.h"
#include "nsNetCID.h"
#include "nsMimeTypes.h"
#include "nsIContentSniffer.h"
#include "nsIScriptSecurityManager.h"
#include "nsMimeTypes.h"
#include "nsIHttpEventSink.h"
#include "nsIHttpChannel.h"
#include "nsIChannelEventSink.h"
#include "nsIStreamConverterService.h"
#include "nsChannelClassifier.h"
#include "nsAsyncRedirectVerifyHelper.h"
#include "nsProxyRelease.h"
#include "nsXULAppAPI.h"
#include "nsContentSecurityManager.h"

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
  explicit ScopedRequestSuspender(nsIRequest *request)
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
  , mAllowThreadRetargeting(true)
  , mWaitingOnAsyncRedirect(false)
  , mStatus(NS_OK)
  , mContentDispositionHint(UINT32_MAX)
  , mContentLength(-1)
  , mWasOpened(false)
{
  mContentType.AssignLiteral(UNKNOWN_CONTENT_TYPE);
}

nsBaseChannel::~nsBaseChannel()
{
  NS_ReleaseOnMainThread(mLoadInfo);
}

nsresult
nsBaseChannel::Redirect(nsIChannel *newChannel, uint32_t redirectFlags,
                        bool openNewChannel)
{
  SUSPEND_PUMP_FOR_SCOPE();

  

  newChannel->SetLoadGroup(mLoadGroup);
  newChannel->SetNotificationCallbacks(mCallbacks);
  newChannel->SetLoadFlags(mLoadFlags | LOAD_REPLACE);
  newChannel->SetLoadInfo(mLoadInfo);

  
  if (mPrivateBrowsingOverriden) {
    nsCOMPtr<nsIPrivateBrowsingChannel> newPBChannel =
      do_QueryInterface(newChannel);
    if (newPBChannel) {
      newPBChannel->SetPrivate(mPrivateBrowsing);
    }
  }

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
    nsresult rv = NS_OK;
    if (mLoadInfo && mLoadInfo->GetEnforceSecurity()) {
      MOZ_ASSERT(!mListenerContext, "mListenerContext should be null!");
      rv = mRedirectChannel->AsyncOpen2(mListener);
    }
    else {
      rv = mRedirectChannel->AsyncOpen(mListener, mListenerContext);
    }
    NS_ENSURE_SUCCESS(rv, rv);
    
    
    if (mLoadInfo) {
      nsCOMPtr<nsIPrincipal> uriPrincipal;
      nsIScriptSecurityManager *sm = nsContentUtils::GetSecurityManager();
      sm->GetChannelURIPrincipal(this, getter_AddRefs(uriPrincipal));
      mLoadInfo->AppendRedirectedPrincipal(uriPrincipal);
    }
  }

  mRedirectChannel = nullptr;

  
  Cancel(NS_BINDING_REDIRECTED);
  ChannelDone();

  return NS_OK;
}

bool
nsBaseChannel::HasContentTypeHint() const
{
  NS_ASSERTION(!Pending(), "HasContentTypeHint called too late");
  return !mContentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE);
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
      mContentLength = -1;
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
    ChannelDone();
  }

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nullptr, mStatus);

  
  mCallbacks = nullptr;
  CallbacksChanged();
}

void
nsBaseChannel::ClassifyURI()
{
  
  
  if (!XRE_IsParentProcess()) {
    return;
  }

  if (mLoadFlags & LOAD_CLASSIFY_URI) {
    nsRefPtr<nsChannelClassifier> classifier = new nsChannelClassifier();
    if (classifier) {
      classifier->Start(this);
    } else {
      Cancel(NS_ERROR_OUT_OF_MEMORY);
    }
  }
}




NS_IMPL_ISUPPORTS_INHERITED(nsBaseChannel,
                            nsHashPropertyBag,
                            nsIRequest,
                            nsIChannel,
                            nsIThreadRetargetableRequest,
                            nsIInterfaceRequestor,
                            nsITransportEventSink,
                            nsIRequestObserver,
                            nsIStreamListener,
                            nsIThreadRetargetableStreamListener,
                            nsIAsyncVerifyRedirectCallback,
                            nsIPrivateBrowsingChannel)




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
  *result = Pending();
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
  if (!CanSetLoadGroup(aLoadGroup)) {
    return NS_ERROR_FAILURE;
  }

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
nsBaseChannel::SetLoadInfo(nsILoadInfo* aLoadInfo)
{
  mLoadInfo = aLoadInfo;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetLoadInfo(nsILoadInfo** aLoadInfo)
{
  NS_IF_ADDREF(*aLoadInfo = mLoadInfo);
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
  if (!CanSetCallbacks(aCallbacks)) {
    return NS_ERROR_FAILURE;
  }

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
  
  if (mContentDispositionHint == UINT32_MAX) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  *aContentDisposition = mContentDispositionHint;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetContentDisposition(uint32_t aContentDisposition)
{
  mContentDispositionHint = aContentDisposition;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetContentDispositionFilename(nsAString &aContentDispositionFilename)
{
  if (!mContentDispositionFilename) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  aContentDispositionFilename = *mContentDispositionFilename;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetContentDispositionFilename(const nsAString &aContentDispositionFilename)
{
  mContentDispositionFilename = new nsString(aContentDispositionFilename);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::GetContentDispositionHeader(nsACString &aContentDispositionHeader)
{
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
nsBaseChannel::GetContentLength(int64_t *aContentLength)
{
  *aContentLength = mContentLength;
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::SetContentLength(int64_t aContentLength)
{
  mContentLength = aContentLength;
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
nsBaseChannel::Open2(nsIInputStream** aStream)
{
  nsCOMPtr<nsIStreamListener> listener;
  nsresult rv = nsContentSecurityManager::doContentSecurityCheck(this, listener);
  NS_ENSURE_SUCCESS(rv, rv);
  return Open(aStream);
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
    ChannelDone();
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
nsBaseChannel::AsyncOpen2(nsIStreamListener *aListener)
{
  nsCOMPtr<nsIStreamListener> listener = aListener;
  nsresult rv = nsContentSecurityManager::doContentSecurityCheck(this, listener);
  NS_ENSURE_SUCCESS(rv, rv);
  return AsyncOpen(listener, nullptr);
}




NS_IMETHODIMP
nsBaseChannel::OnTransportStatus(nsITransport *transport, nsresult status,
                                 int64_t progress, int64_t progressMax)
{
  

  if (!mPump || NS_FAILED(mStatus)) {
    return NS_OK;
  }

  SUSPEND_PUMP_FOR_SCOPE();

  
  if (!mProgressSink) {
    if (mQueriedProgressSink) {
      return NS_OK;
    }
    GetCallback(mProgressSink);
    mQueriedProgressSink = true;
    if (!mProgressSink) {
      return NS_OK;
    }
  }

  if (!HasLoadFlag(LOAD_BACKGROUND)) {
    nsAutoString statusArg;
    if (GetStatusArg(status, statusArg)) {
      mProgressSink->OnStatus(this, mListenerContext, status, statusArg.get());
    }
  }

  if (progress) {
    mProgressSink->OnProgress(this, mListenerContext, progress, progressMax);
  }

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

  nsAutoCString newType;
  NS_SniffContent(NS_CONTENT_SNIFFER_CATEGORY, chan, aData, aCount, newType);
  if (!newType.IsEmpty()) {
    chan->SetContentType(newType);
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
  MOZ_ASSERT(request == mPump);

  
  
  
  if (NS_SUCCEEDED(mStatus) &&
      mContentType.EqualsLiteral(UNKNOWN_CONTENT_TYPE)) {
    mPump->PeekStream(CallUnknownTypeSniffer, static_cast<nsIChannel*>(this));
  }

  
  if (mLoadFlags & LOAD_CALL_CONTENT_SNIFFERS)
    mPump->PeekStream(CallTypeSniffers, static_cast<nsIChannel*>(this));

  SUSPEND_PUMP_FOR_SCOPE();

  if (mListener) 
      return mListener->OnStartRequest(this, mListenerContext);
  return NS_OK;
}

NS_IMETHODIMP
nsBaseChannel::OnStopRequest(nsIRequest *request, nsISupports *ctxt,
                             nsresult status)
{
  
  
  if (NS_SUCCEEDED(mStatus))
    mStatus = status;

  
  mPump = nullptr;

  if (mListener) 
      mListener->OnStopRequest(this, mListenerContext, mStatus);
  ChannelDone();

  
  

  if (mLoadGroup)
    mLoadGroup->RemoveRequest(this, nullptr, mStatus);

  
  mCallbacks = nullptr;
  CallbacksChanged();

  return NS_OK;
}




NS_IMETHODIMP
nsBaseChannel::OnDataAvailable(nsIRequest *request, nsISupports *ctxt,
                               nsIInputStream *stream, uint64_t offset,
                               uint32_t count)
{
  SUSPEND_PUMP_FOR_SCOPE();

  nsresult rv = mListener->OnDataAvailable(this, mListenerContext, stream,
                                           offset, count);
  if (mSynthProgressEvents && NS_SUCCEEDED(rv)) {
    int64_t prog = offset + count;
    if (NS_IsMainThread()) {
      OnTransportStatus(nullptr, NS_NET_STATUS_READING, prog, mContentLength);
    } else {
      class OnTransportStatusAsyncEvent : public nsRunnable
      {
        nsRefPtr<nsBaseChannel> mChannel;
        int64_t mProgress;
        int64_t mContentLength;
      public:
        OnTransportStatusAsyncEvent(nsBaseChannel* aChannel,
                                    int64_t aProgress,
                                    int64_t aContentLength)
          : mChannel(aChannel),
            mProgress(aProgress),
            mContentLength(aContentLength)
        { }

        NS_IMETHOD Run() override
        {
          return mChannel->OnTransportStatus(nullptr, NS_NET_STATUS_READING,
                                             mProgress, mContentLength);
        }
      };

      nsCOMPtr<nsIRunnable> runnable =
        new OnTransportStatusAsyncEvent(this, prog, mContentLength);
      NS_DispatchToMainThread(runnable);
    }
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

NS_IMETHODIMP
nsBaseChannel::RetargetDeliveryTo(nsIEventTarget* aEventTarget)
{
  MOZ_ASSERT(NS_IsMainThread());

  NS_ENSURE_TRUE(mPump, NS_ERROR_NOT_INITIALIZED);

  if (!mAllowThreadRetargeting) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  return mPump->RetargetDeliveryTo(aEventTarget);
}

NS_IMETHODIMP
nsBaseChannel::CheckListenerChain()
{
  MOZ_ASSERT(NS_IsMainThread());

  if (!mAllowThreadRetargeting) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  nsCOMPtr<nsIThreadRetargetableStreamListener> listener =
    do_QueryInterface(mListener);
  if (!listener) {
    return NS_ERROR_NO_INTERFACE;
  }

  return listener->CheckListenerChain();
}
