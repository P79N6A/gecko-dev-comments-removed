





#ifndef mozilla_dom_cache_CachePushStreamChild_h
#define mozilla_dom_cache_CachePushStreamChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/PCachePushStreamChild.h"
#include "nsCOMPtr.h"

class nsIAsyncInputStream;

namespace mozilla {
namespace dom {
namespace cache {

class CachePushStreamChild final : public PCachePushStreamChild
                                 , public ActorChild
{
  friend class CacheChild;

public:
  void Start();

  virtual void StartDestroy() override;

private:
  class Callback;

  
  CachePushStreamChild(Feature* aFeature, nsISupports* aParent,
                       nsIAsyncInputStream* aStream);
  ~CachePushStreamChild();

  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  void DoRead();

  void Wait();

  void OnStreamReady(Callback* aCallback);

  void OnEnd(nsresult aRv);

  nsCOMPtr<nsISupports> mParent;
  nsCOMPtr<nsIAsyncInputStream> mStream;
  nsRefPtr<Callback> mCallback;
  bool mClosed;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
