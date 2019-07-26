




#include "nsAutoWindowStateHelper.h"

#include "mozilla/dom/Event.h"
#include "nsIDocument.h"
#include "nsIDOMEvent.h"
#include "nsIDOMWindow.h"
#include "nsPIDOMWindow.h"
#include "nsString.h"

using namespace mozilla;
using namespace mozilla::dom;





nsAutoWindowStateHelper::nsAutoWindowStateHelper(nsPIDOMWindow *aWindow)
  : mWindow(aWindow),
    mDefaultEnabled(DispatchEventToChrome("DOMWillOpenModalDialog"))
{
  if (mWindow) {
    mWindow->EnterModalState();
  }
}

nsAutoWindowStateHelper::~nsAutoWindowStateHelper()
{
  if (mWindow) {
    mWindow->LeaveModalState();
  }

  if (mDefaultEnabled) {
    DispatchEventToChrome("DOMModalDialogClosed");
  }
}

bool
nsAutoWindowStateHelper::DispatchEventToChrome(const char *aEventName)
{
  
  
  if (!mWindow) {
    return true;
  }

  
  
  nsIDocument* doc = mWindow->GetExtantDoc();
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

  nsCOMPtr<EventTarget> target = do_QueryInterface(mWindow);
  bool defaultActionEnabled;
  target->DispatchEvent(event, &defaultActionEnabled);
  return defaultActionEnabled;
}
