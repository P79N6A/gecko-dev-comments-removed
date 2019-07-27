



#ifndef SANDBOX_SRC_SYNC_DISPATCHER_H_
#define SANDBOX_SRC_SYNC_DISPATCHER_H_

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {


class SyncDispatcher : public Dispatcher {
 public:
  explicit SyncDispatcher(PolicyBase* policy_base);
  ~SyncDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

private:
  
  bool CreateEvent(IPCInfo* ipc,
                   base::string16* name,
                   uint32 event_type,
                   uint32 initial_state);

  
  bool OpenEvent(IPCInfo* ipc, base::string16* name, uint32 desired_access);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(SyncDispatcher);
};

}  

#endif  
