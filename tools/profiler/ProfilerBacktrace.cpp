





#include "ProfilerBacktrace.h"

#include "ProfileJSONWriter.h"
#include "SyncProfile.h"

ProfilerBacktrace::ProfilerBacktrace(SyncProfile* aProfile)
  : mProfile(aProfile)
{
  MOZ_COUNT_CTOR(ProfilerBacktrace);
  MOZ_ASSERT(aProfile);
}

ProfilerBacktrace::~ProfilerBacktrace()
{
  MOZ_COUNT_DTOR(ProfilerBacktrace);
  if (mProfile->ShouldDestroy()) {
    delete mProfile;
  }
}

void
ProfilerBacktrace::StreamJSON(SpliceableJSONWriter& aWriter,
                              UniqueStacks& aUniqueStacks)
{
  ::MutexAutoLock lock(mProfile->GetMutex());
  mProfile->StreamJSON(aWriter, aUniqueStacks);
}
