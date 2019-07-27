



#ifndef mozilla_dom_PromiseWorkerProxy_h
#define mozilla_dom_PromiseWorkerProxy_h


#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseNativeHandler.h"
#include "mozilla/dom/workers/bindings/WorkerFeature.h"
#include "nsProxyRelease.h"

namespace mozilla {
namespace dom {

class Promise;

namespace workers {
class WorkerPrivate;
}





































class PromiseWorkerProxy : public PromiseNativeHandler,
                           public workers::WorkerFeature
{
  friend class PromiseWorkerProxyRunnable;

  
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(PromiseWorkerProxy)

public:
  PromiseWorkerProxy(workers::WorkerPrivate* aWorkerPrivate,
                     Promise* aWorkerPromise,
                     JSStructuredCloneCallbacks* aCallbacks = nullptr);

  workers::WorkerPrivate* GetWorkerPrivate() const;

  Promise* GetWorkerPromise() const;

  void StoreISupports(nsISupports* aSupports);

  void CleanUp(JSContext* aCx);

protected:
  virtual void ResolvedCallback(JSContext* aCx,
                                JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  virtual void RejectedCallback(JSContext* aCx,
                                JS::Handle<JS::Value> aValue) MOZ_OVERRIDE;

  virtual bool Notify(JSContext* aCx, workers::Status aStatus) MOZ_OVERRIDE;

private:
  virtual ~PromiseWorkerProxy();

  
  typedef void (Promise::*RunCallbackFunc)(JSContext*,
                                           JS::Handle<JS::Value>);

  void RunCallback(JSContext* aCx,
                   JS::Handle<JS::Value> aValue,
                   RunCallbackFunc aFunc);

  workers::WorkerPrivate* mWorkerPrivate;

  
  nsRefPtr<Promise> mWorkerPromise;

  bool mCleanedUp; 

  JSStructuredCloneCallbacks* mCallbacks;

  
  
  nsTArray<nsMainThreadPtrHandle<nsISupports>> mSupportsArray;

  
  Mutex mCleanUpLock;
};

} 
} 

#endif 
