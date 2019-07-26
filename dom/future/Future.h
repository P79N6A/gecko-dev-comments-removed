





#ifndef mozilla_dom_Future_h
#define mozilla_dom_Future_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/FutureBinding.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"

struct JSContext;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class FutureInit;
class FutureCallback;
class AnyCallback;
class FutureResolver;

class Future MOZ_FINAL : public nsISupports,
                         public nsWrapperCache
{
  friend class FutureTask;
  friend class FutureResolver;
  friend class FutureResolverTask;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Future)

  Future(nsPIDOMWindow* aWindow);
  ~Future();

  

  nsPIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static already_AddRefed<Future>
  Constructor(const GlobalObject& aGlobal, JSContext* aCx, FutureInit& aInit,
              ErrorResult& aRv);

  already_AddRefed<Future>
  Then(AnyCallback* aResolveCallback, AnyCallback* aRejectCallback);

  already_AddRefed<Future>
  Catch(AnyCallback* aRejectCallback);

  void Done(AnyCallback* aResolveCallback, AnyCallback* aRejectCallback);

private:
  enum FutureState {
    Pending,
    Resolved,
    Rejected
  };

  void SetState(FutureState aState)
  {
    MOZ_ASSERT(mState == Pending);
    MOZ_ASSERT(aState != Pending);
    mState = aState;
  }

  void SetResult(JS::Handle<JS::Value> aValue)
  {
    mResult = aValue;
  }

  
  
  
  
  void RunTask();

  void AppendCallbacks(FutureCallback* aResolveCallback,
                       FutureCallback* aRejectCallback);

  nsRefPtr<nsPIDOMWindow> mWindow;

  nsRefPtr<FutureResolver> mResolver;

  nsTArray<nsRefPtr<FutureCallback> > mResolveCallbacks;
  nsTArray<nsRefPtr<FutureCallback> > mRejectCallbacks;

  JS::Value mResult;
  FutureState mState;
  bool mTaskPending;
};

} 
} 

#endif
