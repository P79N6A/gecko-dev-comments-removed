



#include "sandbox/win/src/target_services.h"

#include <process.h>

#include "base/basictypes.h"
#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/handle_closer_agent.h"
#include "sandbox/win/src/handle_interception.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/process_mitigations.h"
#include "sandbox/win/src/restricted_token_utils.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/sandbox_nt_util.h"

namespace {




bool FlushRegKey(HKEY root) {
  HKEY key;
  if (ERROR_SUCCESS == ::RegOpenKeyExW(root, NULL, 0, MAXIMUM_ALLOWED, &key)) {
    if (ERROR_SUCCESS != ::RegCloseKey(key))
      return false;
  }
  return true;
}






bool FlushCachedRegHandles() {
  return (FlushRegKey(HKEY_LOCAL_MACHINE) &&
          FlushRegKey(HKEY_CLASSES_ROOT) &&
          FlushRegKey(HKEY_USERS));
}


bool CloseOpenHandles() {
  if (sandbox::HandleCloserAgent::NeedsHandlesClosed()) {
    sandbox::HandleCloserAgent handle_closer;

    handle_closer.InitializeHandlesToClose();
    if (!handle_closer.CloseHandles())
      return false;
  }

  return true;
}

}  

namespace sandbox {

SANDBOX_INTERCEPT IntegrityLevel g_shared_delayed_integrity_level =
    INTEGRITY_LEVEL_LAST;
SANDBOX_INTERCEPT MitigationFlags g_shared_delayed_mitigations = 0;

TargetServicesBase::TargetServicesBase() {
}

ResultCode TargetServicesBase::Init() {
  process_state_.SetInitCalled();
  return SBOX_ALL_OK;
}


void TargetServicesBase::LowerToken() {
  if (ERROR_SUCCESS !=
      SetProcessIntegrityLevel(g_shared_delayed_integrity_level))
    ::TerminateProcess(::GetCurrentProcess(), SBOX_FATAL_INTEGRITY);
  process_state_.SetRevertedToSelf();
  
  
  if (!::RevertToSelf())
    ::TerminateProcess(::GetCurrentProcess(), SBOX_FATAL_DROPTOKEN);
  if (!FlushCachedRegHandles())
    ::TerminateProcess(::GetCurrentProcess(), SBOX_FATAL_FLUSHANDLES);
  if (ERROR_SUCCESS != ::RegDisablePredefinedCache())
    ::TerminateProcess(::GetCurrentProcess(), SBOX_FATAL_CACHEDISABLE);
  if (!CloseOpenHandles())
    ::TerminateProcess(::GetCurrentProcess(), SBOX_FATAL_CLOSEHANDLES);
  
  if (g_shared_delayed_mitigations &&
      !ApplyProcessMitigationsToCurrentProcess(g_shared_delayed_mitigations))
    ::TerminateProcess(::GetCurrentProcess(), SBOX_FATAL_MITIGATION);
}

ProcessState* TargetServicesBase::GetState() {
  return &process_state_;
}

TargetServicesBase* TargetServicesBase::GetInstance() {
  static TargetServicesBase instance;
  return &instance;
}


bool TargetServicesBase::TestIPCPing(int version) {
  void* memory = GetGlobalIPCMemory();
  if (NULL == memory) {
    return false;
  }
  SharedMemIPCClient ipc(memory);
  CrossCallReturn answer = {0};

  if (1 == version) {
    uint32 tick1 = ::GetTickCount();
    uint32 cookie = 717115;
    ResultCode code = CrossCall(ipc, IPC_PING1_TAG, cookie, &answer);

    if (SBOX_ALL_OK != code) {
      return false;
    }
    
    
    if ((answer.extended_count != 2)) {
      return false;
    }
    
    
    uint32 tick2 = ::GetTickCount();
    if (tick2 >= tick1) {
      if ((answer.extended[0].unsigned_int < tick1) ||
          (answer.extended[0].unsigned_int > tick2)) {
        return false;
      }
    }

    if (answer.extended[1].unsigned_int != cookie * 2) {
      return false;
    }
  } else if (2 == version) {
    uint32 cookie = 717111;
    InOutCountedBuffer counted_buffer(&cookie, sizeof(cookie));
    ResultCode code = CrossCall(ipc, IPC_PING2_TAG, counted_buffer, &answer);

    if (SBOX_ALL_OK != code) {
      return false;
    }
    if (cookie != 717111 * 3) {
      return false;
    }
  } else {
    return false;
  }
  return true;
}

bool ProcessState::IsKernel32Loaded() {
  return process_state_ != 0;
}

bool ProcessState::InitCalled() {
  return process_state_ > 1;
}

bool ProcessState::RevertedToSelf() {
  return process_state_ > 2;
}

void ProcessState::SetKernel32Loaded() {
  if (!process_state_)
    process_state_ = 1;
}

void ProcessState::SetInitCalled() {
  if (process_state_ < 2)
    process_state_ = 2;
}

void ProcessState::SetRevertedToSelf() {
  if (process_state_ < 3)
    process_state_ = 3;
}

ResultCode TargetServicesBase::DuplicateHandle(HANDLE source_handle,
                                               DWORD target_process_id,
                                               HANDLE* target_handle,
                                               DWORD desired_access,
                                               DWORD options) {
  return sandbox::DuplicateHandleProxy(source_handle, target_process_id,
                                       target_handle, desired_access, options);
}

}  
