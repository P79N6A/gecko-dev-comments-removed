









































#include <windows.h>
#include <stdlib.h>

int WINAPI
wWinMain(HINSTANCE  hInstance, HINSTANCE  hPrevInstance,
         LPWSTR  lpszCmdLine, int  nCmdShow)
{
    







    DWORD regType;
    DWORD regValue = -1;
    DWORD regLength = sizeof regValue;
    HKEY hkeyCU, hkeyLM;
    RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\mozilla.org\\windbgdlg", 0, KEY_READ, &hkeyCU);
    RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software\\mozilla.org\\windbgdlg", 0, KEY_READ, &hkeyLM);
    int argc =0;
    for (int i = __argc - 1; regValue == (DWORD)-1 && i; --i) {
        bool ok = false;
        if (hkeyCU)
            ok = RegQueryValueExW(hkeyCU, __wargv[i], 0, &regType, (LPBYTE)&regValue, &regLength) == ERROR_SUCCESS;
        if (!ok && hkeyLM)
            ok = RegQueryValueExW(hkeyLM, __wargv[i], 0, &regType, (LPBYTE)&regValue, &regLength) == ERROR_SUCCESS;
        if (!ok)
            regValue = -1;
    }
    if (hkeyCU)
        RegCloseKey(hkeyCU);
    if (hkeyLM)
        RegCloseKey(hkeyLM);
    if (regValue != (DWORD)-1 && regValue != (DWORD)-2)
        return regValue;
    static WCHAR msg[4048];

    wsprintfW(msg,
              L"%s\n\nClick Abort to exit the Application.\n"
              L"Click Retry to Debug the Application.\n"
              L"Click Ignore to continue running the Application.",
              lpszCmdLine);

    return MessageBoxW(NULL, msg, L"NSGlue_Assertion",
                       MB_ICONSTOP | MB_SYSTEMMODAL |
                       MB_ABORTRETRYIGNORE | MB_DEFBUTTON3);
}
