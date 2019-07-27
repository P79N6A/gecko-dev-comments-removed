





#ifndef mozilla_dom_cache_CachePushStreamParent_h
#define mozilla_dom_cache_CachePushStreamParent_h

#include "mozilla/dom/cache/PCachePushStreamParent.h"

class nsIAsyncInputStream;
class nsIAsyncOutputStream;
class nsIInputStream;

namespace mozilla {
namespace dom {
namespace cache {

class CachePushStreamParent final : public PCachePushStreamParent
{
public:
  static CachePushStreamParent*
  Create();

  ~CachePushStreamParent();

  already_AddRefed<nsIInputStream>
  TakeReader();

private:
  CachePushStreamParent(nsIAsyncInputStream* aReader,
                        nsIAsyncOutputStream* aWriter);

  
  virtual void
  ActorDestroy(ActorDestroyReason aReason) override;

  virtual bool
  RecvBuffer(const nsCString& aBuffer) override;

  virtual bool
  RecvClose(const nsresult& aRv) override;

  nsCOMPtr<nsIAsyncInputStream> mReader;
  nsCOMPtr<nsIAsyncOutputStream> mWriter;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
