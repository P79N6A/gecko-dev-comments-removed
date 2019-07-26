





#ifndef mozilla_dom_FutureResolver_h
#define mozilla_dom_FutureResolver_h

#include "mozilla/dom/Future.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"

struct JSContext;

namespace mozilla {
namespace dom {

class FutureResolver MOZ_FINAL : public nsISupports,
                                 public nsWrapperCache
{
  friend class FutureResolverTask;
  friend class WrapperFutureCallback;
  friend class ResolveFutureCallback;
  friend class RejectFutureCallback;

private:
  enum FutureTaskSync {
    SyncTask,
    AsyncTask
  };

public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(FutureResolver)

  FutureResolver(Future* aFuture);

  Future* GetParentObject() const
  {
    return mFuture;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void Resolve(JSContext* aCx, const Optional<JS::Handle<JS::Value> >& aValue,
               FutureTaskSync aSync = AsyncTask);

  void Reject(JSContext* aCx, const Optional<JS::Handle<JS::Value> >& aValue,
              FutureTaskSync aSync = AsyncTask);

private:
  void ResolveInternal(JSContext* aCx,
                       const Optional<JS::Handle<JS::Value> >& aValue,
                       FutureTaskSync aSync = AsyncTask);

  void RejectInternal(JSContext* aCx,
                      const Optional<JS::Handle<JS::Value> >& aValue,
                      FutureTaskSync aSync = AsyncTask);

  void RunTask(JS::Handle<JS::Value> aValue,
               Future::FutureState aState, FutureTaskSync aSync);

  nsRefPtr<Future> mFuture;

  bool mResolvePending;
};

} 
} 

#endif
