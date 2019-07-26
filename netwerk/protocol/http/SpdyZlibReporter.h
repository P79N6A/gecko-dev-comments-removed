







#include "mozilla/Assertions.h"
#include "mozilla/Atomics.h"
#include "mozilla/Attributes.h"
#include "nsIMemoryReporter.h"
#include "zlib.h"

namespace mozilla {

class SpdyZlibReporter MOZ_FINAL : public nsIMemoryReporter
{
  ~SpdyZlibReporter() {}

public:
  NS_DECL_ISUPPORTS

  SpdyZlibReporter()
  {
#ifdef DEBUG
    
    
    static bool hasRun = false;
    MOZ_ASSERT(!hasRun);
    hasRun = true;
#endif
    sAmount = 0;
  }

  static void* Alloc(void*, uInt items, uInt size);
  static void Free(void*, void* p);

private:
  
  
  static Atomic<size_t> sAmount;

  MOZ_DEFINE_MALLOC_SIZE_OF(MallocSizeOf)
  MOZ_DEFINE_MALLOC_SIZE_OF_ON_ALLOC(MallocSizeOfOnAlloc)
  MOZ_DEFINE_MALLOC_SIZE_OF_ON_FREE(MallocSizeOfOnFree)

  NS_IMETHODIMP
  CollectReports(nsIHandleReportCallback* aHandleReport, nsISupports* aData,
                 bool aAnonymize);
};

} 
