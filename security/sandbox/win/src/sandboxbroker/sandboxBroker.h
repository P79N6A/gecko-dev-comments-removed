





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

  
  bool SetSecurityLevelForContentProcess();
  bool SetSecurityLevelForPluginProcess();
  bool SetSecurityLevelForIPDLUnitTestProcess();
  bool SetSecurityLevelForGMPlugin();

private:
  static sandbox::BrokerServices *sBrokerService;
  sandbox::TargetPolicy *mPolicy;
};

} 

#endif
