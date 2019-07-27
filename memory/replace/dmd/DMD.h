





#ifndef DMD_h___
#define DMD_h___

#include <string.h>

#include "mozilla/Types.h"
#include "mozilla/UniquePtr.h"

namespace mozilla {

class JSONWriteFunc;

namespace dmd {


MOZ_EXPORT void
Report(const void* aPtr);


MOZ_EXPORT void
ReportOnAlloc(const void* aPtr);







MOZ_EXPORT void
ClearReports();























































































MOZ_EXPORT void
AnalyzeReports(mozilla::UniquePtr<mozilla::JSONWriteFunc>);

struct Sizes
{
  size_t mStackTracesUsed;
  size_t mStackTracesUnused;
  size_t mStackTraceTable;
  size_t mBlockTable;

  Sizes() { Clear(); }
  void Clear() { memset(this, 0, sizeof(Sizes)); }
};



MOZ_EXPORT void
SizeOf(Sizes* aSizes);


MOZ_EXPORT void
StatusMsg(const char* aFmt, ...);


MOZ_EXPORT bool
IsRunning();


MOZ_EXPORT void
SetSampleBelowSize(size_t aSize);


MOZ_EXPORT void
ClearBlocks();

} 
} 

#endif 
