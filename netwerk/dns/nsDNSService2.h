



#include "nsPIDNSService.h"
#include "nsIIDNService.h"
#include "nsIObserver.h"
#include "nsHostResolver.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "mozilla/Mutex.h"
#include "mozilla/Attributes.h"

class nsDNSService MOZ_FINAL : public nsPIDNSService
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
    uint16_t GetAFForLookup(const nsACString &host, uint32_t flags);

    nsRefPtr<nsHostResolver>  mResolver;
    nsCOMPtr<nsIIDNService>   mIDN;

    
    mozilla::Mutex            mLock;

    
    
    
    nsAdoptingCString         mIPv4OnlyDomains;
    bool                      mDisableIPv6;
    bool                      mDisablePrefetch;
    bool                      mFirstTime;
    nsTHashtable<nsCStringHashKey> mLocalDomains;
};
