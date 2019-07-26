



#include "PCOMContentPermissionRequestChild.h"
#include "mozilla/dom/Notification.h"
#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/Preferences.h"
#include "TabChild.h"
#include "nsContentUtils.h"
#include "nsDOMEvent.h"
#include "nsIAlertsService.h"
#include "nsIContentPermissionPrompt.h"
#include "nsIDocument.h"
#include "nsIPermissionManager.h"
#include "nsServiceManagerUtils.h"
#include "nsToolkitCompsCID.h"
#include "nsGlobalWindow.h"
#include "nsDOMJSUtils.h"

namespace mozilla {
namespace dom {

class NotificationPermissionRequest : public nsIContentPermissionRequest,
                                      public PCOMContentPermissionRequestChild,
                                      public nsIRunnable
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_NSICONTENTPERMISSIONREQUEST
  NS_DECL_NSIRUNNABLE
  NS_DECL_CYCLE_COLLECTION_CLASS_AMBIGUOUS(NotificationPermissionRequest,
                                           nsIContentPermissionRequest)

  NotificationPermissionRequest(nsIPrincipal* aPrincipal, nsPIDOMWindow* aWindow,
                                NotificationPermissionCallback* aCallback)
    : mPrincipal(aPrincipal), mWindow(aWindow),
      mPermission(NotificationPermission::Default),
      mCallback(aCallback) {}

  virtual ~NotificationPermissionRequest() {}

  bool Recv__delete__(const bool& aAllow);
  void IPDLRelease() { Release(); }

protected:
  nsresult CallCallback();
  nsresult DispatchCallback();
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  NotificationPermission mPermission;
  nsRefPtr<NotificationPermissionCallback> mCallback;
};

class NotificationObserver : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  NotificationObserver(Notification* aNotification)
    : mNotification(aNotification) {}

  virtual ~NotificationObserver() {}

protected:
  nsRefPtr<Notification> mNotification;
};

class NotificationTask : public nsIRunnable
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIRUNNABLE

  enum NotificationAction {
    eShow,
    eClose
  };

  NotificationTask(Notification* aNotification, NotificationAction aAction)
    : mNotification(aNotification), mAction(aAction) {}

  virtual ~NotificationTask() {}

protected:
  nsRefPtr<Notification> mNotification;
  NotificationAction mAction;
};

uint32_t Notification::sCount = 0;

NS_IMPL_CYCLE_COLLECTION_1(NotificationPermissionRequest, mWindow)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(NotificationPermissionRequest)
  NS_INTERFACE_MAP_ENTRY(nsIContentPermissionRequest)
  NS_INTERFACE_MAP_ENTRY(nsIRunnable)
  NS_INTERFACE_MAP_ENTRY_AMBIGUOUS(nsISupports, nsIContentPermissionRequest)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTING_ADDREF(NotificationPermissionRequest)
NS_IMPL_CYCLE_COLLECTING_RELEASE(NotificationPermissionRequest)

NS_IMETHODIMP
NotificationPermissionRequest::Run()
{
  if (nsContentUtils::IsSystemPrincipal(mPrincipal)) {
    mPermission = NotificationPermission::Granted;
  } else {
    
    nsCOMPtr<nsIURI> uri;
    mPrincipal->GetURI(getter_AddRefs(uri));

    if (uri) {
      bool isFile;
      uri->SchemeIs("file", &isFile);
      if (isFile) {
        mPermission = NotificationPermission::Granted;
      }
    }
  }

  
  if (Preferences::GetBool("notification.prompt.testing", false)) {
    if (Preferences::GetBool("notification.prompt.testing.allow", true)) {
      mPermission = NotificationPermission::Granted;
    } else {
      mPermission = NotificationPermission::Denied;
    }
  }

  if (mPermission != NotificationPermission::Default) {
    return DispatchCallback();
  }

  if (XRE_GetProcessType() == GeckoProcessType_Content) {
    
    
    TabChild* child = GetTabChildFrom(mWindow->GetDocShell());
    if (!child) {
      return NS_ERROR_NOT_AVAILABLE;
    }

    
    
    AddRef();

    NS_NAMED_LITERAL_CSTRING(type, "desktop-notification");
    NS_NAMED_LITERAL_CSTRING(access, "unused");
    child->SendPContentPermissionRequestConstructor(this, type, access,
                                                    IPC::Principal(mPrincipal));

    Sendprompt();
    return NS_OK;
  }

  nsCOMPtr<nsIContentPermissionPrompt> prompt =
    do_GetService(NS_CONTENT_PERMISSION_PROMPT_CONTRACTID);
  if (prompt) {
    prompt->Prompt(this);
  }

  return NS_OK;
}

NS_IMETHODIMP
NotificationPermissionRequest::GetPrincipal(nsIPrincipal** aRequestingPrincipal)
{
  NS_ADDREF(*aRequestingPrincipal = mPrincipal);
  return NS_OK;
}

NS_IMETHODIMP
NotificationPermissionRequest::GetWindow(nsIDOMWindow** aRequestingWindow)
{
  NS_ADDREF(*aRequestingWindow = mWindow);
  return NS_OK;
}

