





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
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class PromiseInit;
class PromiseCallback;
class AnyCallback;

class Promise MOZ_FINAL : public nsISupports,
                          public nsWrapperCache
{
  friend class PromiseTask;
  friend class PromiseResolverTask;
  friend class ResolvePromiseCallback;
  friend class RejectPromiseCallback;
  friend class WrapperPromiseCallback;

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(Promise)

  Promise(nsPIDOMWindow* aWindow);
  ~Promise();

  static bool PrefEnabled();
  static bool EnabledForScope(JSContext* aCx, JSObject* );

  void MaybeResolve(JSContext* aCx,
                    const Optional<JS::Handle<JS::Value> >& aValue);
  void MaybeReject(JSContext* aCx,
                   const Optional<JS::Handle<JS::Value> >& aValue);

  

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

  enum PromiseTaskSync {
    SyncTask,
    AsyncTask
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

  void RunResolveTask(JS::Handle<JS::Value> aValue,
                      Promise::PromiseState aState,
                      PromiseTaskSync aAsynchronous);

  void AppendCallbacks(PromiseCallback* aResolveCallback,
                       PromiseCallback* aRejectCallback);

  
  
  void MaybeReportRejected();

  void MaybeResolveInternal(JSContext* aCx,
                            const Optional<JS::Handle<JS::Value> >& aValue,
                            PromiseTaskSync aSync = AsyncTask);
  void MaybeRejectInternal(JSContext* aCx,
                           const Optional<JS::Handle<JS::Value> >& aValue,
                           PromiseTaskSync aSync = AsyncTask);

  void ResolveInternal(JSContext* aCx,
                       const Optional<JS::Handle<JS::Value> >& aValue,
                       PromiseTaskSync aSync = AsyncTask);

  void RejectInternal(JSContext* aCx,
                      const Optional<JS::Handle<JS::Value> >& aValue,
                      PromiseTaskSync aSync = AsyncTask);

  
  static bool
  JSCallback(JSContext *aCx, unsigned aArgc, JS::Value *aVp);
  static JSObject*
  CreateFunction(JSContext* aCx, JSObject* aParent, Promise* aPromise,
                int32_t aTask);

  nsRefPtr<nsPIDOMWindow> mWindow;

  nsTArray<nsRefPtr<PromiseCallback> > mResolveCallbacks;
  nsTArray<nsRefPtr<PromiseCallback> > mRejectCallbacks;

  JS::Heap<JS::Value> mResult;
  PromiseState mState;
  bool mTaskPending;
  bool mHadRejectCallback;

  bool mResolvePending;
};

} 
} 

#endif
