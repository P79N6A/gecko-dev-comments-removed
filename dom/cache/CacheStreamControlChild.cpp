





#include "mozilla/dom/cache/CacheStreamControlChild.h"

#include "mozilla/DebugOnly.h"
#include "mozilla/unused.h"
#include "mozilla/dom/cache/ActorUtils.h"
#include "mozilla/dom/cache/ReadStream.h"
#include "nsISupportsImpl.h"

namespace mozilla {
namespace dom {
namespace cache {


PCacheStreamControlChild*
AllocPCacheStreamControlChild()
{
  return new CacheStreamControlChild();
}


void
DeallocPCacheStreamControlChild(PCacheStreamControlChild* aActor)
{
  delete aActor;
}

CacheStreamControlChild::CacheStreamControlChild()
  : mDestroyStarted(false)
{
  MOZ_COUNT_CTOR(cache::CacheStreamControlChild);
}

CacheStreamControlChild::~CacheStreamControlChild()
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  MOZ_COUNT_DTOR(cache::CacheStreamControlChild);
}

void
CacheStreamControlChild::AddListener(ReadStream* aListener)
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  MOZ_ASSERT(aListener);
  MOZ_ASSERT(!mListeners.Contains(aListener));
  mListeners.AppendElement(aListener);
}

void
CacheStreamControlChild::RemoveListener(ReadStream* aListener)
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  MOZ_ASSERT(aListener);
  MOZ_ALWAYS_TRUE(mListeners.RemoveElement(aListener));
}

void
CacheStreamControlChild::NoteClosed(const nsID& aId)
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  unused << SendNoteClosed(aId);
}

void
CacheStreamControlChild::StartDestroy()
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  
  
  
  if (mDestroyStarted) {
    return;
  }
  mDestroyStarted = true;

  
  
  RecvCloseAll();
}

void
CacheStreamControlChild::ActorDestroy(ActorDestroyReason aReason)
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  
  
  ReadStreamList::ForwardIterator iter(mListeners);
  while (iter.HasMore()) {
    nsRefPtr<ReadStream> stream = iter.GetNext();
    stream->CloseStreamWithoutReporting();
  }
  mListeners.Clear();

  RemoveFeature();
}

bool
CacheStreamControlChild::RecvClose(const nsID& aId)
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  DebugOnly<uint32_t> closedCount = 0;

  ReadStreamList::ForwardIterator iter(mListeners);
  while (iter.HasMore()) {
    nsRefPtr<ReadStream> stream = iter.GetNext();
    
    if (stream->MatchId(aId)) {
      stream->CloseStream();
      closedCount += 1;
    }
  }

  MOZ_ASSERT(closedCount > 0);

  return true;
}

bool
CacheStreamControlChild::RecvCloseAll()
{
  NS_ASSERT_OWNINGTHREAD(CacheStreamControlChild);
  ReadStreamList::ForwardIterator iter(mListeners);
  while (iter.HasMore()) {
    nsRefPtr<ReadStream> stream = iter.GetNext();
    stream->CloseStream();
  }
  return true;
}

} 
} 
} 
