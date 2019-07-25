





































#ifndef nsCrashOnException_h
#define nsCrashOnException_h

#ifndef _MSC_VER
#error This file only makes sense on Windows with Visual C++.
#endif

#include <nscore.h>
#include <windows.h>

namespace mozilla {



XPCOM_API(LRESULT) CallWindowProcCrashProtected(WNDPROC wndProc, HWND hWnd, UINT msg,
                                               WPARAM wParam, LPARAM lParam);

}

#endif
