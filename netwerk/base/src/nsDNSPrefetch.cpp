




#include "nsDNSPrefetch.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "nsThreadUtils.h"

#include "nsIDNSListener.h"
#include "nsIDNSService.h"
#include "nsICancelable.h"
#include "nsIURI.h"

static nsIDNSService *sDNSService = nullptr;

nsresult
nsDNSPrefetch::Initialize(nsIDNSService *aDNSService)
{
    NS_IF_RELEASE(sDNSService);
    sDNSService =  aDNSService;
    NS_IF_ADDREF(sDNSService);
    return NS_OK;
}

nsresult
nsDNSPrefetch::Shutdown()
{
    NS_IF_RELEASE(sDNSService);
    return NS_OK;
}

nsDNSPrefetch::nsDNSPrefetch(nsIURI *aURI,
                             nsIDNSListener *aListener,
                             bool storeTiming)
    : mStoreTiming(storeTiming)
    , mListener(do_GetWeakReference(aListener))
{
    aURI->GetAsciiHost(mHostname);
}

nsresult 
nsDNSPrefetch::Prefetch(uint16_t flags)
{
    if (mHostname.IsEmpty())
        return NS_ERROR_NOT_AVAILABLE;
  
    if (!sDNSService)
        return NS_ERROR_NOT_AVAILABLE;
    
    nsCOMPtr<nsICancelable> tmpOutstanding;  

    if (mStoreTiming)
        mStartTimestamp = mozilla::TimeStamp::Now();
    
    
    
    
    nsCOMPtr<nsIThread> mainThread = do_GetMainThread();
    return sDNSService->AsyncResolve(mHostname,
                                     flags | nsIDNSService::RESOLVE_SPECULATE,
                                     this, mainThread,
                                     getter_AddRefs(tmpOutstanding));
}

nsresult
nsDNSPrefetch::PrefetchLow(bool refreshDNS)
{
    return Prefetch(nsIDNSService::RESOLVE_PRIORITY_LOW |
      (refreshDNS ? nsIDNSService::RESOLVE_BYPASS_CACHE : 0));
}

nsresult
nsDNSPrefetch::PrefetchMedium(bool refreshDNS)
{
    return Prefetch(nsIDNSService::RESOLVE_PRIORITY_MEDIUM |
      (refreshDNS ? nsIDNSService::RESOLVE_BYPASS_CACHE : 0));
}

nsresult
nsDNSPrefetch::PrefetchHigh(bool refreshDNS)
{
    return Prefetch(refreshDNS ?
                    nsIDNSService::RESOLVE_BYPASS_CACHE : 0);
}


NS_IMPL_ISUPPORTS(nsDNSPrefetch, nsIDNSListener)

NS_IMETHODIMP
nsDNSPrefetch::OnLookupComplete(nsICancelable *request,
                                nsIDNSRecord  *rec,
                                nsresult       status)
{
    MOZ_ASSERT(NS_IsMainThread(), "Expecting DNS callback on main thread.");

    if (mStoreTiming) {
        mEndTimestamp = mozilla::TimeStamp::Now();
    }
    nsCOMPtr<nsIDNSListener> listener = do_QueryReferent(mListener);
    if (listener) {
      listener->OnLookupComplete(request, rec, status);
    }
    return NS_OK;
}
