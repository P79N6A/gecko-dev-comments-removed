





#include "sandboxBroker.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/security_level.h"
#if defined(MOZ_CONTENT_SANDBOX)
#include "mozilla/warnonlysandbox/warnOnlySandbox.h"
#endif

namespace mozilla
{

sandbox::BrokerServices *SandboxBroker::sBrokerService = nullptr;

SandboxBroker::SandboxBroker()
{
  
  
  if (!sBrokerService) {
    sBrokerService = sandbox::SandboxFactory::GetBrokerServices();
    if (sBrokerService) {
      sandbox::ResultCode result = sBrokerService->Init();
      if (result != sandbox::SBOX_ALL_OK) {
        sBrokerService = nullptr;
      }
    }
  }

  mPolicy = sBrokerService->CreatePolicy();
}

bool
SandboxBroker::LaunchApp(const wchar_t *aPath,
                         const wchar_t *aArguments,
                         void **aProcessHandle)
{
  if (!sBrokerService || !mPolicy) {
    return false;
  }

  
  mPolicy->SetStdoutHandle(::GetStdHandle(STD_OUTPUT_HANDLE));
  mPolicy->SetStderrHandle(::GetStdHandle(STD_ERROR_HANDLE));

  
  PROCESS_INFORMATION targetInfo;
  sandbox::ResultCode result;
  result = sBrokerService->SpawnTarget(aPath, aArguments, mPolicy, &targetInfo);

  
  
  ResumeThread(targetInfo.hThread);
  CloseHandle(targetInfo.hThread);

  
  *aProcessHandle = targetInfo.hProcess;

  return true;
}

#if defined(MOZ_CONTENT_SANDBOX)
bool
SandboxBroker::SetSecurityLevelForContentProcess(bool inWarnOnlyMode)
{
  if (!mPolicy) {
    return false;
  }

  auto result = mPolicy->SetJobLevel(sandbox::JOB_NONE, 0);
  bool ret = (sandbox::SBOX_ALL_OK == result);
  result =
    mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                           sandbox::USER_RESTRICTED_SAME_ACCESS);
  ret = ret && (sandbox::SBOX_ALL_OK == result);
  result =
    mPolicy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
  ret = ret && (sandbox::SBOX_ALL_OK == result);
  result = mPolicy->SetAlternateDesktop(true);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  if (inWarnOnlyMode) {
    mozilla::warnonlysandbox::ApplyWarnOnlyPolicy(*mPolicy);
  }
  return ret;
}
#endif

bool
SandboxBroker::SetSecurityLevelForPluginProcess()
{
  if (!mPolicy) {
    return false;
  }

  auto result = mPolicy->SetJobLevel(sandbox::JOB_NONE, 0);
  bool ret = (sandbox::SBOX_ALL_OK == result);
  result = mPolicy->SetTokenLevel(sandbox::USER_UNPROTECTED,
                                  sandbox::USER_UNPROTECTED);
  ret = ret && (sandbox::SBOX_ALL_OK == result);
  return ret;
}

bool
SandboxBroker::SetSecurityLevelForIPDLUnitTestProcess()
{
  if (!mPolicy) {
    return false;
  }

  auto result = mPolicy->SetJobLevel(sandbox::JOB_NONE, 0);
  bool ret = (sandbox::SBOX_ALL_OK == result);
  result =
    mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                           sandbox::USER_RESTRICTED_SAME_ACCESS);
  ret = ret && (sandbox::SBOX_ALL_OK == result);
  return ret;
}

bool
SandboxBroker::SetSecurityLevelForGMPlugin()
{
  if (!mPolicy) {
    return false;
  }

  auto result = mPolicy->SetJobLevel(sandbox::JOB_LOCKDOWN, 0);
  bool ret = (sandbox::SBOX_ALL_OK == result);
  result =
    mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                           sandbox::USER_RESTRICTED);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->SetAlternateDesktop(true);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  
  
  
  
  
  

  result =
    mPolicy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_UNTRUSTED);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                            sandbox::TargetPolicy::FILES_ALLOW_ANY,
                            L"\\??\\pipe\\chrome.*");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

#ifdef DEBUG
  
  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_SYNC,
                            sandbox::TargetPolicy::EVENTS_ALLOW_ANY,
                            L"ChromeIPCLog.*");
  ret = ret && (sandbox::SBOX_ALL_OK == result);
#endif

  
  
  
  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_CURRENT_USER");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_CURRENT_USER\\Control Panel\\Desktop");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_CURRENT_USER\\Control Panel\\Desktop\\LanguageConfiguration");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SideBySide");
  ret = ret && (sandbox::SBOX_ALL_OK == result);


  
  
  
  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Policies\\Microsoft\\MUI\\Settings");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_CURRENT_USER\\Software\\Policies\\Microsoft\\Control Panel\\Desktop");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_CURRENT_USER\\Control Panel\\Desktop\\PreferredUILanguages");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_REGISTRY,
                            sandbox::TargetPolicy::REG_ALLOW_READONLY,
                            L"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\SideBySide\\PreferExternalManifest");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  return ret;
}

bool
SandboxBroker::AllowReadFile(wchar_t const *file)
{
  auto result =
    mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                     sandbox::TargetPolicy::FILES_ALLOW_READONLY,
                     file);
  return (sandbox::SBOX_ALL_OK == result);
}

bool
SandboxBroker::AllowReadWriteFile(wchar_t const *file)
{
  auto result =
    mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                     sandbox::TargetPolicy::FILES_ALLOW_ANY,
                     file);
  return (sandbox::SBOX_ALL_OK == result);
}

bool
SandboxBroker::AllowDirectory(wchar_t const *dir)
{
  auto result =
    mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                     sandbox::TargetPolicy::FILES_ALLOW_DIR_ANY,
                     dir);
  return (sandbox::SBOX_ALL_OK == result);
}

SandboxBroker::~SandboxBroker()
{
  if (mPolicy) {
    mPolicy->Release();
    mPolicy = nullptr;
  }
}

}
