





#include "nsCrashOnException.h"
#include "nsCOMPtr.h"
#include "nsServiceManagerUtils.h"

#ifdef MOZ_CRASHREPORTER
#include "nsICrashReporter.h"
#endif

namespace mozilla {

static int
ReportException(EXCEPTION_POINTERS* aExceptionInfo)
{
#ifdef MOZ_CRASHREPORTER
  nsCOMPtr<nsICrashReporter> cr =
    do_GetService("@mozilla.org/toolkit/crash-reporter;1");
  if (cr) {
    cr->WriteMinidumpForException(aExceptionInfo);
  }
#endif
  return EXCEPTION_EXECUTE_HANDLER;
}

XPCOM_API(LRESULT)
CallWindowProcCrashProtected(WNDPROC aWndProc, HWND aHWnd, UINT aMsg,
                             WPARAM aWParam, LPARAM aLParam)
{
  MOZ_SEH_TRY {
    return aWndProc(aHWnd, aMsg, aWParam, aLParam);
  }
  MOZ_SEH_EXCEPT(ReportException(GetExceptionInformation())) {
    ::TerminateProcess(::GetCurrentProcess(), 253);
  }
  return 0; 
}

}

