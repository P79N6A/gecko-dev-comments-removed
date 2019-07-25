






































#include "nsXPCOM.h"
#include "nsXULAppAPI.h"


#if !defined(OS_WIN)
#include <unistd.h>
#endif

#ifdef XP_WIN
#include <windows.h>

#include "nsWindowsWMain.cpp"
#endif

int
main(int argc, char* argv[])
{
#if defined(MOZ_CRASHREPORTER)
    if (argc < 2)
        return 1;
    const char* const crashReporterArg = argv[--argc];

#  if defined(XP_WIN) || defined(XP_MACOSX)
    
    
    
    if (0 != strcmp("-", crashReporterArg)
        && !XRE_SetRemoteExceptionHandler(crashReporterArg))
        return 1;
#  elif defined(OS_LINUX)
    
    
    if (0 != strcmp("false", crashReporterArg)
        && !XRE_SetRemoteExceptionHandler(NULL))
        return 1;
#  else
#    error "OOP crash reporting unsupported on this platform"
#  endif   
#endif 

#if defined(XP_WIN) && defined(DEBUG_bent)
    MessageBox(NULL, L"Hi", L"Hi", MB_OK);
#endif

    GeckoProcessType proctype =
        XRE_StringToChildProcessType(argv[argc - 1]);

    nsresult rv = XRE_InitChildProcess(argc - 1, argv, proctype);
    NS_ENSURE_SUCCESS(rv, 1);

    return 0;
}
