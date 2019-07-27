





#ifndef nsDNSService2_h__
#define nsDNSService2_h__

#include "nsPIDNSService.h"
#include "nsIIDNService.h"
#include "nsIMemoryReporter.h"
#include "nsIObserver.h"
#include "nsHostResolver.h"
#include "nsAutoPtr.h"
#include "nsString.h"
#include "nsTHashtable.h"
#include "nsHashKeys.h"
#include "nsIObserverService.h"
#include "nsProxyRelease.h"
#include "mozilla/Mutex.h"
#include "mozilla/Attributes.h"

class nsDNSService MOZ_FINAL : public nsPIDNSService
                             , public nsIObserver
                             , public nsIMemoryReporter
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSPIDNSSERVICE
    NS_DECL_NSIDNSSERVICE
    NS_DECL_NSIOBSERVER
    NS_DECL_NSIMEMORYREPORTER

    nsDNSService();

    static nsIDNSService* GetXPCOMSingleton();

    size_t SizeOfIncludingThis(mozilla::MallocSizeOf mallocSizeOf) const;

private:
    ~nsDNSService();

    static nsDNSService* GetSingleton();

    uint16_t GetAFForLookup(const nsACString &host, uint32_t flags);

    nsRefPtr<nsHostResolver>  mResolver;
    nsCOMPtr<nsIIDNService>   mIDN;

    
    mozilla::Mutex            mLock;

    
    
    
    nsAdoptingCString                         mIPv4OnlyDomains;
    bool                                      mDisableIPv6;
    bool                                      mDisablePrefetch;
    bool                                      mFirstTime;
    bool                                      mOffline;
    bool                                      mNotifyResolution;
    nsMainThreadPtrHandle<nsIObserverService> mObserverService;
    nsTHashtable<nsCStringHashKey>            mLocalDomains;
};

#endif 
