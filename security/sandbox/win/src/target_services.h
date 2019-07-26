



#ifndef SANDBOX_SRC_TARGET_SERVICES_H__
#define SANDBOX_SRC_TARGET_SERVICES_H__

#include "base/basictypes.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/win_utils.h"

namespace sandbox {

class ProcessState {
 public:
  ProcessState() : process_state_(0) {}

  
  bool IsKernel32Loaded();

  
  bool InitCalled();

  
  bool RevertedToSelf();

  
  void SetKernel32Loaded();
  void SetInitCalled();
  void SetRevertedToSelf();

 public:
  int process_state_;
  DISALLOW_COPY_AND_ASSIGN(ProcessState);
};





class TargetServicesBase : public TargetServices {
 public:
  TargetServicesBase();

  
  virtual ResultCode Init();
  virtual void LowerToken();
  virtual ProcessState* GetState();
  virtual ResultCode DuplicateHandle(HANDLE source_handle,
                                     DWORD target_process_id,
                                     HANDLE* target_handle,
                                     DWORD desired_access,
                                     DWORD options);

  
  static TargetServicesBase* GetInstance();

  
  
  
  
  bool TestIPCPing(int version);

 private:
  ProcessState process_state_;
  DISALLOW_COPY_AND_ASSIGN(TargetServicesBase);
};

}  

#endif  
