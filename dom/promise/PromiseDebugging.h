





#ifndef mozilla_dom_PromiseDebugging_h
#define mozilla_dom_PromiseDebugging_h

#include "js/TypeDecls.h"
#include "nsTArray.h"
#include "nsRefPtr.h"

namespace mozilla {

class ErrorResult;

namespace dom {
namespace workers {
class WorkerPrivate;
}

class Promise;
struct PromiseDebuggingStateHolder;
class GlobalObject;
class UncaughtRejectionObserver;
class FlushRejections;

class PromiseDebugging
{
public:
  static void Init();
  static void Shutdown();

  static void GetState(GlobalObject&, Promise& aPromise,
                       PromiseDebuggingStateHolder& aState);

  static void GetAllocationStack(GlobalObject&, Promise& aPromise,
                                 JS::MutableHandle<JSObject*> aStack);
  static void GetRejectionStack(GlobalObject&, Promise& aPromise,
                                JS::MutableHandle<JSObject*> aStack);
  static void GetFullfillmentStack(GlobalObject&, Promise& aPromise,
                                   JS::MutableHandle<JSObject*> aStack);
  static void GetDependentPromises(GlobalObject&, Promise& aPromise,
                                   nsTArray<nsRefPtr<Promise>>& aPromises);
  static double GetPromiseLifetime(GlobalObject&, Promise& aPromise);
  static double GetTimeToSettle(GlobalObject&, Promise& aPromise,
                                ErrorResult& aRv);

  static void GetPromiseID(GlobalObject&, Promise&, nsString&);

  
  static void AddUncaughtRejectionObserver(GlobalObject&,
                                           UncaughtRejectionObserver& aObserver);
  static bool RemoveUncaughtRejectionObserver(GlobalObject&,
                                              UncaughtRejectionObserver& aObserver);

  
  static void AddUncaughtRejection(Promise&);
  
  
  static void AddConsumedRejection(Promise&);
  
  
  static void FlushUncaughtRejections();

protected:
  static void FlushUncaughtRejectionsInternal();
  friend class FlushRejections;
  friend class WorkerPrivate;
private:
  
  
  
  
  
  
  
  static nsString sIDPrefix;
};

} 
} 

#endif 
