



#ifndef mozilla_dom_network_MobileConnection_h
#define mozilla_dom_network_MobileConnection_h

#include "nsIDOMMobileConnection.h"
#include "nsIMobileConnectionProvider.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWeakPtr.h"

namespace mozilla {
namespace dom {

class MobileConnection : public nsDOMEventTargetHelper
                       , public nsIDOMMozMobileConnection
{
  







  class Listener;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZMOBILECONNECTION
  NS_DECL_NSIMOBILECONNECTIONLISTENER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  MobileConnection(uint32_t aClientId);

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MobileConnection,
                                           nsDOMEventTargetHelper)

private:
  nsCOMPtr<nsIMobileConnectionProvider> mProvider;
  nsRefPtr<Listener> mListener;
  nsWeakPtr mWindow;

  uint32_t mClientId;

  bool CheckPermission(const char* aType);
};

} 
} 

#endif 
