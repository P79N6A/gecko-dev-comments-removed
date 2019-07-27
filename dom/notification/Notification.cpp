





#include "mozilla/dom/Notification.h"
#include "mozilla/dom/AppNotificationServiceOptionsBinding.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/PromiseWorkerProxy.h"
#include "mozilla/Move.h"
#include "mozilla/Preferences.h"
#include "mozilla/unused.h"
#include "nsContentUtils.h"
#include "nsIAlertsService.h"
#include "nsIAppsService.h"
#include "nsIContentPermissionPrompt.h"
#include "nsIDocument.h"
#include "nsINotificationStorage.h"
#include "nsIPermissionManager.h"
#include "nsIServiceWorkerManager.h"
#include "nsIUUIDGenerator.h"
#include "nsServiceManagerUtils.h"
#include "nsStructuredCloneContainer.h"
#include "nsToolkitCompsCID.h"
#include "nsGlobalWindow.h"
#include "nsDOMJSUtils.h"
#include "nsProxyRelease.h"
#include "nsNetUtil.h"
#include "nsIScriptSecurityManager.h"
#include "nsIXPConnect.h"
#include "mozilla/dom/ServiceWorkerGlobalScopeBinding.h"
#include "mozilla/dom/NotificationEvent.h"
#include "mozilla/dom/PermissionMessageUtils.h"
#include "mozilla/Services.h"
#include "nsContentPermissionHelper.h"
#include "nsILoadContext.h"
#ifdef MOZ_B2G
#include "nsIDOMDesktopNotification.h"
#endif

#include "ServiceWorkerManager.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"
#include "WorkerScope.h"

namespace mozilla {
namespace dom {

using namespace workers;

struct NotificationStrings
{
  const nsString mID;
  const nsString mTitle;
  const nsString mDir;
  const nsString mLang;
  const nsString mBody;
  const nsString mTag;
  const nsString mIcon;
  const nsString mData;
  const nsString mBehavior;
  const nsString mServiceWorkerRegistrationID;
};

class ScopeCheckingGetCallback : public nsINotificationStorageCallback
{
  const nsString mScope;
public:
  NS_DECL_ISUPPORTS

  explicit ScopeCheckingGetCallback(const nsAString& aScope)
    : mScope(aScope)
  {}

  NS_IMETHOD Handle(const nsAString& aID,
                    const nsAString& aTitle,
                    const nsAString& aDir,
                    const nsAString& aLang,
                    const nsAString& aBody,
                    const nsAString& aTag,
                    const nsAString& aIcon,
                    const nsAString& aData,
                    const nsAString& aBehavior,
                    const nsAString& aServiceWorkerRegistrationID,
                    JSContext* aCx) final
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(!aID.IsEmpty());

    
    if (!mScope.IsEmpty() && !mScope.Equals(aServiceWorkerRegistrationID)) {
      return NS_OK;
    }

    NotificationStrings strings = {
      nsString(aID),
      nsString(aTitle),
      nsString(aDir),
      nsString(aLang),
      nsString(aBody),
      nsString(aTag),
      nsString(aIcon),
      nsString(aData),
      nsString(aBehavior),
      nsString(aServiceWorkerRegistrationID),
    };

    mStrings.AppendElement(Move(strings));
    return NS_OK;
  }

  NS_IMETHOD Done(JSContext* aCx) override = 0;

protected:
  virtual ~ScopeCheckingGetCallback()
  {}

  nsTArray<NotificationStrings> mStrings;
};

NS_IMPL_ISUPPORTS(ScopeCheckingGetCallback, nsINotificationStorageCallback)

class NotificationStorageCallback final : public ScopeCheckingGetCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_CLASS(NotificationStorageCallback)

  NotificationStorageCallback(nsIGlobalObject* aWindow, const nsAString& aScope,
                              Promise* aPromise)
    : ScopeCheckingGetCallback(aScope),
      mWindow(aWindow),
      mPromise(aPromise)
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(aWindow);
    MOZ_ASSERT(aPromise);
  }

  NS_IMETHOD Done(JSContext* aCx) final
  {
    AutoJSAPI jsapi;
    DebugOnly<bool> ok = jsapi.Init(mWindow, aCx);
    MOZ_ASSERT(ok);

    ErrorResult result;
    nsAutoTArray<nsRefPtr<Notification>, 5> notifications;

    for (uint32_t i = 0; i < mStrings.Length(); ++i) {
      nsRefPtr<Notification> n =
        Notification::ConstructFromFields(mWindow,
                                          mStrings[i].mID,
                                          mStrings[i].mTitle,
                                          mStrings[i].mDir,
                                          mStrings[i].mLang,
                                          mStrings[i].mBody,
                                          mStrings[i].mTag,
                                          mStrings[i].mIcon,
                                          mStrings[i].mData,
                                          

                                          mStrings[i].mServiceWorkerRegistrationID,
                                          result);

      n->SetStoredState(true);
      unused << NS_WARN_IF(result.Failed());
      if (!result.Failed()) {
        notifications.AppendElement(n.forget());
      }
    }

    mPromise->MaybeResolve(notifications);
    return NS_OK;
  }

private:
  virtual ~NotificationStorageCallback()
  {}

  nsCOMPtr<nsIGlobalObject> mWindow;
  nsRefPtr<Promise> mPromise;
  const nsString mScope;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(NotificationStorageCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(NotificationStorageCallback)
NS_IMPL_CYCLE_COLLECTION(NotificationStorageCallback, mWindow, mPromise);

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(NotificationStorageCallback)
NS_INTERFACE_MAP_END_INHERITING(ScopeCheckingGetCallback)

class NotificationGetRunnable final : public nsRunnable
{
  const nsString mOrigin;
  const nsString mTag;
  nsCOMPtr<nsINotificationStorageCallback> mCallback;
public:
  NotificationGetRunnable(const nsAString& aOrigin,
                          const nsAString& aTag,
                          nsINotificationStorageCallback* aCallback)
    : mOrigin(aOrigin), mTag(aTag), mCallback(aCallback)
  {}

  NS_IMETHOD
  Run() override
  {
    nsresult rv;
    nsCOMPtr<nsINotificationStorage> notificationStorage =
      do_GetService(NS_NOTIFICATION_STORAGE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = notificationStorage->Get(mOrigin, mTag, mCallback);
    
    unused << NS_WARN_IF(NS_FAILED(rv));
    return rv;
  }
};

class NotificationPermissionRequest : public nsIContentPermissionRequest,
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
      mCallback(aCallback)
  {
    mRequester = new nsContentPermissionRequester(mWindow);
  }

protected:
  virtual ~NotificationPermissionRequest() {}

  nsresult CallCallback();
  nsresult DispatchCallback();
  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  NotificationPermission mPermission;
  nsRefPtr<NotificationPermissionCallback> mCallback;
  nsCOMPtr<nsIContentPermissionRequester> mRequester;
};

namespace {
class ReleaseNotificationControlRunnable final : public MainThreadWorkerControlRunnable
{
  Notification* mNotification;

public:
  explicit ReleaseNotificationControlRunnable(Notification* aNotification)
    : MainThreadWorkerControlRunnable(aNotification->mWorkerPrivate)
    , mNotification(aNotification)
  { }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    mNotification->ReleaseObject();
    return true;
  }
};

class GetPermissionRunnable final : public WorkerMainThreadRunnable
{
  NotificationPermission mPermission;

public:
  explicit GetPermissionRunnable(WorkerPrivate* aWorker)
    : WorkerMainThreadRunnable(aWorker)
    , mPermission(NotificationPermission::Denied)
  { }

  bool
  MainThreadRun() override
  {
    ErrorResult result;
    mPermission =
      Notification::GetPermissionInternal(mWorkerPrivate->GetPrincipal(),
                                          result);
    return true;
  }

  NotificationPermission
  GetPermission()
  {
    return mPermission;
  }
};

class FocusWindowRunnable final : public nsRunnable
{
  nsMainThreadPtrHandle<nsPIDOMWindow> mWindow;
public:
  explicit FocusWindowRunnable(const nsMainThreadPtrHandle<nsPIDOMWindow>& aWindow)
    : mWindow(aWindow)
  { }

  NS_IMETHOD
  Run()
  {
    AssertIsOnMainThread();
    if (!mWindow->IsCurrentInnerWindow()) {
      
      return NS_OK;
    }

    nsIDocument* doc = mWindow->GetExtantDoc();
    if (doc) {
      
      
      nsContentUtils::DispatchChromeEvent(doc, mWindow->GetOuterWindow(),
                                          NS_LITERAL_STRING("DOMWebNotificationClicked"),
                                          true, true);
    }

    return NS_OK;
  }
};

nsresult
CheckScope(nsIPrincipal* aPrincipal, const nsACString& aScope)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aPrincipal);

  nsCOMPtr<nsIURI> scopeURI;
  nsresult rv = NS_NewURI(getter_AddRefs(scopeURI), aScope, nullptr, nullptr);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    return rv;
  }

  return aPrincipal->CheckMayLoad(scopeURI,  true,
                                   false);
}
} 



