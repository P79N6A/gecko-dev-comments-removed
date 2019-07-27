




#ifndef mozilla_netwerk_dns_mdns_libmdns_nsDNSServiceDiscovery_h
#define mozilla_netwerk_dns_mdns_libmdns_nsDNSServiceDiscovery_h

#include "nsIDNSServiceDiscovery.h"
#include "nsCOMPtr.h"
#include "nsRefPtr.h"
#include "nsRefPtrHashtable.h"

namespace mozilla {
namespace net {

class BrowseOperator;
class RegisterOperator;

class nsDNSServiceDiscovery final : public nsIDNSServiceDiscovery
{
public:
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIDNSSERVICEDISCOVERY

  explicit nsDNSServiceDiscovery() = default;

  




  nsresult Init();

  nsresult StopDiscovery(nsIDNSServiceDiscoveryListener* aListener);
  nsresult UnregisterService(nsIDNSRegistrationListener* aListener);

private:
  virtual ~nsDNSServiceDiscovery() = default;

  nsRefPtrHashtable<nsISupportsHashKey, BrowseOperator> mDiscoveryMap;
  nsRefPtrHashtable<nsISupportsHashKey, RegisterOperator> mRegisterMap;
};

} 
} 

#endif 
