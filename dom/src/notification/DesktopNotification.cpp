


#include "mozilla/dom/DesktopNotification.h"
#include "mozilla/dom/DesktopNotificationBinding.h"
#include "nsContentPermissionHelper.h"
#include "nsXULAppAPI.h"
#include "mozilla/dom/PBrowserChild.h"
#include "nsIDOMDesktopNotification.h"
#include "TabChild.h"
#include "mozilla/Preferences.h"
#include "nsGlobalWindow.h"
#include "nsIAppsService.h"
#include "PCOMContentPermissionRequestChild.h"
#include "nsIScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"

namespace mozilla {
namespace dom {




class DesktopNotificationRequest : public nsIContentPermissionRequest,
                                   public nsRunnable,
                                   public PCOMContentPermissionRequestChild

{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST

  DesktopNotificationRequest(DesktopNotification* notification)
    : mDesktopNotification(notification) {}

  NS_IMETHOD Run() MOZ_OVERRIDE
  {
    nsCOMPtr<nsIContentPermissionPrompt> prompt =
      do_CreateInstance(NS_CONTENT_PERMISSION_PROMPT_CONTRACTID);
    if (prompt) {
      prompt->Prompt(this);
    }
    return NS_OK;
  }

  ~DesktopNotificationRequest()
  {
  }

  virtual bool Recv__delete__(const bool& aAllow) MOZ_OVERRIDE
  {
    if (aAllow) {
      (void) Allow();
    } else {
     (void) Cancel();
    }
   return true;
  }
  virtual void IPDLRelease() MOZ_OVERRIDE { Release(); }

  nsRefPtr<DesktopNotification> mDesktopNotification;
};





NS_IMPL_ISUPPORTS1(AlertServiceObserver, nsIObserver)





uint32_t DesktopNotification::sCount = 0;

nsresult
DesktopNotification::PostDesktopNotification()
{
  if (!mObserver) {
    mObserver = new AlertServiceObserver(this);
  }

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
                                              mObserver,
                                              EmptyString());
    }
  }
#endif

  nsCOMPtr<nsIAlertsService> alerts = do_GetService("@mozilla.org/alerts-service;1");
  if (!alerts) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  
  
  
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

  SetIsDOMBinding();
}

void
DesktopNotification::Init()
{
  nsRefPtr<DesktopNotificationRequest> request = new DesktopNotificationRequest(this);

  
  if (XRE_GetProcessType() == GeckoProcessType_Content) {

    
    
    
    if (!GetOwner())
      return;

    
    
    TabChild* child = TabChild::GetFrom(GetOwner()->GetDocShell());

    
    
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

  
  if (mShowHasBeenCalled && aAllow) {
    return PostDesktopNotification();
  }

  return NS_OK;
}

void
DesktopNotification::HandleAlertServiceNotification(const char *aTopic)
{
  if (NS_FAILED(CheckInnerWindowCorrectness())) {
    return;
  }

  if (!strcmp("alertclickcallback", aTopic)) {
    DispatchNotificationEvent(NS_LITERAL_STRING("click"));
  } else if (!strcmp("alertfinished", aTopic)) {
    DispatchNotificationEvent(NS_LITERAL_STRING("close"));
  }
}

void
DesktopNotification::Show(ErrorResult& aRv)
{
  mShowHasBeenCalled = true;

  if (!mAllow) {
    return;
  }

  aRv = PostDesktopNotification();
}

JSObject*
DesktopNotification::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DesktopNotificationBinding::Wrap(aCx, aScope, this);
}





NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE_0(DesktopNotificationCenter)
NS_IMPL_CYCLE_COLLECTING_ADDREF(DesktopNotificationCenter)
NS_IMPL_CYCLE_COLLECTING_RELEASE(DesktopNotificationCenter)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(DesktopNotificationCenter)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

already_AddRefed<DesktopNotification>
DesktopNotificationCenter::CreateNotification(const nsAString& aTitle,
                                              const nsAString& aDescription,
                                              const nsAString& aIconURL)
{
  MOZ_ASSERT(mOwner);

  nsRefPtr<DesktopNotification> notification =
    new DesktopNotification(aTitle,
                            aDescription,
                            aIconURL,
                            mOwner,
                            mPrincipal);
  notification->Init();
  return notification.forget();
}

JSObject*
DesktopNotificationCenter::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return DesktopNotificationCenterBinding::Wrap(aCx, aScope, this);
}





NS_IMPL_ISUPPORTS2(DesktopNotificationRequest,
                   nsIContentPermissionRequest,
                   nsIRunnable)

NS_IMETHODIMP
DesktopNotificationRequest::GetPrincipal(nsIPrincipal * *aRequestingPrincipal)
{
  if (!mDesktopNotification) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_IF_ADDREF(*aRequestingPrincipal = mDesktopNotification->mPrincipal);
  return NS_OK;
}

NS_IMETHODIMP
DesktopNotificationRequest::GetWindow(nsIDOMWindow * *aRequestingWindow)
{
  if (!mDesktopNotification) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  NS_IF_ADDREF(*aRequestingWindow = mDesktopNotification->GetOwner());
  return NS_OK;
}

NS_IMETHODIMP
DesktopNotificationRequest::GetElement(nsIDOMElement * *aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  *aElement = nullptr;
  return NS_OK;
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
