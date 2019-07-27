





#include "SyncProfile.h"
#include "UnwinderThread2.h"

SyncProfile::SyncProfile(ThreadInfo* aInfo, int aEntrySize)
  : ThreadProfile(aInfo, aEntrySize)
  , mOwnerState(REFERENCED)
  , mUtb(nullptr)
{
  MOZ_COUNT_CTOR(SyncProfile);
}

SyncProfile::~SyncProfile()
{
  MOZ_COUNT_DTOR(SyncProfile);
  if (mUtb) {
    utb__release_sync_buffer(mUtb);
  }

  
  ThreadInfo* info = GetThreadInfo();
  delete info;
}

bool
SyncProfile::SetUWTBuffer(LinkedUWTBuffer* aBuff)
{
  MOZ_ASSERT(aBuff);
  mUtb = aBuff;
  return true;
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
  if (mUtb) {
    utb__end_sync_buffer_unwind(mUtb);
  }
  if (mOwnerState != ORPHANED) {
    flush();
    mOwnerState = OWNED;
  }
  
  OwnerState ownerState = mOwnerState;
  ThreadProfile::EndUnwind();
  if (ownerState == ORPHANED) {
    delete this;
  }
}

