




#include "OfflineCacheUpdateParent.h"

#include "mozilla/ipc/URIUtils.h"
#include "nsOfflineCacheUpdate.h"
#include "nsIApplicationCache.h"
#include "nsNetUtil.h"

using namespace mozilla::ipc;

#if defined(PR_LOGGING)









extern PRLogModuleInfo *gOfflineCacheUpdateLog;
#endif
#undef LOG
#define LOG(args) PR_LOG(gOfflineCacheUpdateLog, 4, args)
#define LOG_ENABLED() PR_LOG_TEST(gOfflineCacheUpdateLog, 4)

namespace mozilla {
namespace docshell {





NS_IMPL_ISUPPORTS2(OfflineCacheUpdateParent,
                   nsIOfflineCacheUpdateObserver,
                   nsILoadContext)





OfflineCacheUpdateParent::OfflineCacheUpdateParent()
    : mIPCClosed(false)
{
    
    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();
    if (!service)
        return;

    LOG(("OfflineCacheUpdateParent::OfflineCacheUpdateParent [%p]", this));
}

OfflineCacheUpdateParent::~OfflineCacheUpdateParent()
{
    LOG(("OfflineCacheUpdateParent::~OfflineCacheUpdateParent [%p]", this));
}

void
OfflineCacheUpdateParent::ActorDestroy(ActorDestroyReason why)
{
    mIPCClosed = true;
}

nsresult
OfflineCacheUpdateParent::Schedule(const URIParams& aManifestURI,
                                   const URIParams& aDocumentURI,
                                   const bool& isInBrowserElement,
                                   const uint32_t& appId,
                                   const bool& stickDocument)
{
    LOG(("OfflineCacheUpdateParent::RecvSchedule [%p]", this));

    nsRefPtr<nsOfflineCacheUpdate> update;
    nsCOMPtr<nsIURI> manifestURI = DeserializeURI(aManifestURI);
    if (!manifestURI)
        return NS_ERROR_FAILURE;

    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();
    if (!service)
        return NS_ERROR_FAILURE;

    bool offlinePermissionAllowed = false;
    nsresult rv = service->OfflineAppAllowedForURI(
        manifestURI, nullptr, &offlinePermissionAllowed);
    NS_ENSURE_SUCCESS(rv, rv);

    if (!offlinePermissionAllowed)
        return NS_ERROR_DOM_SECURITY_ERR;

    nsCOMPtr<nsIURI> documentURI = DeserializeURI(aDocumentURI);
    if (!documentURI)
        return NS_ERROR_FAILURE;

    if (!NS_SecurityCompareURIs(manifestURI, documentURI, false))
        return NS_ERROR_DOM_SECURITY_ERR;

    service->FindUpdate(manifestURI, appId, isInBrowserElement,
                        getter_AddRefs(update));
    if (!update) {
        update = new nsOfflineCacheUpdate();

        
        
        rv = update->Init(manifestURI, documentURI, nullptr, nullptr,
                          appId, isInBrowserElement);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = update->Schedule();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    update->AddObserver(this, false);

    if (stickDocument) {
        nsCOMPtr<nsIURI> stickURI;
        documentURI->Clone(getter_AddRefs(stickURI));
        update->StickDocument(stickURI);
    }

    return NS_OK;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::UpdateStateChanged(nsIOfflineCacheUpdate *aUpdate, uint32_t state)
{
    if (mIPCClosed)
        return NS_ERROR_UNEXPECTED;

    LOG(("OfflineCacheUpdateParent::StateEvent [%p]", this));

    uint64_t byteProgress;
    aUpdate->GetByteProgress(&byteProgress);
    SendNotifyStateEvent(state, byteProgress);

    if (state == nsIOfflineCacheUpdateObserver::STATE_FINISHED) {
        
        
        
        bool isUpgrade;
        aUpdate->GetIsUpgrade(&isUpgrade);
        bool succeeded;
        aUpdate->GetSucceeded(&succeeded);

        SendFinish(succeeded, isUpgrade);
    }

    return NS_OK;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::ApplicationCacheAvailable(nsIApplicationCache *aApplicationCache)
{
    if (mIPCClosed)
        return NS_ERROR_UNEXPECTED;

    NS_ENSURE_ARG(aApplicationCache);

    nsCString cacheClientId;
    aApplicationCache->GetClientID(cacheClientId);
    nsCString cacheGroupId;
    aApplicationCache->GetGroupID(cacheGroupId);

    SendAssociateDocuments(cacheGroupId, cacheClientId);
    return NS_OK;
}





NS_IMETHODIMP
OfflineCacheUpdateParent::GetAssociatedWindow(nsIDOMWindow * *aAssociatedWindow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::GetTopWindow(nsIDOMWindow * *aTopWindow)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::GetTopFrameElement(nsIDOMElement** aElement)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::IsAppOfType(uint32_t appType, bool *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::GetIsContent(bool *aIsContent)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::GetUsePrivateBrowsing(bool *aUsePrivateBrowsing)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}
NS_IMETHODIMP
OfflineCacheUpdateParent::SetUsePrivateBrowsing(bool aUsePrivateBrowsing)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::SetPrivateBrowsing(bool aUsePrivateBrowsing)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::GetIsInBrowserElement(bool *aIsInBrowserElement)
{
    *aIsInBrowserElement = mIsInBrowserElement;
    return NS_OK;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::GetAppId(uint32_t *aAppId)
{
    *aAppId = mAppId;
    return NS_OK;
}

} 
} 
