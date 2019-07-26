





#include "SyncProfile.h"
#include "UnwinderThread2.h"

SyncProfile::SyncProfile(const char* aName, int aEntrySize, PseudoStack *aStack,
                         Thread::tid_t aThreadId, bool aIsMainThread)
  : ThreadProfile(aName, aEntrySize, aStack, aThreadId,
                 Sampler::AllocPlatformData(aThreadId), aIsMainThread,
                 tlsStackTop.get()),
    mOwnerState(REFERENCED),
    mUtb(nullptr)
{
}

SyncProfile::~SyncProfile()
{
  if (mUtb) {
    utb__release_sync_buffer(mUtb);
  }
  Sampler::FreePlatformData(GetPlatformData());
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