class NotificationWorkerRunnable : public WorkerRunnable
{
protected:
  explicit NotificationWorkerRunnable(WorkerPrivate* aWorkerPrivate)
    : WorkerRunnable(aWorkerPrivate, WorkerThreadUnchangedBusyCount)
  {
  }

  bool
  PreDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    return true;
  }

  void
  PostDispatch(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
               bool aDispatchResult) override
  {
  }

  bool
  WorkerRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    aWorkerPrivate->AssertIsOnWorkerThread();
    aWorkerPrivate->ModifyBusyCountFromWorker(aCx, true);
    WorkerRunInternal(aCx, aWorkerPrivate);
    return true;
  }

  void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
          bool aRunResult) override
  {
    aWorkerPrivate->ModifyBusyCountFromWorker(aCx, false);
  }

  virtual void
  WorkerRunInternal(JSContext* aCx, WorkerPrivate* aWorkerPrivate) = 0;
};



class NotificationEventWorkerRunnable final : public NotificationWorkerRunnable
{
  Notification* mNotification;
  const nsString mEventName;
public:
  NotificationEventWorkerRunnable(Notification* aNotification,
                                  const nsString& aEventName)
    : NotificationWorkerRunnable(aNotification->mWorkerPrivate)
    , mNotification(aNotification)
    , mEventName(aEventName)
  {}

  void
  WorkerRunInternal(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    mNotification->DispatchTrustedEvent(mEventName);
  }
};



class NotificationRef final {
  friend class WorkerNotificationObserver;

private:
  Notification* mNotification;
  bool mInited;

  
  void
  Forget()
  {
    mNotification = nullptr;
  }

public:
  explicit NotificationRef(Notification* aNotification)
    : mNotification(aNotification)
  {
    MOZ_ASSERT(mNotification);
    if (mNotification->mWorkerPrivate) {
      mNotification->mWorkerPrivate->AssertIsOnWorkerThread();
    } else {
      AssertIsOnMainThread();
    }

    mInited = mNotification->AddRefObject();
  }

  
  
  
  
  bool
  Initialized()
  {
    return mInited;
  }

  ~NotificationRef()
  {
    if (Initialized() && mNotification) {
      if (mNotification->mWorkerPrivate && NS_IsMainThread()) {
        nsRefPtr<ReleaseNotificationControlRunnable> r =
          new ReleaseNotificationControlRunnable(mNotification);
        AutoSafeJSContext cx;
        if (!r->Dispatch(cx)) {
          MOZ_CRASH("Will leak worker thread Notification!");
        }
      } else {
        mNotification->AssertIsOnTargetThread();
        mNotification->ReleaseObject();
      }
      mNotification = nullptr;
    }
  }

  
  
  Notification*
  GetNotification()
  {
    MOZ_ASSERT(Initialized());
    return mNotification;
  }
};

class NotificationTask : public nsRunnable
{
public:
  enum NotificationAction {
    eShow,
    eClose
  };

  NotificationTask(UniquePtr<NotificationRef> aRef, NotificationAction aAction)
    : mNotificationRef(Move(aRef)), mAction(aAction)
  {}

  NS_IMETHOD
  Run() override;
protected:
  virtual ~NotificationTask() {}

  UniquePtr<NotificationRef> mNotificationRef;
  NotificationAction mAction;
};

uint32_t Notification::sCount = 0;

NS_IMPL_CYCLE_COLLECTION(NotificationPermissionRequest, mWindow)

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

  return nsContentPermissionUtils::AskPermission(this, mWindow);
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
NotificationPermissionRequest::Allow(JS::HandleValue aChoices)
{
  MOZ_ASSERT(aChoices.isUndefined());

  mPermission = NotificationPermission::Granted;
  return DispatchCallback();
}

NS_IMETHODIMP
NotificationPermissionRequest::GetRequester(nsIContentPermissionRequester** aRequester)
{
  NS_ENSURE_ARG_POINTER(aRequester);

  nsCOMPtr<nsIContentPermissionRequester> requester = mRequester;
  requester.forget(aRequester);
  return NS_OK;
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
  return rv.StealNSResult();
}

NS_IMETHODIMP
NotificationPermissionRequest::GetTypes(nsIArray** aTypes)
{
  nsTArray<nsString> emptyOptions;
  return nsContentPermissionUtils::CreatePermissionArray(NS_LITERAL_CSTRING("desktop-notification"),
                                                         NS_LITERAL_CSTRING("unused"),
                                                         emptyOptions,
                                                         aTypes);
}

class NotificationObserver : public nsIObserver
{
public:
  UniquePtr<NotificationRef> mNotificationRef;
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  explicit NotificationObserver(UniquePtr<NotificationRef> aRef)
    : mNotificationRef(Move(aRef))
  {
    AssertIsOnMainThread();
  }

protected:
  virtual ~NotificationObserver()
  {
    AssertIsOnMainThread();
  }
};

NS_IMPL_ISUPPORTS(NotificationObserver, nsIObserver)

NS_IMETHODIMP
NotificationTask::Run()
{
  AssertIsOnMainThread();

  
  
  
  Notification* notif = mNotificationRef->GetNotification();
  notif->mTempRef.swap(mNotificationRef);
  if (mAction == eShow) {
    notif->ShowInternal();
  } else if (mAction == eClose) {
    notif->CloseInternal();
  } else {
    MOZ_CRASH("Invalid action");
  }

  MOZ_ASSERT(!mNotificationRef);
  return NS_OK;
}


bool
Notification::PrefEnabled(JSContext* aCx, JSObject* aObj)
{
  if (NS_IsMainThread()) {
    return Preferences::GetBool("dom.webnotifications.enabled", false);
  }

  WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
  if (!workerPrivate) {
    return false;
  }

  return workerPrivate->DOMWorkerNotificationEnabled();
}


