




#ifndef nsOfflineCacheUpdateChild_h
#define nsOfflineCacheUpdateChild_h

#include "mozilla/docshell/POfflineCacheUpdateChild.h"
#include "nsIOfflineCacheUpdate.h"

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
#include "nsICacheService.h"
#include "nsIDOMDocument.h"
#include "nsIObserver.h"
#include "nsIObserverService.h"
#include "nsIURI.h"
#include "nsString.h"
#include "nsWeakReference.h"

namespace mozilla {
namespace docshell {

class OfflineCacheUpdateChild : public nsIOfflineCacheUpdate
                              , public POfflineCacheUpdateChild
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATE

    virtual bool
    RecvNotifyStateEvent(const uint32_t& stateEvent,
                         const uint64_t& byteProgress) MOZ_OVERRIDE;

    virtual bool
    RecvAssociateDocuments(
            const nsCString& cacheGroupId,
            const nsCString& cacheClientId) MOZ_OVERRIDE;

    virtual bool
    RecvFinish(const bool& succeeded,
               const bool& isUpgrade) MOZ_OVERRIDE;

    OfflineCacheUpdateChild(nsIDOMWindow* aWindow);
    ~OfflineCacheUpdateChild();

    void SetDocument(nsIDOMDocument *aDocument);

private:
    nsresult AssociateDocument(nsIDOMDocument *aDocument,
                               nsIApplicationCache *aApplicationCache);
    void GatherObservers(nsCOMArray<nsIOfflineCacheUpdateObserver> &aObservers);
    nsresult Finish();

    void RefcountHitZero();

    enum {
        STATE_UNINITIALIZED,
        STATE_INITIALIZED,
        STATE_CHECKING,
        STATE_DOWNLOADING,
        STATE_CANCELLED,
        STATE_FINISHED
    } mState;

    bool mIsUpgrade;
    bool mSucceeded;
    bool mIPCActivated;

    nsCString mUpdateDomain;
    nsCOMPtr<nsIURI> mManifestURI;
    nsCOMPtr<nsIURI> mDocumentURI;

    nsCOMPtr<nsIObserverService> mObserverService;

    uint32_t mAppID;
    bool mInBrowser;

    
    nsCOMArray<nsIWeakReference> mWeakObservers;
    nsCOMArray<nsIOfflineCacheUpdateObserver> mObservers;

    
    nsCOMPtr<nsIDOMDocument> mDocument;

    

    nsCOMPtr<nsIDOMWindow> mWindow;

    uint64_t mByteProgress;
};

}
}

#endif
