



#ifndef SANDBOX_WIN_SRC_TARGET_PROCESS_H_
#define SANDBOX_WIN_SRC_TARGET_PROCESS_H_

#include <windows.h>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "base/win/scoped_handle.h"
#include "base/win/scoped_process_information.h"
#include "sandbox/win/src/crosscall_server.h"
#include "sandbox/win/src/sandbox_types.h"

namespace base {
namespace win {

class StartupInformation;

};  
};  

namespace sandbox {

class AttributeList;
class SharedMemIPCServer;
class ThreadProvider;



class TargetProcess {
 public:
  
  TargetProcess(HANDLE initial_token, HANDLE lockdown_token, HANDLE job,
                ThreadProvider* thread_pool);
  ~TargetProcess();

  
  
  
  
  void AddRef() {}
  void Release() {}

  
  DWORD Create(const wchar_t* exe_path,
               const wchar_t* command_line,
               bool inherit_handles,
               const base::win::StartupInformation& startup_info,
               base::win::ScopedProcessInformation* target_info);

  
  void Terminate();

  
  
  DWORD Init(Dispatcher* ipc_dispatcher, void* policy,
             uint32 shared_IPC_size, uint32 shared_policy_size);

  
  HANDLE Process() const {
    return sandbox_process_info_.process_handle();
  }

  
  HANDLE Job() const {
    return job_;
  }

  
  
  HMODULE MainModule() const {
    return reinterpret_cast<HMODULE>(base_address_);
  }

  
  const wchar_t* Name() const {
    return exe_name_.get();
  }

  
  DWORD ProcessId() const {
    return sandbox_process_info_.process_id();
  }

  
  HANDLE MainThread() const {
    return sandbox_process_info_.thread_handle();
  }

  
  ResultCode TransferVariable(const char* name, void* address, size_t size);

 private:
  
  base::win::ScopedProcessInformation sandbox_process_info_;
  
  
  base::win::ScopedHandle lockdown_token_;
  
  
  base::win::ScopedHandle initial_token_;
  
  base::win::ScopedHandle shared_section_;
  
  HANDLE job_;
  
  scoped_ptr<SharedMemIPCServer> ipc_server_;
  
  ThreadProvider* thread_pool_;
  
  void* base_address_;
  
  scoped_ptr_malloc<wchar_t> exe_name_;

  
  friend TargetProcess* MakeTestTargetProcess(HANDLE process,
                                              HMODULE base_address);

  DISALLOW_IMPLICIT_CONSTRUCTORS(TargetProcess);
};



TargetProcess* MakeTestTargetProcess(HANDLE process, HMODULE base_address);


}  

#endif  