bool
Notification::IsGetEnabled(JSContext* aCx, JSObject* aObj)
{
  return NS_IsMainThread();
}

Notification::Notification(nsIGlobalObject* aGlobal, const nsAString& aID,
                           const nsAString& aTitle, const nsAString& aBody,
                           NotificationDirection aDir, const nsAString& aLang,
                           const nsAString& aTag, const nsAString& aIconUrl,
                           const NotificationBehavior& aBehavior)
  : DOMEventTargetHelper(),
    mWorkerPrivate(nullptr), mObserver(nullptr),
    mID(aID), mTitle(aTitle), mBody(aBody), mDir(aDir), mLang(aLang),
    mTag(aTag), mIconUrl(aIconUrl), mBehavior(aBehavior), mIsClosed(false),
    mIsStored(false), mTaskCount(0)
{
  if (NS_IsMainThread()) {
    
    
    
    
    
    BindToOwner(aGlobal);
  } else {
    mWorkerPrivate = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(mWorkerPrivate);
  }
}

void
Notification::SetAlertName()
{
  AssertIsOnMainThread();
  if (!mAlertName.IsEmpty()) {
    return;
  }

  nsAutoString alertName;
  DebugOnly<nsresult> rv = GetOrigin(GetPrincipal(), alertName);
  MOZ_ASSERT(NS_SUCCEEDED(rv), "GetOrigin should not have failed");

  
  
  alertName.Append('#');
  if (!mTag.IsEmpty()) {
    alertName.AppendLiteral("tag:");
    alertName.Append(mTag);
  } else {
    alertName.AppendLiteral("notag:");
    alertName.Append(mID);
  }

  mAlertName = alertName;
}



already_AddRefed<Notification>
Notification::Constructor(const GlobalObject& aGlobal,
                          const nsAString& aTitle,
                          const NotificationOptions& aOptions,
                          ErrorResult& aRv)
{
  
  ServiceWorkerGlobalScope* scope = nullptr;
  UNWRAP_WORKER_OBJECT(ServiceWorkerGlobalScope, aGlobal.Get(), scope);
  if (scope) {
    aRv.ThrowTypeError(MSG_NOTIFICATION_NO_CONSTRUCTOR_IN_SERVICEWORKER);
    return nullptr;
  }

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<Notification> notification =
    CreateAndShow(global, aTitle, aOptions, EmptyString(), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  
  
  return notification.forget();
}


already_AddRefed<Notification>
Notification::ConstructFromFields(
    nsIGlobalObject* aGlobal,
    const nsAString& aID,
    const nsAString& aTitle,
    const nsAString& aDir,
    const nsAString& aLang,
    const nsAString& aBody,
    const nsAString& aTag,
    const nsAString& aIcon,
    const nsAString& aData,
    const nsAString& aServiceWorkerRegistrationID,
    ErrorResult& aRv)
{
  MOZ_ASSERT(aGlobal);

  AutoJSAPI jsapi;
  DebugOnly<bool> ok = jsapi.Init(aGlobal);
  MOZ_ASSERT(ok);

  RootedDictionary<NotificationOptions> options(jsapi.cx());
  options.mDir = Notification::StringToDirection(nsString(aDir));
  options.mLang = aLang;
  options.mBody = aBody;
  options.mTag = aTag;
  options.mIcon = aIcon;
  nsRefPtr<Notification> notification = CreateInternal(aGlobal, aID, aTitle,
                                                       options);

  notification->InitFromBase64(jsapi.cx(), aData, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  notification->SetScope(aServiceWorkerRegistrationID);

  return notification.forget();
}

nsresult
Notification::PersistNotification()
{
  AssertIsOnMainThread();
  nsresult rv;
  nsCOMPtr<nsINotificationStorage> notificationStorage =
    do_GetService(NS_NOTIFICATION_STORAGE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    return rv;
  }

  nsString origin;
  rv = GetOrigin(GetPrincipal(), origin);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  nsString id;
  GetID(id);

  nsString alertName;
  GetAlertName(alertName);

  nsString dataString;
  nsCOMPtr<nsIStructuredCloneContainer> scContainer;
  scContainer = GetDataCloneContainer();
  if (scContainer) {
    scContainer->GetDataAsBase64(dataString);
  }

  nsAutoString behavior;
  if (!mBehavior.ToJSON(behavior)) {
    return NS_ERROR_FAILURE;
  }

  rv = notificationStorage->Put(origin,
                                id,
                                mTitle,
                                DirectionToString(mDir),
                                mLang,
                                mBody,
                                mTag,
                                mIconUrl,
                                alertName,
                                dataString,
                                behavior,
                                mScope);

  if (NS_FAILED(rv)) {
    return rv;
  }

  SetStoredState(true);
  return NS_OK;
}

void
Notification::UnpersistNotification()
{
  AssertIsOnMainThread();
  if (IsStored()) {
    nsCOMPtr<nsINotificationStorage> notificationStorage =
      do_GetService(NS_NOTIFICATION_STORAGE_CONTRACTID);
    if (notificationStorage) {
      nsString origin;
      nsresult rv = GetOrigin(GetPrincipal(), origin);
      if (NS_SUCCEEDED(rv)) {
        notificationStorage->Delete(origin, mID);
      }
    }
    SetStoredState(false);
  }
}

already_AddRefed<Notification>
Notification::CreateInternal(nsIGlobalObject* aGlobal,
                             const nsAString& aID,
                             const nsAString& aTitle,
                             const NotificationOptions& aOptions)
{
  nsString id;
  if (!aID.IsEmpty()) {
    id = aID;
  } else {
    nsCOMPtr<nsIUUIDGenerator> uuidgen =
      do_GetService("@mozilla.org/uuid-generator;1");
    NS_ENSURE_TRUE(uuidgen, nullptr);
    nsID uuid;
    nsresult rv = uuidgen->GenerateUUIDInPlace(&uuid);
    NS_ENSURE_SUCCESS(rv, nullptr);

    char buffer[NSID_LENGTH];
    uuid.ToProvidedString(buffer);
    NS_ConvertASCIItoUTF16 convertedID(buffer);
    id = convertedID;
  }

  nsRefPtr<Notification> notification = new Notification(aGlobal, id, aTitle,
                                                         aOptions.mBody,
                                                         aOptions.mDir,
                                                         aOptions.mLang,
                                                         aOptions.mTag,
                                                         aOptions.mIcon,
                                                         aOptions.mMozbehavior);
  return notification.forget();
}

Notification::~Notification()
{
  AssertIsOnTargetThread();
  MOZ_ASSERT(!mFeature);
  MOZ_ASSERT(!mTempRef);
}

NS_IMPL_CYCLE_COLLECTION_CLASS(Notification)
NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(Notification, DOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mData)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mDataObjectContainer)
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(Notification, DOMEventTargetHelper)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mData)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mDataObjectContainer)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_ADDREF_INHERITED(Notification, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(Notification, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(Notification)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

nsIPrincipal*
Notification::GetPrincipal()
{
  AssertIsOnMainThread();
  if (mWorkerPrivate) {
    return mWorkerPrivate->GetPrincipal();
  } else {
    nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(GetOwner());
    NS_ENSURE_TRUE(sop, nullptr);
    return sop->GetPrincipal();
  }
}

class WorkerNotificationObserver final : public NotificationObserver
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIOBSERVER

  explicit WorkerNotificationObserver(UniquePtr<NotificationRef> aRef)
    : NotificationObserver(Move(aRef))
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(mNotificationRef->GetNotification()->mWorkerPrivate);
  }

  void
  ForgetNotification()
  {
    AssertIsOnMainThread();
    mNotificationRef->Forget();
  }

protected:
  virtual ~WorkerNotificationObserver()
  {
    AssertIsOnMainThread();

    MOZ_ASSERT(mNotificationRef);
    Notification* notification = mNotificationRef->GetNotification();
    if (!notification) {
      return;
    }

    
    
    
    
    
    
    
    
    class ReleaseNotificationRunnable final : public NotificationWorkerRunnable
    {
      UniquePtr<NotificationRef> mNotificationRef;
    public:
      explicit ReleaseNotificationRunnable(UniquePtr<NotificationRef> aRef)
        : NotificationWorkerRunnable(aRef->GetNotification()->mWorkerPrivate)
        , mNotificationRef(Move(aRef))
      {}

      void
      WorkerRunInternal(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
      {
        UniquePtr<NotificationRef> ref;
        mozilla::Swap(ref, mNotificationRef);
        
      }
    };

    nsRefPtr<ReleaseNotificationRunnable> r =
      new ReleaseNotificationRunnable(Move(mNotificationRef));
    notification = nullptr;

    AutoJSAPI jsapi;
    jsapi.Init();
    r->Dispatch(jsapi.cx());
  }
};

NS_IMPL_ISUPPORTS_INHERITED0(WorkerNotificationObserver, NotificationObserver)

class ServiceWorkerNotificationObserver final : public nsIObserver
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIOBSERVER

  ServiceWorkerNotificationObserver(const nsAString& aScope,
                                    nsIPrincipal* aPrincipal,
                                    const nsAString& aID)
    : mScope(aScope), mID(aID), mPrincipal(aPrincipal)
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(aPrincipal);
  }

