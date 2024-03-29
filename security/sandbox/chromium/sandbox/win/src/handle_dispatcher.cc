



#include "sandbox/win/src/handle_dispatcher.h"

#include "base/win/scoped_handle.h"
#include "sandbox/win/src/handle_interception.h"
#include "sandbox/win/src/handle_policy.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_broker.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/sandbox.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sandbox_types.h"
#include "sandbox/win/src/sandbox_utils.h"

namespace sandbox {

HandleDispatcher::HandleDispatcher(PolicyBase* policy_base)
    : policy_base_(policy_base) {
  static const IPCCall duplicate_handle_proxy = {
    {IPC_DUPLICATEHANDLEPROXY_TAG, VOIDPTR_TYPE, UINT32_TYPE, UINT32_TYPE,
     UINT32_TYPE},
    reinterpret_cast<CallbackGeneric>(&HandleDispatcher::DuplicateHandleProxy)
  };

  ipc_calls_.push_back(duplicate_handle_proxy);
}

bool HandleDispatcher::SetupService(InterceptionManager* manager,
                                    int service) {
  
  switch (service) {
    case IPC_DUPLICATEHANDLEPROXY_TAG:
    return true;
  }

  return false;
}

bool HandleDispatcher::DuplicateHandleProxy(IPCInfo* ipc,
                                            HANDLE source_handle,
                                            uint32 target_process_id,
                                            uint32 desired_access,
                                            uint32 options) {
  static NtQueryObject QueryObject = NULL;
  if (!QueryObject)
    ResolveNTFunctionPtr("NtQueryObject", &QueryObject);

  
  HANDLE handle_temp;
  if (!::DuplicateHandle(ipc->client_info->process, source_handle,
                         ::GetCurrentProcess(), &handle_temp,
                         0, FALSE, DUPLICATE_SAME_ACCESS | options)) {
    ipc->return_info.win32_result = ::GetLastError();
    return false;
  }
  options &= ~DUPLICATE_CLOSE_SOURCE;
  base::win::ScopedHandle handle(handle_temp);

  
  BYTE buffer[sizeof(OBJECT_TYPE_INFORMATION) + 32 * sizeof(wchar_t)];
  OBJECT_TYPE_INFORMATION* type_info =
      reinterpret_cast<OBJECT_TYPE_INFORMATION*>(buffer);
  ULONG size = sizeof(buffer) - sizeof(wchar_t);
  NTSTATUS error =
      QueryObject(handle.Get(), ObjectTypeInformation, type_info, size, &size);
  if (!NT_SUCCESS(error)) {
    ipc->return_info.nt_status = error;
    return false;
  }
  type_info->Name.Buffer[type_info->Name.Length / sizeof(wchar_t)] = L'\0';

  CountedParameterSet<HandleTarget> params;
  params[HandleTarget::NAME] = ParamPickerMake(type_info->Name.Buffer);
  params[HandleTarget::TARGET] = ParamPickerMake(target_process_id);

  EvalResult eval = policy_base_->EvalPolicy(IPC_DUPLICATEHANDLEPROXY_TAG,
                                             params.GetBase());
  ipc->return_info.win32_result =
      HandlePolicy::DuplicateHandleProxyAction(eval, handle.Get(),
                                               target_process_id,
                                               &ipc->return_info.handle,
                                               desired_access, options);
  return true;
}

}  

