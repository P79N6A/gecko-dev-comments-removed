





#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

#include "mozilla/Types.h"

namespace mozilla {


enum SandboxStatus {
  
  
  kSandboxingWouldFail,
  
  kSandboxingSupported,
  
  
  kSandboxingDisabled,
};


#ifdef MOZ_CONTENT_SANDBOX

MOZ_EXPORT SandboxStatus ContentProcessSandboxStatus();
MOZ_EXPORT void SetContentProcessSandbox();
#else
static inline SandboxStatus ContentProcessSandboxStatus()
{
  return kSandboxingDisabled;
}
static inline void SetContentProcessSandbox()
{
}
#endif

#ifdef MOZ_GMP_SANDBOX

MOZ_EXPORT SandboxStatus MediaPluginSandboxStatus();
MOZ_EXPORT void SetMediaPluginSandbox(const char *aFilePath);
#else
static inline SandboxStatus MediaPluginSandboxStatus()
{
  return kSandboxingDisabled;
}
static inline void SetMediaPluginSandbox()
{
}
#endif



enum SandboxFeatureFlags {
  kSandboxFeatureSeccompBPF = 1 << 0,
};

MOZ_EXPORT SandboxFeatureFlags GetSandboxFeatureFlags();

} 

#endif 
