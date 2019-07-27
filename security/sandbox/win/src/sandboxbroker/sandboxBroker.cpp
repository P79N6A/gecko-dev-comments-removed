





#include "sandboxBroker.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/security_level.h"

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

bool
SandboxBroker::SetSecurityLevelForContentProcess()
{
  if (!mPolicy) {
    return false;
  }

  mPolicy->SetJobLevel(sandbox::JOB_NONE, 0);
  mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                         sandbox::USER_RESTRICTED_SAME_ACCESS);
  mPolicy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
  mPolicy->SetAlternateDesktop(true);
  return true;
}

bool
SandboxBroker::SetSecurityLevelForPluginProcess()
{
  if (!mPolicy) {
    return false;
  }

  mPolicy->SetJobLevel(sandbox::JOB_NONE, 0);
  mPolicy->SetTokenLevel(sandbox::USER_UNPROTECTED,
                         sandbox::USER_UNPROTECTED);
  return true;
}

bool
SandboxBroker::SetSecurityLevelForIPDLUnitTestProcess()
{
  if (!mPolicy) {
    return false;
  }

  mPolicy->SetJobLevel(sandbox::JOB_NONE, 0);
  mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                         sandbox::USER_RESTRICTED_SAME_ACCESS);
  return true;
}

bool
SandboxBroker::SetSecurityLevelForGMPlugin()
{
  if (!mPolicy) {
    return false;
  }

  mPolicy->SetJobLevel(sandbox::JOB_LOCKDOWN, 0);
  mPolicy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                         sandbox::USER_RESTRICTED_SAME_ACCESS);
  mPolicy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_LOW);
  mPolicy->SetAlternateDesktop(true);
  return true;
}


SandboxBroker::~SandboxBroker()
{
  if (mPolicy) {
    mPolicy->Release();
    mPolicy = nullptr;
  }
}

}
