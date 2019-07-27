

















#ifndef SANDBOX_WIN_SRC_SANDBOX_H_
#define SANDBOX_WIN_SRC_SANDBOX_H_

#include <windows.h>

#include "base/basictypes.h"
#include "sandbox/win/src/sandbox_policy.h"
#include "sandbox/win/src/sandbox_types.h"


namespace sandbox {

class BrokerServices;
class ProcessState;
class TargetPolicy;
class TargetServices;














class BrokerServices {
 public:
  
  
  
  
  virtual ResultCode Init() = 0;

  
  
  
  virtual TargetPolicy* CreatePolicy() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual ResultCode SpawnTarget(const wchar_t* exe_path,
                                 const wchar_t* command_line,
                                 TargetPolicy* policy,
                                 PROCESS_INFORMATION* target) = 0;

  
  
  
  
  
  virtual ResultCode WaitForAllTargets() = 0;

  
  
  
  
  
  
  virtual ResultCode AddTargetPeer(HANDLE peer_process) = 0;

  
  
  virtual ResultCode InstallAppContainer(const wchar_t* sid,
                                         const wchar_t* name) = 0;

  
  
  virtual ResultCode UninstallAppContainer(const wchar_t* sid) = 0;
};
























class TargetServices {
 public:
  
  
  
  
  virtual ResultCode Init() = 0;

  
  
  
  virtual void LowerToken() = 0;

  
  
  
  virtual ProcessState* GetState() = 0;

  
  
  
  
  
  
  
  
  virtual ResultCode DuplicateHandle(HANDLE source_handle,
                                     DWORD target_process_id,
                                     HANDLE* target_handle,
                                     DWORD desired_access,
                                     DWORD options) = 0;
};

}  


#endif  
