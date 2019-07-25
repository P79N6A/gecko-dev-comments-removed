





































#ifndef nsOfflineCacheUpdateParent_h
#define nsOfflineCacheUpdateParent_h

#include "mozilla/docshell/POfflineCacheUpdateParent.h"
#include "nsIOfflineCacheUpdate.h"

#include "nsString.h"

namespace mozilla {
namespace docshell {

class OfflineCacheUpdateParent : public POfflineCacheUpdateParent
                               , public nsIOfflineCacheUpdateObserver
{
    NS_DECL_ISUPPORTS
    NS_DECL_NSIOFFLINECACHEUPDATEOBSERVER

    nsresult
    Schedule(const URI& manifestURI,
             const URI& documentURI,
             const nsCString& clientID,
             const bool& stickDocument);

    OfflineCacheUpdateParent();
    ~OfflineCacheUpdateParent();

private:
    void RefcountHitZero();
};

}
}

#endif
