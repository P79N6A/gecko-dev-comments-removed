





#ifndef mozilla_dom_cache_CacheStreamControlParent_h
#define mozilla_dom_cache_CacheStreamControlParent_h

#include "mozilla/dom/cache/PCacheStreamControlParent.h"
#include "mozilla/dom/cache/StreamControl.h"
#include "nsTObserverArray.h"

namespace mozilla {
namespace dom {
namespace cache {

class ReadStream;
class StreamList;

class CacheStreamControlParent final : public PCacheStreamControlParent
                                     , public StreamControl
{
public:
  CacheStreamControlParent();
  ~CacheStreamControlParent();

  void SetStreamList(StreamList* aStreamList);
  void Close(const nsID& aId);
  void CloseAll();
  void Shutdown();

  
  virtual void
  SerializeControl(CacheReadStream* aReadStreamOut) override;

  virtual void
  SerializeFds(CacheReadStream* aReadStreamOut,
               const nsTArray<mozilla::ipc::FileDescriptor>& aFds) override;

  virtual void
  DeserializeFds(const CacheReadStream& aReadStream,
                 nsTArray<mozilla::ipc::FileDescriptor>& aFdsOut) override;

private:
  virtual void
  NoteClosedAfterForget(const nsID& aId) override;

#ifdef DEBUG
  virtual void
  AssertOwningThread() override;
#endif

  
  virtual void ActorDestroy(ActorDestroyReason aReason) override;
  virtual bool RecvNoteClosed(const nsID& aId) override;

  void NotifyClose(const nsID& aId);
  void NotifyCloseAll();

  
  
  
  nsRefPtr<StreamList> mStreamList;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
