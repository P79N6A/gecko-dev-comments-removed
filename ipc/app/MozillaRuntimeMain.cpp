






































#include "nsXPCOM.h"
#include "nsXULAppAPI.h"



#if !defined(OS_WIN)
#include <unistd.h>
#endif

#ifdef XP_WIN
#include <windows.h>

#include "nsWindowsWMain.cpp"
#endif

class ScopedLogging
{
public:
    ScopedLogging() { NS_LogInit(); }
    ~ScopedLogging() { NS_LogTerm(); }
};

int
main(int argc, char* argv[])
{
    ScopedLogging log;

    GeckoChildProcessType proctype =
        XRE_StringToChildProcessType(argv[argc-1]);
    nsresult rv = XRE_InitChildProcess(--argc, argv, proctype);
    NS_ENSURE_SUCCESS(rv, 1);
    return 0;
}
