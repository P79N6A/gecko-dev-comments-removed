



#ifndef mozilla_dom_mobileconnection_MobileConnectionIPCService_h
#define mozilla_dom_mobileconnection_MobileConnectionIPCService_h

#include "nsCOMPtr.h"
#include "mozilla/dom/mobileconnection/MobileConnectionChild.h"
#include "nsIMobileConnectionService.h"

namespace mozilla {
namespace dom {
namespace mobileconnection {

class MobileConnectionIPCService final : public nsIMobileConnectionService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONSERVICE

  MobileConnectionIPCService();

private:
  
  ~MobileConnectionIPCService();

  nsTArray<nsRefPtr<MobileConnectionChild>> mItems;
};

} 
} 
} 

#endif 
