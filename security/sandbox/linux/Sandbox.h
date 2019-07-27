





#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

#include "mozilla/Types.h"





#ifdef ANDROID

#define MOZ_SANDBOX_EXPORT MOZ_EXPORT
#else

#define MOZ_SANDBOX_EXPORT MOZ_EXPORT __attribute__((weak))
#endif

namespace mozilla {

#ifdef MOZ_CONTENT_SANDBOX


MOZ_SANDBOX_EXPORT void SetContentProcessSandbox();
#endif

#ifdef MOZ_GMP_SANDBOX


MOZ_SANDBOX_EXPORT void SetMediaPluginSandbox(const char *aFilePath);
#endif

} 

#endif 
