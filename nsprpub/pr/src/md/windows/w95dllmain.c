











































#include <windows.h>
#include <primpl.h>

BOOL WINAPI DllMain(
    HINSTANCE hinstDLL,
    DWORD fdwReason,
    LPVOID lpvReserved)
{
PRThread *me;

    switch (fdwReason) {
        case DLL_PROCESS_ATTACH:
            break;
        case DLL_THREAD_ATTACH:
            break;
        case DLL_THREAD_DETACH:
            if (_pr_initialized) {
                me = _MD_GET_ATTACHED_THREAD();
                if ((me != NULL) && (me->flags & _PR_ATTACHED))
                    _PRI_DetachThread();
            }
            break;
        case DLL_PROCESS_DETACH:
            break;
    }
    return TRUE;
}
