



#include "nsIStreamTransportService.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

class nsIThreadPool;

class nsStreamTransportService MOZ_FINAL : public nsIStreamTransportService
                                         , public nsIEventTarget
                                         , public nsIObserver
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSISTREAMTRANSPORTSERVICE
    NS_DECL_NSIEVENTTARGET
    NS_DECL_NSIOBSERVER

    nsresult Init();

    nsStreamTransportService() {}

private:
    ~nsStreamTransportService();

    nsCOMPtr<nsIThreadPool> mPool;
};