private:
  ~ServiceWorkerNotificationObserver()
  {}

  const nsString mScope;
  const nsString mID;
  nsCOMPtr<nsIPrincipal> mPrincipal;
};

NS_IMPL_ISUPPORTS(ServiceWorkerNotificationObserver, nsIObserver)


bool
Notification::DispatchNotificationClickEvent()
{
  MOZ_ASSERT(mWorkerPrivate);
  MOZ_ASSERT(mWorkerPrivate->IsServiceWorker());
  mWorkerPrivate->AssertIsOnWorkerThread();

  NotificationEventInit options;
  options.mNotification = this;

  ErrorResult result;
  nsRefPtr<EventTarget> target = mWorkerPrivate->GlobalScope();
  nsRefPtr<NotificationEvent> event =
    NotificationEvent::Constructor(target,
                                   NS_LITERAL_STRING("notificationclick"),
                                   options,
                                   result);
  if (NS_WARN_IF(result.Failed())) {
    return false;
  }

  event->SetTrusted(true);
  WantsPopupControlCheck popupControlCheck(event);
  target->DispatchDOMEvent(nullptr, event, nullptr, nullptr);
  
  
  
  return false;
}

bool
Notification::DispatchClickEvent()
{
  AssertIsOnTargetThread();
  nsCOMPtr<nsIDOMEvent> event;
  NS_NewDOMEvent(getter_AddRefs(event), this, nullptr, nullptr);
  nsresult rv = event->InitEvent(NS_LITERAL_STRING("click"), false, true);
  NS_ENSURE_SUCCESS(rv, false);
  event->SetTrusted(true);
  WantsPopupControlCheck popupControlCheck(event);
  bool doDefaultAction = true;
  DispatchEvent(event, &doDefaultAction);
  return doDefaultAction;
}



class NotificationClickWorkerRunnable final : public NotificationWorkerRunnable
{
  Notification* mNotification;
  
  
  nsMainThreadPtrHandle<nsPIDOMWindow> mWindow;
public:
  NotificationClickWorkerRunnable(Notification* aNotification,
                                  const nsMainThreadPtrHandle<nsPIDOMWindow>& aWindow)
    : NotificationWorkerRunnable(aNotification->mWorkerPrivate)
    , mNotification(aNotification)
    , mWindow(aWindow)
  {
    MOZ_ASSERT_IF(mWorkerPrivate->IsServiceWorker(), !mWindow);
  }

  void
  WorkerRunInternal(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    bool doDefaultAction = mNotification->DispatchClickEvent();
    MOZ_ASSERT_IF(mWorkerPrivate->IsServiceWorker(), !doDefaultAction);
    if (doDefaultAction) {
      nsRefPtr<FocusWindowRunnable> r = new FocusWindowRunnable(mWindow);
      NS_DispatchToMainThread(r);
    }
  }
};

NS_IMETHODIMP
NotificationObserver::Observe(nsISupports* aSubject, const char* aTopic,
                              const char16_t* aData)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(mNotificationRef);
  Notification* notification = mNotificationRef->GetNotification();
  MOZ_ASSERT(notification);
  if (!strcmp("alertclickcallback", aTopic)) {
    nsCOMPtr<nsPIDOMWindow> window = notification->GetOwner();
    if (NS_WARN_IF(!window || !window->IsCurrentInnerWindow())) {
      
      return NS_ERROR_FAILURE;
    }

    bool doDefaultAction = notification->DispatchClickEvent();
    if (doDefaultAction) {
      nsIDocument* doc = window ? window->GetExtantDoc() : nullptr;
      if (doc) {
        
        
        nsContentUtils::DispatchChromeEvent(doc, window->GetOuterWindow(),
                                            NS_LITERAL_STRING("DOMWebNotificationClicked"),
                                            true, true);
      }
    }
  } else if (!strcmp("alertfinished", aTopic)) {
    notification->UnpersistNotification();
    notification->mIsClosed = true;
    notification->DispatchTrustedEvent(NS_LITERAL_STRING("close"));
  } else if (!strcmp("alertshow", aTopic)) {
    notification->DispatchTrustedEvent(NS_LITERAL_STRING("show"));
  }
  return NS_OK;
}

NS_IMETHODIMP
WorkerNotificationObserver::Observe(nsISupports* aSubject, const char* aTopic,
                                    const char16_t* aData)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(mNotificationRef);
  
  
  Notification* notification = mNotificationRef->GetNotification();
  
  if (NS_WARN_IF(!notification)) {
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(notification->mWorkerPrivate);

  nsRefPtr<WorkerRunnable> r;
  if (!strcmp("alertclickcallback", aTopic)) {
    nsPIDOMWindow* window = nullptr;
    if (!notification->mWorkerPrivate->IsServiceWorker()) {
      WorkerPrivate* top = notification->mWorkerPrivate;
      while (top->GetParent()) {
        top = top->GetParent();
      }

      window = top->GetWindow();
      if (NS_WARN_IF(!window || !window->IsCurrentInnerWindow())) {
        
        return NS_ERROR_FAILURE;
      }
    }

    
    
    nsMainThreadPtrHandle<nsPIDOMWindow> windowHandle(
      new nsMainThreadPtrHolder<nsPIDOMWindow>(window));

    r = new NotificationClickWorkerRunnable(notification, windowHandle);
  } else if (!strcmp("alertfinished", aTopic)) {
    notification->UnpersistNotification();
    notification->mIsClosed = true;
    r = new NotificationEventWorkerRunnable(notification,
                                            NS_LITERAL_STRING("close"));
  } else if (!strcmp("alertshow", aTopic)) {
    r = new NotificationEventWorkerRunnable(notification,
                                            NS_LITERAL_STRING("show"));
  }

  MOZ_ASSERT(r);
  AutoSafeJSContext cx;
  if (!r->Dispatch(cx)) {
    NS_WARNING("Could not dispatch event to worker notification");
  }
  return NS_OK;
}

