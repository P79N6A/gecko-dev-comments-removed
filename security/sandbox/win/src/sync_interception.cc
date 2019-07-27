



#include "sandbox/win/src/sync_interception.h"

#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/policy_target.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/target_services.h"
#ifdef MOZ_CONTENT_SANDBOX 
#include "mozilla/warnonlysandbox/warnOnlySandbox.h"
#endif

namespace sandbox {

HANDLE WINAPI TargetCreateEventW(CreateEventWFunction orig_CreateEvent,
                                 LPSECURITY_ATTRIBUTES security_attributes,
                                 BOOL manual_reset, BOOL initial_state,
                                 LPCWSTR name) {
  
  HANDLE handle = orig_CreateEvent(security_attributes, manual_reset,
                                   initial_state, name);
  DWORD original_error = ::GetLastError();
  if (NULL != handle)
    return handle;

#ifdef MOZ_CONTENT_SANDBOX
  mozilla::warnonlysandbox::LogBlocked("CreateEventW", name);
#endif

  
  if (!SandboxFactory::GetTargetServices()->GetState()->InitCalled())
    return NULL;

  do {
    if (security_attributes)
      break;

    void* memory = GetGlobalIPCMemory();
    if (NULL == memory)
      break;

    CountedParameterSet<NameBased> params;
    params[NameBased::NAME] = ParamPickerMake(name);

    if (!QueryBroker(IPC_CREATEEVENT_TAG, params.GetBase()))
      break;

    SharedMemIPCClient ipc(memory);
    CrossCallReturn answer = {0};
    ResultCode code = CrossCall(ipc, IPC_CREATEEVENT_TAG, name, manual_reset,
                                initial_state, &answer);

    if (SBOX_ALL_OK != code)
      break;

    ::SetLastError(answer.win32_result);
#ifdef MOZ_CONTENT_SANDBOX
    mozilla::warnonlysandbox::LogAllowed("CreateEventW", name);
#endif
    return answer.handle;
  } while (false);

  ::SetLastError(original_error);
  return NULL;
}



HANDLE WINAPI TargetOpenEventW(OpenEventWFunction orig_OpenEvent,
                               ACCESS_MASK desired_access, BOOL inherit_handle,
                               LPCWSTR name) {
  
  HANDLE handle = orig_OpenEvent(desired_access, inherit_handle, name);
  DWORD original_error = ::GetLastError();
  if (NULL != handle)
    return handle;

#ifdef MOZ_CONTENT_SANDBOX
  mozilla::warnonlysandbox::LogBlocked("OpenEventW", name);
#endif

  
  if (!SandboxFactory::GetTargetServices()->GetState()->InitCalled())
    return NULL;

  do {
    void* memory = GetGlobalIPCMemory();
    if (NULL == memory)
      break;

    uint32 inherit_handle_ipc = inherit_handle;
    CountedParameterSet<OpenEventParams> params;
    params[OpenEventParams::NAME] = ParamPickerMake(name);
    params[OpenEventParams::ACCESS] = ParamPickerMake(desired_access);

    if (!QueryBroker(IPC_OPENEVENT_TAG, params.GetBase()))
      break;

    SharedMemIPCClient ipc(memory);
    CrossCallReturn answer = {0};
    ResultCode code = CrossCall(ipc, IPC_OPENEVENT_TAG, name, desired_access,
                                inherit_handle_ipc, &answer);

    if (SBOX_ALL_OK != code)
      break;

    ::SetLastError(answer.win32_result);
#ifdef MOZ_CONTENT_SANDBOX
    mozilla::warnonlysandbox::LogAllowed("OpenEventW", name);
#endif
    return answer.handle;
  } while (false);

  ::SetLastError(original_error);
  return NULL;
}

}  
