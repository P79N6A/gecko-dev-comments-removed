





#ifndef replace_malloc_bridge_h
#define replace_malloc_bridge_h








































struct ReplaceMallocBridge;

#include "mozilla/Types.h"

MOZ_BEGIN_EXTERN_C

#ifndef REPLACE_MALLOC_IMPL

MFBT_API ReplaceMallocBridge* get_bridge();
#endif




#define MALLOC_DECL(name, return_type, ...) \
  typedef return_type(name ## _impl_t)(__VA_ARGS__);

#include "malloc_decls.h"

#define MALLOC_DECL(name, return_type, ...) \
  name ## _impl_t * name;

typedef struct {
#include "malloc_decls.h"
} malloc_table_t;














#define MALLOC_DECL(name, return_type, ...) \
  return_type (*name ## _hook)(return_type, __VA_ARGS__);
#define MALLOC_DECL_VOID(name, ...) \
  void (*name ## _hook)(__VA_ARGS__);

typedef struct {
#include "malloc_decls.h"
  

  void (*realloc_hook_before)(void* aPtr);
} malloc_hook_table_t;

MOZ_END_EXTERN_C

#ifdef __cplusplus

namespace mozilla {
namespace dmd {
struct DMDFuncs;
}



struct DebugFdRegistry
{
  virtual void RegisterHandle(intptr_t aFd);

  virtual void UnRegisterHandle(intptr_t aFd);
};

} 

struct ReplaceMallocBridge
{
  ReplaceMallocBridge() : mVersion(3) {}

  
  virtual mozilla::dmd::DMDFuncs* GetDMDFuncs() { return nullptr; }

  



  virtual void InitDebugFd(mozilla::DebugFdRegistry&) {}

  












  virtual const malloc_table_t*
  RegisterHook(const char* aName, const malloc_table_t* aTable,
               const malloc_hook_table_t* aHookTable) { return nullptr; }

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

  static void InitDebugFd(mozilla::DebugFdRegistry& aRegistry)
  {
    auto singleton = ReplaceMallocBridge::Get( 2);
    if (singleton) {
      singleton->InitDebugFd(aRegistry);
    }
  }

  static const malloc_table_t*
  RegisterHook(const char* aName, const malloc_table_t* aTable,
               const malloc_hook_table_t* aHookTable)
  {
    auto singleton = ReplaceMallocBridge::Get( 3);
    return singleton ? singleton->RegisterHook(aName, aTable, aHookTable)
                     : nullptr;
  }
};
#endif

#endif 

#endif 
