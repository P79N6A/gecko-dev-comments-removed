





#ifndef mozilla_dom_workers_WorkerThread_h__
#define mozilla_dom_workers_WorkerThread_h__

#include "mozilla/Attributes.h"
#include "mozilla/DebugOnly.h"
#include "nsISupportsImpl.h"
#include "nsRefPtr.h"
#include "nsThread.h"

class nsIRunnable;

namespace mozilla {
namespace dom {
namespace workers {

class WorkerPrivate;
class WorkerRunnable;

class WorkerThread MOZ_FINAL
  : public nsThread
{
  class Observer;

  WorkerPrivate* mWorkerPrivate;
  nsRefPtr<Observer> mObserver;

#ifdef DEBUG
  
  bool mAcceptingNonWorkerRunnables;
#endif

public:
  static already_AddRefed<WorkerThread>
  Create();

  void
  SetWorker(WorkerPrivate* aWorkerPrivate);

  NS_DECL_ISUPPORTS_INHERITED

  NS_IMETHOD
  Dispatch(nsIRunnable* aRunnable, uint32_t aFlags) MOZ_OVERRIDE;

#ifdef DEBUG
  bool
  IsAcceptingNonWorkerRunnables()
  {
    MutexAutoLock lock(mLock);
    return mAcceptingNonWorkerRunnables;
  }

  void
  SetAcceptingNonWorkerRunnables(bool aAcceptingNonWorkerRunnables)
  {
    MutexAutoLock lock(mLock);
    mAcceptingNonWorkerRunnables = aAcceptingNonWorkerRunnables;
  }
#endif

private:
  WorkerThread();

  ~WorkerThread();
};

} 
} 
} 

#endif 
