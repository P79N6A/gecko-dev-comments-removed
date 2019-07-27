





#ifndef mozilla_net_ChildDNSService_h
#define mozilla_net_ChildDNSService_h


#include "nsPIDNSService.h"
#include "nsIObserver.h"
#include "mozilla/Attributes.h"
#include "mozilla/Mutex.h"
#include "DNSRequestChild.h"
#include "nsHashKeys.h"
#include "nsClassHashtable.h"

namespace mozilla {
namespace net {

class ChildDNSService MOZ_FINAL
  : public nsPIDNSService
  , public nsIObserver
{
public:
  
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSPIDNSSERVICE
  NS_DECL_NSIDNSSERVICE
  NS_DECL_NSIOBSERVER

  ChildDNSService();

  static ChildDNSService* GetSingleton();

  void NotifyRequestDone(DNSRequestChild *aDnsRequest);
private:
  virtual ~ChildDNSService();

  void MOZ_ALWAYS_INLINE GetDNSRecordHashKey(const nsACString &aHost,
                                             uint32_t aFlags,
                                             nsIDNSListener* aListener,
                                             nsACString &aHashKey);

  bool mFirstTime;
  bool mOffline;
  bool mDisablePrefetch;

  
  nsClassHashtable<nsCStringHashKey, nsTArray<nsRefPtr<DNSRequestChild>>> mPendingRequests;
  Mutex mPendingRequestsLock;
};

} 
} 
#endif 
