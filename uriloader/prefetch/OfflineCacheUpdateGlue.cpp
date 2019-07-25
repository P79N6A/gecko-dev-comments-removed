





































#include "OfflineCacheUpdateGlue.h"
#include "nsOfflineCacheUpdate.h"
#include "mozilla/Services.h"

#include "nsIApplicationCache.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIApplicationCacheContainer.h"
#include "nsIChannel.h"
#include "nsIDocument.h"
#include "prlog.h"

#if defined(PR_LOGGING)









extern PRLogModuleInfo *gOfflineCacheUpdateLog;
#endif
#define LOG(args) PR_LOG(gOfflineCacheUpdateLog, 4, args)
#define LOG_ENABLED() PR_LOG_TEST(gOfflineCacheUpdateLog, 4)

namespace mozilla {
namespace docshell {





NS_IMPL_ISUPPORTS3(OfflineCacheUpdateGlue,
                   nsIOfflineCacheUpdate,
                   nsIOfflineCacheUpdateObserver,
                   nsISupportsWeakReference)





OfflineCacheUpdateGlue::OfflineCacheUpdateGlue()
{
    LOG(("OfflineCacheUpdateGlue::OfflineCacheUpdateGlue [%p]", this));
}

OfflineCacheUpdateGlue::~OfflineCacheUpdateGlue()
{
    LOG(("OfflineCacheUpdateGlue::~OfflineCacheUpdateGlue [%p]", this));
}

nsIOfflineCacheUpdate*
OfflineCacheUpdateGlue::EnsureUpdate()
{
    if (!mUpdate) {
        mUpdate = new nsOfflineCacheUpdate();
        LOG(("OfflineCacheUpdateGlue [%p] is using update [%p]", this, mUpdate.get()));
    }

    return mUpdate;
}

NS_IMETHODIMP
OfflineCacheUpdateGlue::Schedule()
{
    nsCOMPtr<nsIObserverService> observerService =
        mozilla::services::GetObserverService();
    if (observerService) {
        LOG(("Calling offline-cache-update-added"));
        observerService->NotifyObservers(static_cast<nsIOfflineCacheUpdate*>(this),
                                         "offline-cache-update-added",
                                         nsnull);
        LOG(("Done offline-cache-update-added"));
    }

    if (!EnsureUpdate())
        return NS_ERROR_NULL_POINTER;

    
    mUpdate->AddObserver(this, false);

    return mUpdate->Schedule();
}

NS_IMETHODIMP
OfflineCacheUpdateGlue::Init(nsIURI *aManifestURI, 
                             nsIURI *aDocumentURI,
                             nsIDOMDocument *aDocument)
{
    if (!EnsureUpdate())
        return NS_ERROR_NULL_POINTER;

    mDocumentURI = aDocumentURI;

    if (aDocument)
        SetDocument(aDocument);

    return mUpdate->Init(aManifestURI, aDocumentURI, nsnull);
}

void
OfflineCacheUpdateGlue::SetDocument(nsIDOMDocument *aDocument)
{
    
    NS_ASSERTION(!mDocument, 
                 "Setting more then a single document on an instance of OfflineCacheUpdateGlue");

    LOG(("Document %p added to update glue %p", aDocument, this));

    
    
    
    
    nsCOMPtr<nsIDocument> document = do_QueryInterface(aDocument);
    if (!document)
        return;

    nsIChannel* channel = document->GetChannel();
    nsCOMPtr<nsIApplicationCacheChannel> appCacheChannel =
        do_QueryInterface(channel);
    if (!appCacheChannel)
        return;

    bool loadedFromAppCache;
    appCacheChannel->GetLoadedFromApplicationCache(&loadedFromAppCache);
    if (loadedFromAppCache)
        return;

    if (EnsureUpdate()) {
        mUpdate->StickDocument(mDocumentURI);
    }

    mDocument = aDocument;
}

NS_IMETHODIMP
OfflineCacheUpdateGlue::UpdateStateChanged(nsIOfflineCacheUpdate *aUpdate, PRUint32 state)
{
    if (state == nsIOfflineCacheUpdateObserver::STATE_FINISHED) {
        LOG(("OfflineCacheUpdateGlue got STATE_FINISHED [%p]", this));

        nsCOMPtr<nsIObserverService> observerService =
          mozilla::services::GetObserverService();
        if (observerService) {
            LOG(("Calling offline-cache-update-completed"));
            observerService->NotifyObservers(static_cast<nsIOfflineCacheUpdate*>(this),
                                             "offline-cache-update-completed",
                                             nsnull);
            LOG(("Done offline-cache-update-completed"));
        }

        aUpdate->RemoveObserver(this);
    }

    return NS_OK;
}

NS_IMETHODIMP
OfflineCacheUpdateGlue::ApplicationCacheAvailable(nsIApplicationCache *aApplicationCache)
{
    NS_ENSURE_ARG(aApplicationCache);

    
    
    
    nsCOMPtr<nsIApplicationCacheContainer> container =
        do_QueryInterface(mDocument);
    if (!container)
        return NS_OK;

    nsCOMPtr<nsIApplicationCache> existingCache;
    nsresult rv = container->GetApplicationCache(getter_AddRefs(existingCache));
    NS_ENSURE_SUCCESS(rv, rv);

    if (!existingCache) {
#if defined(PR_LOGGING)
        if (LOG_ENABLED()) {
            nsCAutoString clientID;
            if (aApplicationCache) {
                aApplicationCache->GetClientID(clientID);
            }
            LOG(("Update %p: associating app cache %s to document %p",
                 this, clientID.get(), mDocument.get()));
        }
#endif

        rv = container->SetApplicationCache(aApplicationCache);
        NS_ENSURE_SUCCESS(rv, rv);
    }

    return NS_OK;
}

}
}
