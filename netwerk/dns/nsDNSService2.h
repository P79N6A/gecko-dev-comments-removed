



































#include "nsPIDNSService.h"
#include "nsIIDNService.h"
#include "nsIObserver.h"
#include "nsHostResolver.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "mozilla/Mutex.h"

class nsDNSService : public nsPIDNSService
                   , public nsIObserver
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSPIDNSSERVICE
    NS_DECL_NSIDNSSERVICE
    NS_DECL_NSIOBSERVER

    nsDNSService();
    ~nsDNSService();

private:
    PRUint16 GetAFForLookup(const nsACString &host, PRUint32 flags);

    nsRefPtr<nsHostResolver>  mResolver;
    nsCOMPtr<nsIIDNService>   mIDN;

    
    mozilla::Mutex            mLock;

    
    
    
    nsAdoptingCString         mIPv4OnlyDomains;
    bool                      mDisableIPv6;
    bool                      mDisablePrefetch;
    bool                      mFirstTime;
};
