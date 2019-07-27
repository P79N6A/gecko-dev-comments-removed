





#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

#include "mozilla/Types.h"

namespace mozilla {









#ifdef MOZ_CONTENT_SANDBOX

MOZ_EXPORT bool CanSandboxContentProcess();
MOZ_EXPORT void SetContentProcessSandbox();
#endif
#ifdef MOZ_GMP_SANDBOX

MOZ_EXPORT bool CanSandboxMediaPlugin();
MOZ_EXPORT void SetMediaPluginSandbox(const char *aFilePath);
#endif

} 

#endif 
