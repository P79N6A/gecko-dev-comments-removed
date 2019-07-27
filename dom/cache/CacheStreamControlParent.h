





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

class CacheStreamControlParent : public PCacheStreamControlParent
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
  SerializeControl(PCacheReadStream* aReadStreamOut) MOZ_OVERRIDE;

  virtual void
  SerializeFds(PCacheReadStream* aReadStreamOut,
               const nsTArray<mozilla::ipc::FileDescriptor>& aFds) MOZ_OVERRIDE;

  virtual void
  DeserializeFds(const PCacheReadStream& aReadStream,
                 nsTArray<mozilla::ipc::FileDescriptor>& aFdsOut) MOZ_OVERRIDE;

private:
  virtual void
  NoteClosedAfterForget(const nsID& aId) MOZ_OVERRIDE;

#ifdef DEBUG
  virtual void
  AssertOwningThread() MOZ_OVERRIDE;
#endif

  
  virtual void ActorDestroy(ActorDestroyReason aReason) MOZ_OVERRIDE;
  virtual bool RecvNoteClosed(const nsID& aId) MOZ_OVERRIDE;

  void NotifyClose(const nsID& aId);
  void NotifyCloseAll();

  
  
  
  nsRefPtr<StreamList> mStreamList;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
