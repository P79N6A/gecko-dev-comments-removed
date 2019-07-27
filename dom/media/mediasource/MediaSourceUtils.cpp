




#include "MediaSourceUtils.h"

#include "prlog.h"
#include "mozilla/dom/TimeRanges.h"
#include "nsPrintfCString.h"

namespace mozilla {

#if defined(PR_LOGGING)
nsCString
DumpTimeRanges(dom::TimeRanges* aRanges)
{
  nsCString dump;

  dump = "[";

  for (uint32_t i = 0; i < aRanges->Length(); ++i) {
    if (i > 0) {
      dump += ", ";
    }
    ErrorResult dummy;
    dump += nsPrintfCString("(%f, %f)", aRanges->Start(i, dummy), aRanges->End(i, dummy));
  }

  dump += "]";

  return dump;
}
#endif

} 
