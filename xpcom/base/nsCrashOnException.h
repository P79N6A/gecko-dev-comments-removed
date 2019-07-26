





#ifndef nsCrashOnException_h
#define nsCrashOnException_h

#include <nscore.h>
#include <windows.h>

namespace mozilla {



XPCOM_API(LRESULT) CallWindowProcCrashProtected(WNDPROC aWndProc, HWND aHWnd,
                                                UINT aMsg, WPARAM aWParam,
                                                LPARAM aLParam);

}

#endif
