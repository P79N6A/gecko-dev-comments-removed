




#undef WINVER
#undef _WIN32_WINNT
#define WINVER 0x602
#define _WIN32_WINNT 0x602

#include <windows.h>
#include <atlbase.h>
#include <shlobj.h>


extern HANDLE sCon;

void Log(const wchar_t *fmt, ...);

#if defined(SHOW_CONSOLE)
void SetupConsole();
#endif

AHE_TYPE GetLastAHE();
bool SetLastAHE(AHE_TYPE ahe);
bool IsDX10Available();
bool GetDWORDRegKey(LPCWSTR name, DWORD &value);
bool SetDWORDRegKey(LPCWSTR name, DWORD value);
bool IsImmersiveProcessDynamic(HANDLE process);
bool IsMetroProcessRunning();
bool IsDesktopProcessRunning();
