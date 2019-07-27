




#ifndef mozilla_dom_network_Connection_h
#define mozilla_dom_network_Connection_h

#include "Types.h"
#include "mozilla/DOMEventTargetHelper.h"
#include "mozilla/Observer.h"
#include "mozilla/dom/NetworkInformationBinding.h"
#include "nsCycleCollectionParticipant.h"
#include "nsINetworkProperties.h"

namespace mozilla {

namespace hal {
class NetworkInformation;
} 

namespace dom {
namespace network {

class Connection MOZ_FINAL : public DOMEventTargetHelper
                           , public NetworkObserver
                           , public nsINetworkProperties
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSINETWORKPROPERTIES

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(DOMEventTargetHelper)

  Connection();

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  
  void Notify(const hal::NetworkInformation& aNetworkInfo);

  

  virtual JSObject* WrapObject(JSContext* aCx) MOZ_OVERRIDE;

  ConnectionType Type() const { return mType; }

  IMPL_EVENT_HANDLER(typechange)

private:
  ~Connection() {}

  



  void UpdateFromNetworkInfo(const hal::NetworkInformation& aNetworkInfo);

  


  ConnectionType mType;

  


  bool mIsWifi;

  


  uint32_t mDHCPGateway;
};

} 
} 
} 

#endif 
