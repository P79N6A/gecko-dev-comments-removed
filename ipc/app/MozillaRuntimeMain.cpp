






































#include "nsXPCOM.h"
#include "nsXULAppAPI.h"



#if !defined(OS_WIN)
#include <unistd.h>
#endif

#ifdef XP_WIN
#include <windows.h>

#include "nsWindowsWMain.cpp"
#endif

#if defined(MOZ_WIDGET_GTK2)
#include <gtk/gtk.h>
#endif

int
main(int argc, char* argv[])
{
#if 0
    MessageBox(NULL, L"Hi", L"Hi", MB_OK);
#endif

    GeckoProcessType proctype =
        XRE_StringToChildProcessType(argv[argc - 1]);

    nsresult rv = XRE_InitChildProcess(argc - 1, argv, proctype);
    NS_ENSURE_SUCCESS(rv, 1);

    return 0;
}
