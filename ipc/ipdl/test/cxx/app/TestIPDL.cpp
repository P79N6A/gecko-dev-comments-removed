



































#include "nsXULAppAPI.h"

#if defined(XP_WIN)
#include <windows.h>
#include "nsWindowsWMain.cpp"
#endif

int
main(int argc, char** argv)
{
    
    if (argc < 2)
        return 1;

    return XRE_RunIPDLTest(argc, argv);
}
