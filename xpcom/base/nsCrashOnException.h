





































#ifndef nsCrashOnException_h
#define nsCrashOnException_h

#include <nscore.h>
#include <windows.h>

namespace mozilla {



XPCOM_API(LRESULT) CallWindowProcCrashProtected(WNDPROC wndProc, HWND hWnd, UINT msg,
                                               WPARAM wParam, LPARAM lParam);

}

#endif
