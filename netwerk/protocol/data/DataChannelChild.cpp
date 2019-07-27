





#include "DataChannelChild.h"

#include "mozilla/unused.h"
#include "mozilla/net/NeckoChild.h"

namespace mozilla {
namespace net {

NS_IMPL_ISUPPORTS_INHERITED(DataChannelChild, nsDataChannel, nsIChildChannel)

DataChannelChild::DataChannelChild(nsIURI* aURI)
    : nsDataChannel(aURI)
    , mIPCOpen(false)
{
}

DataChannelChild::~DataChannelChild()
{
}

NS_IMETHODIMP
DataChannelChild::ConnectParent(uint32_t aId)
{
    if (!gNeckoChild->SendPDataChannelConstructor(this, aId)) {
        return NS_ERROR_FAILURE;
    }

    
    AddIPDLReference();
    return NS_OK;
}

NS_IMETHODIMP
DataChannelChild::CompleteRedirectSetup(nsIStreamListener *aListener,
                                        nsISupports *aContext)
{
    nsresult rv = AsyncOpen(aListener, aContext);
    if (NS_WARN_IF(NS_FAILED(rv))) {
        return rv;
    }

    if (mIPCOpen) {
        unused << Send__delete__(this);
    }
    return NS_OK;
}

void
DataChannelChild::AddIPDLReference()
{
    AddRef();
    mIPCOpen = true;
}

void
DataChannelChild::ActorDestroy(ActorDestroyReason why)
{
    MOZ_ASSERT(mIPCOpen);
    mIPCOpen = false;
    Release();
}

} 
} 
