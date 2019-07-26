



#ifndef SANDBOX_SRC_SYNC_DISPATCHER_H_
#define SANDBOX_SRC_SYNC_DISPATCHER_H_

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {


class SyncDispatcher : public Dispatcher {
 public:
  explicit SyncDispatcher(PolicyBase* policy_base);
  ~SyncDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

private:
  
  bool CreateEvent(IPCInfo* ipc, std::wstring* name, DWORD manual_reset,
                   DWORD initial_state);

  
  bool OpenEvent(IPCInfo* ipc, std::wstring* name, DWORD desired_access,
                 DWORD inherit_handle);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(SyncDispatcher);
};

}  

#endif  
