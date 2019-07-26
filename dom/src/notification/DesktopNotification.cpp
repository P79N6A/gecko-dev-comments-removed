



#include "mozilla/dom/DesktopNotification.h"

#include "nsContentPermissionHelper.h"
#include "nsXULAppAPI.h"

#include "mozilla/dom/PBrowserChild.h"
#include "TabChild.h"
#include "mozilla/Preferences.h"
#include "nsGlobalWindow.h"
#include "nsIAppsService.h"
#include "nsIDOMDesktopNotification.h"

DOMCI_DATA(DesktopNotification, mozilla::dom::DesktopNotification)
DOMCI_DATA(DesktopNotificationCenter, mozilla::dom::DesktopNotificationCenter)

namespace mozilla {
namespace dom {





NS_IMPL_ISUPPORTS1(AlertServiceObserver, nsIObserver)





uint32_t DesktopNotification::sCount = 0;

nsresult
DesktopNotification::PostDesktopNotification()
{
  if (!mObserver)
    mObserver = new AlertServiceObserver(this);

#ifdef MOZ_B2G
  nsCOMPtr<nsIAppNotificationService> appNotifier =
    do_GetService("@mozilla.org/system-alerts-service;1");
  if (appNotifier) {
    nsCOMPtr<nsPIDOMWindow> window = GetOwner();
    uint32_t appId = (window.get())->GetDoc()->NodePrincipal()->GetAppId();

    if (appId != nsIScriptSecurityManager::UNKNOWN_APP_ID) {
      nsCOMPtr<nsIAppsService> appsService = do_GetService("@mozilla.org/AppsService;1");
      nsString manifestUrl = EmptyString();
      appsService->GetManifestURLByLocalId(appId, manifestUrl);
      return appNotifier->ShowAppNotification(mIconURL, mTitle, mDescription,
                                              true,
                                              manifestUrl,
                                              mObserver);
    }
  }
#endif

  nsCOMPtr<nsIAlertsService> alerts = do_GetService("@mozilla.org/alerts-service;1");
  if (!alerts)
    return NS_ERROR_NOT_IMPLEMENTED;

  
  
  
  
  nsString uniqueName = NS_LITERAL_STRING("desktop-notification:");
  uniqueName.AppendInt(sCount++);
  return alerts->ShowAlertNotification(mIconURL, mTitle, mDescription,
                                       true,
                                       uniqueName,
                                       mObserver,
                                       uniqueName,
                                       NS_LITERAL_STRING("auto"),
                                       EmptyString());
}

NS_INTERFACE_MAP_BEGIN(DesktopNotification)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDesktopNotification)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DesktopNotification)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(DesktopNotification, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(DesktopNotification, nsDOMEventTargetHelper)

NS_IMPL_EVENT_HANDLER(DesktopNotification, click)
NS_IMPL_EVENT_HANDLER(DesktopNotification, close)

DesktopNotification::DesktopNotification(const nsAString & title,
                                         const nsAString & description,
                                         const nsAString & iconURL,
                                         nsPIDOMWindow *aWindow,
                                         nsIPrincipal* principal)
  : mTitle(title)
  , mDescription(description)
  , mIconURL(iconURL)
  , mPrincipal(principal)
  , mAllow(false)
  , mShowHasBeenCalled(false)
{
  BindToOwner(aWindow);
  if (Preferences::GetBool("notification.disabled", false)) {
    return;
  }

  
  
  if (Preferences::GetBool("notification.prompt.testing", false) &&
      Preferences::GetBool("notification.prompt.testing.allow", true)) {
    mAllow = true;
  }
}

void
DesktopNotification::Init()
{
  nsRefPtr<DesktopNotificationRequest> request = new DesktopNotificationRequest(this);

  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {

    
    
    
    if (!GetOwner())
      return;

    
    
    TabChild* child = GetTabChildFrom(GetOwner()->GetDocShell());

    
    
    nsRefPtr<DesktopNotificationRequest> copy = request;

    child->SendPContentPermissionRequestConstructor(copy.forget().get(),
                                                    NS_LITERAL_CSTRING("desktop-notification"),
                                                    NS_LITERAL_CSTRING("unused"),
                                                    IPC::Principal(mPrincipal));

    request->Sendprompt();
    return;
  }

  
  NS_DispatchToMainThread(request);
}

DesktopNotification::~DesktopNotification()
{
  if (mObserver) {
    mObserver->Disconnect();
  }
}

void
DesktopNotification::DispatchNotificationEvent(const nsString& aName)
{
  if (NS_FAILED(CheckInnerWindowCorrectness())) {
    return;
  }

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  if (NS_SUCCEEDED(rv)) {
    
    rv = event->InitEvent(aName, false, false);
    if (NS_SUCCEEDED(rv)) {
      event->SetTrusted(true);
      DispatchDOMEvent(nullptr, event, nullptr, nullptr);
    }
  }
}

nsresult
DesktopNotification::SetAllow(bool aAllow)
{
  mAllow = aAllow;

  
  if (mShowHasBeenCalled && aAllow)
    return PostDesktopNotification();

  return NS_OK;
}

void
DesktopNotification::HandleAlertServiceNotification(const char *aTopic)
{
  if (NS_FAILED(CheckInnerWindowCorrectness()))
    return;

  if (!strcmp("alertclickcallback", aTopic)) {
    DispatchNotificationEvent(NS_LITERAL_STRING("click"));
  } else if (!strcmp("alertfinished", aTopic)) {
    DispatchNotificationEvent(NS_LITERAL_STRING("close"));
  }
}

NS_IMETHODIMP
DesktopNotification::Show()
{
  mShowHasBeenCalled = true;

  if (!mAllow)
    return NS_OK;

  return PostDesktopNotification();
}





NS_INTERFACE_MAP_BEGIN(DesktopNotificationCenter)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDesktopNotificationCenter)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDesktopNotificationCenter)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DesktopNotificationCenter)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(DesktopNotificationCenter)
NS_IMPL_RELEASE(DesktopNotificationCenter)

