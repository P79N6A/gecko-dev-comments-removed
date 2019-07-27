





#ifndef mozilla_dom_cache_CacheStreamControlChild_h
#define mozilla_dom_cache_CacheStreamControlChild_h

#include "mozilla/dom/cache/ActorChild.h"
#include "mozilla/dom/cache/PCacheStreamControlChild.h"
#include "mozilla/dom/cache/StreamControl.h"
#include "nsTObserverArray.h"

namespace mozilla {
namespace dom {
namespace cache {

class ReadStream;

class CacheStreamControlChild MOZ_FINAL : public PCacheStreamControlChild
                                        , public StreamControl
                                        , public ActorChild
{
public:
  CacheStreamControlChild();
  ~CacheStreamControlChild();

  
  virtual void StartDestroy() MOZ_OVERRIDE;

  
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
  virtual bool RecvClose(const nsID& aId) MOZ_OVERRIDE;
  virtual bool RecvCloseAll() MOZ_OVERRIDE;

  bool mDestroyStarted;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
