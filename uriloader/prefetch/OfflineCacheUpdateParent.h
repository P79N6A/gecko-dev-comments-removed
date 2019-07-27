




#ifndef nsOfflineCacheUpdateParent_h
#define nsOfflineCacheUpdateParent_h

#include "mozilla/docshell/POfflineCacheUpdateParent.h"
#include "nsIOfflineCacheUpdate.h"

#include "nsString.h"
#include "nsILoadContext.h"

namespace mozilla {

namespace ipc {
class URIParams;
} 

namespace docshell {

class OfflineCacheUpdateParent : public POfflineCacheUpdateParent
                               , public nsIOfflineCacheUpdateObserver
                               , public nsILoadContext
{
    typedef mozilla::ipc::URIParams URIParams;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATEOBSERVER
    NS_DECL_NSILOADCONTEXT

    nsresult
    Schedule(const URIParams& manifestURI,
             const URIParams& documentURI,
             const bool& stickDocument);

    void
    StopSendingMessagesToChild()
    {
      mIPCClosed = true;
    }

    OfflineCacheUpdateParent(uint32_t aAppId, bool aIsInBrowser);

    virtual void ActorDestroy(ActorDestroyReason aWhy) override;

private:
    ~OfflineCacheUpdateParent();

    bool mIPCClosed;

    bool     mIsInBrowserElement;
    uint32_t mAppId;
};

} 
} 

#endif
