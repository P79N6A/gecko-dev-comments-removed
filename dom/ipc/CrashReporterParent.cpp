





































#include "CrashReporterParent.h"
#include "nsExceptionHandler.h"

#include "base/process_util.h"

using namespace base;

namespace mozilla {
namespace dom {

void
CrashReporterParent::ActorDestroy(ActorDestroyReason why)
{
#if defined(__ANDROID__)
  CrashReporter::RemoveLibraryMappingsForChild(ProcessId(OtherProcess()));
#endif
}

bool
CrashReporterParent::RecvAddLibraryMappings(const InfallibleTArray<Mapping>& mappings)
{
#if defined(__ANDROID__)
  for (PRUint32 i = 0; i < mappings.Length(); i++) {
    const Mapping& m = mappings[i];
    CrashReporter::AddLibraryMappingForChild(ProcessId(OtherProcess()),
                                             m.library_name().get(),
                                             m.file_id().get(),
                                             m.start_address(),
                                             m.mapping_length(),
                                             m.file_offset());
  }
#endif
  return true;
}

CrashReporterParent::CrashReporterParent()
{
    MOZ_COUNT_CTOR(CrashReporterParent);
}

CrashReporterParent::~CrashReporterParent()
{
    MOZ_COUNT_DTOR(CrashReporterParent);
}

} 
} 
