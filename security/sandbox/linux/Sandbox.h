





#ifndef mozilla_Sandbox_h
#define mozilla_Sandbox_h

namespace mozilla {









#ifdef MOZ_CONTENT_SANDBOX

bool CanSandboxContentProcess();
void SetContentProcessSandbox();
#endif
#ifdef MOZ_GMP_SANDBOX

bool CanSandboxMediaPlugin();
void SetMediaPluginSandbox(const char *aFilePath);
#endif

} 

#endif 