class NotificationClickEventCallback final : public nsINotificationStorageCallback
{
public:
  NS_DECL_ISUPPORTS

  NotificationClickEventCallback(nsIPrincipal* aPrincipal,
                                 const nsAString& aScope)
  : mPrincipal(aPrincipal), mScope(aScope)
  {
    MOZ_ASSERT(aPrincipal);
  }

  NS_IMETHOD Handle(const nsAString& aID,
                    const nsAString& aTitle,
                    const nsAString& aDir,
                    const nsAString& aLang,
                    const nsAString& aBody,
                    const nsAString& aTag,
                    const nsAString& aIcon,
                    const nsAString& aData,
                    const nsAString& aBehavior,
                    const nsAString& aServiceWorkerRegistrationID,
                    JSContext* aCx) override
  {
    MOZ_ASSERT(!aID.IsEmpty());
    MOZ_ASSERT(mScope.Equals(aServiceWorkerRegistrationID));

    AssertIsOnMainThread();

    nsAutoCString originSuffix;
    nsresult rv = mPrincipal->GetOriginSuffix(originSuffix);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsIServiceWorkerManager> swm =
      mozilla::services::GetServiceWorkerManager();

    if (swm) {
      swm->SendNotificationClickEvent(originSuffix,
                                      NS_ConvertUTF16toUTF8(mScope),
                                      aID,
                                      aTitle,
                                      aDir,
                                      aLang,
                                      aBody,
                                      aTag,
                                      aIcon,
                                      aData,
                                      aBehavior);
    }
    return NS_OK;
  }

  NS_IMETHOD Done(JSContext* aCx) override
  {
    return NS_OK;
  }

private:
  ~NotificationClickEventCallback()
  {
  }

  nsCOMPtr<nsIPrincipal> mPrincipal;
  nsString mScope;
};

NS_IMPL_ISUPPORTS(NotificationClickEventCallback, nsINotificationStorageCallback)

NS_IMETHODIMP
ServiceWorkerNotificationObserver::Observe(nsISupports* aSubject,
                                           const char* aTopic,
                                           const char16_t* aData)
{
  AssertIsOnMainThread();
  
  if (!strcmp("alertclickcallback", aTopic)) {
    nsresult rv;
    nsCOMPtr<nsINotificationStorage> notificationStorage =
      do_GetService(NS_NOTIFICATION_STORAGE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    nsCOMPtr<nsINotificationStorageCallback> callback =
      new NotificationClickEventCallback(mPrincipal, mScope);

    nsAutoString origin;
    rv = Notification::GetOrigin(mPrincipal, origin);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }

    rv = notificationStorage->GetByID(origin, mID, callback);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      return rv;
    }
  }

  return NS_OK;
}

void
Notification::ShowInternal()
{
  AssertIsOnMainThread();
  MOZ_ASSERT(mTempRef, "Notification should take ownership of itself before"
                       "calling ShowInternal!");
  
  MOZ_ASSERT(!mObserver);

  
  
  UniquePtr<NotificationRef> ownership;
  mozilla::Swap(ownership, mTempRef);
  MOZ_ASSERT(ownership->GetNotification() == this);

  nsresult rv = PersistNotification();
  if (NS_FAILED(rv)) {
    NS_WARNING("Could not persist Notification");
  }

  nsCOMPtr<nsIAlertsService> alertService =
    do_GetService(NS_ALERTSERVICE_CONTRACTID);

  ErrorResult result;
  NotificationPermission permission = NotificationPermission::Denied;
  if (mWorkerPrivate) {
    permission = GetPermissionInternal(mWorkerPrivate->GetPrincipal(), result);
  } else {
    permission = GetPermissionInternal(GetOwner(), result);
  }
  if (permission != NotificationPermission::Granted || !alertService) {
    if (mWorkerPrivate) {
      nsRefPtr<NotificationEventWorkerRunnable> r =
        new NotificationEventWorkerRunnable(this,
                                            NS_LITERAL_STRING("error"));
      AutoSafeJSContext cx;
      if (!r->Dispatch(cx)) {
        NS_WARNING("Could not dispatch event to worker notification");
      }
    } else {
      DispatchTrustedEvent(NS_LITERAL_STRING("error"));
    }
    return;
  }

  nsAutoString iconUrl;
  nsAutoString soundUrl;
  ResolveIconAndSoundURL(iconUrl, soundUrl);

  nsCOMPtr<nsIObserver> observer;
  if (mScope.IsEmpty()) {
    
    if (mWorkerPrivate) {
      
      MOZ_ASSERT(!mWorkerPrivate->IsServiceWorker());
      
      
      mObserver = new WorkerNotificationObserver(Move(ownership));
      observer = mObserver;
    } else {
      observer = new NotificationObserver(Move(ownership));
    }
  } else {
    
    
    
    
    observer = new ServiceWorkerNotificationObserver(mScope, GetPrincipal(), mID);
  }
  MOZ_ASSERT(observer);

  
  
  nsString dataStr;
  if (mDataObjectContainer) {
    mDataObjectContainer->GetDataAsBase64(dataStr);
  }

#ifdef MOZ_B2G
  nsCOMPtr<nsIAppNotificationService> appNotifier =
    do_GetService("@mozilla.org/system-alerts-service;1");
  if (appNotifier) {
    uint32_t appId = nsIScriptSecurityManager::UNKNOWN_APP_ID;
    if (mWorkerPrivate) {
      appId = mWorkerPrivate->GetPrincipal()->GetAppId();
    } else {
      nsCOMPtr<nsPIDOMWindow> window = GetOwner();
      appId = (window.get())->GetDoc()->NodePrincipal()->GetAppId();
    }

    if (appId != nsIScriptSecurityManager::UNKNOWN_APP_ID) {
      nsCOMPtr<nsIAppsService> appsService = do_GetService("@mozilla.org/AppsService;1");
      nsString manifestUrl = EmptyString();
      nsresult rv = appsService->GetManifestURLByLocalId(appId, manifestUrl);
      if (NS_SUCCEEDED(rv)) {
        mozilla::AutoSafeJSContext cx;
        JS::Rooted<JS::Value> val(cx);
        AppNotificationServiceOptions ops;
        ops.mTextClickable = true;
        ops.mManifestURL = manifestUrl;
        GetAlertName(ops.mId);
        ops.mDbId = mID;
        ops.mDir = DirectionToString(mDir);
        ops.mLang = mLang;
        ops.mTag = mTag;
        ops.mData = dataStr;
        ops.mMozbehavior = mBehavior;
        ops.mMozbehavior.mSoundFile = soundUrl;

        if (!ToJSValue(cx, ops, &val)) {
          NS_WARNING("Converting dict to object failed!");
          return;
        }

        appNotifier->ShowAppNotification(iconUrl, mTitle, mBody,
                                         observer, val);
        return;
      }
    }
  }
#endif

  
  
  nsString uniqueCookie = NS_LITERAL_STRING("notification:");
  uniqueCookie.AppendInt(sCount++);
  
  bool inPrivateBrowsing = false;
  nsIDocument* doc = mWorkerPrivate ? mWorkerPrivate->GetDocument()
                                    : GetOwner()->GetExtantDoc();
  if (doc) {
    nsCOMPtr<nsILoadContext> loadContext = doc->GetLoadContext();
    inPrivateBrowsing = loadContext && loadContext->UsePrivateBrowsing();
  } else if (mWorkerPrivate) {
    
    
    nsCOMPtr<nsILoadGroup> loadGroup = mWorkerPrivate->GetLoadGroup();
    nsCOMPtr<nsILoadContext> loadContext;
    NS_QueryNotificationCallbacks(nullptr, loadGroup, NS_GET_IID(nsILoadContext),
                                  getter_AddRefs(loadContext));
    inPrivateBrowsing = loadContext && loadContext->UsePrivateBrowsing();
  }

  nsAutoString alertName;
  GetAlertName(alertName);
  alertService->ShowAlertNotification(iconUrl, mTitle, mBody, true,
                                      uniqueCookie, observer, alertName,
                                      DirectionToString(mDir), mLang,
                                      dataStr, GetPrincipal(),
                                      inPrivateBrowsing);
}

 bool
