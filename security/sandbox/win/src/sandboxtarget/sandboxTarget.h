





#ifndef __SECURITY_SANDBOX_SANDBOXTARGET_H__
#define __SECURITY_SANDBOX_SANDBOXTARGET_H__

#include <windows.h>

#include "base/MissingBasicTypes.h"
#include "sandbox/win/src/sandbox.h"

#ifdef TARGET_SANDBOX_EXPORTS
#define TARGET_SANDBOX_EXPORT __declspec(dllexport)
#else
#define TARGET_SANDBOX_EXPORT __declspec(dllimport)
#endif
namespace mozilla {


class TARGET_SANDBOX_EXPORT SandboxTarget
{
public:
  


  static SandboxTarget* Instance()
  {
    static SandboxTarget sb;
    return &sb;
  }

  





  sandbox::ResultCode
  InitTargetServices(sandbox::TargetServices* aTargetServices)
  {
    MOZ_ASSERT(aTargetServices);
    MOZ_ASSERT(!mTargetServices,
               "Sandbox TargetServices must only be initialized once.");

    sandbox::ResultCode result = aTargetServices->Init();
    if (sandbox::SBOX_ALL_OK == result) {
      mTargetServices = aTargetServices;
    }

    return result;
  }

  



  void StartSandbox()
  {
    if (mTargetServices) {
      mTargetServices->LowerToken();
    }
  }

  



  bool BrokerDuplicateHandle(HANDLE aSourceHandle, DWORD aTargetProcessId,
                             HANDLE* aTargetHandle, DWORD aDesiredAccess,
                             DWORD aOptions)
  {
    if (!mTargetServices) {
      return false;
    }

    sandbox::ResultCode result =
      mTargetServices->DuplicateHandle(aSourceHandle, aTargetProcessId,
                                       aTargetHandle, aDesiredAccess, aOptions);
    return (sandbox::SBOX_ALL_OK == result);
  }

protected:
  SandboxTarget() :
    mTargetServices(nullptr)
  {
  }

  sandbox::TargetServices* mTargetServices;
};


} 

#endif
