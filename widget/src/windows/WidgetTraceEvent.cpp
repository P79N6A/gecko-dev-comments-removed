









































#include <stdio.h>
#include <windows.h>

#include "mozilla/WidgetTraceEvent.h"
#include "nsAppShellCID.h"
#include "nsComponentManagerUtils.h"
#include "nsCOMPtr.h"
#include "nsIAppShellService.h"
#include "nsIBaseWindow.h"
#include "nsIDocShell.h"
#include "nsIWidget.h"
#include "nsIXULWindow.h"
#include "nsAutoPtr.h"
#include "nsServiceManagerUtils.h"
#include "nsThreadUtils.h"
#include "nsWindowDefs.h"

namespace {


HANDLE event_handle = NULL;



class HWNDGetter : public nsRunnable {
public:
  HWNDGetter() : hidden_window_hwnd(NULL) {}

  HWND hidden_window_hwnd;

  NS_IMETHOD Run() {
    
    nsCOMPtr<nsIAppShellService> appShell(do_GetService(NS_APPSHELLSERVICE_CONTRACTID));
    nsCOMPtr<nsIXULWindow> hiddenWindow;

    nsresult rv = appShell->GetHiddenWindow(getter_AddRefs(hiddenWindow));
    if (NS_FAILED(rv)) {
      return rv;
    }

    nsCOMPtr<nsIDocShell> docShell;
    rv = hiddenWindow->GetDocShell(getter_AddRefs(docShell));
    if (NS_FAILED(rv) || !docShell) {
      return rv;
    }

    nsCOMPtr<nsIBaseWindow> baseWindow(do_QueryInterface(docShell));
    
    if (!baseWindow)
      return NS_ERROR_FAILURE;

    nsCOMPtr<nsIWidget> widget;
    baseWindow->GetMainWidget(getter_AddRefs(widget));

    if (!widget)
      return NS_ERROR_FAILURE;

    hidden_window_hwnd = (HWND)widget->GetNativeData(NS_NATIVE_WINDOW);

    return NS_OK;
  }
};

HWND GetHiddenWindowHWND()
{
  
  
  nsRefPtr<HWNDGetter> getter = new HWNDGetter();
  NS_DispatchToMainThread(getter, NS_DISPATCH_SYNC);
  return getter->hidden_window_hwnd;
}

} 

namespace mozilla {


void SignalTracerThread()
{
  if (event_handle != NULL)
    SetEvent(event_handle);
}


bool FireAndWaitForTracerEvent()
{
  
  static HWND hidden_window = NULL;
  if (hidden_window == NULL) {
    hidden_window = GetHiddenWindowHWND();
  }

  if (hidden_window == NULL)
    return false;

  if (event_handle == NULL)
    event_handle = CreateEvent(NULL, FALSE, FALSE, NULL);

  
  
  PostMessage(hidden_window, MOZ_WM_TRACE, 0, 0);
  WaitForSingleObject(event_handle, INFINITE);
  return true;
}

}  
