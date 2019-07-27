



#include "sandbox/win/src/nt_internals.h"
#include "sandbox/win/src/sandbox_types.h"

#ifndef SANDBOX_SRC_NAMED_PIPE_INTERCEPTION_H__
#define SANDBOX_SRC_NAMED_PIPE_INTERCEPTION_H__

namespace sandbox {

extern "C" {

typedef HANDLE (WINAPI *CreateNamedPipeWFunction) (
    LPCWSTR lpName,
    DWORD dwOpenMode,
    DWORD dwPipeMode,
    DWORD nMaxInstances,
    DWORD nOutBufferSize,
    DWORD nInBufferSize,
    DWORD nDefaultTimeOut,
    LPSECURITY_ATTRIBUTES lpSecurityAttributes);


SANDBOX_INTERCEPT HANDLE WINAPI TargetCreateNamedPipeW(
    CreateNamedPipeWFunction orig_CreateNamedPipeW, LPCWSTR pipe_name,
    DWORD open_mode, DWORD pipe_mode, DWORD max_instance, DWORD out_buffer_size,
    DWORD in_buffer_size, DWORD default_timeout,
    LPSECURITY_ATTRIBUTES security_attributes);

}  

}  

#endif  
