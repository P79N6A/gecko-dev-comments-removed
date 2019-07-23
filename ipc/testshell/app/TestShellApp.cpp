



































#include "nsXULAppAPI.h"

#if defined(XP_WIN)
#include <windows.h>
#include "nsWindowsWMain.cpp"
#endif

int
main(int argc, char* argv[])
{
    return XRE_RunTestShell(argc, argv);
}
