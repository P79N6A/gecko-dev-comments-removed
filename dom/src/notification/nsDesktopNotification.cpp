



#include "nsDesktopNotification.h"

#include "nsContentPermissionHelper.h"
#include "nsXULAppAPI.h"

#include "mozilla/dom/PBrowserChild.h"
#include "TabChild.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;





NS_IMPL_ISUPPORTS1(AlertServiceObserver, nsIObserver)





nsresult
nsDOMDesktopNotification::PostDesktopNotification()
{
  nsCOMPtr<nsIAlertsService> alerts = do_GetService("@mozilla.org/alerts-service;1");
  if (!alerts)
    return NS_ERROR_NOT_IMPLEMENTED;

  if (!mObserver)
    mObserver = new AlertServiceObserver(this);

  return alerts->ShowAlertNotification(mIconURL, mTitle, mDescription,
                                       true,
                                       EmptyString(),
                                       mObserver,
                                       EmptyString());
}

DOMCI_DATA(DesktopNotification, nsDOMDesktopNotification)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMDesktopNotification)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMDesktopNotification)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDesktopNotification)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDesktopNotification)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DesktopNotification)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)

NS_IMPL_EVENT_HANDLER(nsDOMDesktopNotification, click)
NS_IMPL_EVENT_HANDLER(nsDOMDesktopNotification, close)

nsDOMDesktopNotification::nsDOMDesktopNotification(const nsAString & title,
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
nsDOMDesktopNotification::Init()
{
  nsRefPtr<nsDesktopNotificationRequest> request = new nsDesktopNotificationRequest(this);

  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {

    
    
    
    if (!GetOwner())
      return;

    
    
    TabChild* child = GetTabChildFrom(GetOwner()->GetDocShell());

    
    
    nsRefPtr<nsDesktopNotificationRequest> copy = request;

    child->SendPContentPermissionRequestConstructor(request,
                                                    NS_LITERAL_CSTRING("desktop-notification"),
                                                    NS_LITERAL_CSTRING("unused"),
                                                    IPC::Principal(mPrincipal));

    request->Sendprompt();
    return;
  }

  
  NS_DispatchToMainThread(request);
}

nsDOMDesktopNotification::~nsDOMDesktopNotification()
{
  if (mObserver) {
    mObserver->Disconnect();
  }
}

void
nsDOMDesktopNotification::DispatchNotificationEvent(const nsString& aName)
{
  if (NS_FAILED(CheckInnerWindowCorrectness())) {
    return;
  }

  nsCOMPtr<nsIDOMEvent> event;
  nsresult rv = NS_NewDOMEvent(getter_AddRefs(event), nullptr, nullptr);
  if (NS_SUCCEEDED(rv)) {
    
    rv = event->InitEvent(aName, false, false);
    if (NS_SUCCEEDED(rv)) {
      event->SetTrusted(true);
      DispatchDOMEvent(nullptr, event, nullptr, nullptr);
    }
  }
}

nsresult
nsDOMDesktopNotification::SetAllow(bool aAllow)
{
  mAllow = aAllow;

  
  if (mShowHasBeenCalled && aAllow)
    return PostDesktopNotification();

  return NS_OK;
}

void
nsDOMDesktopNotification::HandleAlertServiceNotification(const char *aTopic)
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
nsDOMDesktopNotification::Show()
{
  mShowHasBeenCalled = true;

  if (!mAllow)
    return NS_OK;

  return PostDesktopNotification();
}





DOMCI_DATA(DesktopNotificationCenter, nsDesktopNotificationCenter)

NS_INTERFACE_MAP_BEGIN(nsDesktopNotificationCenter)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDesktopNotificationCenter)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDesktopNotificationCenter)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DesktopNotificationCenter)
NS_INTERFACE_MAP_END

NS_IMPL_ADDREF(nsDesktopNotificationCenter)
NS_IMPL_RELEASE(nsDesktopNotificationCenter)

NS_IMETHODIMP
nsDesktopNotificationCenter::CreateNotification(const nsAString & title,
                                                const nsAString & description,
                                                const nsAString & iconURL,
                                                nsIDOMDesktopNotification **aResult)
{
  NS_ENSURE_STATE(mOwner);
  nsRefPtr<nsDOMDesktopNotification> notification = new nsDOMDesktopNotification(title, 
                                                                                 description,
                                                                                 iconURL,
                                                                                 mOwner,
                                                                                 mPrincipal);
  notification->Init();
  notification.forget(aResult);
  return NS_OK;
}






NS_IMPL_ISUPPORTS2(nsDesktopNotificationRequest,
                   nsIContentPermissionRequest,
                   nsIRunnable)

NS_IMETHODIMP
nsDesktopNotificationRequest::GetPrincipal(nsIPrincipal * *aRequestingPrincipal)
{
  if (!mDesktopNotification)
    return NS_ERROR_NOT_INITIALIZED;

  NS_IF_ADDREF(*aRequestingPrincipal = mDesktopNotification->mPrincipal);
  return NS_OK;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::GetWindow(nsIDOMWindow * *aRequestingWindow)
{
  if (!mDesktopNotification)
    return NS_ERROR_NOT_INITIALIZED;

  nsCOMPtr<nsIDOMWindow> window =
    do_QueryInterface(mDesktopNotification->GetOwner());
  NS_IF_ADDREF(*aRequestingWindow = window);
  return NS_OK;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::GetElement(nsIDOMElement * *aElement)
{
  return NS_ERROR_FAILURE;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::Cancel()
{
  nsresult rv = mDesktopNotification->SetAllow(false);
  mDesktopNotification = nullptr;
  return rv;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::Allow()
{
  nsresult rv = mDesktopNotification->SetAllow(true);
  mDesktopNotification = nullptr;
  return rv;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::GetType(nsACString & aType)
{
  aType = "desktop-notification";
  return NS_OK;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::GetAccess(nsACString & aAccess)
{
  aAccess = "unused";
  return NS_OK;
}
