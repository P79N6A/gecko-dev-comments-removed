




#include "TimeChangeObserver.h"
#include "mozilla/ClearOnShutdown.h"
#include "mozilla/Services.h"
#include "mozilla/StaticPtr.h"
#include "nsPIDOMWindow.h"
#include "nsDOMEvent.h"
#include "nsContentUtils.h"
#include "nsIObserverService.h"

using namespace mozilla;
using namespace mozilla::hal;
using namespace mozilla::services;

StaticAutoPtr<nsSystemTimeChangeObserver> sObserver;

nsSystemTimeChangeObserver* nsSystemTimeChangeObserver::GetInstance()
{
  if (!sObserver) {
    sObserver = new nsSystemTimeChangeObserver();
    ClearOnShutdown(&sObserver);
  }
  return sObserver;
}

nsSystemTimeChangeObserver::~nsSystemTimeChangeObserver()
{
  UnregisterSystemClockChangeObserver(this);
  UnregisterSystemTimezoneChangeObserver(this);
}

void
nsSystemTimeChangeObserver::FireMozTimeChangeEvent()
{
  ListenerArray::ForwardIterator iter(mWindowListeners);
  while (iter.HasMore()) {
    nsWeakPtr weakWindow = iter.GetNext();
    nsCOMPtr<nsPIDOMWindow> window = do_QueryReferent(weakWindow);
    nsCOMPtr<nsIDocument> document;
    if (!window || !(document = window->GetDoc())) {
      mWindowListeners.RemoveElement(weakWindow);
      continue;
    }

    nsContentUtils::DispatchTrustedEvent(document, window,
      NS_LITERAL_STRING("moztimechange"),  true,
       false);
  }
}

void
nsSystemTimeChangeObserver::Notify(const int64_t& aClockDeltaMS)
{
  
  nsCOMPtr<nsIObserverService> observerService = GetObserverService();
  if (observerService) {
    nsString dataStr;
    dataStr.AppendFloat(static_cast<double>(aClockDeltaMS));
    observerService->NotifyObservers(
      nullptr, "system-clock-change", dataStr.get());
  }

  FireMozTimeChangeEvent();
}

void
nsSystemTimeChangeObserver::Notify(
  const SystemTimezoneChangeInformation& aSystemTimezoneChangeInfo)
{
  FireMozTimeChangeEvent();
}

nsresult
nsSystemTimeChangeObserver::AddWindowListener(nsIDOMWindow* aWindow)
{
  return GetInstance()->AddWindowListenerImpl(aWindow);
}

nsresult
nsSystemTimeChangeObserver::AddWindowListenerImpl(nsIDOMWindow* aWindow)
{
  if (!aWindow) {
    return NS_ERROR_ILLEGAL_VALUE;
  }

  nsWeakPtr windowWeakRef = do_GetWeakReference(aWindow);
  NS_ASSERTION(windowWeakRef, "nsIDOMWindow implementations shuld support weak ref");

  if (mWindowListeners.IndexOf(windowWeakRef) !=
      ListenerArray::array_type::NoIndex) {
    return NS_OK;
  }

  if (mWindowListeners.IsEmpty()) {
    RegisterSystemClockChangeObserver(sObserver);
    RegisterSystemTimezoneChangeObserver(sObserver);
  }

  mWindowListeners.AppendElement(windowWeakRef);
  return NS_OK;
}

nsresult
nsSystemTimeChangeObserver::RemoveWindowListener(nsIDOMWindow* aWindow)
{
  if (!sObserver) {
    return NS_OK;
  }

  return GetInstance()->RemoveWindowListenerImpl(aWindow);
}

nsresult
nsSystemTimeChangeObserver::RemoveWindowListenerImpl(nsIDOMWindow* aWindow)
{
  mWindowListeners.RemoveElement(NS_GetWeakReference(aWindow));

  if (mWindowListeners.IsEmpty()) {
    UnregisterSystemClockChangeObserver(sObserver);
    UnregisterSystemTimezoneChangeObserver(sObserver);
  }

  return NS_OK;
}
