



#include "sandbox/win/src/process_mitigations_win32k_interception.h"

namespace sandbox {

BOOL WINAPI TargetGdiDllInitialize(
    GdiDllInitializeFunction orig_gdi_dll_initialize,
    HANDLE dll,
    DWORD reason) {
  return TRUE;
}

HGDIOBJ WINAPI TargetGetStockObject(
    GetStockObjectFunction orig_get_stock_object,
    int object) {
  return reinterpret_cast<HGDIOBJ>(NULL);
}

ATOM WINAPI TargetRegisterClassW(
    RegisterClassWFunction orig_register_class_function,
    const WNDCLASS* wnd_class) {
  return TRUE;
}

}  

