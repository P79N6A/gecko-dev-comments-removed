





#include "mozilla/dom/PromiseDebugging.h"

#include "js/Value.h"

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseDebuggingBinding.h"

namespace mozilla {
namespace dom {

 void
PromiseDebugging::GetState(GlobalObject&, Promise& aPromise,
                           PromiseDebuggingStateHolder& aState)
{
  switch (aPromise.mState) {
  case Promise::Pending:
    aState.mState = PromiseDebuggingState::Pending;
    break;
  case Promise::Resolved:
    aState.mState = PromiseDebuggingState::Fulfilled;
    JS::ExposeValueToActiveJS(aPromise.mResult);
    aState.mValue = aPromise.mResult;
    break;
  case Promise::Rejected:
    aState.mState = PromiseDebuggingState::Rejected;
    JS::ExposeValueToActiveJS(aPromise.mResult);
    aState.mReason = aPromise.mResult;
    break;
  }
}

 void
PromiseDebugging::GetAllocationStack(GlobalObject&, Promise& aPromise,
                                     JS::MutableHandle<JSObject*> aStack)
{
  aStack.set(aPromise.mAllocationStack);
}

 void
PromiseDebugging::GetRejectionStack(GlobalObject&, Promise& aPromise,
                                    JS::MutableHandle<JSObject*> aStack)
{
  aStack.set(aPromise.mRejectionStack);
}

 void
PromiseDebugging::GetFullfillmentStack(GlobalObject&, Promise& aPromise,
                                       JS::MutableHandle<JSObject*> aStack)
{
  aStack.set(aPromise.mFullfillmentStack);
}

 void
PromiseDebugging::GetDependentPromises(GlobalObject&, Promise& aPromise,
                                       nsTArray<nsRefPtr<Promise>>& aPromises)
{
  aPromise.GetDependentPromises(aPromises);
}

} 
} 
