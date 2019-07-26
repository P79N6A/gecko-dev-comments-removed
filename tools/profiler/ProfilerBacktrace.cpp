





#include "JSCustomObjectBuilder.h"
#include "JSObjectBuilder.h"
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

template<typename Builder> void
ProfilerBacktrace::BuildJSObject(Builder& aObjBuilder,
                                 typename Builder::ObjectHandle aScope)
{
  mozilla::MutexAutoLock lock(*mProfile->GetMutex());
  mProfile->BuildJSObject(aObjBuilder, aScope);
}

template void
ProfilerBacktrace::BuildJSObject<JSCustomObjectBuilder>(
                                    JSCustomObjectBuilder& aObjBuilder,
                                    JSCustomObjectBuilder::ObjectHandle aScope);
template void
ProfilerBacktrace::BuildJSObject<JSObjectBuilder>(JSObjectBuilder& aObjBuilder,
                                          JSObjectBuilder::ObjectHandle aScope);
