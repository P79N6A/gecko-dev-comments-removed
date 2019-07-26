





#ifndef mozilla_dom_Promise_h
#define mozilla_dom_Promise_h

#include "mozilla/Attributes.h"
#include "mozilla/ErrorResult.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/dom/PromiseBinding.h"
#include "nsWrapperCache.h"
#include "nsAutoPtr.h"
#include "nsPIDOMWindow.h"

struct JSContext;

namespace mozilla {
namespace dom {

class PromiseInit;
class PromiseCallback;
class AnyCallback;
class PromiseResolver;

class Promise MOZ_FINAL : public nsISupports,
                          public nsWrapperCache
{
  friend class PromiseTask;
  friend class PromiseResolver;
  friend class PromiseResolverTask;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Promise)

  Promise(nsPIDOMWindow* aWindow);
  ~Promise();

  static bool PrefEnabled();
  static bool EnabledForScope(JSContext* aCx, JSObject* );

  

  nsPIDOMWindow* GetParentObject() const
  {
    return mWindow;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  static already_AddRefed<Promise>
  Constructor(const GlobalObject& aGlobal, PromiseInit& aInit,
              ErrorResult& aRv);

  static already_AddRefed<Promise>
  Resolve(const GlobalObject& aGlobal, JSContext* aCx,
          JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  static already_AddRefed<Promise>
  Reject(const GlobalObject& aGlobal, JSContext* aCx,
         JS::Handle<JS::Value> aValue, ErrorResult& aRv);

  already_AddRefed<Promise>
  Then(const Optional<OwningNonNull<AnyCallback> >& aResolveCallback,
       const Optional<OwningNonNull<AnyCallback> >& aRejectCallback);


  already_AddRefed<Promise>
  Catch(const Optional<OwningNonNull<AnyCallback> >& aRejectCallback);

private:
  enum PromiseState {
    Pending,
    Resolved,
    Rejected
  };

  void SetState(PromiseState aState)
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

  void AppendCallbacks(PromiseCallback* aResolveCallback,
                       PromiseCallback* aRejectCallback);

  nsRefPtr<nsPIDOMWindow> mWindow;

  nsRefPtr<PromiseResolver> mResolver;

  nsTArray<nsRefPtr<PromiseCallback> > mResolveCallbacks;
  nsTArray<nsRefPtr<PromiseCallback> > mRejectCallbacks;

  JS::Heap<JS::Value> mResult;
  PromiseState mState;
  bool mTaskPending;
};

} 
} 

#endif
