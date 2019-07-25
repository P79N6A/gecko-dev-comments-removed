







































#include "mozilla/dom/ContentChild.h"
#include "nsXULAppAPI.h"
using mozilla::dom::ContentChild;

#include "nsAlertsService.h"

#ifdef ANDROID
#include "AndroidBridge.h"
#else

#include "nsISupportsArray.h"
#include "nsXPCOM.h"
#include "nsISupportsPrimitives.h"
#include "nsIServiceManager.h"
#include "nsIDOMWindowInternal.h"
#include "nsIWindowWatcher.h"
#include "nsDependentString.h"
#include "nsWidgetsCID.h"
#include "nsILookAndFeel.h"
#include "nsToolkitCompsCID.h"

static NS_DEFINE_CID(kLookAndFeelCID, NS_LOOKANDFEEL_CID);

#define ALERT_CHROME_URL "chrome://global/content/alerts/alert.xul"

#endif 

NS_IMPL_THREADSAFE_ISUPPORTS2(nsAlertsService, nsIAlertsService, nsIAlertsProgressListener)

nsAlertsService::nsAlertsService()
{
}

nsAlertsService::~nsAlertsService()
{}

NS_IMETHODIMP nsAlertsService::ShowAlertNotification(const nsAString & aImageUrl, const nsAString & aAlertTitle, 
                                                     const nsAString & aAlertText, PRBool aAlertTextClickable,
                                                     const nsAString & aAlertCookie,
                                                     nsIObserver * aAlertListener,
                                                     const nsAString & aAlertName)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();

    if (aAlertListener)
      cpc->AddRemoteAlertObserver(nsDependentString(aAlertCookie), aAlertListener);

    cpc->SendShowAlertNotification(nsAutoString(aImageUrl),
                                   nsAutoString(aAlertTitle),
                                   nsAutoString(aAlertText),
                                   aAlertTextClickable,
                                   nsAutoString(aAlertCookie),
                                   nsAutoString(aAlertName));
    return NS_OK;
  }

#ifdef ANDROID
  mozilla::AndroidBridge::Bridge()->ShowAlertNotification(aImageUrl, aAlertTitle, aAlertText, aAlertCookie,
                                                          aAlertListener, aAlertName);
  return NS_OK;
#else
  
  nsCOMPtr<nsIAlertsService> sysAlerts(do_GetService(NS_SYSTEMALERTSERVICE_CONTRACTID));
  nsresult rv;
  if (sysAlerts) {
    rv = sysAlerts->ShowAlertNotification(aImageUrl, aAlertTitle, aAlertText, aAlertTextClickable,
                                          aAlertCookie, aAlertListener, aAlertName);
    if (NS_SUCCEEDED(rv))
      return rv;
  }

  nsCOMPtr<nsIWindowWatcher> wwatch(do_GetService(NS_WINDOWWATCHER_CONTRACTID));
  nsCOMPtr<nsIDOMWindow> newWindow;

  nsCOMPtr<nsISupportsArray> argsArray;
  rv = NS_NewISupportsArray(getter_AddRefs(argsArray));
  NS_ENSURE_SUCCESS(rv, rv);

  
  nsCOMPtr<nsISupportsString> scriptableImageUrl (do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  NS_ENSURE_TRUE(scriptableImageUrl, NS_ERROR_FAILURE);

  scriptableImageUrl->SetData(aImageUrl);
  rv = argsArray->AppendElement(scriptableImageUrl);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> scriptableAlertTitle (do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  NS_ENSURE_TRUE(scriptableAlertTitle, NS_ERROR_FAILURE);

  scriptableAlertTitle->SetData(aAlertTitle);
  rv = argsArray->AppendElement(scriptableAlertTitle);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> scriptableAlertText (do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  NS_ENSURE_TRUE(scriptableAlertText, NS_ERROR_FAILURE);

  scriptableAlertText->SetData(aAlertText);
  rv = argsArray->AppendElement(scriptableAlertText);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsPRBool> scriptableIsClickable (do_CreateInstance(NS_SUPPORTS_PRBOOL_CONTRACTID));
  NS_ENSURE_TRUE(scriptableIsClickable, NS_ERROR_FAILURE);

  scriptableIsClickable->SetData(aAlertTextClickable);
  rv = argsArray->AppendElement(scriptableIsClickable);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsString> scriptableAlertCookie (do_CreateInstance(NS_SUPPORTS_STRING_CONTRACTID));
  NS_ENSURE_TRUE(scriptableAlertCookie, NS_ERROR_FAILURE);

  scriptableAlertCookie->SetData(aAlertCookie);
  rv = argsArray->AppendElement(scriptableAlertCookie);
  NS_ENSURE_SUCCESS(rv, rv);

  nsCOMPtr<nsISupportsPRInt32> scriptableOrigin (do_CreateInstance(NS_SUPPORTS_PRINT32_CONTRACTID));
  NS_ENSURE_TRUE(scriptableOrigin, NS_ERROR_FAILURE);
  nsCOMPtr<nsILookAndFeel> lookAndFeel = do_GetService("@mozilla.org/widget/lookandfeel;1");
  if (lookAndFeel)
  {
    PRInt32 origin;
    lookAndFeel->GetMetric(nsILookAndFeel::eMetric_AlertNotificationOrigin,
                           origin);
    scriptableOrigin->SetData(origin);
  }
  rv = argsArray->AppendElement(scriptableOrigin);
  NS_ENSURE_SUCCESS(rv, rv);

  if (aAlertListener)
  {
    nsCOMPtr<nsISupportsInterfacePointer> ifptr = do_CreateInstance(NS_SUPPORTS_INTERFACE_POINTER_CONTRACTID, &rv);
    NS_ENSURE_SUCCESS(rv, rv);

    nsCOMPtr<nsISupports> iSupports (do_QueryInterface(aAlertListener));
    ifptr->SetData(iSupports);
    ifptr->SetDataIID(&NS_GET_IID(nsIObserver));
    rv = argsArray->AppendElement(ifptr);
    NS_ENSURE_SUCCESS(rv, rv);
  }

  rv = wwatch->OpenWindow(0, ALERT_CHROME_URL, "_blank",
                 "chrome,dialog=yes,titlebar=no,popup=yes", argsArray,
                 getter_AddRefs(newWindow));
  return rv;
#endif 
}

NS_IMETHODIMP nsAlertsService::OnProgress(const nsAString & aAlertName,
                                          PRInt64 aProgress,
                                          PRInt64 aProgressMax,
                                          const nsAString & aAlertText)
{
#ifdef ANDROID
  mozilla::AndroidBridge::Bridge()->AlertsProgressListener_OnProgress(aAlertName, aProgress, aProgressMax, aAlertText);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}

NS_IMETHODIMP nsAlertsService::OnCancel(const nsAString & aAlertName)
{
#ifdef ANDROID
  mozilla::AndroidBridge::Bridge()->AlertsProgressListener_OnCancel(aAlertName);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}
