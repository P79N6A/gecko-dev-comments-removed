




#ifndef nsOfflineCacheUpdateChild_h
#define nsOfflineCacheUpdateChild_h

#include "mozilla/docshell/POfflineCacheUpdateChild.h"
#include "nsIOfflineCacheUpdate.h"

#include "nsCOMArray.h"
#include "nsCOMPtr.h"
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

    explicit OfflineCacheUpdateChild(nsIDOMWindow* aWindow);

    void SetDocument(nsIDOMDocument *aDocument);

private:
    ~OfflineCacheUpdateChild();

    nsresult AssociateDocument(nsIDOMDocument *aDocument,
                               nsIApplicationCache *aApplicationCache);
    void GatherObservers(nsCOMArray<nsIOfflineCacheUpdateObserver> &aObservers);
    nsresult Finish();

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
