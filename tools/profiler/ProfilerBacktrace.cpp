





#include "JSStreamWriter.h"
#include "ProfilerBacktrace.h"
#include "SyncProfile.h"


ProfilerBacktrace::ProfilerBacktrace(SyncProfile* aProfile)
  : mProfile(aProfile)
{
  MOZ_ASSERT(aProfile);
}

ProfilerBacktrace::~ProfilerBacktrace()
{
  if (mProfile->ShouldDestroy()) {
    delete mProfile;
  }
}

void
ProfilerBacktrace::StreamJSObject(JSStreamWriter& b)
{
  mozilla::MutexAutoLock lock(*mProfile->GetMutex());
  mProfile->StreamJSObject(b);
}
