



 
#include <windows.h>
#include <delayimp.h>
#include "nsToolkit.h"

#if defined(__GNUC__)

extern "C" {
#endif

BOOL APIENTRY DllMain(  
                      HINSTANCE hModule, 
                      DWORD reason, 
                      LPVOID lpReserved )
{
    switch( reason ) {
        case DLL_PROCESS_ATTACH:
            nsToolkit::Startup((HINSTANCE)hModule);
            break;

        case DLL_THREAD_ATTACH:
            break;
    
        case DLL_THREAD_DETACH:
            break;
    
        case DLL_PROCESS_DETACH:
            nsToolkit::Shutdown();
            break;

    }

    return TRUE;
}

#if defined(MOZ_METRO)














static bool IsWin8OrHigher()
{
  static PRInt32 version = 0;

  if (version) {
    return (version >= 0x602);
  }

  
  OSVERSIONINFOEX osInfo;
  osInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEX);
  ::GetVersionEx((OSVERSIONINFO*)&osInfo);
  version =
    (osInfo.dwMajorVersion & 0xff) << 8 | (osInfo.dwMinorVersion & 0xff);
  return (version >= 0x602);
}

const char* kvccorlib = "vccorlib";
const char* kwinrtprelim = "api-ms-win-core-winrt";

static bool IsWinRTDLLPresent(PDelayLoadInfo pdli, const char* aLibToken)
{
  return (!IsWin8OrHigher() && pdli->szDll &&
          !strnicmp(pdli->szDll, aLibToken, strlen(aLibToken)));
}

FARPROC WINAPI DelayDllLoadHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
  if (dliNotify == dliNotePreLoadLibrary) {
    if (IsWinRTDLLPresent(pdli, kvccorlib)) {
      return (FARPROC)LoadLibraryA("dummyvccorlib.dll");
    }
    NS_ASSERTION(!IsWinRTDLLPresent(pdli, kwinrtprelim),
      "Attempting to load winrt libs in non-metro environment. "
      "(Winrt variable type placed in global scope?)");
  }
  if (dliNotify == dliFailGetProc && IsWinRTDLLPresent(pdli, kvccorlib)) {
    NS_WARNING("Attempting to access winrt vccorlib entry point in non-metro environment.");
    NS_WARNING(pdli->szDll);
    NS_WARNING(pdli->dlp.szProcName);
    NS_ABORT();
  }
  return NULL;
}

ExternC PfnDliHook __pfnDliNotifyHook2 = DelayDllLoadHook;
ExternC PfnDliHook __pfnDliFailureHook2 = DelayDllLoadHook;

#endif 

#if defined(__GNUC__)
} 
#endif
