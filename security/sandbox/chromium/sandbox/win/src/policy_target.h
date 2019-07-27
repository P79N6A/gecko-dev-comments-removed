



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_POLICY_TARGET_H__
#define SANDBOX_SRC_POLICY_TARGET_H__

namespace sandbox {

struct CountedParameterSetBase;



bool QueryBroker(int ipc_id, CountedParameterSetBase* params);

extern "C" {



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtSetInformationThread(
    NtSetInformationThreadFunction orig_SetInformationThread, HANDLE thread,
    NT_THREAD_INFORMATION_CLASS thread_info_class, PVOID thread_information,
    ULONG thread_information_bytes);



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenThreadToken(
    NtOpenThreadTokenFunction orig_OpenThreadToken, HANDLE thread,
    ACCESS_MASK desired_access, BOOLEAN open_as_self, PHANDLE token);



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenThreadTokenEx(
    NtOpenThreadTokenExFunction orig_OpenThreadTokenEx, HANDLE thread,
    ACCESS_MASK desired_access, BOOLEAN open_as_self, ULONG handle_attributes,
    PHANDLE token);

}  

}  

#endif  
