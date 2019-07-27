





#include "RtspChannelChild.h"
#include "mozilla/ipc/URIUtils.h"

using namespace mozilla::ipc;

namespace mozilla {
namespace net {




RtspChannelChild::RtspChannelChild(nsIURI *aUri)
  : mIPCOpen(false)
  , mCanceled(false)
{
  nsBaseChannel::SetURI(aUri);
  DisallowThreadRetargeting();
}

RtspChannelChild::~RtspChannelChild()
{
}

nsIStreamingProtocolController*
RtspChannelChild::GetController()
{
  return mMediaStreamController;
}

void
RtspChannelChild::ReleaseController()
{
  if (mMediaStreamController) {
    mMediaStreamController = nullptr;
  }
}




void
RtspChannelChild::AddIPDLReference()
{
  MOZ_ASSERT(!mIPCOpen,
             "Attempt to retain more than one IPDL reference");
  mIPCOpen = true;
  AddRef();
}

void
RtspChannelChild::ReleaseIPDLReference()
{
  MOZ_ASSERT(mIPCOpen, "Attempt to release nonexistent IPDL reference");
  mIPCOpen = false;
  Release();
}




NS_IMPL_ISUPPORTS_INHERITED(RtspChannelChild,
                            nsBaseChannel,
                            nsIChannel,
                            nsIChildChannel)




NS_IMETHODIMP
RtspChannelChild::GetContentType(nsACString& aContentType)
{
  aContentType.AssignLiteral("RTSP");
  return NS_OK;
}

class CallListenerOnStartRequestEvent : public nsRunnable
{
public:
  CallListenerOnStartRequestEvent(nsIStreamListener *aListener,
                                  nsIRequest *aRequest, nsISupports *aContext)
    : mListener(aListener)
    , mRequest(aRequest)
    , mContext(aContext)
  {
    MOZ_RELEASE_ASSERT(aListener);
  }
  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mListener->OnStartRequest(mRequest, mContext);
    return NS_OK;
  }
private:
  nsRefPtr<nsIStreamListener> mListener;
  nsRefPtr<nsIRequest> mRequest;
  nsRefPtr<nsISupports> mContext;
};

NS_IMETHODIMP
RtspChannelChild::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
  
  MOZ_ASSERT(aListener);
  nsCOMPtr<nsIURI> uri = nsBaseChannel::URI();
  NS_ENSURE_TRUE(uri, NS_ERROR_ILLEGAL_VALUE);

  
  nsCOMPtr<nsIStreamingProtocolControllerService> mediaControllerService =
    do_GetService(MEDIASTREAMCONTROLLERSERVICE_CONTRACTID);
  MOZ_RELEASE_ASSERT(mediaControllerService,
    "Cannot proceed if media controller service is unavailable!");
  mediaControllerService->Create(this, getter_AddRefs(mMediaStreamController));
  MOZ_ASSERT(mMediaStreamController);

  
  if (mLoadGroup) {
    mLoadGroup->AddRequest(this, nullptr);
  }

  
  
  
  mListener = aListener;
  mListenerContext = aContext;
  NS_DispatchToMainThread(
    new CallListenerOnStartRequestEvent(mListener, this, mListenerContext));

  return NS_OK;
}




NS_IMETHODIMP
RtspChannelChild::OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
{
  MOZ_CRASH("Should never be called");
}

NS_IMETHODIMP
RtspChannelChild::OnStopRequest(nsIRequest *aRequest, nsISupports *aContext,
                                nsresult aStatusCode)
{
  MOZ_CRASH("Should never be called");
}




NS_IMETHODIMP
RtspChannelChild::OnDataAvailable(nsIRequest *aRequest,
                                  nsISupports *aContext,
                                  nsIInputStream *aInputStream,
                                  uint64_t aOffset,
                                  uint32_t aCount)
{
  MOZ_CRASH("Should never be called");
}




class CallListenerOnStopRequestEvent : public nsRunnable
{
public:
  CallListenerOnStopRequestEvent(nsIStreamListener *aListener,
                                 nsIRequest *aRequest,
                                 nsISupports *aContext, nsresult aStatus)
    : mListener(aListener)
    , mRequest(aRequest)
    , mContext(aContext)
    , mStatus(aStatus)
  {
    MOZ_RELEASE_ASSERT(aListener);
  }
  NS_IMETHOD Run()
  {
    MOZ_ASSERT(NS_IsMainThread());
    mListener->OnStopRequest(mRequest, mContext, mStatus);
    return NS_OK;
  }
private:
  nsRefPtr<nsIStreamListener> mListener;
  nsRefPtr<nsIRequest> mRequest;
  nsRefPtr<nsISupports> mContext;
  nsresult mStatus;
};

NS_IMETHODIMP
RtspChannelChild::Cancel(nsresult status)
{
  if (mCanceled) {
    return NS_OK;
  }

  mCanceled = true;
  
  if (mMediaStreamController) {
    mMediaStreamController->Stop();
  }

  
  NS_DispatchToMainThread(
    new CallListenerOnStopRequestEvent(mListener, this,
                                       mListenerContext, status));
  mListener = nullptr;
  mListenerContext = nullptr;

  
  if (mLoadGroup) {
    mLoadGroup->RemoveRequest(this, nullptr, status);
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspChannelChild::Suspend()
{
  MOZ_CRASH("Should never be called");
}

NS_IMETHODIMP
RtspChannelChild::Resume()
{
  MOZ_CRASH("Should never be called");
}




NS_IMETHODIMP
RtspChannelChild::OpenContentStream(bool aAsync,
                                    nsIInputStream **aStream,
                                    nsIChannel **aChannel)
{
  MOZ_CRASH("Should never be called");
}




NS_IMETHODIMP
RtspChannelChild::ConnectParent(uint32_t id)
{
  
  AddIPDLReference();
  RtspChannelConnectArgs connectArgs;
  SerializeURI(nsBaseChannel::URI(), connectArgs.uri());
  connectArgs.channelId() = id;
  if (!gNeckoChild->SendPRtspChannelConstructor(this, connectArgs)) {
    return NS_ERROR_FAILURE;
  }

  return NS_OK;
}

NS_IMETHODIMP
RtspChannelChild::CompleteRedirectSetup(nsIStreamListener *aListener,
                                        nsISupports *aContext)
{
  return AsyncOpen(aListener, aContext);
}

} 
} 
