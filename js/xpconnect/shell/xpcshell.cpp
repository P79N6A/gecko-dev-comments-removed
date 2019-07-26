








#include <stdio.h>

#include "mozilla/Util.h"

#include "nsXULAppAPI.h"
#ifdef XP_MACOSX
#include "xpcshellMacUtils.h"
#endif
#ifdef XP_WIN
#include <windows.h>
#include <shlobj.h>


#define XRE_DONT_PROTECT_DLL_LOAD
#define XRE_WANT_ENVIRON
#include "nsWindowsWMain.cpp"
#endif

int
main(int argc, char** argv, char** envp)
{
#ifdef XP_MACOSX
    InitAutoreleasePool();
#endif

#ifdef HAVE_SETBUF
    
    
    setbuf(stdout, 0);
#endif

#ifdef XRE_HAS_DLL_BLOCKLIST
    XRE_SetupDllBlocklist();
#endif

    int result = XRE_XPCShellMain(argc, argv, envp);

#ifdef XP_MACOSX
    FinishAutoreleasePool();
#endif

    return result;
}
