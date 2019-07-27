





#include "DataChannelParent.h"
#include "mozilla/Assertions.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS(DataChannelParent, nsIParentChannel, nsIStreamListener)

DataChannelParent::~DataChannelParent()
{
}

bool
DataChannelParent::Init(const uint32_t &channelId)
{
    nsCOMPtr<nsIChannel> channel;
    MOZ_ALWAYS_TRUE(NS_SUCCEEDED(
        NS_LinkRedirectChannels(channelId, this, getter_AddRefs(channel))));

    return true;
}

NS_IMETHODIMP
DataChannelParent::SetParentListener(HttpChannelParentListener* aListener)
{
    
    return NS_OK;
}

NS_IMETHODIMP
DataChannelParent::NotifyTrackingProtectionDisabled()
{
    
    return NS_OK;
}

NS_IMETHODIMP
DataChannelParent::Delete()
{
    
    return NS_OK;
}

void
DataChannelParent::ActorDestroy(ActorDestroyReason why)
{
}

NS_IMETHODIMP
DataChannelParent::OnStartRequest(nsIRequest *aRequest,
                                  nsISupports *aContext)
{
    
    
    
    return NS_BINDING_ABORTED;
}

NS_IMETHODIMP
DataChannelParent::OnStopRequest(nsIRequest *aRequest,
                                 nsISupports *aContext,
                                 nsresult aStatusCode)
{
    
    MOZ_ASSERT(NS_FAILED(aStatusCode));
    return NS_OK;
}

NS_IMETHODIMP
DataChannelParent::OnDataAvailable(nsIRequest *aRequest,
                                   nsISupports *aContext,
                                   nsIInputStream *aInputStream,
                                   uint64_t aOffset,
                                   uint32_t aCount)
{
    
    MOZ_CRASH("Should never be called");
}

} 
} 
