



#include "sandbox/win/src/named_pipe_interception.h"

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

HANDLE WINAPI TargetCreateNamedPipeW(
    CreateNamedPipeWFunction orig_CreateNamedPipeW, LPCWSTR pipe_name,
    DWORD open_mode, DWORD pipe_mode, DWORD max_instance, DWORD out_buffer_size,
    DWORD in_buffer_size, DWORD default_timeout,
    LPSECURITY_ATTRIBUTES security_attributes) {
  HANDLE pipe = orig_CreateNamedPipeW(pipe_name, open_mode, pipe_mode,
                                      max_instance, out_buffer_size,
                                      in_buffer_size, default_timeout,
                                      security_attributes);
  if (INVALID_HANDLE_VALUE != pipe)
    return pipe;

#ifdef MOZ_CONTENT_SANDBOX
  mozilla::warnonlysandbox::LogBlocked("CreateNamedPipeW", pipe_name);
#endif
  DWORD original_error = ::GetLastError();

  
  if (!SandboxFactory::GetTargetServices()->GetState()->InitCalled())
    return INVALID_HANDLE_VALUE;

  
  if (security_attributes)
    return INVALID_HANDLE_VALUE;

  do {
    void* memory = GetGlobalIPCMemory();
    if (NULL == memory)
      break;

    CountedParameterSet<NameBased> params;
    params[NameBased::NAME] = ParamPickerMake(pipe_name);

    if (!QueryBroker(IPC_CREATENAMEDPIPEW_TAG, params.GetBase()))
      break;

    SharedMemIPCClient ipc(memory);
    CrossCallReturn answer = {0};
    ResultCode code = CrossCall(ipc, IPC_CREATENAMEDPIPEW_TAG, pipe_name,
                                open_mode, pipe_mode, max_instance,
                                out_buffer_size, in_buffer_size,
                                default_timeout, &answer);
    if (SBOX_ALL_OK != code)
      break;

    ::SetLastError(answer.win32_result);

    if (ERROR_SUCCESS != answer.win32_result)
      return INVALID_HANDLE_VALUE;

#ifdef MOZ_CONTENT_SANDBOX
    mozilla::warnonlysandbox::LogAllowed("CreateNamedPipeW", pipe_name);
#endif
    return answer.handle;
  } while (false);

  ::SetLastError(original_error);
  return INVALID_HANDLE_VALUE;
}

}  
