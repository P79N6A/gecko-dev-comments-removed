



#include "mozilla/net/ChildDNSService.h"
#include "nsIDNSListener.h"
#include "nsNetUtil.h"
#include "nsIThread.h"
#include "nsThreadUtils.h"
#include "nsIXPConnect.h"
#include "nsIPrefService.h"
#include "nsIProtocolProxyService.h"
#include "mozilla/net/NeckoChild.h"
#include "mozilla/net/DNSListenerProxy.h"

namespace mozilla {
namespace net {





static ChildDNSService *gChildDNSService;
static const char kPrefNameDisablePrefetch[] = "network.dns.disablePrefetch";

ChildDNSService* ChildDNSService::GetSingleton()
{
  MOZ_ASSERT(IsNeckoChild());

  if (!gChildDNSService) {
    gChildDNSService = new ChildDNSService();
  }

  NS_ADDREF(gChildDNSService);
  return gChildDNSService;
}

NS_IMPL_ISUPPORTS(ChildDNSService,
                  nsIDNSService,
                  nsPIDNSService,
                  nsIObserver)

ChildDNSService::ChildDNSService()
  : mFirstTime(true)
  , mOffline(false)
  , mPendingRequestsLock("DNSPendingRequestsLock")
{
  MOZ_ASSERT(IsNeckoChild());
}

ChildDNSService::~ChildDNSService()
{

}

void
ChildDNSService::GetDNSRecordHashKey(const nsACString &aHost,
                                     uint32_t aFlags,
                                     nsIDNSListener* aListener,
                                     nsACString &aHashKey)
{
  aHashKey.Assign(aHost);
  aHashKey.AppendInt(aFlags);
  aHashKey.AppendPrintf("%p", aListener);
}





NS_IMETHODIMP
ChildDNSService::AsyncResolve(const nsACString  &hostname,
                              uint32_t           flags,
                              nsIDNSListener    *listener,
                              nsIEventTarget    *target_,
                              nsICancelable    **result)
{
  NS_ENSURE_TRUE(gNeckoChild != nullptr, NS_ERROR_FAILURE);

  if (mDisablePrefetch && (flags & RESOLVE_SPECULATE)) {
    return NS_ERROR_DNS_LOOKUP_QUEUE_FULL;
  }

  
  uint32_t originalFlags = flags;

  
  
  if (mOffline) {
    flags |= RESOLVE_OFFLINE;
  }

  
  nsIDNSListener *originalListener = listener;

  
  nsCOMPtr<nsIEventTarget> target = target_;
  nsCOMPtr<nsIXPConnectWrappedJS> wrappedListener = do_QueryInterface(listener);
  if (wrappedListener && !target) {
    nsCOMPtr<nsIThread> mainThread;
    NS_GetMainThread(getter_AddRefs(mainThread));
    target = do_QueryInterface(mainThread);
  }
  if (target) {
    
    
    listener = new DNSListenerProxy(listener, target);
  }

  nsRefPtr<DNSRequestChild> childReq =
    new DNSRequestChild(nsCString(hostname), flags, listener, target);

  {
    MutexAutoLock lock(mPendingRequestsLock);
    nsCString key;
    GetDNSRecordHashKey(hostname, originalFlags, originalListener, key);
    nsTArray<nsRefPtr<DNSRequestChild>> *hashEntry;
    if (mPendingRequests.Get(key, &hashEntry)) {
      hashEntry->AppendElement(childReq);
    } else {
      hashEntry = new nsTArray<nsRefPtr<DNSRequestChild>>();
      hashEntry->AppendElement(childReq);
      mPendingRequests.Put(key, hashEntry);
    }
  }

  childReq->StartRequest();

  childReq.forget(result);
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSService::CancelAsyncResolve(const nsACString  &aHostname,
                                    uint32_t           aFlags,
                                    nsIDNSListener    *aListener,
                                    nsresult           aReason)
{
  if (mDisablePrefetch && (aFlags & RESOLVE_SPECULATE)) {
    return NS_ERROR_DNS_LOOKUP_QUEUE_FULL;
  }

  MutexAutoLock lock(mPendingRequestsLock);
  nsTArray<nsRefPtr<DNSRequestChild>> *hashEntry;
  nsCString key;
  GetDNSRecordHashKey(aHostname, aFlags, aListener, key);
  if (mPendingRequests.Get(key, &hashEntry)) {
    
    hashEntry->ElementAt(0)->Cancel(aReason);
  }

  return NS_OK;
}

NS_IMETHODIMP
ChildDNSService::Resolve(const nsACString &hostname,
                         uint32_t          flags,
                         nsIDNSRecord    **result)
{
  
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
ChildDNSService::GetDNSCacheEntries(nsTArray<mozilla::net::DNSCacheEntries> *args)
{
  
  
  
  return NS_ERROR_NOT_AVAILABLE;
}

NS_IMETHODIMP
ChildDNSService::GetMyHostName(nsACString &result)
{
  
  return NS_ERROR_NOT_AVAILABLE;
}

void
ChildDNSService::NotifyRequestDone(DNSRequestChild *aDnsRequest)
{
  
  uint32_t originalFlags = aDnsRequest->mFlags & ~RESOLVE_OFFLINE;
  nsCOMPtr<nsIDNSListener> originalListener = aDnsRequest->mListener;
  nsCOMPtr<nsIDNSListenerProxy> wrapper = do_QueryInterface(originalListener);
  if (wrapper) {
    wrapper->GetOriginalListener(getter_AddRefs(originalListener));
    if (NS_WARN_IF(!originalListener)) {
      MOZ_ASSERT(originalListener);
      return;
    }
  }

  MutexAutoLock lock(mPendingRequestsLock);

  nsCString key;
  GetDNSRecordHashKey(aDnsRequest->mHost, originalFlags, originalListener, key);

  nsTArray<nsRefPtr<DNSRequestChild>> *hashEntry;

  if (mPendingRequests.Get(key, &hashEntry)) {
    int idx;
    if ((idx = hashEntry->IndexOf(aDnsRequest))) {
      hashEntry->RemoveElementAt(idx);
      if (hashEntry->IsEmpty()) {
        mPendingRequests.Remove(key);
      }
    }
  }
}





nsresult
ChildDNSService::Init()
{
  
  
  bool disablePrefetch = false;
  int  proxyType = nsIProtocolProxyService::PROXYCONFIG_DIRECT;

  nsCOMPtr<nsIPrefBranch> prefs = do_GetService(NS_PREFSERVICE_CONTRACTID);
  prefs->GetBoolPref(kPrefNameDisablePrefetch, &disablePrefetch);
  if (prefs) {
    prefs->GetIntPref("network.proxy.type", &proxyType);
    prefs->GetBoolPref(kPrefNameDisablePrefetch, &disablePrefetch);
  }

  if (mFirstTime) {
    mFirstTime = false;
    if (prefs) {
      prefs->AddObserver(kPrefNameDisablePrefetch, this, false);

      
      
      prefs->AddObserver("network.proxy.type", this, false);
    }
  }

  mDisablePrefetch = disablePrefetch ||
                     (proxyType == nsIProtocolProxyService::PROXYCONFIG_MANUAL);

  return NS_OK;
}

nsresult
ChildDNSService::Shutdown()
{
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSService::GetPrefetchEnabled(bool *outVal)
{
  *outVal = !mDisablePrefetch;
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSService::SetPrefetchEnabled(bool inVal)
{
  mDisablePrefetch = !inVal;
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSService::GetOffline(bool* aResult)
{
  *aResult = mOffline;
  return NS_OK;
}

NS_IMETHODIMP
ChildDNSService::SetOffline(bool value)
{
  mOffline = value;
  return NS_OK;
}





NS_IMETHODIMP
ChildDNSService::Observe(nsISupports *subject, const char *topic,
                         const char16_t *data)
{
  
  NS_ASSERTION(strcmp(topic, NS_PREFBRANCH_PREFCHANGE_TOPIC_ID) == 0,
               "unexpected observe call");

  
  Init();
  return NS_OK;
}

} 
} 
