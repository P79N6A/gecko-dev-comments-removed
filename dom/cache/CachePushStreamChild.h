





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
public:
  CachePushStreamChild(Feature* aFeature, nsIAsyncInputStream* aStream);
  ~CachePushStreamChild();

  virtual void StartDestroy() override;

  void Start();

private:
  class Callback;

  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  void DoRead();

  void Wait();

  void OnStreamReady(Callback* aCallback);

  void OnEnd(nsresult aRv);

  nsCOMPtr<nsIAsyncInputStream> mStream;
  nsRefPtr<Callback> mCallback;
  bool mClosed;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
