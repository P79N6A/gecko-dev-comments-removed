





#ifndef mozilla_dom_cache_Context_h
#define mozilla_dom_cache_Context_h

#include "mozilla/dom/cache/Types.h"
#include "nsAutoPtr.h"
#include "nsCOMPtr.h"
#include "nsISupportsImpl.h"
#include "nsString.h"
#include "nsTArray.h"

class nsIEventTarget;
class nsIFile;

namespace mozilla {
namespace dom {
namespace cache {

class Action;
class Manager;



















class Context MOZ_FINAL
{
public:
  static already_AddRefed<Context>
  Create(Manager* aManager, Action* aQuotaIOThreadAction);

  
  
  
  
  void Dispatch(nsIEventTarget* aTarget, Action* aAction);

  
  
  
  
  
  void CancelAll();

  
  
  
  
  void CancelForCacheId(CacheId aCacheId);

private:
  class QuotaInitRunnable;
  class ActionRunnable;

  enum State
  {
    STATE_CONTEXT_INIT,
    STATE_CONTEXT_READY,
    STATE_CONTEXT_CANCELED
  };

  struct PendingAction
  {
    nsCOMPtr<nsIEventTarget> mTarget;
    nsRefPtr<Action> mAction;
  };

  explicit Context(Manager* aManager);
  ~Context();
  void DispatchAction(nsIEventTarget* aTarget, Action* aAction);
  void OnQuotaInit(nsresult aRv, const QuotaInfo& aQuotaInfo);
  void OnActionRunnableComplete(ActionRunnable* const aAction);

  nsRefPtr<Manager> mManager;
  State mState;
  QuotaInfo mQuotaInfo;
  nsTArray<PendingAction> mPendingActions;

  
  nsTArray<ActionRunnable*> mActionRunnables;

public:
  NS_INLINE_DECL_REFCOUNTING(cache::Context)
};

} 
} 
} 

#endif
