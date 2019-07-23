






































#include "nsXPCOM.h"
#include "nsXULAppAPI.h"

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
    nsresult rv = XRE_InitChildProcess(argc, argv, "TabThread");
    NS_ENSURE_SUCCESS(rv, 1);

    return 0;
}
