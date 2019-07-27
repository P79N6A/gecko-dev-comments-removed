





#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

#include "mozilla/Types.h"

namespace mozilla {









#ifdef MOZ_CONTENT_SANDBOX

MFBT_API bool CanSandboxContentProcess();
MFBT_API void SetContentProcessSandbox();
#endif
#ifdef MOZ_GMP_SANDBOX

MFBT_API bool CanSandboxMediaPlugin();
MFBT_API void SetMediaPluginSandbox(const char *aFilePath);
#endif

} 

#endif 
