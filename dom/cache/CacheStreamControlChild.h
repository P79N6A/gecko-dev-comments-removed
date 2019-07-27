





#ifndef mozilla_dom_cache_CacheStreamControlChild_h
#define mozilla_dom_cache_CacheStreamControlChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/PCacheStreamControlChild.h"
#include "nsTObserverArray.h"

namespace mozilla {
namespace dom {
namespace cache {

class ReadStream;

class CacheStreamControlChild MOZ_FINAL : public PCacheStreamControlChild
                                        , public ActorChild
{
public:
  CacheStreamControlChild();
  ~CacheStreamControlChild();

  void AddListener(ReadStream* aListener);
  void RemoveListener(ReadStream* aListener);

  void NoteClosed(const nsID& aId);

  
  virtual void StartDestroy() MOZ_OVERRIDE;

private:
  
  virtual void ActorDestroy(ActorDestroyReason aReason) MOZ_OVERRIDE;
  virtual bool RecvClose(const nsID& aId) MOZ_OVERRIDE;
  virtual bool RecvCloseAll() MOZ_OVERRIDE;

  typedef nsTObserverArray<ReadStream*> ReadStreamList;
  ReadStreamList mListeners;

  bool mDestroyStarted;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
