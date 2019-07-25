



#ifndef mozilla_dom_icc_IccManager_h
#define mozilla_dom_icc_IccManager_h

#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMIccManager.h"
#include "nsIObserver.h"

namespace mozilla {
namespace dom {
namespace icc {

class IccManager : public nsDOMEventTargetHelper
                 , public nsIDOMMozIccManager
                 , public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER
  NS_DECL_NSIDOMMOZICCMANAGER

  NS_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper::)

  IccManager();

  void Init(nsPIDOMWindow *aWindow);
  void Shutdown();

  NS_DECL_CYCLE_COLLECTION_CLASS_INHERITED(IccManager,
                                           nsDOMEventTargetHelper)

private:
  NS_DECL_EVENT_HANDLER(stkcommand)
  NS_DECL_EVENT_HANDLER(stksessionend)
};

} 
} 
} 

#endif
