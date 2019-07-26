




#include "nsSystemAlertsService.h"
#include "nsAlertsIconListener.h"
#include "nsAutoPtr.h"

NS_IMPL_THREADSAFE_ADDREF(nsSystemAlertsService)
NS_IMPL_THREADSAFE_RELEASE(nsSystemAlertsService)

NS_INTERFACE_MAP_BEGIN(nsSystemAlertsService)
   NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIAlertsService)
   NS_INTERFACE_MAP_ENTRY(nsIAlertsService)
NS_INTERFACE_MAP_END_THREADSAFE

nsSystemAlertsService::nsSystemAlertsService()
{}

nsSystemAlertsService::~nsSystemAlertsService()
{}

nsresult
nsSystemAlertsService::Init()
{
  return NS_OK;
}

NS_IMETHODIMP
nsSystemAlertsService::ShowAlertNotification(const nsAString & aImageUrl,
                                             const nsAString & aAlertTitle, 
                                             const nsAString & aAlertText,
                                             bool aAlertTextClickable,
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
