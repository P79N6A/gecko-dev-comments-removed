




#include "MediaSourceUtils.h"

#include "mozilla/Logging.h"
#include "nsPrintfCString.h"

namespace mozilla {

nsCString
DumpTimeRanges(const media::TimeIntervals& aRanges)
{
  nsCString dump;

  dump = "[";

  for (uint32_t i = 0; i < aRanges.Length(); ++i) {
    if (i > 0) {
      dump += ", ";
    }
    dump += nsPrintfCString("(%f, %f)",
                            aRanges.Start(i).ToSeconds(),
                            aRanges.End(i).ToSeconds());
  }

  dump += "]";

  return dump;
}

} 
