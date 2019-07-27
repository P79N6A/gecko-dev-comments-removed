





#ifndef GMP_LOADER_H__
#define GMP_LOADER_H__

#include <stdint.h>
#include "gmp-entrypoints.h"

#if defined(XP_MACOSX)
#include "mozilla/Sandbox.h"
#endif

namespace mozilla {
namespace gmp {

class SandboxStarter {
public:
  virtual ~SandboxStarter() {}
  virtual bool Start(const char* aLibPath) = 0;
#if defined(XP_MACOSX)
  
  
  
  virtual void SetSandboxInfo(MacSandboxInfo* aSandboxInfo) = 0;
#endif
};

















class GMPLoader {
public:
  virtual ~GMPLoader() {}

  
  
  
  virtual bool Load(const char* aLibPath,
                    uint32_t aLibPathLen,
                    char* aOriginSalt,
                    uint32_t aOriginSaltLen,
                    const GMPPlatformAPI* aPlatformAPI) = 0;

  
  virtual GMPErr GetAPI(const char* aAPIName, void* aHostAPI, void** aPluginAPI) = 0;

  
  
  virtual void Shutdown() = 0;

#if defined(XP_MACOSX)
  
  
  
  virtual void SetSandboxInfo(MacSandboxInfo* aSandboxInfo) = 0;
#endif
};



GMPLoader* CreateGMPLoader(SandboxStarter* aStarter);

} 
} 

#endif 
