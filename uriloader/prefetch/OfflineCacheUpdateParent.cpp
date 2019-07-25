





































#include "OfflineCacheUpdateParent.h"
#include "nsOfflineCacheUpdate.h"
#include "nsIApplicationCache.h"

#if defined(PR_LOGGING)









extern PRLogModuleInfo *gOfflineCacheUpdateLog;
#endif
#define LOG(args) PR_LOG(gOfflineCacheUpdateLog, 4, args)
#define LOG_ENABLED() PR_LOG_TEST(gOfflineCacheUpdateLog, 4)

namespace mozilla {
namespace docshell {





NS_IMPL_ISUPPORTS1(OfflineCacheUpdateParent,
                   nsIOfflineCacheUpdateObserver)





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
OfflineCacheUpdateParent::Schedule(const URI& aManifestURI,
                                   const URI& aDocumentURI,
                                   const nsCString& aClientID,
                                   const bool& stickDocument)
{
    LOG(("OfflineCacheUpdateParent::RecvSchedule [%p]", this));

    nsRefPtr<nsOfflineCacheUpdate> update;
    nsCOMPtr<nsIURI> manifestURI(aManifestURI);
    nsCOMPtr<nsIURI> documentURI(aDocumentURI);

    nsOfflineCacheUpdateService* service =
        nsOfflineCacheUpdateService::EnsureService();
    if (!service)
        return NS_ERROR_FAILURE;

    service->FindUpdate(manifestURI, documentURI, getter_AddRefs(update));
    if (!update) {
        update = new nsOfflineCacheUpdate();

        nsresult rv;
        
        
        rv = update->Init(manifestURI, documentURI, nsnull);
        NS_ENSURE_SUCCESS(rv, rv);

        rv = update->Schedule();
        NS_ENSURE_SUCCESS(rv, rv);
    }

    update->AddObserver(this, PR_FALSE);

    if (stickDocument) {
        nsCOMPtr<nsIURI> stickURI;
        documentURI->Clone(getter_AddRefs(stickURI));
        update->StickDocument(stickURI);
    }

    return NS_OK;
}

NS_IMETHODIMP
OfflineCacheUpdateParent::UpdateStateChanged(nsIOfflineCacheUpdate *aUpdate, PRUint32 state)
{
    if (mIPCClosed)
        return NS_ERROR_UNEXPECTED;

    LOG(("OfflineCacheUpdateParent::StateEvent [%p]", this));

    SendNotifyStateEvent(state);

    if (state == nsIOfflineCacheUpdateObserver::STATE_FINISHED) {
        
        
        
        PRBool isUpgrade;
        aUpdate->GetIsUpgrade(&isUpgrade);
        PRBool succeeded;
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

} 
} 
