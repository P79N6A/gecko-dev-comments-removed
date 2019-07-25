



#include "nsDesktopNotification.h"

#include "nsContentPermissionHelper.h"
#include "nsXULAppAPI.h"

#include "mozilla/dom/PBrowserChild.h"
#include "TabChild.h"
#include "mozilla/Preferences.h"

using namespace mozilla;
using namespace mozilla::dom;





NS_IMPL_ISUPPORTS1(AlertServiceObserver, nsIObserver)





void
nsDOMDesktopNotification::PostDesktopNotification()
{
  nsCOMPtr<nsIAlertsService> alerts = do_GetService("@mozilla.org/alerts-service;1");
  if (!alerts)
    return;

  if (!mObserver)
    mObserver = new AlertServiceObserver(this);

  alerts->ShowAlertNotification(mIconURL, mTitle, mDescription,
                                true, 
                                EmptyString(),
                                mObserver,
                                EmptyString());
}

DOMCI_DATA(DesktopNotification, nsDOMDesktopNotification)

NS_IMPL_CYCLE_COLLECTION_CLASS(nsDOMDesktopNotification)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnClickCallback)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_NSCOMPTR(mOnCloseCallback)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnClickCallback)
  NS_IMPL_CYCLE_COLLECTION_UNLINK_NSCOMPTR(mOnCloseCallback)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(nsDOMDesktopNotification)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIDOMDesktopNotification)
  NS_INTERFACE_MAP_ENTRY(nsIDOMDesktopNotification)
  NS_DOM_INTERFACE_MAP_ENTRY_CLASSINFO(DesktopNotification)
NS_INTERFACE_MAP_END_INHERITING(nsDOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(nsDOMDesktopNotification, nsDOMEventTargetHelper)

nsDOMDesktopNotification::nsDOMDesktopNotification(const nsAString & title,
                                                   const nsAString & description,
                                                   const nsAString & iconURL,
                                                   nsPIDOMWindow *aWindow,
                                                   nsIURI* uri)
  : mTitle(title)
  , mDescription(description)
  , mIconURL(iconURL)
  , mURI(uri)
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
    return;
  }

  nsRefPtr<nsDesktopNotificationRequest> request = new nsDesktopNotificationRequest(this);

  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {

    
    
    
    if (!GetOwner())
      return;

    
    
    TabChild* child = GetTabChildFrom(GetOwner()->GetDocShell());
    
    
    
    nsRefPtr<nsDesktopNotificationRequest> copy = request;

    nsCString type = NS_LITERAL_CSTRING("desktop-notification");
    child->SendPContentPermissionRequestConstructor(copy.forget().get(), type, IPC::URI(mURI));
    
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

void
nsDOMDesktopNotification::SetAllow(bool aAllow)
{
  mAllow = aAllow;

  
  if (mShowHasBeenCalled && aAllow)
    PostDesktopNotification();
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

  PostDesktopNotification();
  return NS_OK;
}

NS_IMETHODIMP
nsDOMDesktopNotification::GetOnclick(nsIDOMEventListener * *aOnclick)
{
  return GetInnerEventListener(mOnClickCallback, aOnclick);
}

NS_IMETHODIMP nsDOMDesktopNotification::SetOnclick(nsIDOMEventListener * aOnclick)
{
  return RemoveAddEventListener(NS_LITERAL_STRING("click"),
                                mOnClickCallback,
                                aOnclick);
}

NS_IMETHODIMP
nsDOMDesktopNotification::GetOnclose(nsIDOMEventListener * *aOnclose)
{
  return GetInnerEventListener(mOnCloseCallback, aOnclose);
}

NS_IMETHODIMP nsDOMDesktopNotification::SetOnclose(nsIDOMEventListener * aOnclose)
{
  return RemoveAddEventListener(NS_LITERAL_STRING("close"),
                                mOnCloseCallback,
                                aOnclose);
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
  nsRefPtr<nsIDOMDesktopNotification> notification = new nsDOMDesktopNotification(title, 
                                                                                  description,
                                                                                  iconURL,
                                                                                  mOwner,
                                                                                  mURI);
  notification.forget(aResult);
  return NS_OK;
}






NS_IMPL_ISUPPORTS2(nsDesktopNotificationRequest,
                   nsIContentPermissionRequest,
                   nsIRunnable)

NS_IMETHODIMP
nsDesktopNotificationRequest::GetUri(nsIURI * *aRequestingURI)
{
  if (!mDesktopNotification)
    return NS_ERROR_NOT_INITIALIZED;

  NS_IF_ADDREF(*aRequestingURI = mDesktopNotification->mURI);
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
  mDesktopNotification->SetAllow(false);
  mDesktopNotification = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::Allow()
{
  mDesktopNotification->SetAllow(true);
  mDesktopNotification = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
nsDesktopNotificationRequest::GetType(nsACString & aType)
{
  aType = "desktop-notification";
  return NS_OK;
}

