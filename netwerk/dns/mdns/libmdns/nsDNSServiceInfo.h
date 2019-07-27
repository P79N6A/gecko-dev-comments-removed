




#ifndef mozilla_netwerk_dns_mdns_libmdns_nsDNSServiceInfo_h
#define mozilla_netwerk_dns_mdns_libmdns_nsDNSServiceInfo_h

#include "nsCOMPtr.h"
#include "nsIDNSServiceDiscovery.h"
#include "nsIPropertyBag2.h"
#include "nsString.h"

namespace mozilla {
namespace net {

class nsDNSServiceInfo final : public nsIDNSServiceInfo
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDNSSERVICEINFO

  explicit nsDNSServiceInfo() = default;
  explicit nsDNSServiceInfo(nsIDNSServiceInfo* aServiceInfo);

private:
  virtual ~nsDNSServiceInfo() = default;

private:
  nsCString mHost;
  uint16_t mPort = 0;
  nsCString mServiceName;
  nsCString mServiceType;
  nsCString mDomainName;
  nsCOMPtr<nsIPropertyBag2> mAttributes;

  bool mIsHostSet = false;
  bool mIsPortSet = false;
  bool mIsServiceNameSet = false;
  bool mIsServiceTypeSet = false;
  bool mIsDomainNameSet = false;
  bool mIsAttributesSet = false;
};

} 
} 

#endif 
