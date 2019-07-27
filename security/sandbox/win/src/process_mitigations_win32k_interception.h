



#ifndef SANDBOX_SRC_PROCESS_MITIGATIONS_WIN32K_INTERCEPTION_H_
#define SANDBOX_SRC_PROCESS_MITIGATIONS_WIN32K_INTERCEPTION_H_

#include <windows.h>
#include "base/basictypes.h"
#include "sandbox/win/src/sandbox_types.h"

namespace sandbox {

extern "C" {

typedef BOOL (WINAPI* GdiDllInitializeFunction) (
    HANDLE dll,
    DWORD reason,
    LPVOID reserved);

typedef HGDIOBJ (WINAPI *GetStockObjectFunction) (int object);

typedef ATOM (WINAPI *RegisterClassWFunction) (const WNDCLASS* wnd_class);


SANDBOX_INTERCEPT BOOL WINAPI TargetGdiDllInitialize(
    GdiDllInitializeFunction orig_gdi_dll_initialize,
    HANDLE dll,
    DWORD reason);


SANDBOX_INTERCEPT HGDIOBJ WINAPI TargetGetStockObject(
    GetStockObjectFunction orig_get_stock_object,
    int object);


SANDBOX_INTERCEPT ATOM WINAPI TargetRegisterClassW(
    RegisterClassWFunction orig_register_class_function,
    const WNDCLASS* wnd_class);

}  

}  

#endif  

