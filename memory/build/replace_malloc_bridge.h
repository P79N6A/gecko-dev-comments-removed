





#ifndef replace_malloc_bridge_h
#define replace_malloc_bridge_h








































struct ReplaceMallocBridge;

#ifdef __cplusplus

#include "mozilla/NullPtr.h"
#include "mozilla/Types.h"

#ifndef REPLACE_MALLOC_IMPL

extern "C" MFBT_API ReplaceMallocBridge* get_bridge();
#endif

namespace mozilla {
namespace dmd {
struct DMDFuncs;
}
}

struct ReplaceMallocBridge
{
  ReplaceMallocBridge() : mVersion(1) {}

  
  virtual mozilla::dmd::DMDFuncs* GetDMDFuncs() { return nullptr; }

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
  

  static mozilla::dmd::DMDFuncs* GetDMDFuncs()
  {
    auto singleton = ReplaceMallocBridge::Get( 1);
    return singleton ? singleton->GetDMDFuncs() : nullptr;
  }
};
#endif

#endif 

#endif 
