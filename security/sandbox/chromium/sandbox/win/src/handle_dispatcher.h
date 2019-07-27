



#ifndef SANDBOX_SRC_HANDLE_DISPATCHER_H_
#define SANDBOX_SRC_HANDLE_DISPATCHER_H_

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {


class HandleDispatcher : public Dispatcher {
 public:
  explicit HandleDispatcher(PolicyBase* policy_base);
  ~HandleDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

 private:
  
  
  bool DuplicateHandleProxy(IPCInfo* ipc,
                            HANDLE source_handle,
                            uint32 target_process_id,
                            uint32 desired_access,
                            uint32 options);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(HandleDispatcher);
};

}  

#endif  

