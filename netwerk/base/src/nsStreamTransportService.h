



#include "nsIStreamTransportService.h"
#include "nsIEventTarget.h"
#include "nsIObserver.h"
#include "nsCOMPtr.h"
#include "nsThreadUtils.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"

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

    nsStreamTransportService() : mShutdownLock("nsStreamTransportService.mShutdownLock"),
                                 mIsShutdown(false) {}

private:
    ~nsStreamTransportService();

    nsCOMPtr<nsIThreadPool> mPool;

    mozilla::Mutex mShutdownLock;
    bool mIsShutdown;
};
