









































#include <windows.h>
#include <stdlib.h>

int WINAPI 
WinMain(HINSTANCE  hInstance, HINSTANCE  hPrevInstance, 
        LPSTR  lpszCmdLine, int  nCmdShow)
{
    







    DWORD regType;
    DWORD regValue = -1;
    DWORD regLength = sizeof regValue;
    HKEY hkeyCU, hkeyLM;
    RegOpenKeyEx(HKEY_CURRENT_USER, "Software\\mozilla.org\\windbgdlg", 0, KEY_READ, &hkeyCU);
    RegOpenKeyEx(HKEY_LOCAL_MACHINE, "Software\\mozilla.org\\windbgdlg", 0, KEY_READ, &hkeyLM);
    const char * const * argv = __argv;
    for (int i = __argc - 1; regValue == (DWORD)-1 && i; --i) {
        bool ok = false;
        if (hkeyCU)
            ok = RegQueryValueEx(hkeyCU, argv[i], 0, &regType, (LPBYTE)&regValue, &regLength) == ERROR_SUCCESS;
        if (!ok && hkeyLM)
            ok = RegQueryValueEx(hkeyLM, argv[i], 0, &regType, (LPBYTE)&regValue, &regLength) == ERROR_SUCCESS;
        if (!ok)
            regValue = -1;
    }
    if (hkeyCU)
        RegCloseKey(hkeyCU);
    if (hkeyLM)
        RegCloseKey(hkeyLM);
    if (regValue != (DWORD)-1 && regValue != (DWORD)-2)
        return regValue;
    static char msg[4048];

    wsprintf(msg,
             "%s\n\nClick Abort to exit the Application.\n"
             "Click Retry to Debug the Application..\n"
             "Click Ignore to continue running the Application.", 
             lpszCmdLine);
             
    return MessageBox(NULL, msg, "NSGlue_Assertion",
                      MB_ICONSTOP | MB_SYSTEMMODAL| 
                      MB_ABORTRETRYIGNORE | MB_DEFBUTTON3);
}
