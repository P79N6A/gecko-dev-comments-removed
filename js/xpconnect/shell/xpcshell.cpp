







#include <stdio.h>

#include "mozilla/WindowsDllBlocklist.h"

#include "nsXULAppAPI.h"
#ifdef XP_MACOSX
#include "xpcshellMacUtils.h"
#endif
#ifdef XP_WIN
#include <windows.h>
#include <shlobj.h>


#define XRE_DONT_PROTECT_DLL_LOAD
#define XRE_DONT_SUPPORT_XPSP2
#define XRE_WANT_ENVIRON
#include "nsWindowsWMain.cpp"
#endif

int
main(int argc, char** argv, char** envp)
{
#ifdef XP_MACOSX
    InitAutoreleasePool();
#endif

    
    
    setbuf(stdout, 0);

#ifdef HAS_DLL_BLOCKLIST
    DllBlocklist_Initialize();
#endif

    int result = XRE_XPCShellMain(argc, argv, envp);

#ifdef XP_MACOSX
    FinishAutoreleasePool();
#endif

    return result;
}
