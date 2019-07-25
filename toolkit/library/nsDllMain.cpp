





































 
#include <windows.h>
#include "nsToolkit.h"

#ifdef MOZ_ENABLE_LIBXUL
#include "../xre/nsWindowsDllBlocklist.cpp"
#endif

#if defined(__GNUC__)

extern "C" {
#endif

BOOL APIENTRY DllMain(  
#ifdef WINCE
                      HANDLE hModule, 
#else 
                      HINSTANCE hModule, 
#endif
                      DWORD reason, 
                      LPVOID lpReserved )
{
    switch( reason ) {
        case DLL_PROCESS_ATTACH:
#ifdef MOZ_ENABLE_LIBXUL
            SetupDllBlocklist();
#endif
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

#if defined(__GNUC__)
} 
#endif