NS_IMETHODIMP
NotificationPermissionRequest::GetElement(nsIDOMElement** aElement)
{
  NS_ENSURE_ARG_POINTER(aElement);
  *aElement = nullptr;
  return NS_OK;
}

NS_IMETHODIMP
NotificationPermissionRequest::Cancel()
{
  mPermission = NotificationPermission::Denied;
  return DispatchCallback();
}

NS_IMETHODIMP
NotificationPermissionRequest::Allow()
{
  mPermission = NotificationPermission::Granted;
  return DispatchCallback();
}

inline nsresult
NotificationPermissionRequest::DispatchCallback()
{
  if (!mCallback) {
    return NS_OK;
  }

  nsCOMPtr<nsIRunnable> callbackRunnable = NS_NewRunnableMethod(this,
    &NotificationPermissionRequest::CallCallback);
  return NS_DispatchToMainThread(callbackRunnable);
}

nsresult
NotificationPermissionRequest::CallCallback()
{
  ErrorResult rv;
  mCallback->Call(mPermission, rv);
  return rv.ErrorCode();
}

NS_IMETHODIMP
NotificationPermissionRequest::GetAccess(nsACString& aAccess)
{
  aAccess.AssignLiteral("unused");
  return NS_OK;
}

NS_IMETHODIMP
NotificationPermissionRequest::GetType(nsACString& aType)
{
  aType.AssignLiteral("desktop-notification");
  return NS_OK;
}

bool
NotificationPermissionRequest::Recv__delete__(const bool& aAllow)
{
  if (aAllow) {
    (void) Allow();
  } else {
    (void) Cancel();
  }
  return true;
}

NS_IMPL_ISUPPORTS1(NotificationTask, nsIRunnable)

NS_IMETHODIMP
NotificationTask::Run()
{
  switch (mAction) {
  case eShow:
    return mNotification->ShowInternal();
  case eClose:
    return mNotification->CloseInternal();
  default:
    MOZ_CRASH("Unexpected action for NotificationTask.");
  }
}

NS_IMPL_ISUPPORTS1(NotificationObserver, nsIObserver)

NS_IMETHODIMP
NotificationObserver::Observe(nsISupports* aSubject, const char* aTopic,
                              const PRUnichar* aData)
{
  if (!strcmp("alertclickcallback", aTopic)) {
    mNotification->DispatchTrustedEvent(NS_LITERAL_STRING("click"));
  } else if (!strcmp("alertfinished", aTopic)) {
    mNotification->mIsClosed = true;
    mNotification->DispatchTrustedEvent(NS_LITERAL_STRING("close"));
  } else if (!strcmp("alertshow", aTopic)) {
    mNotification->DispatchTrustedEvent(NS_LITERAL_STRING("show"));
  }

  return NS_OK;
}

Notification::Notification(const nsAString& aTitle, const nsAString& aBody,
                           NotificationDirection aDir, const nsAString& aLang,
                           const nsAString& aTag, const nsAString& aIconUrl)
  : mTitle(aTitle), mBody(aBody), mDir(aDir), mLang(aLang),
    mTag(aTag), mIconUrl(aIconUrl), mIsClosed(false)
{
  SetIsDOMBinding();
}

already_AddRefed<Notification>
Notification::Constructor(const GlobalObject& aGlobal,
                          const nsAString& aTitle,
                          const NotificationOptions& aOptions,
                          ErrorResult& aRv)
{
  nsString tag;
  if (aOptions.mTag.WasPassed()) {
    tag.Append(NS_LITERAL_STRING("tag:"));
    tag.Append(aOptions.mTag.Value());
  } else {
    tag.Append(NS_LITERAL_STRING("notag:"));
    tag.AppendInt(sCount++);
  }

  nsRefPtr<Notification> notification = new Notification(aTitle,
                                                         aOptions.mBody,
                                                         aOptions.mDir,
                                                         aOptions.mLang,
                                                         tag,
                                                         aOptions.mIcon);

  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  MOZ_ASSERT(window, "Window should not be null.");
  notification->BindToOwner(window);

  
  nsCOMPtr<nsIRunnable> showNotificationTask =
    new NotificationTask(notification, NotificationTask::eShow);
  NS_DispatchToMainThread(showNotificationTask);

  return notification.forget();
}

