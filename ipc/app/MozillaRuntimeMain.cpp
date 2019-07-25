






































#include "nsXPCOM.h"
#include "nsXULAppAPI.h"


#if !defined(OS_WIN)
#include <unistd.h>
#endif

#ifdef XP_WIN
#include <windows.h>


#define XRE_DONT_PROTECT_DLL_LOAD
#include "nsWindowsWMain.cpp"

#include "nsSetDllDirectory.h"
#endif

int
main(int argc, char* argv[])
{
#if defined(XP_WIN) && defined(DEBUG_bent)
    MessageBox(NULL, L"Hi", L"Hi", MB_OK);
#endif

    
    
    if (argc < 1)
      return 1;
    GeckoProcessType proctype = XRE_StringToChildProcessType(argv[--argc]);

#ifdef XP_WIN
    
    
    
    if (proctype != GeckoProcessType_Plugin) {
        mozilla::SanitizeEnvironmentVariables();
        mozilla::NS_SetDllDirectory(L"");
    }
#endif

    nsresult rv = XRE_InitChildProcess(argc, argv, proctype);
    NS_ENSURE_SUCCESS(rv, 1);

    return 0;
}
