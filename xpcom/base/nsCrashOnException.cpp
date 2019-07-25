





































#ifndef _MSC_VER
#error This file only makes sense on Windows.
#endif

#include "nsCrashOnException.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"

#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#endif

namespace mozilla {

static int ReportException(EXCEPTION_POINTERS *aExceptionInfo)
{
#ifdef MOZ_CRASHREPORTER
  nsCOMPtr<nsICrashReporter> cr =
    do_GetService("@mozilla.org/toolkit/crash-reporter;1");
  if (cr)
    cr->WriteMinidumpForException(aExceptionInfo);
#endif
  return EXCEPTION_EXECUTE_HANDLER;
}

XPCOM_API(LRESULT)
CallWindowProcCrashProtected(WNDPROC wndProc, HWND hWnd, UINT msg,
                            WPARAM wParam, LPARAM lParam)
{
  __try {
    return wndProc(hWnd, msg, wParam, lParam);
  }
  __except(ReportException(GetExceptionInformation())) {
    ::TerminateProcess(::GetCurrentProcess(), 253);
  }
  return 0; 
}

}

