





#ifndef __SECURITY_SANDBOX_SANDBOXBROKER_H__
#define __SECURITY_SANDBOX_SANDBOXBROKER_H__

#ifdef SANDBOX_EXPORTS
#define SANDBOX_EXPORT __declspec(dllexport)
#else
#define SANDBOX_EXPORT __declspec(dllimport)
#endif

namespace sandbox {
  class BrokerServices;
  class TargetPolicy;
}

namespace mozilla {

class SANDBOX_EXPORT SandboxBroker
{
public:
  SandboxBroker();
  bool LaunchApp(const wchar_t *aPath,
                 const wchar_t *aArguments,
                 void **aProcessHandle);
  virtual ~SandboxBroker();

  
#if defined(MOZ_CONTENT_SANDBOX)
  bool SetSecurityLevelForContentProcess(bool inWarnOnlyMode);
#endif
  bool SetSecurityLevelForPluginProcess();
  bool SetSecurityLevelForIPDLUnitTestProcess();
  bool SetSecurityLevelForGMPlugin();

  
  bool AllowReadFile(wchar_t const *file);
  bool AllowReadWriteFile(wchar_t const *file);
  bool AllowDirectory(wchar_t const *dir);

private:
  static sandbox::BrokerServices *sBrokerService;
  sandbox::TargetPolicy *mPolicy;
};

} 

#endif
