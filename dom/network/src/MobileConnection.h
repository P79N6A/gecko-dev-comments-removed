



#ifndef mozilla_dom_network_MobileConnection_h
#define mozilla_dom_network_MobileConnection_h

#include "nsIDOMMobileConnection.h"
#include "nsIMobileConnectionProvider.h"
#include "nsDOMEventTargetHelper.h"
#include "nsCycleCollectionParticipant.h"

namespace mozilla {
namespace dom {

namespace icc {
  class IccManager;
} 

namespace network {

class MobileConnection : public nsDOMEventTargetHelper
                       , public nsIDOMMozMobileConnection
{
  







  class Listener;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZMOBILECONNECTION
  NS_DECL_NSIMOBILECONNECTIONLISTENER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  MobileConnection();

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(MobileConnection,
                                           nsDOMEventTargetHelper)

private:
  nsCOMPtr<nsIMobileConnectionProvider> mProvider;
  nsRefPtr<Listener> mListener;
  nsRefPtr<icc::IccManager> mIccManager;

  nsIDOMEventTarget*
  ToIDOMEventTarget() const
  {
    return static_cast<nsDOMEventTargetHelper*>(
           const_cast<MobileConnection*>(this));
  }
};

} 
} 
} 

#endif 
