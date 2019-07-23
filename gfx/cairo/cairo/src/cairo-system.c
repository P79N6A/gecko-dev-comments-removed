














































#include "cairoint.h"



#if CAIRO_MUTEX_IMPL_WIN32
#if !CAIRO_WIN32_STATIC_BUILD

#define WIN32_LEAN_AND_MEAN

#if !defined(WINVER) || (WINVER < 0x0500)
# define WINVER 0x0500
#endif
#if !defined(_WIN32_WINNT) || (_WIN32_WINNT < 0x0500)
# define _WIN32_WINNT 0x0500
#endif

#include "cairo-clip-private.h"
#include "cairo-paginated-private.h"
#include "cairo-win32-private.h"
#include "cairo-scaled-font-subsets-private.h"

#include <windows.h>


BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved);

BOOL WINAPI
DllMain (HINSTANCE hinstDLL,
         DWORD     fdwReason,
         LPVOID    lpvReserved)
{
    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            CAIRO_MUTEX_INITIALIZE ();
            break;

        case DLL_PROCESS_DETACH:
            CAIRO_MUTEX_FINALIZE ();
            break;
    }

    return TRUE;
}

#endif
#endif