Notification::RequestPermissionEnabledForScope(JSContext* aCx, JSObject* )
{
  
  
  
  
  return NS_IsMainThread();
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
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());
  return GetPermission(global, aRv);
}


NotificationPermission
Notification::GetPermission(nsIGlobalObject* aGlobal, ErrorResult& aRv)
{
  if (NS_IsMainThread()) {
    return GetPermissionInternal(aGlobal, aRv);
  } else {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    nsRefPtr<GetPermissionRunnable> r =
      new GetPermissionRunnable(worker);
    if (!r->Dispatch(worker->GetJSContext())) {
      aRv.Throw(NS_ERROR_DOM_ABORT_ERR);
      return NotificationPermission::Denied;
    }

    return r->GetPermission();
  }
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
  return GetPermissionInternal(principal, aRv);
}

 NotificationPermission
Notification::GetPermissionInternal(nsIPrincipal* aPrincipal,
                                    ErrorResult& aRv)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aPrincipal);

  if (nsContentUtils::IsSystemPrincipal(aPrincipal)) {
    return NotificationPermission::Granted;
  } else {
    
    nsCOMPtr<nsIURI> uri;
    aPrincipal->GetURI(getter_AddRefs(uri));
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
    services::GetPermissionManager();

  permissionManager->TestPermissionFromPrincipal(aPrincipal,
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

nsresult
Notification::ResolveIconAndSoundURL(nsString& iconUrl, nsString& soundUrl)
{
  AssertIsOnMainThread();
  nsresult rv = NS_OK;

  nsCOMPtr<nsIURI> baseUri;

  
  
  
  
  
  
  
  
  const char* charset = "UTF-8";

  if (mWorkerPrivate) {
    baseUri = mWorkerPrivate->GetBaseURI();
  } else {
    nsIDocument* doc = GetOwner()->GetExtantDoc();
    if (doc) {
      baseUri = doc->GetBaseURI();
      charset = doc->GetDocumentCharacterSet().get();
    } else {
      NS_WARNING("No document found for main thread notification!");
      return NS_ERROR_FAILURE;
    }
  }

  if (baseUri) {
    if (mIconUrl.Length() > 0) {
      nsCOMPtr<nsIURI> srcUri;
      rv = NS_NewURI(getter_AddRefs(srcUri), mIconUrl, charset, baseUri);
      if (NS_SUCCEEDED(rv)) {
        nsAutoCString src;
        srcUri->GetSpec(src);
        iconUrl = NS_ConvertUTF8toUTF16(src);
      }
    }
    if (mBehavior.mSoundFile.Length() > 0) {
      nsCOMPtr<nsIURI> srcUri;
      rv = NS_NewURI(getter_AddRefs(srcUri), mBehavior.mSoundFile, charset, baseUri);
      if (NS_SUCCEEDED(rv)) {
        nsAutoCString src;
        srcUri->GetSpec(src);
        soundUrl = NS_ConvertUTF8toUTF16(src);
      }
    }
  }

  return rv;
}

already_AddRefed<Promise>
Notification::Get(nsPIDOMWindow* aWindow,
                  const GetNotificationOptions& aFilter,
                  const nsAString& aScope,
                  ErrorResult& aRv)
{
  MOZ_ASSERT(aWindow);

  nsCOMPtr<nsIDocument> doc = aWindow->GetExtantDoc();
  if (!doc) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsString origin;
  aRv = GetOrigin(doc->NodePrincipal(), origin);
  if (aRv.Failed()) {
    return nullptr;
  }

  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aWindow);
  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  nsCOMPtr<nsINotificationStorageCallback> callback =
    new NotificationStorageCallback(global, aScope, promise);

  nsRefPtr<NotificationGetRunnable> r =
    new NotificationGetRunnable(origin, aFilter.mTag, callback);

  aRv = NS_DispatchToMainThread(r);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return promise.forget();
}

already_AddRefed<Promise>
Notification::Get(const GlobalObject& aGlobal,
                  const GetNotificationOptions& aFilter,
                  ErrorResult& aRv)
{
  AssertIsOnMainThread();
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  MOZ_ASSERT(global);
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(global);

  return Get(window, aFilter, EmptyString(), aRv);
}

class WorkerGetResultRunnable final : public NotificationWorkerRunnable
{
  nsRefPtr<PromiseWorkerProxy> mPromiseProxy;
  const nsTArray<NotificationStrings> mStrings;
public:
  WorkerGetResultRunnable(WorkerPrivate* aWorkerPrivate,
                          PromiseWorkerProxy* aPromiseProxy,
                          const nsTArray<NotificationStrings>&& aStrings)
    : NotificationWorkerRunnable(aWorkerPrivate)
    , mPromiseProxy(aPromiseProxy)
    , mStrings(Move(aStrings))
  {
  }

  void
  WorkerRunInternal(JSContext* aCx, WorkerPrivate* aWorkerPrivate) override
  {
    nsRefPtr<Promise> workerPromise = mPromiseProxy->GetWorkerPromise();

    ErrorResult result;
    nsAutoTArray<nsRefPtr<Notification>, 5> notifications;
    for (uint32_t i = 0; i < mStrings.Length(); ++i) {
      nsRefPtr<Notification> n =
        Notification::ConstructFromFields(aWorkerPrivate->GlobalScope(),
                                          mStrings[i].mID,
                                          mStrings[i].mTitle,
                                          mStrings[i].mDir,
                                          mStrings[i].mLang,
                                          mStrings[i].mBody,
                                          mStrings[i].mTag,
                                          mStrings[i].mIcon,
                                          mStrings[i].mData,
                                          

                                          mStrings[i].mServiceWorkerRegistrationID,
                                          result);

      n->SetStoredState(true);
      unused << NS_WARN_IF(result.Failed());
      if (!result.Failed()) {
        notifications.AppendElement(n.forget());
      }
    }

    workerPromise->MaybeResolve(notifications);
    mPromiseProxy->CleanUp(aCx);
  }
};

class WorkerGetCallback final : public ScopeCheckingGetCallback
{
  nsRefPtr<PromiseWorkerProxy> mPromiseProxy;
public:
  NS_DECL_ISUPPORTS_INHERITED

  WorkerGetCallback(PromiseWorkerProxy* aProxy, const nsAString& aScope)
    : ScopeCheckingGetCallback(aScope), mPromiseProxy(aProxy)
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(aProxy);
  }

  NS_IMETHOD Done(JSContext* aCx) final
  {
    AssertIsOnMainThread();
    MOZ_ASSERT(mPromiseProxy, "Was Done() called twice?");
    MutexAutoLock lock(mPromiseProxy->GetCleanUpLock());
    if (mPromiseProxy->IsClean()) {
      return NS_OK;
    }

    MOZ_ASSERT(mPromiseProxy->GetWorkerPrivate());
    nsRefPtr<WorkerGetResultRunnable> r =
      new WorkerGetResultRunnable(mPromiseProxy->GetWorkerPrivate(),
                                  mPromiseProxy,
                                  Move(mStrings));

    if (!r->Dispatch(aCx)) {
      nsRefPtr<PromiseWorkerProxyControlRunnable> cr =
        new PromiseWorkerProxyControlRunnable(mPromiseProxy->GetWorkerPrivate(),
                                              mPromiseProxy);

      DebugOnly<bool> ok = cr->Dispatch(aCx);
      MOZ_ASSERT(ok);
    }

    mPromiseProxy = nullptr;
    return NS_OK;
  }

private:
  ~WorkerGetCallback()
  {}
};

