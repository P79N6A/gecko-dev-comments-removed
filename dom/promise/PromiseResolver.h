





#ifndef mozilla_dom_PromiseResolver_h
#define mozilla_dom_PromiseResolver_h

#include "mozilla/dom/Promise.h"
#include "mozilla/Attributes.h"
#include "mozilla/dom/BindingDeclarations.h"
#include "nsCycleCollectionParticipant.h"
#include "nsWrapperCache.h"
#include "js/TypeDecls.h"

namespace mozilla {
namespace dom {

class PromiseResolver MOZ_FINAL : public nsWrapperCache
{
  friend class PromiseResolverTask;
  friend class WrapperPromiseCallback;
  friend class ResolvePromiseCallback;
  friend class RejectPromiseCallback;

private:
  enum PromiseTaskSync {
    SyncTask,
    AsyncTask
  };

public:
  NS_INLINE_DECL_CYCLE_COLLECTING_NATIVE_REFCOUNTING(PromiseResolver)
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_NATIVE_CLASS(PromiseResolver)

  PromiseResolver(Promise* aPromise);
  virtual ~PromiseResolver();

  Promise* GetParentObject() const
  {
    return mPromise;
  }

  virtual JSObject*
  WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope) MOZ_OVERRIDE;

  void Resolve(JSContext* aCx, const Optional<JS::Handle<JS::Value> >& aValue,
               PromiseTaskSync aSync = AsyncTask);

  void Reject(JSContext* aCx, const Optional<JS::Handle<JS::Value> >& aValue,
              PromiseTaskSync aSync = AsyncTask);

private:
  void ResolveInternal(JSContext* aCx,
                       const Optional<JS::Handle<JS::Value> >& aValue,
                       PromiseTaskSync aSync = AsyncTask);

  void RejectInternal(JSContext* aCx,
                      const Optional<JS::Handle<JS::Value> >& aValue,
                      PromiseTaskSync aSync = AsyncTask);

  void RunTask(JS::Handle<JS::Value> aValue,
               Promise::PromiseState aState, PromiseTaskSync aSync);

  nsRefPtr<Promise> mPromise;

  bool mResolvePending;
};

} 
} 

#endif
