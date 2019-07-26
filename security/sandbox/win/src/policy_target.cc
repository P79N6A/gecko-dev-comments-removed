



#include "sandbox/win/src/policy_target.h"

#include "sandbox/win/src/crosscall_client.h"
#include "sandbox/win/src/ipc_tags.h"
#include "sandbox/win/src/policy_engine_processor.h"
#include "sandbox/win/src/policy_low_level.h"
#include "sandbox/win/src/policy_params.h"
#include "sandbox/win/src/sandbox_factory.h"
#include "sandbox/win/src/sandbox_nt_util.h"
#include "sandbox/win/src/sharedmem_ipc_client.h"
#include "sandbox/win/src/target_services.h"

namespace sandbox {


extern void* g_heap;


SANDBOX_INTERCEPT NtExports g_nt;


extern void* volatile     g_shared_policy_memory;
SANDBOX_INTERCEPT size_t  g_shared_policy_size;

bool QueryBroker(int ipc_id, CountedParameterSetBase* params) {
  DCHECK_NT(static_cast<size_t>(ipc_id) < kMaxServiceCount);
  DCHECK_NT(g_shared_policy_memory);
  DCHECK_NT(g_shared_policy_size > 0);

  if (static_cast<size_t>(ipc_id) >= kMaxServiceCount)
    return false;

  PolicyGlobal* global_policy =
      reinterpret_cast<PolicyGlobal*>(g_shared_policy_memory);

  if (!global_policy->entry[ipc_id])
    return false;

  PolicyBuffer* policy = reinterpret_cast<PolicyBuffer*>(
      reinterpret_cast<char*>(g_shared_policy_memory) +
      reinterpret_cast<size_t>(global_policy->entry[ipc_id]));

  if ((reinterpret_cast<size_t>(global_policy->entry[ipc_id]) >
       global_policy->data_size) ||
      (g_shared_policy_size < global_policy->data_size)) {
    NOTREACHED_NT();
    return false;
  }

  for (int i = 0; i < params->count; i++) {
    if (!params->parameters[i].IsValid()) {
      NOTREACHED_NT();
      return false;
    }
  }

  PolicyProcessor processor(policy);
  PolicyResult result = processor.Evaluate(kShortEval, params->parameters,
                                           params->count);
  DCHECK_NT(POLICY_ERROR != result);

  return POLICY_MATCH == result && ASK_BROKER == processor.GetAction();
}





NTSTATUS WINAPI TargetNtSetInformationThread(
    NtSetInformationThreadFunction orig_SetInformationThread, HANDLE thread,
    NT_THREAD_INFORMATION_CLASS thread_info_class, PVOID thread_information,
    ULONG thread_information_bytes) {
  do {
    if (SandboxFactory::GetTargetServices()->GetState()->RevertedToSelf())
      break;
    if (ThreadImpersonationToken != thread_info_class)
      break;
    if (!thread_information)
      break;
    HANDLE token;
    if (sizeof(token) > thread_information_bytes)
      break;

    NTSTATUS ret = CopyData(&token, thread_information, sizeof(token));
    if (!NT_SUCCESS(ret) || NULL != token)
      break;

    
    return STATUS_SUCCESS;
  } while (false);

  return orig_SetInformationThread(thread, thread_info_class,
                                   thread_information,
                                   thread_information_bytes);
}






NTSTATUS WINAPI TargetNtOpenThreadToken(
    NtOpenThreadTokenFunction orig_OpenThreadToken, HANDLE thread,
    ACCESS_MASK desired_access, BOOLEAN open_as_self, PHANDLE token) {
  if (!SandboxFactory::GetTargetServices()->GetState()->RevertedToSelf())
    open_as_self = FALSE;

  return orig_OpenThreadToken(thread, desired_access, open_as_self, token);
}


NTSTATUS WINAPI TargetNtOpenThreadTokenEx(
    NtOpenThreadTokenExFunction orig_OpenThreadTokenEx, HANDLE thread,
    ACCESS_MASK desired_access, BOOLEAN open_as_self, ULONG handle_attributes,
    PHANDLE token) {
  if (!SandboxFactory::GetTargetServices()->GetState()->RevertedToSelf())
    open_as_self = FALSE;

  return orig_OpenThreadTokenEx(thread, desired_access, open_as_self,
                                handle_attributes, token);
}

}  
