



#ifndef mozilla_dom_mobileconnection_MobileConnectionIPCService_h
#define mozilla_dom_mobileconnection_MobileConnectionIPCService_h

#include "nsCOMPtr.h"
#include "MobileConnectionChild.h"
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

  
  nsresult
  SendRequest(uint32_t aClientId, MobileConnectionRequest aRequest,
              nsIMobileConnectionCallback* aRequestCallback);

  nsTArray<nsRefPtr<MobileConnectionChild>> mClients;
};

} 
} 
} 

#endif 
