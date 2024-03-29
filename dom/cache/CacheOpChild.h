





#ifndef mozilla_dom_cache_CacheOpChild_h
#define mozilla_dom_cache_CacheOpChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/PCacheOpChild.h"
#include "mozilla/dom/cache/TypeUtils.h"
#include "nsRefPtr.h"

class nsIGlobalObject;

namespace mozilla {
namespace dom {

class Promise;

namespace cache {

class CacheOpChild final : public PCacheOpChild
                         , public ActorChild
                         , public TypeUtils
{
  friend class CacheChild;
  friend class CacheStorageChild;

private:
  
  
  CacheOpChild(Feature* aFeature, nsIGlobalObject* aGlobal,
               nsISupports* aParent, Promise* aPromise);
  ~CacheOpChild();

  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  virtual bool
  Recv__delete__(const ErrorResult& aRv, const CacheOpResult& aResult) override;

  
  virtual void
  StartDestroy() override;

  
  virtual nsIGlobalObject*
  GetGlobalObject() const override;

#ifdef DEBUG
  virtual void
  AssertOwningThread() const override;
#endif

  virtual CachePushStreamChild*
  CreatePushStream(nsIAsyncInputStream* aStream) override;

  
  void
  HandleResponse(const CacheResponseOrVoid& aResponseOrVoid);

  void
  HandleResponseList(const nsTArray<CacheResponse>& aResponseList);

  void
  HandleRequestList(const nsTArray<CacheRequest>& aRequestList);

  nsCOMPtr<nsIGlobalObject> mGlobal;
  
  
  nsCOMPtr<nsISupports> mParent;
  nsRefPtr<Promise> mPromise;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
