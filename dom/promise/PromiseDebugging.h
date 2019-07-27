





#ifndef mozilla_dom_PromiseDebugging_h
#define mozilla_dom_PromiseDebugging_h

#include "js/TypeDecls.h"
#include "nsTArray.h"
#include "nsRefPtr.h"

namespace mozilla {

class ErrorResult;

namespace dom {

class Promise;
struct PromiseDebuggingStateHolder;
class GlobalObject;

class PromiseDebugging
{
public:
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
};

} 
} 

#endif 
