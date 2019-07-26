




#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x602
#define _WIN32_WINNT 0x602

#include <windows.h>
#include <d3d10_1.h>
#include <dxgi.h>
#include <d3d10misc.h>
#include <atlbase.h>


extern HANDLE sCon;
extern LPCWSTR metroDX10Available;

void Log(const wchar_t *fmt, ...);

#if defined(SHOW_CONSOLE)
void SetupConsole();
#endif

bool IsDX10Available();
bool GetDWORDRegKey(LPCWSTR name, DWORD &value);
bool SetDWORDRegKey(LPCWSTR name, DWORD value);