nsresult
Notification::ShowInternal()
{
  nsCOMPtr<nsIAlertsService> alertService =
    do_GetService(NS_ALERTSERVICE_CONTRACTID);

  ErrorResult result;
  if (GetPermissionInternal(GetOwner(), result) !=
    NotificationPermission::Granted || !alertService) {
    
    
    return DispatchTrustedEvent(NS_LITERAL_STRING("error"));
  }

  nsresult rv;
  nsAutoString absoluteUrl;
  if (mIconUrl.Length() > 0) {
    
    nsIDocument* doc = GetOwner()->GetExtantDoc();
    NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);
    nsCOMPtr<nsIURI> baseUri = doc->GetBaseURI();
    NS_ENSURE_TRUE(baseUri, NS_ERROR_UNEXPECTED);
    nsCOMPtr<nsIURI> srcUri;
    rv = nsContentUtils::NewURIWithDocumentCharset(getter_AddRefs(srcUri),
                                                   mIconUrl, doc, baseUri);
    NS_ENSURE_SUCCESS(rv, rv);
    if (srcUri) {
      nsAutoCString src;
      srcUri->GetSpec(src);
      absoluteUrl = NS_ConvertUTF8toUTF16(src);
    }
  }

  nsString alertName;
  rv = GetAlertName(alertName);
  NS_ENSURE_SUCCESS(rv, rv);

  
  
  nsString uniqueCookie = NS_LITERAL_STRING("notification:");
  uniqueCookie.AppendInt(sCount++);
  nsCOMPtr<nsIObserver> observer = new NotificationObserver(this);
  return alertService->ShowAlertNotification(absoluteUrl, mTitle, mBody, true,
                                             uniqueCookie, observer, alertName,
                                             DirectionToString(mDir), mLang);
}

void
Notification::RequestPermission(const GlobalObject& aGlobal,
                                const Optional<OwningNonNull<NotificationPermissionCallback> >& aCallback,
                                ErrorResult& aRv)
{
  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aGlobal.GetAsSupports());
  if (!sop) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return;
  }
  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();

  NotificationPermissionCallback* permissionCallback = nullptr;
  if (aCallback.WasPassed()) {
    permissionCallback = &aCallback.Value();
  }
  nsCOMPtr<nsIRunnable> request =
    new NotificationPermissionRequest(principal, window, permissionCallback);

  NS_DispatchToMainThread(request);
}

NotificationPermission
Notification::GetPermission(const GlobalObject& aGlobal, ErrorResult& aRv)
{
  return GetPermissionInternal(aGlobal.GetAsSupports(), aRv);
}

NotificationPermission
Notification::GetPermissionInternal(nsISupports* aGlobal, ErrorResult& aRv)
{
  
  nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aGlobal);
  if (!sop) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return NotificationPermission::Denied;
  }

  nsCOMPtr<nsIPrincipal> principal = sop->GetPrincipal();
  if (nsContentUtils::IsSystemPrincipal(principal)) {
    return NotificationPermission::Granted;
  } else {
    
    nsCOMPtr<nsIURI> uri;
    principal->GetURI(getter_AddRefs(uri));
    if (uri) {
      bool isFile;
      uri->SchemeIs("file", &isFile);
      if (isFile) {
        return NotificationPermission::Granted;
      }
    }
  }

  
  if (Preferences::GetBool("notification.prompt.testing", false)) {
    if (Preferences::GetBool("notification.prompt.testing.allow", true)) {
      return NotificationPermission::Granted;
    } else {
      return NotificationPermission::Denied;
    }
  }

  uint32_t permission = nsIPermissionManager::UNKNOWN_ACTION;

  nsCOMPtr<nsIPermissionManager> permissionManager =
    do_GetService(NS_PERMISSIONMANAGER_CONTRACTID);

  permissionManager->TestPermissionFromPrincipal(principal,
                                                 "desktop-notification",
                                                 &permission);

  
  switch (permission) {
  case nsIPermissionManager::ALLOW_ACTION:
    return NotificationPermission::Granted;
  case nsIPermissionManager::DENY_ACTION:
    return NotificationPermission::Denied;
  default:
    return NotificationPermission::Default;
  }
}

bool
Notification::PrefEnabled()
{
  return Preferences::GetBool("dom.webnotifications.enabled", false);
}

JSObject*
Notification::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aScope)
{
  return mozilla::dom::NotificationBinding::Wrap(aCx, aScope, this);
}

void
Notification::Close()
{
  
  nsCOMPtr<nsIRunnable> showNotificationTask =
    new NotificationTask(this, NotificationTask::eClose);
  NS_DispatchToMainThread(showNotificationTask);
}

nsresult
Notification::CloseInternal()
{
  if (!mIsClosed) {
    nsCOMPtr<nsIAlertsService> alertService =
      do_GetService(NS_ALERTSERVICE_CONTRACTID);

    if (alertService) {
      nsString alertName;
      nsresult rv = GetAlertName(alertName);
      NS_ENSURE_SUCCESS(rv, rv);

      rv = alertService->CloseAlert(alertName);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  return NS_OK;
}

nsresult
Notification::GetAlertName(nsString& aAlertName)
{
  
  

  nsPIDOMWindow* owner = GetOwner();
  NS_ENSURE_TRUE(owner, NS_ERROR_UNEXPECTED);

  nsIDocument* doc = owner->GetExtantDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);

  nsresult rv = nsContentUtils::GetUTFOrigin(doc->NodePrincipal(),
                                             aAlertName);
  NS_ENSURE_SUCCESS(rv, rv);
  aAlertName.AppendLiteral("#");
  aAlertName.Append(mTag);
  return NS_OK;
}

} 
} 

