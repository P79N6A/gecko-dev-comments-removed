



#ifndef mozilla_dom_IccManager_h
#define mozilla_dom_IccManager_h

#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMIccManager.h"
#include "nsIIccProvider.h"

namespace mozilla {
namespace dom {

class IccManager : public nsDOMEventTargetHelper
                 , public nsIDOMMozIccManager
{
  





  class Listener;

public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZICCMANAGER
  NS_DECL_NSIICCLISTENER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  IccManager();

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

private:
  nsCOMPtr<nsIIccProvider> mProvider;
  nsRefPtr<Listener> mListener;
};

} 
} 

#endif 
