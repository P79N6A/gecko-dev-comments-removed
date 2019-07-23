




































#include "nsIStreamTransportService.h"
#include "nsIEventTarget.h"
#include "nsIThreadPool.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"

class nsStreamTransportService : public nsIStreamTransportService
                               , public nsIEventTarget 
                               , public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSISTREAMTRANSPORTSERVICE
    NS_DECL_NSIEVENTTARGET
    NS_DECL_NSIOBSERVER

    nsresult Init();

    nsStreamTransportService() {}

private:
    ~nsStreamTransportService();

    nsCOMPtr<nsIThreadPool> mPool;
};
