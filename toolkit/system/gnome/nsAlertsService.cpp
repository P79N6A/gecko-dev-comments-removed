




































#include "nsAlertsService.h"
#include "nsAlertsIconListener.h"
#include "nsAutoPtr.h"

NS_IMPL_THREADSAFE_ADDREF(nsAlertsService)
NS_IMPL_THREADSAFE_RELEASE(nsAlertsService)

NS_INTERFACE_MAP_BEGIN(nsAlertsService)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAlertsService)
   NS_INTERFACE_MAP_ENTRY(nsIAlertsService)
NS_INTERFACE_MAP_END_THREADSAFE

nsAlertsService::nsAlertsService()
{}

nsAlertsService::~nsAlertsService()
{}

nsresult
nsAlertsService::Init()
{
  return NS_OK;
}

NS_IMETHODIMP nsAlertsService::ShowAlertNotification(const nsAString & aImageUrl, const nsAString & aAlertTitle, 
                                                     const nsAString & aAlertText, PRBool aAlertTextClickable,
                                                     const nsAString & aAlertCookie,
                                                     nsIObserver * aAlertListener,
                                                     const nsAString & aAlertName)
{
  nsRefPtr<nsAlertsIconListener> alertListener = new nsAlertsIconListener();
  if (!alertListener)
    return NS_ERROR_OUT_OF_MEMORY;

  return alertListener->InitAlertAsync(aImageUrl, aAlertTitle, aAlertText, aAlertTextClickable,
                                       aAlertCookie, aAlertListener);
}
