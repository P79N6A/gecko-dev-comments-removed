




#ifndef mozilla_dom_network_Connection_h
#define mozilla_dom_network_Connection_h

#include "nsIDOMConnection.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Observer.h"
#include "Types.h"

namespace mozilla {

namespace hal {
class NetworkInformation;
} 

namespace dom {
namespace network {

class Connection : public nsDOMEventTargetHelper
                 , public nsIDOMMozConnection
                 , public NetworkObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZCONNECTION

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  Connection();

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  
  void Notify(const hal::NetworkInformation& aNetworkInfo);

private:
  



  void UpdateFromNetworkInfo(const hal::NetworkInformation& aNetworkInfo);

  


  bool mCanBeMetered;

  


  double mBandwidth;

  static const char* sMeteredPrefName;
  static const bool  sMeteredDefaultValue;
};

} 
} 
} 

#endif 
