





#ifndef mozilla_dom_cache_CacheStreamControlParent_h
#define mozilla_dom_cache_CacheStreamControlParent_h

#include "mozilla/dom/cache/PCacheStreamControlParent.h"
#include "nsTObserverArray.h"

namespace mozilla {
namespace dom {
namespace cache {

class ReadStream;
class StreamList;

class CacheStreamControlParent : public PCacheStreamControlParent
{
public:
  CacheStreamControlParent();
  ~CacheStreamControlParent();

  void AddListener(ReadStream* aListener);
  void RemoveListener(ReadStream* aListener);

  void SetStreamList(StreamList* aStreamList);
  void Close(const nsID& aId);
  void CloseAll();
  void Shutdown();

  
  virtual void ActorDestroy(ActorDestroyReason aReason) MOZ_OVERRIDE;
  virtual bool RecvNoteClosed(const nsID& aId) MOZ_OVERRIDE;

private:
  void NotifyClose(const nsID& aId);
  void NotifyCloseAll();

  
  
  
  nsRefPtr<StreamList> mStreamList;

  typedef nsTObserverArray<ReadStream*> ReadStreamList;
  ReadStreamList mListeners;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
