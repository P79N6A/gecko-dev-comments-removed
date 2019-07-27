





#include "GMPLoader.h"
#include <stdio.h>
#include "mozilla/Attributes.h"
#include "gmp-entrypoints.h"
#include "prlink.h"

#include <string>

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
#include "mozilla/Scoped.h"
#include "windows.h"
#include <intrin.h>
#include <assert.h>
#endif

#if defined(HASH_NODE_ID_WITH_DEVICE_ID)








#include "rlz/lib/machine_id.h"
#include "rlz/lib/string_utils.h"
#include "sha256.h"
#endif

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
namespace {


struct ScopedActCtxHandleTraits
{
  typedef HANDLE type;

  static type empty()
  {
    return INVALID_HANDLE_VALUE;
  }

  static void release(type aActCtxHandle)
  {
    if (aActCtxHandle != INVALID_HANDLE_VALUE) {
      ReleaseActCtx(aActCtxHandle);
    }
  }
};
typedef mozilla::Scoped<ScopedActCtxHandleTraits> ScopedActCtxHandle;

} 
#endif

namespace mozilla {
namespace gmp {

class GMPLoaderImpl : public GMPLoader {
public:
  explicit GMPLoaderImpl(SandboxStarter* aStarter)
    : mSandboxStarter(aStarter)
  {}
  virtual ~GMPLoaderImpl() {}

  virtual bool Load(const char* aLibPath,
                    uint32_t aLibPathLen,
                    char* aOriginSalt,
                    uint32_t aOriginSaltLen,
                    const GMPPlatformAPI* aPlatformAPI) override;

  virtual GMPErr GetAPI(const char* aAPIName,
                        void* aHostAPI,
                        void** aPluginAPI) override;

