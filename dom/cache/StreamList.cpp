





#include "mozilla/dom/cache/StreamList.h"

#include "mozilla/dom/cache/CacheStreamControlParent.h"
#include "mozilla/dom/cache/Context.h"
#include "mozilla/dom/cache/Manager.h"
#include "nsIInputStream.h"

namespace mozilla {
namespace dom {
namespace cache {

StreamList::StreamList(Manager* aManager, Context* aContext)
  : mManager(aManager)
  , mContext(aContext)
  , mCacheId(INVALID_CACHE_ID)
  , mStreamControl(nullptr)
  , mActivated(false)
{
  MOZ_ASSERT(mManager);
  mContext->AddActivity(this);
}

void
StreamList::SetStreamControl(CacheStreamControlParent* aStreamControl)
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  MOZ_ASSERT(aStreamControl);

  
  
  
  if (mStreamControl) {
    MOZ_ASSERT(aStreamControl == mStreamControl);
    return;
  }

  mStreamControl = aStreamControl;
  mStreamControl->SetStreamList(this);
}

void
StreamList::RemoveStreamControl(CacheStreamControlParent* aStreamControl)
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  MOZ_ASSERT(mStreamControl);
  MOZ_ASSERT(mStreamControl == aStreamControl);
  mStreamControl = nullptr;
}

void
StreamList::Activate(CacheId aCacheId)
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  MOZ_ASSERT(!mActivated);
  MOZ_ASSERT(mCacheId == INVALID_CACHE_ID);
  mActivated = true;
  mCacheId = aCacheId;
  mManager->AddRefCacheId(mCacheId);
  mManager->AddStreamList(this);

  for (uint32_t i = 0; i < mList.Length(); ++i) {
    mManager->AddRefBodyId(mList[i].mId);
  }
}

void
StreamList::Add(const nsID& aId, nsIInputStream* aStream)
{
  
  
  MOZ_ASSERT(!mStreamControl);
  MOZ_ASSERT(aStream);
  Entry* entry = mList.AppendElement();
  entry->mId = aId;
  entry->mStream = aStream;
}

already_AddRefed<nsIInputStream>
StreamList::Extract(const nsID& aId)
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  for (uint32_t i = 0; i < mList.Length(); ++i) {
    if (mList[i].mId == aId) {
      return mList[i].mStream.forget();
    }
  }
  return nullptr;
}

void
StreamList::NoteClosed(const nsID& aId)
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  for (uint32_t i = 0; i < mList.Length(); ++i) {
    if (mList[i].mId == aId) {
      mList.RemoveElementAt(i);
      mManager->ReleaseBodyId(aId);
      break;
    }
  }

  if (mList.IsEmpty() && mStreamControl) {
    mStreamControl->Shutdown();
  }
}

void
StreamList::NoteClosedAll()
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  for (uint32_t i = 0; i < mList.Length(); ++i) {
    mManager->ReleaseBodyId(mList[i].mId);
  }
  mList.Clear();

  if (mStreamControl) {
    mStreamControl->Shutdown();
  }
}

void
StreamList::Close(const nsID& aId)
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  if (mStreamControl) {
    mStreamControl->Close(aId);
  }
}

void
StreamList::CloseAll()
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  if (mStreamControl) {
    mStreamControl->CloseAll();
  }
}

void
StreamList::Cancel()
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  CloseAll();
}

bool
StreamList::MatchesCacheId(CacheId aCacheId) const
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  return aCacheId == mCacheId;
}

StreamList::~StreamList()
{
  NS_ASSERT_OWNINGTHREAD(StreamList);
  MOZ_ASSERT(!mStreamControl);
  if (mActivated) {
    mManager->RemoveStreamList(this);
    for (uint32_t i = 0; i < mList.Length(); ++i) {
      mManager->ReleaseBodyId(mList[i].mId);
    }
    mManager->ReleaseCacheId(mCacheId);
  }
  mContext->RemoveActivity(this);
}

} 
} 
} 