NS_IMETHODIMP
DesktopNotificationCenter::CreateNotification(const nsAString & title,
                                              const nsAString & description,
                                              const nsAString & iconURL,
                                              nsIDOMDesktopNotification **aResult)
{
  NS_ENSURE_STATE(mOwner);
  nsRefPtr<DesktopNotification> notification = new DesktopNotification(title,
                                                                       description,
                                                                       iconURL,
                                                                       mOwner,
                                                                       mPrincipal);
  notification->Init();
  notification.forget(aResult);
  return NS_OK;
}






NS_IMPL_ISUPPORTS2(DesktopNotificationRequest,
                   nsIContentPermissionRequest,
                   nsIRunnable)

NS_IMETHODIMP
DesktopNotificationRequest::GetPrincipal(nsIPrincipal * *aRequestingPrincipal)
{
  if (!mDesktopNotification)
    return NS_ERROR_NOT_INITIALIZED;

  NS_IF_ADDREF(*aRequestingPrincipal = mDesktopNotification->mPrincipal);
  return NS_OK;
}

NS_IMETHODIMP
DesktopNotificationRequest::GetWindow(nsIDOMWindow * *aRequestingWindow)
{
  if (!mDesktopNotification)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMWindow> window =
    do_QueryInterface(mDesktopNotification->GetOwner());
  NS_IF_ADDREF(*aRequestingWindow = window);
  return NS_OK;
}

NS_IMETHODIMP
DesktopNotificationRequest::GetElement(nsIDOMElement * *aElement)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
DesktopNotificationRequest::Cancel()
{
  nsresult rv = mDesktopNotification->SetAllow(false);
  mDesktopNotification = nullptr;
  return rv;
}

NS_IMETHODIMP
DesktopNotificationRequest::Allow()
{
  nsresult rv = mDesktopNotification->SetAllow(true);
  mDesktopNotification = nullptr;
  return rv;
}

NS_IMETHODIMP
DesktopNotificationRequest::GetType(nsACString & aType)
{
  aType = "desktop-notification";
  return NS_OK;
}

NS_IMETHODIMP
DesktopNotificationRequest::GetAccess(nsACString & aAccess)
{
  aAccess = "unused";
  return NS_OK;
}

} 
} 