  virtual void Shutdown() override;

#if defined(XP_MACOSX)
  virtual void SetSandboxInfo(MacSandboxInfo* aSandboxInfo) override;
#endif

private:
  PRLibrary* mLib;
  GMPGetAPIFunc mGetAPIFunc;
  SandboxStarter* mSandboxStarter;
};

GMPLoader* CreateGMPLoader(SandboxStarter* aStarter) {
  return static_cast<GMPLoader*>(new GMPLoaderImpl(aStarter));
}

#if defined(XP_WIN) && defined(HASH_NODE_ID_WITH_DEVICE_ID)
MOZ_NEVER_INLINE
static bool
GetStackAfterCurrentFrame(uint8_t** aOutTop, uint8_t** aOutBottom)
{
  
  
  uint8_t* top = (uint8_t*)_AddressOfReturnAddress();

  
  MEMORY_BASIC_INFORMATION memInfo = {0};
  uint8_t* bottom = top;
  while (1) {
    if (!VirtualQuery(bottom, &memInfo, sizeof(memInfo))) {
      return false;
    }
    if ((memInfo.Protect & PAGE_GUARD) == PAGE_GUARD) {
      bottom = (uint8_t*)memInfo.BaseAddress + memInfo.RegionSize;
#ifdef DEBUG
      if (!VirtualQuery(bottom, &memInfo, sizeof(memInfo))) {
        return false;
      }
      assert(!(memInfo.Protect & PAGE_GUARD)); 
#endif
      break;
    } else if (memInfo.State != MEM_COMMIT ||
               (memInfo.AllocationProtect & PAGE_READWRITE) != PAGE_READWRITE) {
      return false;
    }
    bottom = (uint8_t*)memInfo.BaseAddress - 1;
  }
  *aOutTop = top;
  *aOutBottom = bottom;
  return true;
}
#endif

bool
GMPLoaderImpl::Load(const char* aLibPath,
                    uint32_t aLibPathLen,
                    char* aOriginSalt,
                    uint32_t aOriginSaltLen,
                    const GMPPlatformAPI* aPlatformAPI)
{
  std::string nodeId;
#ifdef HASH_NODE_ID_WITH_DEVICE_ID
  if (aOriginSaltLen > 0) {
    string16 deviceId;
    int volumeId;
    if (!rlz_lib::GetRawMachineId(&deviceId, &volumeId)) {
      return false;
    }

    SHA256Context ctx;
    SHA256_Begin(&ctx);
    SHA256_Update(&ctx, (const uint8_t*)aOriginSalt, aOriginSaltLen);
    SHA256_Update(&ctx, (const uint8_t*)deviceId.c_str(), deviceId.size() * sizeof(string16::value_type));
    SHA256_Update(&ctx, (const uint8_t*)&volumeId, sizeof(int));
    uint8_t digest[SHA256_LENGTH] = {0};
    unsigned int digestLen = 0;
    SHA256_End(&ctx, digest, &digestLen, SHA256_LENGTH);

    
    
    
    memset(&ctx, 0, sizeof(ctx));
    memset(aOriginSalt, 0, aOriginSaltLen);
    volumeId = 0;
    memset(&deviceId[0], '*', sizeof(string16::value_type) * deviceId.size());
    deviceId = L"";

    if (!rlz_lib::BytesToString(digest, SHA256_LENGTH, &nodeId)) {
      return false;
    }
    
    
    
    
    uint8_t* top;
    uint8_t* bottom;
    if (!GetStackAfterCurrentFrame(&top, &bottom)) {
      return false;
    }
    assert(top >= bottom);
    
    
    
    
    for (volatile uint8_t* p = (volatile uint8_t*)bottom; p < top; p++) {
      *p = 0;
    }
  } else
#endif
  {
    nodeId = std::string(aOriginSalt, aOriginSalt + aOriginSaltLen);
  }

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
  
  
  
  
  int pathLen = MultiByteToWideChar(CP_ACP, 0, aLibPath, -1, nullptr, 0);
  if (pathLen == 0) {
    return false;
  }

  wchar_t* widePath = new wchar_t[pathLen];
  if (MultiByteToWideChar(CP_ACP, 0, aLibPath, -1, widePath, pathLen) == 0) {
    delete[] widePath;
    return false;
  }

  ACTCTX actCtx = { sizeof(actCtx) };
  actCtx.dwFlags = ACTCTX_FLAG_RESOURCE_NAME_VALID;
  actCtx.lpSource = widePath;
  actCtx.lpResourceName = ISOLATIONAWARE_MANIFEST_RESOURCE_ID;
  ScopedActCtxHandle actCtxHandle(CreateActCtx(&actCtx));
  delete[] widePath;
#endif

  
  
  
  if (mSandboxStarter && !mSandboxStarter->Start(aLibPath)) {
    return false;
  }

  
  PRLibSpec libSpec;
  libSpec.value.pathname = aLibPath;
  libSpec.type = PR_LibSpec_Pathname;
  mLib = PR_LoadLibraryWithFlags(libSpec, 0);
  if (!mLib) {
    return false;
  }

  GMPInitFunc initFunc = reinterpret_cast<GMPInitFunc>(PR_FindFunctionSymbol(mLib, "GMPInit"));
  if (!initFunc) {
    return false;
  }

  if (initFunc(aPlatformAPI) != GMPNoErr) {
    return false;
  }

  GMPSetNodeIdFunc setNodeIdFunc = reinterpret_cast<GMPSetNodeIdFunc>(PR_FindFunctionSymbol(mLib, "GMPSetNodeId"));
  if (setNodeIdFunc) {
    setNodeIdFunc(nodeId.c_str(), nodeId.size());
  }

  mGetAPIFunc = reinterpret_cast<GMPGetAPIFunc>(PR_FindFunctionSymbol(mLib, "GMPGetAPI"));
  if (!mGetAPIFunc) {
    return false;
  }

  return true;
}

GMPErr
GMPLoaderImpl::GetAPI(const char* aAPIName,
                      void* aHostAPI,
                      void** aPluginAPI)
{
  return mGetAPIFunc ? mGetAPIFunc(aAPIName, aHostAPI, aPluginAPI)
                     : GMPGenericErr;
}

void
GMPLoaderImpl::Shutdown()
{
  if (mLib) {
    GMPShutdownFunc shutdownFunc = reinterpret_cast<GMPShutdownFunc>(PR_FindFunctionSymbol(mLib, "GMPShutdown"));
    if (shutdownFunc) {
      shutdownFunc();
    }
    PR_UnloadLibrary(mLib);
    mLib = nullptr;
  }
}

#if defined(XP_MACOSX)
void
GMPLoaderImpl::SetSandboxInfo(MacSandboxInfo* aSandboxInfo)
{
  if (mSandboxStarter) {
    mSandboxStarter->SetSandboxInfo(aSandboxInfo);
  }
}
#endif
} 
} 

