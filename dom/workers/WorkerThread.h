





#ifndef mozilla_dom_workers_WorkerThread_h__
#define mozilla_dom_workers_WorkerThread_h__

#include "mozilla/Attributes.h"
#include "mozilla/CondVar.h"
#include "mozilla/DebugOnly.h"
#include "nsISupportsImpl.h"
#include "nsRefPtr.h"
#include "nsThread.h"

class nsIRunnable;

namespace mozilla {
namespace dom {
namespace workers {

class RuntimeService;
class WorkerPrivate;
template <class> class WorkerPrivateParent;
class WorkerRunnable;




class WorkerThreadFriendKey
{
  friend class RuntimeService;
  friend class WorkerPrivate;
  friend class WorkerPrivateParent<WorkerPrivate>;

  WorkerThreadFriendKey();
  ~WorkerThreadFriendKey();
};

class WorkerThread MOZ_FINAL
  : public nsThread
{
  class Observer;

  CondVar mWorkerPrivateCondVar;

  
  WorkerPrivate* mWorkerPrivate;

  
  nsRefPtr<Observer> mObserver;

  
  bool mOtherThreadDispatchingViaEventTarget;

  
  DebugOnly<bool> mAcceptingNonWorkerRunnables;

public:
  static already_AddRefed<WorkerThread>
  Create(const WorkerThreadFriendKey& aKey);

  void
  SetWorker(const WorkerThreadFriendKey& aKey, WorkerPrivate* aWorkerPrivate);

  nsresult
  DispatchPrimaryRunnable(const WorkerThreadFriendKey& aKey,
                          nsIRunnable* aRunnable);

  nsresult
  Dispatch(const WorkerThreadFriendKey& aKey,
           WorkerRunnable* aWorkerRunnable);

  NS_DECL_ISUPPORTS_INHERITED

private:
  WorkerThread();
  ~WorkerThread();

  
  
  NS_IMETHOD
  Dispatch(nsIRunnable* aRunnable, uint32_t aFlags) MOZ_OVERRIDE;
};

} 
} 
} 

#endif 
