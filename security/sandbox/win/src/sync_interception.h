



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_SYNC_INTERCEPTION_H__
#define SANDBOX_SRC_SYNC_INTERCEPTION_H__

namespace sandbox {

extern "C" {

typedef HANDLE (WINAPI *CreateEventWFunction) (
    LPSECURITY_ATTRIBUTES lpEventAttributes,
    DWORD dwDesiredAccess,
    BOOL bInheritHandle,
    LPCWSTR lpName);

typedef HANDLE (WINAPI *OpenEventWFunction) (
    BOOL bManualReset,
    BOOL bInitialState,
    LPCWSTR lpName);


SANDBOX_INTERCEPT HANDLE WINAPI TargetCreateEventW(
    CreateEventWFunction orig_CreateEvent,
    LPSECURITY_ATTRIBUTES security_attributes, BOOL manual_reset,
    BOOL initial_state, LPCWSTR name);


SANDBOX_INTERCEPT HANDLE WINAPI TargetOpenEventW(
    OpenEventWFunction orig_OpenEvent, ACCESS_MASK desired_access,
    BOOL inherit_handle, LPCWSTR name);

}  

}  

#endif  
