




#include "mozilla/dom/ContentChild.h"
#include "mozilla/dom/PermissionMessageUtils.h"
#include "nsXULAppAPI.h"

#include "nsAlertsService.h"

#ifdef MOZ_WIDGET_ANDROID
#include "AndroidBridge.h"
using namespace mozilla::widget::android;
#else

#include "nsXPCOM.h"
#include "nsIServiceManager.h"
#include "nsIDOMWindow.h"
#include "nsPromiseFlatString.h"
#include "nsToolkitCompsCID.h"

#endif 

using namespace mozilla;

using mozilla::dom::ContentChild;

NS_IMPL_ISUPPORTS(nsAlertsService, nsIAlertsService, nsIAlertsProgressListener)

nsAlertsService::nsAlertsService()
{
}

nsAlertsService::~nsAlertsService()
{}

bool nsAlertsService::ShouldShowAlert()
{
  bool result = true;

#ifdef XP_WIN
  HMODULE shellDLL = ::LoadLibraryW(L"shell32.dll");
  if (!shellDLL)
    return result;

  SHQueryUserNotificationStatePtr pSHQueryUserNotificationState =
    (SHQueryUserNotificationStatePtr) ::GetProcAddress(shellDLL, "SHQueryUserNotificationState");

  if (pSHQueryUserNotificationState) {
    MOZ_QUERY_USER_NOTIFICATION_STATE qstate;
    if (SUCCEEDED(pSHQueryUserNotificationState(&qstate))) {
      if (qstate != QUNS_ACCEPTS_NOTIFICATIONS) {
         result = false;
      }
    }
  }

  ::FreeLibrary(shellDLL);
#endif

  return result;
}

NS_IMETHODIMP nsAlertsService::ShowAlertNotification(const nsAString & aImageUrl, const nsAString & aAlertTitle, 
                                                     const nsAString & aAlertText, bool aAlertTextClickable,
                                                     const nsAString & aAlertCookie,
                                                     nsIObserver * aAlertListener,
                                                     const nsAString & aAlertName,
                                                     const nsAString & aBidi,
                                                     const nsAString & aLang,
                                                     const nsAString & aData,
                                                     nsIPrincipal * aPrincipal)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();

    if (aAlertListener)
      cpc->AddRemoteAlertObserver(PromiseFlatString(aAlertCookie), aAlertListener);

    cpc->SendShowAlertNotification(PromiseFlatString(aImageUrl),
                                   PromiseFlatString(aAlertTitle),
                                   PromiseFlatString(aAlertText),
                                   aAlertTextClickable,
                                   PromiseFlatString(aAlertCookie),
                                   PromiseFlatString(aAlertName),
                                   PromiseFlatString(aBidi),
                                   PromiseFlatString(aLang),
                                   PromiseFlatString(aData),
                                   IPC::Principal(aPrincipal));
    return NS_OK;
  }

#ifdef MOZ_WIDGET_ANDROID
  mozilla::AndroidBridge::Bridge()->ShowAlertNotification(aImageUrl, aAlertTitle, aAlertText, aAlertCookie,
                                                          aAlertListener, aAlertName);
  return NS_OK;
#else
  
  nsCOMPtr<nsIAlertsService> sysAlerts(do_GetService(NS_SYSTEMALERTSERVICE_CONTRACTID));
  nsresult rv;
  if (sysAlerts) {
    return sysAlerts->ShowAlertNotification(aImageUrl, aAlertTitle, aAlertText, aAlertTextClickable,
                                            aAlertCookie, aAlertListener, aAlertName,
                                            aBidi, aLang, aData,
                                            IPC::Principal(aPrincipal));
  }

  if (!ShouldShowAlert()) {
    
    if (aAlertListener)
      aAlertListener->Observe(nullptr, "alertfinished", PromiseFlatString(aAlertCookie).get());
    return NS_OK;
  }

  
  rv = mXULAlerts.ShowAlertNotification(aImageUrl, aAlertTitle, aAlertText, aAlertTextClickable,
                                        aAlertCookie, aAlertListener, aAlertName,
                                        aBidi, aLang);
  return rv;
#endif 
}

NS_IMETHODIMP nsAlertsService::CloseAlert(const nsAString& aAlertName,
                                          nsIPrincipal* aPrincipal)
{
  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    ContentChild* cpc = ContentChild::GetSingleton();
    cpc->SendCloseAlert(nsAutoString(aAlertName), IPC::Principal(aPrincipal));
    return NS_OK;
  }

#ifdef MOZ_WIDGET_ANDROID
  mozilla::widget::android::GeckoAppShell::CloseNotification(aAlertName);
  return NS_OK;
#else

  
  nsCOMPtr<nsIAlertsService> sysAlerts(do_GetService(NS_SYSTEMALERTSERVICE_CONTRACTID));
  if (sysAlerts) {
    return sysAlerts->CloseAlert(aAlertName, nullptr);
  }

  return mXULAlerts.CloseAlert(aAlertName);
#endif 
}


NS_IMETHODIMP nsAlertsService::OnProgress(const nsAString & aAlertName,
                                          int64_t aProgress,
                                          int64_t aProgressMax,
                                          const nsAString & aAlertText)
{
#ifdef MOZ_WIDGET_ANDROID
  mozilla::widget::android::GeckoAppShell::AlertsProgressListener_OnProgress(aAlertName,
                                                                             aProgress, aProgressMax,
                                                                             aAlertText);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}

NS_IMETHODIMP nsAlertsService::OnCancel(const nsAString & aAlertName)
{
#ifdef MOZ_WIDGET_ANDROID
  mozilla::widget::android::GeckoAppShell::CloseNotification(aAlertName);
  return NS_OK;
#else
  return NS_ERROR_NOT_IMPLEMENTED;
#endif 
}
