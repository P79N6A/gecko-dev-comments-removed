





#include "SyncProfile.h"

SyncProfile::SyncProfile(ThreadInfo* aInfo, int aEntrySize)
  : ThreadProfile(aInfo, new ProfileBuffer(aEntrySize))
  , mOwnerState(REFERENCED)
{
  MOZ_COUNT_CTOR(SyncProfile);
}

SyncProfile::~SyncProfile()
{
  MOZ_COUNT_DTOR(SyncProfile);

  
  ThreadInfo* info = GetThreadInfo();
  delete info;
}

bool
SyncProfile::ShouldDestroy()
{
  GetMutex()->AssertNotCurrentThreadOwns();
  mozilla::MutexAutoLock lock(*GetMutex());
  if (mOwnerState == OWNED) {
    mOwnerState = OWNER_DESTROYING;
    return true;
  }
  mOwnerState = ORPHANED;
  return false;
}

void
SyncProfile::EndUnwind()
{
  
  GetMutex()->AssertCurrentThreadOwns();
  if (mOwnerState != ORPHANED) {
    mOwnerState = OWNED;
  }
  
  OwnerState ownerState = mOwnerState;
  ThreadProfile::EndUnwind();
  if (ownerState == ORPHANED) {
    delete this;
  }
}

