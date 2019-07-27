





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

class CacheStreamControlChild final : public PCacheStreamControlChild
                                    , public StreamControl
                                    , public ActorChild
{
public:
  CacheStreamControlChild();
  ~CacheStreamControlChild();

  
  virtual void StartDestroy() override;

  
  virtual void
  SerializeControl(PCacheReadStream* aReadStreamOut) override;

  virtual void
  SerializeFds(PCacheReadStream* aReadStreamOut,
               const nsTArray<mozilla::ipc::FileDescriptor>& aFds) override;

  virtual void
  DeserializeFds(const PCacheReadStream& aReadStream,
                 nsTArray<mozilla::ipc::FileDescriptor>& aFdsOut) override;

private:
  virtual void
  NoteClosedAfterForget(const nsID& aId) override;

#ifdef DEBUG
  virtual void
  AssertOwningThread() override;
#endif

  
  virtual void ActorDestroy(ActorDestroyReason aReason) override;
  virtual bool RecvClose(const nsID& aId) override;
  virtual bool RecvCloseAll() override;

  bool mDestroyStarted;

  NS_DECL_OWNINGTHREAD
};

} 
} 
} 

#endif
