




































#include "nsAutoWindowStateHelper.h"

#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsIDOMEventTarget.h"
#include "nsIDOMEvent.h"
#include "nsIPrivateDOMEvent.h"
#include "nsIDOMDocumentEvent.h"
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

PRBool
nsAutoWindowStateHelper::DispatchCustomEvent(const char *aEventName)
{
  if (!mWindow) {
    return PR_TRUE;
  }

#ifdef DEBUG
  {
    nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mWindow));
  }
#endif

  nsCOMPtr<nsIDOMDocument> domdoc;
  mWindow->GetDocument(getter_AddRefs(domdoc));

  nsCOMPtr<nsIDOMDocumentEvent> docevent(do_QueryInterface(domdoc));
  nsCOMPtr<nsIDOMEvent> event;

  PRBool defaultActionEnabled = PR_TRUE;

  if (docevent) {
    docevent->CreateEvent(NS_LITERAL_STRING("Events"), getter_AddRefs(event));

    nsCOMPtr<nsIPrivateDOMEvent> privateEvent(do_QueryInterface(event));
    if (privateEvent) {
      event->InitEvent(NS_ConvertASCIItoUTF16(aEventName), PR_TRUE, PR_TRUE);

      privateEvent->SetTrusted(PR_TRUE);

      nsCOMPtr<nsIDOMEventTarget> target(do_QueryInterface(mWindow));

      target->DispatchEvent(event, &defaultActionEnabled);
    }
  }

  return defaultActionEnabled;
}

