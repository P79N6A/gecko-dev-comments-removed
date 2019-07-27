





#include "RtspChannelParent.h"

using namespace mozilla::ipc;

namespace mozilla {
namespace net {




RtspChannelParent::RtspChannelParent(nsIURI *aUri)
  : mIPCClosed(false)
{
  nsBaseChannel::SetURI(aUri);
  DisallowThreadRetargeting();
}

RtspChannelParent::~RtspChannelParent()
{
}

void
RtspChannelParent::ActorDestroy(ActorDestroyReason why)
{
  mIPCClosed = true;
}




NS_IMPL_ISUPPORTS_INHERITED(RtspChannelParent,
                            nsBaseChannel,
                            nsIParentChannel)




bool
RtspChannelParent::Init(const RtspChannelConnectArgs& aArgs)
{
  return ConnectChannel(aArgs.channelId());
}

bool
RtspChannelParent::ConnectChannel(const uint32_t& channelId)
{
  nsresult rv;
  nsCOMPtr<nsIChannel> channel;
  rv = NS_LinkRedirectChannels(channelId, this, getter_AddRefs(channel));

  return true;
}




NS_IMETHODIMP
RtspChannelParent::GetContentType(nsACString& aContentType)
{
  aContentType.AssignLiteral("RTSP");
  return NS_OK;
}

NS_IMETHODIMP
RtspChannelParent::AsyncOpen(nsIStreamListener *aListener, nsISupports *aContext)
{
  return NS_OK;
}




NS_IMETHODIMP
RtspChannelParent::OnStartRequest(nsIRequest *aRequest,
                            nsISupports *aContext)
{
  MOZ_CRASH("Should never be called");
}

NS_IMETHODIMP
RtspChannelParent::OnStopRequest(nsIRequest *aRequest,
                           nsISupports *aContext,
                           nsresult aStatusCode)
{
  MOZ_CRASH("Should never be called");
}




NS_IMETHODIMP
RtspChannelParent::OnDataAvailable(nsIRequest *aRequest,
                             nsISupports *aContext,
                             nsIInputStream *aInputStream,
                             uint64_t aOffset,
                             uint32_t aCount)
{
  MOZ_CRASH("Should never be called");
}




NS_IMETHODIMP
RtspChannelParent::Cancel(nsresult status)
{
  
  
  
  
  
  return NS_OK;
}

NS_IMETHODIMP
RtspChannelParent::Suspend()
{
  MOZ_CRASH("Should never be called");
}

NS_IMETHODIMP
RtspChannelParent::Resume()
{
  MOZ_CRASH("Should never be called");
}




NS_IMETHODIMP
RtspChannelParent::OpenContentStream(bool aAsync,
                               nsIInputStream **aStream,
                               nsIChannel **aChannel)
{
  MOZ_CRASH("Should never be called");
}




NS_IMETHODIMP
RtspChannelParent::SetParentListener(HttpChannelParentListener *aListener)
{
  return NS_OK;
}

NS_IMETHODIMP
RtspChannelParent::NotifyTrackingProtectionDisabled()
{
  
  return NS_OK;
}

NS_IMETHODIMP
RtspChannelParent::Delete()
{
  return NS_OK;
}

} 
} 
