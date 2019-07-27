





#include "sandboxBroker.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/security_level.h"
#include "mozilla/sandboxing/sandboxLogging.h"

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
                         const bool aEnableLogging,
                         void **aProcessHandle)
{
  if (!sBrokerService || !mPolicy) {
    return false;
  }

  
  mPolicy->SetStdoutHandle(::GetStdHandle(STD_OUTPUT_HANDLE));
  mPolicy->SetStderrHandle(::GetStdHandle(STD_ERROR_HANDLE));

  
  if (aEnableLogging) {
    mozilla::sandboxing::ApplyLoggingPolicy(*mPolicy);
  }

  
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
SandboxBroker::SetSecurityLevelForContentProcess(int32_t aSandboxLevel)
{
  if (!mPolicy) {
    return false;
  }

  sandbox::JobLevel jobLevel;
  sandbox::TokenLevel accessTokenLevel;
  sandbox::IntegrityLevel initialIntegrityLevel;
  sandbox::IntegrityLevel delayedIntegrityLevel;

  if (aSandboxLevel > 2) {
    jobLevel = sandbox::JOB_LOCKDOWN;
    accessTokenLevel = sandbox::USER_LOCKDOWN;
    initialIntegrityLevel = sandbox::INTEGRITY_LEVEL_LOW;
    delayedIntegrityLevel = sandbox::INTEGRITY_LEVEL_UNTRUSTED;
  } else if (aSandboxLevel == 2) {
    jobLevel = sandbox::JOB_RESTRICTED;
    accessTokenLevel = sandbox::USER_LIMITED;
    initialIntegrityLevel = sandbox::INTEGRITY_LEVEL_LOW;
    delayedIntegrityLevel = sandbox::INTEGRITY_LEVEL_LOW;
  } else if (aSandboxLevel == 1) {
    jobLevel = sandbox::JOB_NONE;
    accessTokenLevel = sandbox::USER_NON_ADMIN;
    initialIntegrityLevel = sandbox::INTEGRITY_LEVEL_LOW;
    delayedIntegrityLevel = sandbox::INTEGRITY_LEVEL_LOW;
  } else {
    jobLevel = sandbox::JOB_NONE;
    accessTokenLevel = sandbox::USER_NON_ADMIN;
    
    
    initialIntegrityLevel = sandbox::INTEGRITY_LEVEL_LAST;
    delayedIntegrityLevel = sandbox::INTEGRITY_LEVEL_MEDIUM;
  }

  sandbox::ResultCode result = mPolicy->SetJobLevel(jobLevel,
                                                    0 );
  bool ret = (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                                  accessTokenLevel);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->SetIntegrityLevel(initialIntegrityLevel);
  ret = ret && (sandbox::SBOX_ALL_OK == result);
  result = mPolicy->SetDelayedIntegrityLevel(delayedIntegrityLevel);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  if (aSandboxLevel > 1) {
    result = mPolicy->SetAlternateDesktop(true);
    ret = ret && (sandbox::SBOX_ALL_OK == result);
  }

  
  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                            sandbox::TargetPolicy::FILES_ALLOW_ANY,
                            L"\\??\\pipe\\chrome.*");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                            sandbox::TargetPolicy::FILES_ALLOW_ANY,
                            L"\\??\\pipe\\gecko-crash-server-pipe.*");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_HANDLES,
                            sandbox::TargetPolicy::HANDLES_DUP_BROKER,
                            L"File");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_HANDLES,
                            sandbox::TargetPolicy::HANDLES_DUP_BROKER,
                            L"Section");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  return ret;
}
#endif

bool
SandboxBroker::SetSecurityLevelForPluginProcess(int32_t aSandboxLevel)
{
  if (!mPolicy) {
    return false;
  }

  sandbox::ResultCode result;
  bool ret;
  if (aSandboxLevel >= 2) {
    result = mPolicy->SetJobLevel(sandbox::JOB_UNPROTECTED,
                                     0 );
    ret = (sandbox::SBOX_ALL_OK == result);

    sandbox::TokenLevel tokenLevel;
    if (aSandboxLevel >= 3) {
      tokenLevel = sandbox::USER_LIMITED;
    } else {
      tokenLevel = sandbox::USER_INTERACTIVE;
    }

    result = mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                                    tokenLevel);
    ret = ret && (sandbox::SBOX_ALL_OK == result);

    sandbox::MitigationFlags mitigations =
      sandbox::MITIGATION_BOTTOM_UP_ASLR |
      sandbox::MITIGATION_HEAP_TERMINATE |
      sandbox::MITIGATION_SEHOP |
      sandbox::MITIGATION_DEP_NO_ATL_THUNK |
      sandbox::MITIGATION_DEP;

    result = mPolicy->SetProcessMitigations(mitigations);
    ret = ret && (sandbox::SBOX_ALL_OK == result);

    mitigations =
      sandbox::MITIGATION_STRICT_HANDLE_CHECKS;

    result = mPolicy->SetDelayedProcessMitigations(mitigations);
    ret = ret && (sandbox::SBOX_ALL_OK == result);

    
    result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                              sandbox::TargetPolicy::FILES_ALLOW_ANY,
                              L"\\??\\pipe\\jpi2_pid*_pipe*");
    ret = ret && (sandbox::SBOX_ALL_OK == result);

  } else {
    result = mPolicy->SetJobLevel(sandbox::JOB_NONE,
                                     0 );
    ret = (sandbox::SBOX_ALL_OK == result);

    result = mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                                    sandbox::USER_NON_ADMIN);
    ret = ret && (sandbox::SBOX_ALL_OK == result);
  }

  result = mPolicy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_MEDIUM);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                            sandbox::TargetPolicy::FILES_ALLOW_ANY,
                            L"\\??\\pipe\\chrome.*");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_HANDLES,
                            sandbox::TargetPolicy::HANDLES_DUP_ANY,
                            L"Section");
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
                           sandbox::USER_LOCKDOWN);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->SetAlternateDesktop(true);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result = mPolicy->SetIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  result =
    mPolicy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_UNTRUSTED);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  sandbox::MitigationFlags mitigations =
    sandbox::MITIGATION_BOTTOM_UP_ASLR |
    sandbox::MITIGATION_HEAP_TERMINATE |
    sandbox::MITIGATION_SEHOP |
    sandbox::MITIGATION_DEP_NO_ATL_THUNK |
    sandbox::MITIGATION_DEP;

  result = mPolicy->SetProcessMitigations(mitigations);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  mitigations =
    sandbox::MITIGATION_STRICT_HANDLE_CHECKS |
    sandbox::MITIGATION_DLL_SEARCH_ORDER;

  result = mPolicy->SetDelayedProcessMitigations(mitigations);
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  
  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                            sandbox::TargetPolicy::FILES_ALLOW_ANY,
                            L"\\??\\pipe\\chrome.*");
  ret = ret && (sandbox::SBOX_ALL_OK == result);

  
  result = mPolicy->AddRule(sandbox::TargetPolicy::SUBSYS_FILES,
                            sandbox::TargetPolicy::FILES_ALLOW_ANY,
                            L"\\??\\pipe\\gecko-crash-server-pipe.*");
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
