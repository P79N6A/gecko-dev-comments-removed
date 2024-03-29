





#include "mozilla/Monitor.h"
#include "mozilla/ReentrantMonitor.h"

#include "MediaSystemResourceClient.h"

namespace mozilla {

Atomic<uint32_t> MediaSystemResourceClient::sSerialCounter(0);

MediaSystemResourceClient::MediaSystemResourceClient(MediaSystemResourceType aReourceType)
  : mResourceType(aReourceType)
  , mId(++sSerialCounter)
  , mListener(nullptr)
  , mResourceState(RESOURCE_STATE_START)
  , mIsSync(false)
  , mAcquireSyncWaitMonitor(nullptr)
  , mAcquireSyncWaitDone(nullptr)
{
  mManager = MediaSystemResourceManager::Get();
  if (mManager) {
    mManager->Register(this);
  }
}

MediaSystemResourceClient::~MediaSystemResourceClient()
{
  ReleaseResource();
  if (mManager) {
    mManager->Unregister(this);
  }
}

bool
MediaSystemResourceClient::SetListener(MediaSystemResourceReservationListener* aListener)
{
  if (!mManager) {
    return false;
  }
  return mManager->SetListener(this, aListener);
}

void
MediaSystemResourceClient::Acquire()
{
  if (!mManager) {
    return;
  }
  mManager->Acquire(this);
}

bool
MediaSystemResourceClient::AcquireSyncNoWait()
{
  if (!mManager) {
    return false;
  }
  return mManager->AcquireSyncNoWait(this);
}

void
MediaSystemResourceClient::ReleaseResource()
{
  if (!mManager) {
    return;
  }
  mManager->ReleaseResource(this);
}

} 
