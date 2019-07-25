




#ifndef nsOfflineCacheUpdateParent_h
#define nsOfflineCacheUpdateParent_h

#include "mozilla/docshell/POfflineCacheUpdateParent.h"
#include "nsIOfflineCacheUpdate.h"

#include "nsString.h"

namespace mozilla {

namespace ipc {
class URIParams;
} 

namespace docshell {

class OfflineCacheUpdateParent : public POfflineCacheUpdateParent
                               , public nsIOfflineCacheUpdateObserver
{
    typedef mozilla::ipc::URIParams URIParams;

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATEOBSERVER

    nsresult
    Schedule(const URIParams& manifestURI,
             const URIParams& documentURI,
             const nsCString& clientID,
             const bool& stickDocument);

    OfflineCacheUpdateParent();
    ~OfflineCacheUpdateParent();

    virtual void ActorDestroy(ActorDestroyReason why);

private:
    void RefcountHitZero();
    bool mIPCClosed;
};

} 
} 

#endif
