





#include "SpdyZlibReporter.h"

namespace mozilla {

NS_IMPL_ISUPPORTS(SpdyZlibReporter, nsIMemoryReporter)

 Atomic<size_t> SpdyZlibReporter::sAmount;

 void*
SpdyZlibReporter::Alloc(void*, uInt items, uInt size)
{
  void* p = moz_xmalloc(items * size);
  sAmount += MallocSizeOfOnAlloc(p);
  return p;
}

 void
SpdyZlibReporter::Free(void*, void* p)
{
  sAmount -= MallocSizeOfOnFree(p);
  moz_free(p);
}

NS_IMETHODIMP
SpdyZlibReporter::CollectReports(nsIHandleReportCallback* aHandleReport,
                                 nsISupports* aData, bool aAnonymize)
{
  return MOZ_COLLECT_REPORT(
    "explicit/network/spdy-zlib-buffers", KIND_HEAP, UNITS_BYTES, sAmount,
    "Memory allocated for SPDY zlib send and receive buffers.");
}

} 
