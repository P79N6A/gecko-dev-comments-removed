



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_REGISTRY_INTERCEPTION_H__
#define SANDBOX_SRC_REGISTRY_INTERCEPTION_H__

namespace sandbox {

extern "C" {



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtCreateKey(
    NtCreateKeyFunction orig_CreateKey, PHANDLE key, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, ULONG title_index,
    PUNICODE_STRING class_name, ULONG create_options, PULONG disposition);



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenKey(
    NtOpenKeyFunction orig_OpenKey, PHANDLE key, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes);



SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenKeyEx(
    NtOpenKeyExFunction orig_OpenKeyEx, PHANDLE key, ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes, ULONG open_options);

}  

}  

#endif  
