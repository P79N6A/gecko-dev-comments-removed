




































#include "nsAutoWindowStateHelper.h"

#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsString.h"





nsAutoWindowStateHelper::nsAutoWindowStateHelper(nsIDOMWindow *aWindow)
  : mWindow(aWindow),
    mDefaultEnabled(DispatchCustomEvent("DOMWillOpenModalDialog"))
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));

  if (window) {
    mCallerWindow = window->EnterModalState();
  }
}

nsAutoWindowStateHelper::~nsAutoWindowStateHelper()
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mWindow));

  if (window) {
    window->LeaveModalState(mCallerWindow);
  }

  if (mDefaultEnabled) {
    DispatchCustomEvent("DOMModalDialogClosed");
  }
}

bool
nsAutoWindowStateHelper::DispatchCustomEvent(const char *aEventName)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mWindow);
  if (!window) {
    return true;
  }

  return window->DispatchCustomEvent(aEventName);
}

