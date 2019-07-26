





#include "sandboxBroker.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_factory.h"

namespace mozilla
{

SandboxBroker::SandboxBroker() :
  mBrokerService(nullptr)
{
}

bool
SandboxBroker::LaunchApp(const wchar_t *aPath,
                           const wchar_t *aArguments,
                           void **aProcessHandle)
{
  sandbox::ResultCode result;

  
  if (!mBrokerService) {
    mBrokerService = sandbox::SandboxFactory::GetBrokerServices();
    if (!mBrokerService) {
      return false;
    }

    result = mBrokerService->Init();
    if (result != sandbox::SBOX_ALL_OK) {
      return false;
    }
  }

  
  
  
  
  sandbox::TargetPolicy *policy = mBrokerService->CreatePolicy();
  policy->SetJobLevel(sandbox::JOB_NONE, 0);
  policy->SetTokenLevel(sandbox::USER_RESTRICTED_SAME_ACCESS,
                        sandbox::USER_RESTRICTED_SAME_ACCESS);
  policy->SetDelayedIntegrityLevel(sandbox::INTEGRITY_LEVEL_MEDIUM);

  
  PROCESS_INFORMATION targetInfo;
  result = mBrokerService->SpawnTarget(aPath, aArguments, policy, &targetInfo);

  
  
  ResumeThread(targetInfo.hThread);
  CloseHandle(targetInfo.hThread);

  
  *aProcessHandle = targetInfo.hProcess;

  policy->Release();

  return true;
}

SandboxBroker::~SandboxBroker()
{
}

}
