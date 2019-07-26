




#ifndef mozilla_dom_network_Connection_h
#define mozilla_dom_network_Connection_h

#include "nsINetworkProperties.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Observer.h"
#include "Types.h"
#include "mozilla/dom/NetworkInformationBinding.h"

namespace mozilla {

namespace hal {
class NetworkInformation;
} 

namespace dom {
namespace network {

class Connection MOZ_FINAL : public nsDOMEventTargetHelper
                           , public NetworkObserver
                           , public nsINetworkProperties
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSINETWORKPROPERTIES

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  Connection();

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  
  void Notify(const hal::NetworkInformation& aNetworkInfo);

  

  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  ConnectionType Type() const { return mType; }

  IMPL_EVENT_HANDLER(typechange)

private:
  



  void UpdateFromNetworkInfo(const hal::NetworkInformation& aNetworkInfo);

  


  ConnectionType mType;

  


  bool mIsWifi;

  


  uint32_t mDHCPGateway;
};

} 
} 
} 

#endif 
