



#ifndef mozilla_dom_IccManager_h
#define mozilla_dom_IccManager_h

#include "nsCycleCollectionParticipant.h"
#include "nsDOMEventTargetHelper.h"
#include "nsIDOMIccManager.h"
#include "nsIIccProvider.h"
#include "nsTArrayHelpers.h"

namespace mozilla {
namespace dom {

class IccListener;

class IccManager MOZ_FINAL : public nsDOMEventTargetHelper
                           , public nsIDOMMozIccManager
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIDOMMOZICCMANAGER

  NS_REALLY_FORWARD_NSIDOMEVENTTARGET(nsDOMEventTargetHelper)

  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS_INHERITED(IccManager,
                                                         nsDOMEventTargetHelper)

  IccManager(nsPIDOMWindow* aWindow);
  ~IccManager();

  void
  Shutdown();

  nsresult
  NotifyIccAdd(const nsAString& aIccId);

  nsresult
  NotifyIccRemove(const nsAString& aIccId);

private:
  nsTArray<nsRefPtr<IccListener>> mIccListeners;

  
  
  
  JS::Heap<JSObject*> mJsIccIds;
  bool mRooted;

  void Root();
  void Unroot();
};

} 
} 

#endif 
