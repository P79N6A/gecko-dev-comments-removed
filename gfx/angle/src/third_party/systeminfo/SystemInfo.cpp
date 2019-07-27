
























#include <windows.h>
#include "common/platform.h"

#if _WIN32_WINNT_WINBLUE
#include <versionhelpers.h>
#endif

namespace rx {

#ifndef _WIN32_WINNT_WINBLUE
static bool IsWindowsVistaOrGreater()
{
    OSVERSIONINFOEXW osvi = { };
    osvi.dwOSVersionInfoSize = sizeof(osvi);
    osvi.dwMajorVersion = HIBYTE(_WIN32_WINNT_VISTA);
    osvi.dwMinorVersion = LOBYTE(_WIN32_WINNT_VISTA);
    DWORDLONG condition = 0;
    VER_SET_CONDITION(condition, VER_MAJORVERSION, VER_GREATER_EQUAL);
    VER_SET_CONDITION(condition, VER_MINORVERSION, VER_GREATER_EQUAL);
    return !!::VerifyVersionInfoW(&osvi, VER_MAJORVERSION | VER_MINORVERSION, condition);
}
#endif

bool isWindowsVistaOrGreater()
{
    static bool initialized = false;
    static bool cachedIsWindowsVistaOrGreater;

    if (!initialized) {
        initialized = true;
#if defined(ANGLE_ENABLE_WINDOWS_STORE)
        cachedIsWindowsVistaOrGreater = true;
#else
        cachedIsWindowsVistaOrGreater = IsWindowsVistaOrGreater();
#endif
    }
    return cachedIsWindowsVistaOrGreater;
}

} 
