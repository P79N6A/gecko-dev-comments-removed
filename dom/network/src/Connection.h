




































#ifndef mozilla_dom_network_Connection_h
#define mozilla_dom_network_Connection_h

#include "nsIDOMConnection.h"
#include "nsDOMEventTargetWrapperCache.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Observer.h"
#include "Types.h"

namespace mozilla {

namespace hal {
class NetworkInformation;
} 

namespace dom {
namespace network {

class Connection : public nsDOMEventTargetWrapperCache
                 , public nsIDOMMozConnection
                 , public NetworkObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIDOMMOZCONNECTION

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetWrapperCache::)

  Connection();

  void Init(nsPIDOMWindow *aWindow, nsIScriptContext* aScriptContext);
  void Shutdown();

  
  void Notify(const hal::NetworkInformation& aNetworkInfo);

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(Connection,
                                           nsDOMEventTargetWrapperCache)

private:
  


  nsresult DispatchTrustedEventToSelf(const nsAString& aEventName);

  



  void UpdateFromNetworkInfo(const hal::NetworkInformation& aNetworkInfo);

  


  bool mCanBeMetered;

  


  double mBandwidth;

  NS_DECL_EVENT_HANDLER(change)

  static const char* sMeteredPrefName;
  static const bool  sMeteredDefaultValue;
};

} 
} 
} 

#endif 