NS_IMPL_ISUPPORTS_INHERITED0(WorkerGetCallback, ScopeCheckingGetCallback)

class WorkerGetRunnable final : public nsRunnable
{
  nsRefPtr<PromiseWorkerProxy> mPromiseProxy;
  const nsString mTag;
  const nsString mScope;
public:
  WorkerGetRunnable(WorkerPrivate* aWorkerPrivate,
                    Promise* aWorkerPromise,
                    const nsAString& aTag,
                    const nsAString& aScope)
    : mTag(aTag), mScope(aScope)
  {
    aWorkerPrivate->AssertIsOnWorkerThread();
    mPromiseProxy =
      PromiseWorkerProxy::Create(aWorkerPrivate,
                                 aWorkerPromise);

    if (!mPromiseProxy || !mPromiseProxy->GetWorkerPromise()) {
      aWorkerPromise->MaybeReject(NS_ERROR_DOM_ABORT_ERR);
      mPromiseProxy = nullptr;
    }
  }

  NS_IMETHOD
  Run() override
  {
    AssertIsOnMainThread();
    if (!mPromiseProxy) {
      return NS_OK;
    }

    nsCOMPtr<nsINotificationStorageCallback> callback =
      new WorkerGetCallback(mPromiseProxy, mScope);

    AutoJSAPI jsapi;
    jsapi.Init();

    nsresult rv;
    nsCOMPtr<nsINotificationStorage> notificationStorage =
      do_GetService(NS_NOTIFICATION_STORAGE_CONTRACTID, &rv);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      callback->Done(jsapi.cx());
      return rv;
    }

    MutexAutoLock lock(mPromiseProxy->GetCleanUpLock());
    if (mPromiseProxy->IsClean()) {
      return NS_OK;
    }

    nsString origin;
    rv =
      Notification::GetOrigin(mPromiseProxy->GetWorkerPrivate()->GetPrincipal(),
                              origin);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      callback->Done(jsapi.cx());
      return rv;
    }

    rv = notificationStorage->Get(origin, mTag, callback);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      callback->Done(jsapi.cx());
      return rv;
    }

    return NS_OK;
  }
private:
  ~WorkerGetRunnable()
  {}
};

already_AddRefed<Promise>
Notification::WorkerGet(WorkerPrivate* aWorkerPrivate,
                        const GetNotificationOptions& aFilter,
                        const nsAString& aScope,
                        ErrorResult& aRv)
{
  MOZ_ASSERT(aWorkerPrivate);
  aWorkerPrivate->AssertIsOnWorkerThread();
  nsRefPtr<Promise> p = Promise::Create(aWorkerPrivate->GlobalScope(), aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  nsRefPtr<WorkerGetRunnable> r =
    new WorkerGetRunnable(aWorkerPrivate, p, aFilter.mTag, aScope);
  if (NS_WARN_IF(NS_FAILED(NS_DispatchToMainThread(r)))) {
    aRv.Throw(NS_ERROR_DOM_ABORT_ERR);
    return nullptr;
  }

  return p.forget();
}

JSObject*
Notification::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  return mozilla::dom::NotificationBinding::Wrap(aCx, this, aGivenProto);
}

void
Notification::Close()
{
  AssertIsOnTargetThread();
  auto ref = MakeUnique<NotificationRef>(this);
  if (!ref->Initialized()) {
    return;
  }

  nsCOMPtr<nsIRunnable> closeNotificationTask =
    new NotificationTask(Move(ref), NotificationTask::eClose);
  nsresult rv = NS_DispatchToMainThread(closeNotificationTask);

  if (NS_FAILED(rv)) {
    DispatchTrustedEvent(NS_LITERAL_STRING("error"));
    
    
  }
}

void
Notification::CloseInternal()
{
  AssertIsOnMainThread();
  
  
  
  UniquePtr<NotificationRef> ownership;
  mozilla::Swap(ownership, mTempRef);

  SetAlertName();
  UnpersistNotification();
  if (!mIsClosed) {
    nsCOMPtr<nsIAlertsService> alertService =
      do_GetService(NS_ALERTSERVICE_CONTRACTID);
    if (alertService) {
      nsAutoString alertName;
      GetAlertName(alertName);
      alertService->CloseAlert(alertName, GetPrincipal());
    }
  }
}

nsresult
Notification::GetOrigin(nsIPrincipal* aPrincipal, nsString& aOrigin)
{
  MOZ_ASSERT(aPrincipal);
  uint16_t appStatus = aPrincipal->GetAppStatus();
  uint32_t appId = aPrincipal->GetAppId();

  nsresult rv;
  if (appStatus == nsIPrincipal::APP_STATUS_NOT_INSTALLED ||
      appId == nsIScriptSecurityManager::NO_APP_ID ||
      appId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    rv = nsContentUtils::GetUTFOrigin(aPrincipal, aOrigin);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    
    nsCOMPtr<nsIAppsService> appsService =
      do_GetService("@mozilla.org/AppsService;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    appsService->GetManifestURLByLocalId(appId, aOrigin);
  }

  return NS_OK;
}

nsIStructuredCloneContainer* Notification::GetDataCloneContainer()
{
  return mDataObjectContainer;
}

void
Notification::GetData(JSContext* aCx,
                      JS::MutableHandle<JS::Value> aRetval)
{
  if (!mData && mDataObjectContainer) {
    nsresult rv;
    rv = mDataObjectContainer->DeserializeToVariant(aCx, getter_AddRefs(mData));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      aRetval.setNull();
      return;
    }
  }
  if (!mData) {
    aRetval.setNull();
    return;
  }
  VariantToJsval(aCx, mData, aRetval);
}

void
Notification::InitFromJSVal(JSContext* aCx, JS::Handle<JS::Value> aData,
                            ErrorResult& aRv)
{
  if (mDataObjectContainer || aData.isNull()) {
    return;
  }
  mDataObjectContainer = new nsStructuredCloneContainer();
  aRv = mDataObjectContainer->InitFromJSVal(aData, aCx);
}

void Notification::InitFromBase64(JSContext* aCx, const nsAString& aData,
                                  ErrorResult& aRv)
{
  if (mDataObjectContainer || aData.IsEmpty()) {
    return;
  }

  auto container = new nsStructuredCloneContainer();
  aRv = container->InitFromBase64(aData, JS_STRUCTURED_CLONE_VERSION,
                                  aCx);
  mDataObjectContainer = container;
}

