





#ifndef replace_malloc_bridge_h
#define replace_malloc_bridge_h








































struct ReplaceMallocBridge;

#ifdef __cplusplus

#include "mozilla/NullPtr.h"
#include "mozilla/Types.h"

#ifndef REPLACE_MALLOC_IMPL

extern "C" MFBT_API ReplaceMallocBridge* get_bridge();
#endif

struct ReplaceMallocBridge
{
  ReplaceMallocBridge() : mVersion(0) {}

#ifndef REPLACE_MALLOC_IMPL
  

  static ReplaceMallocBridge* Get(int aMinimumVersion) {
    static ReplaceMallocBridge* sSingleton = get_bridge();
    return (sSingleton && sSingleton->mVersion >= aMinimumVersion)
      ? sSingleton : nullptr;
  }
#endif

protected:
  const int mVersion;
};

#ifndef REPLACE_MALLOC_IMPL






struct ReplaceMalloc
{
};
#endif

#endif 

#endif 
