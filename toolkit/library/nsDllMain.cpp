



 
#include <windows.h>
#include <delayimp.h>
#include "nsToolkit.h"
#include "mozilla/Assertions.h"
#include "mozilla/WindowsVersion.h"

using mozilla::IsWin8OrLater;

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

























const char* kvccorlib = "vccorlib";
const char* kwinrtprelim = "api-ms-win-core-winrt";
const char* kfailfast = "?__abi_FailFast";

static bool IsWinRTDLLNotPresent(PDelayLoadInfo pdli, const char* aLibToken)
{
  return (!IsWin8OrLater() && pdli->szDll &&
          !strnicmp(pdli->szDll, aLibToken, strlen(aLibToken)));
}

static bool IsWinRTDLLPresent(PDelayLoadInfo pdli, const char* aLibToken)
{
  return (IsWin8OrLater() && pdli->szDll &&
          !strnicmp(pdli->szDll, aLibToken, strlen(aLibToken)));
}

void __stdcall __abi_MozFailFast()
{
  MOZ_CRASH();
}

FARPROC WINAPI DelayDllLoadHook(unsigned dliNotify, PDelayLoadInfo pdli)
{
  if (dliNotify == dliNotePreLoadLibrary) {
    if (IsWinRTDLLNotPresent(pdli, kvccorlib)) {
      return (FARPROC)LoadLibraryA("dummyvccorlib.dll");
    }
    NS_ASSERTION(!IsWinRTDLLNotPresent(pdli, kwinrtprelim),
      "Attempting to load winrt libs in non-metro environment. "
      "(Winrt variable type placed in global scope?)");
  }
  if (dliNotify == dliFailGetProc && IsWinRTDLLNotPresent(pdli, kvccorlib)) {
    NS_WARNING("Attempting to access winrt vccorlib entry point in non-metro environment.");
    NS_WARNING(pdli->szDll);
    NS_WARNING(pdli->dlp.szProcName);
    NS_ABORT();
  }
  if (dliNotify == dliNotePreGetProcAddress &&
      IsWinRTDLLPresent(pdli, kvccorlib) &&
      pdli->dlp.szProcName &&
      !strnicmp(pdli->dlp.szProcName, kfailfast, strlen(kfailfast))) {
    return (FARPROC)__abi_MozFailFast;
  }
  return nullptr;
}

ExternC PfnDliHook __pfnDliNotifyHook2 = DelayDllLoadHook;
ExternC PfnDliHook __pfnDliFailureHook2 = DelayDllLoadHook;

#endif 

#if defined(__GNUC__)
} 
#endif
