



#ifndef SANDBOX_SRC_NAMED_PIPE_DISPATCHER_H__
#define SANDBOX_SRC_NAMED_PIPE_DISPATCHER_H__

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {


class NamedPipeDispatcher : public Dispatcher {
 public:
  explicit NamedPipeDispatcher(PolicyBase* policy_base);
  ~NamedPipeDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

 private:
  
  
  bool CreateNamedPipe(IPCInfo* ipc, std::wstring* name, DWORD open_mode,
                       DWORD pipe_mode, DWORD max_instances,
                       DWORD out_buffer_size, DWORD in_buffer_size,
                       DWORD default_timeout);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(NamedPipeDispatcher);
};

}  

#endif  
