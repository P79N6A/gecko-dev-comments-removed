




#include "nsAutoWindowStateHelper.h"

#include "mozilla/dom/Event.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsString.h"

using namespace mozilla;
using namespace mozilla::dom;





nsAutoWindowStateHelper::nsAutoWindowStateHelper(nsIDOMWindow *aWindow)
  : mWindow(aWindow),
    mDefaultEnabled(DispatchEventToChrome("DOMWillOpenModalDialog"))
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(aWindow));

  if (window) {
    window->EnterModalState();
  }
}

nsAutoWindowStateHelper::~nsAutoWindowStateHelper()
{
  nsCOMPtr<nsPIDOMWindow> window(do_QueryInterface(mWindow));

  if (window) {
    window->LeaveModalState();
  }

  if (mDefaultEnabled) {
    DispatchEventToChrome("DOMModalDialogClosed");
  }
}

bool
nsAutoWindowStateHelper::DispatchEventToChrome(const char *aEventName)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(mWindow);
  if (!window || (window->IsInnerWindow() && !window->IsCurrentInnerWindow())) {
    return true;
  }

  
  
  nsIDocument* doc = window->GetExtantDoc();
  if (!doc) {
    return true;
  }

  ErrorResult rv;
  nsRefPtr<Event> event = doc->CreateEvent(NS_LITERAL_STRING("Events"), rv);
  if (rv.Failed()) {
    return false;
  }
  NS_ENSURE_TRUE(NS_SUCCEEDED(event->InitEvent(NS_ConvertASCIItoUTF16(aEventName), true, true)), false);
  event->SetTrusted(true);
  event->GetInternalNSEvent()->mFlags.mOnlyChromeDispatch = true;

  nsCOMPtr<EventTarget> target = do_QueryInterface(window);
  bool defaultActionEnabled;
  target->DispatchEvent(event, &defaultActionEnabled);
  return defaultActionEnabled;
}
