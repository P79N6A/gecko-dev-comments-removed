





#include "mozilla/dom/NavigatorBinding.h"
#include "PaymentProviderUtils.h"
#include "nsGlobalWindow.h"
#include "nsJSUtils.h"
#include "nsIDocShell.h"

using namespace mozilla::dom;

 bool
PaymentProviderUtils::EnabledForScope(JSContext* aCx,
                                      JSObject* aGlobal)
{
  nsCOMPtr<nsPIDOMWindow> win =
    do_QueryInterface(nsJSUtils::GetStaticScriptGlobal(aGlobal));
  NS_ENSURE_TRUE(win, false);

  nsIDocShell *docShell = win->GetDocShell();
  NS_ENSURE_TRUE(docShell, false);

  nsString paymentRequestId;
  docShell->GetPaymentRequestId(paymentRequestId);

  return !paymentRequestId.IsEmpty();
}
