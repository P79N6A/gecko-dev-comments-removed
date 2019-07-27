





#ifndef GMP_LOADER_H__
#define GMP_LOADER_H__

#include <stdint.h>
#include "gmp-entrypoints.h"

namespace mozilla {
namespace gmp {

class SandboxStarter {
public:
  virtual ~SandboxStarter() {}
  virtual void Start() = 0;
};

#if (defined(XP_LINUX) || defined(XP_MACOSX))
#define SANDBOX_NOT_STATICALLY_LINKED_INTO_PLUGIN_CONTAINER 1
#endif

















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

#ifdef SANDBOX_NOT_STATICALLY_LINKED_INTO_PLUGIN_CONTAINER
  
  
  virtual void SetStartSandboxStarter(SandboxStarter* aStarter) = 0;
#endif
};



GMPLoader* CreateGMPLoader(SandboxStarter* aStarter);

} 
} 

#endif 
