





#include "GMPLoader.h"
#include <stdio.h>
#include "mozilla/Attributes.h"
#include "mozilla/NullPtr.h"
#include "gmp-entrypoints.h"
#include "prlink.h"

#include <string>

#if defined(XP_WIN) && defined(MOZ_SANDBOX)
#include "mozilla/sandboxTarget.h"
#include "windows.h"
#include <intrin.h>
#include <assert.h>
#endif

#if defined(HASH_NODE_ID_WITH_DEVICE_ID)








#include "rlz/lib/machine_id.h"
#include "rlz/lib/string_utils.h"
#include "sha256.h"
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
                    const GMPPlatformAPI* aPlatformAPI) MOZ_OVERRIDE;

  virtual GMPErr GetAPI(const char* aAPIName,
                        void* aHostAPI,
                        void** aPluginAPI) MOZ_OVERRIDE;

  virtual void Shutdown() MOZ_OVERRIDE;

#ifdef SANDBOX_NOT_STATICALLY_LINKED_INTO_PLUGIN_CONTAINER
  virtual void SetStartSandboxStarter(SandboxStarter* aStarter) MOZ_OVERRIDE {
    mSandboxStarter = aStarter;
  }
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
    SecureZeroMemory(bottom, (top - bottom));
  } else
#endif
  {
    nodeId = std::string(aOriginSalt, aOriginSalt + aOriginSaltLen);
  }

#if defined(MOZ_GMP_SANDBOX)
  
  
  
  if (mSandboxStarter) {
    mSandboxStarter->Start(aLibPath);
  }
#endif

  
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

} 
} 

