




#include <windows.h>




extern "C" {
__declspec(dllexport) long __stdcall __InitializeWinRTRuntime(unsigned long data) { return S_OK; }
}

namespace Platform {
namespace Details {
__declspec(dllexport) HRESULT InitializeData(int __threading_model) { return S_OK; }
__declspec(dllexport) void UninitializeData(int __threading_model) { }
}
}
