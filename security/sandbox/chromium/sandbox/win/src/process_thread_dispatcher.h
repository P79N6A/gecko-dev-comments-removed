



#ifndef SANDBOX_SRC_PROCESS_THREAD_DISPATCHER_H_
#define SANDBOX_SRC_PROCESS_THREAD_DISPATCHER_H_

#include "base/basictypes.h"
#include "base/strings/string16.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_policy_base.h"

namespace sandbox {


class ThreadProcessDispatcher : public Dispatcher {
 public:
  explicit ThreadProcessDispatcher(PolicyBase* policy_base);
  ~ThreadProcessDispatcher() {}

  
  virtual bool SetupService(InterceptionManager* manager, int service);

 private:
  
  bool NtOpenThread(IPCInfo* ipc, uint32 desired_access, uint32 thread_id);

  
  bool NtOpenProcess(IPCInfo* ipc, uint32 desired_access, uint32 process_id);

  
  bool NtOpenProcessToken(IPCInfo* ipc, HANDLE process, uint32 desired_access);

  
  bool NtOpenProcessTokenEx(IPCInfo* ipc,
                            HANDLE process,
                            uint32 desired_access,
                            uint32 attributes);

  
  bool CreateProcessW(IPCInfo* ipc,
                      base::string16* name,
                      base::string16* cmd_line,
                      base::string16* cur_dir,
                      CountedBuffer* info);

  PolicyBase* policy_base_;
  DISALLOW_COPY_AND_ASSIGN(ThreadProcessDispatcher);
};

}  

#endif  