bool
Notification::AddRefObject()
{
  AssertIsOnTargetThread();
  MOZ_ASSERT_IF(mWorkerPrivate && !mFeature, mTaskCount == 0);
  MOZ_ASSERT_IF(mWorkerPrivate && mFeature, mTaskCount > 0);
  if (mWorkerPrivate && !mFeature) {
    if (!RegisterFeature()) {
      return false;
    }
  }
  AddRef();
  ++mTaskCount;
  return true;
}

void
Notification::ReleaseObject()
{
  AssertIsOnTargetThread();
  MOZ_ASSERT(mTaskCount > 0);
  MOZ_ASSERT_IF(mWorkerPrivate, mFeature);

  --mTaskCount;
  if (mWorkerPrivate && mTaskCount == 0) {
    UnregisterFeature();
  }
  Release();
}

NotificationFeature::NotificationFeature(Notification* aNotification)
  : mNotification(aNotification)
{
  MOZ_ASSERT(mNotification->mWorkerPrivate);
  mNotification->mWorkerPrivate->AssertIsOnWorkerThread();
}







class CloseNotificationRunnable final
  : public WorkerMainThreadRunnable
{
  Notification* mNotification;

  public:
  explicit CloseNotificationRunnable(Notification* aNotification)
    : WorkerMainThreadRunnable(aNotification->mWorkerPrivate)
    , mNotification(aNotification)
  {}

  bool
  MainThreadRun() override
  {
    if (mNotification->mObserver) {
      
      mNotification->mObserver->ForgetNotification();
      mNotification->mObserver = nullptr;
    }
    mNotification->CloseInternal();
    return true;
  }
};

bool
NotificationFeature::Notify(JSContext* aCx, Status aStatus)
{
  MOZ_ASSERT(aStatus >= Canceling);

  
  nsRefPtr<CloseNotificationRunnable> r =
    new CloseNotificationRunnable(mNotification);
  r->Dispatch(aCx);

  mNotification->ReleaseObject();
  
  
  
  return true;
}

bool
Notification::RegisterFeature()
{
  MOZ_ASSERT(mWorkerPrivate);
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(!mFeature);
  mFeature = MakeUnique<NotificationFeature>(this);
  return mWorkerPrivate->AddFeature(mWorkerPrivate->GetJSContext(),
                                    mFeature.get());
}

void
Notification::UnregisterFeature()
{
  MOZ_ASSERT(mWorkerPrivate);
  mWorkerPrivate->AssertIsOnWorkerThread();
  MOZ_ASSERT(mFeature);
  mWorkerPrivate->RemoveFeature(mWorkerPrivate->GetJSContext(),
                                mFeature.get());
  mFeature = nullptr;
}








class CheckLoadRunnable final : public WorkerMainThreadRunnable
{
  nsresult mRv;
  nsCString mScope;

public:
  explicit CheckLoadRunnable(WorkerPrivate* aWorker, const nsACString& aScope)
    : WorkerMainThreadRunnable(aWorker)
    , mRv(NS_ERROR_DOM_SECURITY_ERR)
    , mScope(aScope)
  { }

  bool
  MainThreadRun() override
  {
    nsIPrincipal* principal = mWorkerPrivate->GetPrincipal();
    mRv = CheckScope(principal, mScope);

    if (NS_FAILED(mRv)) {
      return true;
    }

    nsRefPtr<ServiceWorkerManager> swm = ServiceWorkerManager::GetInstance();
    nsRefPtr<ServiceWorkerRegistrationInfo> registration =
      swm->GetRegistration(principal, mScope);

    
    MOZ_ASSERT(registration);

    if (!registration->mActiveWorker ||
        registration->mActiveWorker->ID() != mWorkerPrivate->ServiceWorkerID()) {
      mRv = NS_ERROR_NOT_AVAILABLE;
    }

    return true;
  }

  nsresult
  Result()
  {
    return mRv;
  }

};


already_AddRefed<Promise>
Notification::ShowPersistentNotification(nsIGlobalObject *aGlobal,
                                         const nsAString& aScope,
                                         const nsAString& aTitle,
                                         const NotificationOptions& aOptions,
                                         ErrorResult& aRv)
{
  MOZ_ASSERT(aGlobal);

  
  
  
  
  
  
  if (NS_IsMainThread()) {
    nsCOMPtr<nsIScriptObjectPrincipal> sop = do_QueryInterface(aGlobal);
    if (NS_WARN_IF(!sop)) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    nsIPrincipal* principal = sop->GetPrincipal();
    if (NS_WARN_IF(!principal)) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    aRv = CheckScope(principal, NS_ConvertUTF16toUTF8(aScope));
    if (NS_WARN_IF(aRv.Failed())) {
      aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
      return nullptr;
    }
  } else {
    WorkerPrivate* worker = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();
    nsRefPtr<CheckLoadRunnable> loadChecker =
      new CheckLoadRunnable(worker, NS_ConvertUTF16toUTF8(aScope));
    if (!loadChecker->Dispatch(worker->GetJSContext())) {
      aRv.Throw(NS_ERROR_DOM_ABORT_ERR);
      return nullptr;
    }

    if (NS_WARN_IF(NS_FAILED(loadChecker->Result()))) {
      if (loadChecker->Result() == NS_ERROR_NOT_AVAILABLE) {
        aRv.ThrowTypeError(MSG_NO_ACTIVE_WORKER);
      } else {
        aRv.Throw(NS_ERROR_DOM_SECURITY_ERR);
      }
      return nullptr;
    }
  }


  nsRefPtr<Promise> p = Promise::Create(aGlobal, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  
  
  NotificationPermission permission = GetPermission(aGlobal, aRv);

  
  if (NS_WARN_IF(aRv.Failed()) || permission == NotificationPermission::Denied) {
    ErrorResult result;
    result.ThrowTypeError(MSG_NOTIFICATION_PERMISSION_DENIED);
    p->MaybeReject(result);
    return p.forget();
  }

  
  
  
  p->MaybeResolve(JS::UndefinedHandleValue);

  nsRefPtr<Notification> notification =
    CreateAndShow(aGlobal, aTitle, aOptions, aScope, aRv);
  if (NS_WARN_IF(aRv.Failed())) {
    return nullptr;
  }

  return p.forget();
}

 already_AddRefed<Notification>
Notification::CreateAndShow(nsIGlobalObject* aGlobal,
                            const nsAString& aTitle,
                            const NotificationOptions& aOptions,
                            const nsAString& aScope,
                            ErrorResult& aRv)
{
  MOZ_ASSERT(aGlobal);

  AutoJSAPI jsapi;
  jsapi.Init(aGlobal);
  JSContext* cx = jsapi.cx();

  nsRefPtr<Notification> notification = CreateInternal(aGlobal, EmptyString(),
                                                       aTitle, aOptions);

  
  JS::Rooted<JS::Value> data(cx, aOptions.mData);
  notification->InitFromJSVal(cx, data, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  notification->SetScope(aScope);

  auto ref = MakeUnique<NotificationRef>(notification);
  if (!ref->Initialized()) {
    aRv.Throw(NS_ERROR_DOM_ABORT_ERR);
    return nullptr;
  }

  
  nsCOMPtr<nsIRunnable> showNotificationTask =
    new NotificationTask(Move(ref), NotificationTask::eShow);
  nsresult rv = NS_DispatchToMainThread(showNotificationTask);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    notification->DispatchTrustedEvent(NS_LITERAL_STRING("error"));
  }

  return notification.forget();
}
} 
} 

