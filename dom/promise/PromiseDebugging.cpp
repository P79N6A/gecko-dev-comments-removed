





#include "js/Value.h"
#include "nsThreadUtils.h"

#include "mozilla/CycleCollectedJSRuntime.h"
#include "mozilla/ThreadLocal.h"
#include "mozilla/TimeStamp.h"

#include "mozilla/dom/BindingDeclarations.h"
#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseDebugging.h"
#include "mozilla/dom/PromiseDebuggingBinding.h"

namespace mozilla {
namespace dom {

namespace {

class FlushRejections: public nsCancelableRunnable
{
public:
  static void Init() {
    if (!sDispatched.init()) {
      MOZ_CRASH("Could not initialize FlushRejections::sDispatched");
    }
    sDispatched.set(false);
  }
  static void DispatchNeeded() {
    if (sDispatched.get()) {
      
      
      return;
    }
    sDispatched.set(true);
    NS_DispatchToCurrentThread(new FlushRejections());
  }
  nsresult Run()
  {
    sDispatched.set(false);

    
    
    
    
    
    PromiseDebugging::FlushUncaughtRejections();
    return NS_OK;
  }
private:
  
  
  static ThreadLocal<bool> sDispatched;
};

 ThreadLocal<bool>
FlushRejections::sDispatched;

} 

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

 nsString
PromiseDebugging::sIDPrefix;

 void
PromiseDebugging::Init()
{
  FlushRejections::Init();

  
  sIDPrefix = NS_LITERAL_STRING("PromiseDebugging.");
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    sIDPrefix.AppendInt(ContentChild::GetSingleton()->GetID());
    sIDPrefix.Append('.');
  } else {
    sIDPrefix.AppendLiteral("0.");
  }
}

 void
PromiseDebugging::Shutdown()
{
  sIDPrefix.SetIsVoid(true);
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

 double
PromiseDebugging::GetPromiseLifetime(GlobalObject&, Promise& aPromise)
{
  return (TimeStamp::Now() - aPromise.mCreationTimestamp).ToMilliseconds();
}

 double
PromiseDebugging::GetTimeToSettle(GlobalObject&, Promise& aPromise,
                                  ErrorResult& aRv)
{
  if (aPromise.mState == Promise::Pending) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return 0;
  }
  return (aPromise.mSettlementTimestamp -
          aPromise.mCreationTimestamp).ToMilliseconds();
}

 void
PromiseDebugging::AddUncaughtRejectionObserver(GlobalObject&,
                                               UncaughtRejectionObserver& aObserver)
{
  CycleCollectedJSRuntime* storage = CycleCollectedJSRuntime::Get();
  nsTArray<nsRefPtr<UncaughtRejectionObserver>>& observers = storage->mUncaughtRejectionObservers;
  observers.AppendElement(&aObserver);
}

 void
PromiseDebugging::RemoveUncaughtRejectionObserver(GlobalObject&,
                                                  UncaughtRejectionObserver& aObserver)
{
  CycleCollectedJSRuntime* storage = CycleCollectedJSRuntime::Get();
  nsTArray<nsRefPtr<UncaughtRejectionObserver>>& observers = storage->mUncaughtRejectionObservers;
  for (size_t i = 0; i < observers.Length(); ++i) {
    if (*observers[i] == aObserver) {
      observers.RemoveElementAt(i);
      return;
    }
  }
}

 void
PromiseDebugging::AddUncaughtRejection(Promise& aPromise)
{
  CycleCollectedJSRuntime::Get()->mUncaughtRejections.AppendElement(&aPromise);
  FlushRejections::DispatchNeeded();
}

 void
PromiseDebugging::AddConsumedRejection(Promise& aPromise)
{
  CycleCollectedJSRuntime::Get()->mConsumedRejections.AppendElement(&aPromise);
  FlushRejections::DispatchNeeded();
}

 void
PromiseDebugging::GetPromiseID(GlobalObject&,
                               Promise& aPromise,
                               nsString& aID)
{
  uint64_t promiseID = aPromise.GetID();
  aID = sIDPrefix;
  aID.AppendInt(promiseID);
}

 void
PromiseDebugging::FlushUncaughtRejections()
{
  CycleCollectedJSRuntime* storage = CycleCollectedJSRuntime::Get();

  
  
  nsTArray<nsRefPtr<Promise>> uncaught;
  storage->mUncaughtRejections.SwapElements(uncaught);

  
  
  nsTArray<nsRefPtr<Promise>> consumed;
  storage->mConsumedRejections.SwapElements(consumed);

  nsTArray<nsRefPtr<UncaughtRejectionObserver>>& observers = storage->mUncaughtRejectionObservers;

  

  for (size_t i = 0; i < uncaught.Length(); ++i) {
    nsRefPtr<Promise> promise = uncaught[i];
    if (!promise->IsLastInChain()) {
      
      
      continue;
    }

    
    
    
    for (size_t j = 0; j < observers.Length(); ++j) {
      ErrorResult rv;
      observers[j]->OnLeftUncaught(*promise, rv);
      
    }

    promise->SetNotifiedAsUncaught();
  }

  

  for (size_t i = 0; i < consumed.Length(); ++i) {
    nsRefPtr<Promise> promise = consumed[i];
    if (!promise->WasNotifiedAsUncaught()) {
      continue;
    }

    MOZ_ASSERT(!promise->IsLastInChain());
    for (size_t j = 0; j < observers.Length(); ++j) {
      ErrorResult rv;
      observers[j]->OnConsumed(*promise, rv); 
    }
  }
}

} 
} 
