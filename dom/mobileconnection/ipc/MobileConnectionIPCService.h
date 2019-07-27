



#ifndef mozilla_dom_mobileconnection_MobileConnectionIPCService_h
#define mozilla_dom_mobileconnection_MobileConnectionIPCService_h

#include "nsCOMPtr.h"
#include "mozilla/dom/mobileconnection/MobileConnectionChild.h"
#include "nsIMobileConnectionService.h"

namespace mozilla {
namespace dom {
namespace mobileconnection {

class MobileConnectionIPCService MOZ_FINAL : public nsIMobileConnectionService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMOBILECONNECTIONSERVICE

  static MobileConnectionIPCService*
  GetSingleton();

private:
  MobileConnectionIPCService();

  
  ~MobileConnectionIPCService();

  nsTArray<nsRefPtr<MobileConnectionChild>> mItems;
};

} 
} 
} 

#endif 
