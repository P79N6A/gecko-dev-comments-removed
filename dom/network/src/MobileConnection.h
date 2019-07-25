



#ifndef mozilla_dom_network_MobileConnection_h
#define mozilla_dom_network_MobileConnection_h

#include "nsIObserver.h"
#include "nsIDOMMobileConnection.h"
#include "nsIMobileConnectionProvider.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {
namespace network {

class MobileConnection : public nsDOMEventTargetHelper
                       , public nsIDOMMozMobileConnection
                       , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMMOZMOBILECONNECTION

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  MobileConnection();

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MobileConnection,
                                           nsDOMEventTargetHelper)

private:
  nsCOMPtr<nsIMobileConnectionProvider> mProvider;

  nsresult InternalDispatchEvent(const nsAString& aType);

  NS_DECL_EVENT_HANDLER(cardstatechange)
  NS_DECL_EVENT_HANDLER(voicechange)
  NS_DECL_EVENT_HANDLER(datachange)
};

} 
} 
} 

#endif
