



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_SYNC_INTERCEPTION_H__
#define SANDBOX_SRC_SYNC_INTERCEPTION_H__

namespace sandbox {

extern "C" {

typedef NTSTATUS (WINAPI* NtCreateEventFunction) (
    PHANDLE EventHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes,
    EVENT_TYPE EventType,
    BOOLEAN InitialState);

typedef NTSTATUS (WINAPI *NtOpenEventFunction) (
    PHANDLE EventHandle,
    ACCESS_MASK DesiredAccess,
    POBJECT_ATTRIBUTES ObjectAttributes);


SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtCreateEvent(
    NtCreateEventFunction orig_CreateEvent,
    PHANDLE event_handle,
    ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes,
    EVENT_TYPE event_type,
    BOOLEAN initial_state);

SANDBOX_INTERCEPT NTSTATUS WINAPI TargetNtOpenEvent(
    NtOpenEventFunction orig_OpenEvent,
    PHANDLE event_handle,
    ACCESS_MASK desired_access,
    POBJECT_ATTRIBUTES object_attributes);

}  

}  

#endif  
