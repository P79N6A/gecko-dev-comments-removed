






#include "AsyncTransactionTracker.h"

#include "mozilla/layers/ImageBridgeChild.h"  

namespace mozilla {
namespace layers {

uint64_t AsyncTransactionTracker::sSerialCounter(0);
Mutex* AsyncTransactionTracker::sLock = nullptr;

AsyncTransactionTracker::AsyncTransactionTracker()
    : mSerial(GetNextSerial())
    , mCompletedMonitor("AsyncTransactionTracker.mCompleted")
    , mCompleted(false)
{
}

AsyncTransactionTracker::~AsyncTransactionTracker()
{
}

void
AsyncTransactionTracker::WaitComplete()
{
  MOZ_ASSERT(!InImageBridgeChildThread());

  MonitorAutoLock mon(mCompletedMonitor);
  int count = 0;
  const int maxCount = 5;
  while (!mCompleted && (count < maxCount)) {
    if (!NS_SUCCEEDED(mCompletedMonitor.Wait(PR_MillisecondsToInterval(10000)))) {
      NS_WARNING("Failed to wait Monitor");
      return;
    }
    if (count > 1) {
      printf_stderr("Waiting async transaction complete.\n");
    }
    count++;
  }

  if (!mCompleted) {
    printf_stderr("Timeout of waiting transaction complete.");
  }
}

void
AsyncTransactionTracker::NotifyComplete()
{
  MonitorAutoLock mon(mCompletedMonitor);
  MOZ_ASSERT(!mCompleted);
  mCompleted = true;
  Complete();
  mCompletedMonitor.Notify();
}

void
AsyncTransactionTracker::NotifyCancel()
{
  MonitorAutoLock mon(mCompletedMonitor);
  MOZ_ASSERT(!mCompleted);
  mCompleted = true;
  Cancel();
  mCompletedMonitor.Notify();
}

uint64_t AsyncTransactionTrackersHolder::sSerialCounter(0);
Mutex* AsyncTransactionTrackersHolder::sHolderLock = nullptr;

std::map<uint64_t, AsyncTransactionTrackersHolder*> AsyncTransactionTrackersHolder::sTrackersHolders;

AsyncTransactionTrackersHolder::AsyncTransactionTrackersHolder()
  : mSerial(GetNextSerial())
  , mIsTrackersHolderDestroyed(false)
{
  MOZ_COUNT_CTOR(AsyncTransactionTrackersHolder);
  {
    MOZ_ASSERT(sHolderLock);
    MutexAutoLock lock(*sHolderLock);
    sTrackersHolders[mSerial] = this;
  }
}

AsyncTransactionTrackersHolder::~AsyncTransactionTrackersHolder()
{
  if (!mIsTrackersHolderDestroyed) {
    DestroyAsyncTransactionTrackersHolder();
  }

  {
    if (sHolderLock) {
      sHolderLock->Lock();
    }
    sTrackersHolders.erase(mSerial);
    if (sHolderLock) {
      sHolderLock->Unlock();
    }
  }
  MOZ_COUNT_DTOR(AsyncTransactionTrackersHolder);
}

void
AsyncTransactionTrackersHolder::HoldUntilComplete(AsyncTransactionTracker* aTransactionTracker)
{
  if (!aTransactionTracker) {
    return;
  }

  if (mIsTrackersHolderDestroyed && aTransactionTracker) {
    aTransactionTracker->NotifyComplete();
    return;
  }

  if (aTransactionTracker) {
    MutexAutoLock lock(*sHolderLock);
    mAsyncTransactionTrackers[aTransactionTracker->GetId()] = aTransactionTracker;
  }
}

void
AsyncTransactionTrackersHolder::TransactionCompleteted(uint64_t aTransactionId)
{
  MutexAutoLock lock(*sHolderLock);
  TransactionCompletetedInternal(aTransactionId);
}

void
AsyncTransactionTrackersHolder::TransactionCompletetedInternal(uint64_t aTransactionId)
{
  std::map<uint64_t, RefPtr<AsyncTransactionTracker> >::iterator it
    = mAsyncTransactionTrackers.find(aTransactionId);
  if (it != mAsyncTransactionTrackers.end()) {
    it->second->NotifyComplete();
    mAsyncTransactionTrackers.erase(it);
  }
}

void
AsyncTransactionTrackersHolder::SetReleaseFenceHandle(FenceHandle& aReleaseFenceHandle,
                                                      uint64_t aTransactionId)
{
  std::map<uint64_t, RefPtr<AsyncTransactionTracker> >::iterator it
    = mAsyncTransactionTrackers.find(aTransactionId);
  if (it != mAsyncTransactionTrackers.end()) {
    it->second->SetReleaseFenceHandle(aReleaseFenceHandle);
  }
}

 void
AsyncTransactionTrackersHolder::TransactionCompleteted(uint64_t aHolderId, uint64_t aTransactionId)
{
  MutexAutoLock lock(*sHolderLock);
  AsyncTransactionTrackersHolder* holder = sTrackersHolders[aHolderId];
  if (!holder) {
    return;
  }
  holder->TransactionCompletetedInternal(aTransactionId);
}

 void
AsyncTransactionTrackersHolder::SetReleaseFenceHandle(FenceHandle& aReleaseFenceHandle,
                                                      uint64_t aHolderId,
                                                      uint64_t aTransactionId)
{
  MutexAutoLock lock(*sHolderLock);
  AsyncTransactionTrackersHolder* holder = sTrackersHolders[aHolderId];
  if (!holder) {
    return;
  }
  holder->SetReleaseFenceHandle(aReleaseFenceHandle, aTransactionId);
}

void
AsyncTransactionTrackersHolder::ClearAllAsyncTransactionTrackers()
{
  if (sHolderLock) {
    sHolderLock->Lock();
  }
  std::map<uint64_t, RefPtr<AsyncTransactionTracker> >::iterator it;
  for (it = mAsyncTransactionTrackers.begin();
       it != mAsyncTransactionTrackers.end(); it++) {
    it->second->NotifyCancel();
  }
  mAsyncTransactionTrackers.clear();
  if (sHolderLock) {
    sHolderLock->Unlock();
  }
}

void
AsyncTransactionTrackersHolder::DestroyAsyncTransactionTrackersHolder() {
  mIsTrackersHolderDestroyed = true;
  ClearAllAsyncTransactionTrackers();
}


} 
} 
