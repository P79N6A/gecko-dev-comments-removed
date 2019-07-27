



#include "sandbox/win/src/handle_interception.h"

#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/target_services.h"
#ifdef MOZ_CONTENT_SANDBOX 
#include "mozilla/warnonlysandbox/warnOnlySandbox.h"
#endif

namespace sandbox {

ResultCode DuplicateHandleProxy(HANDLE source_handle,
                                DWORD target_process_id,
                                HANDLE* target_handle,
                                DWORD desired_access,
                                DWORD options) {
  *target_handle = NULL;

  void* memory = GetGlobalIPCMemory();
  if (NULL == memory)
    return SBOX_ERROR_NO_SPACE;

  SharedMemIPCClient ipc(memory);
  CrossCallReturn answer = {0};
  ResultCode code = CrossCall(ipc, IPC_DUPLICATEHANDLEPROXY_TAG,
                              source_handle, target_process_id,
                              desired_access, options, &answer);
  if (SBOX_ALL_OK != code)
    return code;

  if (answer.win32_result) {
    ::SetLastError(answer.nt_status);
#ifdef MOZ_CONTENT_SANDBOX
    mozilla::warnonlysandbox::LogBlocked("DuplicateHandle");
#endif
    return SBOX_ERROR_GENERIC;
  }

  *target_handle = answer.handle;
#ifdef MOZ_CONTENT_SANDBOX
  mozilla::warnonlysandbox::LogAllowed("DuplicateHandle");
#endif
  return SBOX_ALL_OK;
}

}  

