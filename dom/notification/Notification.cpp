





#include "mozilla/dom/Notification.h"
#include "mozilla/dom/AppNotificationServiceOptionsBinding.h"
#include "mozilla/dom/BindingUtils.h"
#include "mozilla/dom/OwningNonNull.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/Move.h"
#include "mozilla/Preferences.h"
#include "nsContentUtils.h"
#include "nsIAlertsService.h"
#include "nsIAppsService.h"
#include "nsIContentPermissionPrompt.h"
#include "nsIDocument.h"
#include "nsINotificationStorage.h"
#include "nsIPermissionManager.h"
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
#include "mozilla/dom/PermissionMessageUtils.h"
#include "mozilla/dom/Event.h"
#include "mozilla/Services.h"
#include "nsContentPermissionHelper.h"
#include "nsILoadContext.h"
#ifdef MOZ_B2G
#include "nsIDOMDesktopNotification.h"
#endif

#include "WorkerPrivate.h"
#include "WorkerRunnable.h"

namespace mozilla {
namespace dom {

using namespace workers;

class NotificationStorageCallback final : public nsINotificationStorageCallback
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(NotificationStorageCallback)

  NotificationStorageCallback(const GlobalObject& aGlobal, nsPIDOMWindow* aWindow, Promise* aPromise)
    : mCount(0),
      mGlobal(aGlobal.Get()),
      mWindow(aWindow),
      mPromise(aPromise)
  {
    MOZ_ASSERT(aWindow);
    MOZ_ASSERT(aPromise);
    JSContext* cx = aGlobal.Context();
    JSAutoCompartment ac(cx, mGlobal);
    mNotifications = JS_NewArrayObject(cx, 0);
    HoldData();
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
                    JSContext* aCx) override
  {
    MOZ_ASSERT(!aID.IsEmpty());

    RootedDictionary<NotificationOptions> options(aCx);
    options.mDir = Notification::StringToDirection(nsString(aDir));
    options.mLang = aLang;
    options.mBody = aBody;
    options.mTag = aTag;
    options.mIcon = aIcon;
    options.mMozbehavior.Init(aBehavior);
    nsRefPtr<Notification> notification;
    nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(mWindow);
    notification = Notification::CreateInternal(global,
                                                aID,
                                                aTitle,
                                                options);
    ErrorResult rv;
    notification->InitFromBase64(aCx, aData, rv);
    if (rv.Failed()) {
      return rv.StealNSResult();
    }

    notification->SetStoredState(true);

    JSAutoCompartment ac(aCx, mGlobal);
    JS::Rooted<JSObject*> element(aCx, notification->WrapObject(aCx, nullptr));
    NS_ENSURE_TRUE(element, NS_ERROR_FAILURE);

    JS::Rooted<JSObject*> notifications(aCx, mNotifications);
    if (!JS_DefineElement(aCx, notifications, mCount++, element, 0)) {
      return NS_ERROR_FAILURE;
    }
    return NS_OK;
  }

  NS_IMETHOD Done(JSContext* aCx) override
  {
    JSAutoCompartment ac(aCx, mGlobal);
    JS::Rooted<JS::Value> result(aCx, JS::ObjectValue(*mNotifications));
    mPromise->MaybeResolve(aCx, result);
    return NS_OK;
  }

private:
  virtual ~NotificationStorageCallback()
  {
    DropData();
  }

  void HoldData()
  {
    mozilla::HoldJSObjects(this);
  }

  void DropData()
  {
    mGlobal = nullptr;
    mNotifications = nullptr;
    mozilla::DropJSObjects(this);
  }

  uint32_t  mCount;
  JS::Heap<JSObject *> mGlobal;
  nsCOMPtr<nsPIDOMWindow> mWindow;
  nsRefPtr<Promise> mPromise;
  JS::Heap<JSObject *> mNotifications;
};

NS_IMPL_CYCLE_COLLECTING_ADDREF(NotificationStorageCallback)
NS_IMPL_CYCLE_COLLECTING_RELEASE(NotificationStorageCallback)
NS_IMPL_CYCLE_COLLECTION_CLASS(NotificationStorageCallback)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(NotificationStorageCallback)
  NS_INTERFACE_MAP_ENTRY(nsINotificationStorageCallback)
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

NS_IMPL_CYCLE_COLLECTION_TRACE_BEGIN(NotificationStorageCallback)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mGlobal)
  NS_IMPL_CYCLE_COLLECTION_TRACE_JS_MEMBER_CALLBACK(mNotifications)
NS_IMPL_CYCLE_COLLECTION_TRACE_END

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN(NotificationStorageCallback)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE_SCRIPT_OBJECTS
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mWindow)
  NS_IMPL_CYCLE_COLLECTION_TRAVERSE(mPromise)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN(NotificationStorageCallback)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mWindow)
  NS_IMPL_CYCLE_COLLECTION_UNLINK(mPromise)
  tmp->DropData();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

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
    WorkerRunInternal();
    return true;
  }

  void
  PostRun(JSContext* aCx, WorkerPrivate* aWorkerPrivate,
          bool aRunResult) override
  {
    aWorkerPrivate->ModifyBusyCountFromWorker(aCx, false);
  }

  virtual void
  WorkerRunInternal() = 0;
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
  WorkerRunInternal() override
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
    : mNotificationRef(Move(aRef)), mAction(aAction) {}

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

Notification::Notification(const nsAString& aID, const nsAString& aTitle, const nsAString& aBody,
                           NotificationDirection aDir, const nsAString& aLang,
                           const nsAString& aTag, const nsAString& aIconUrl,
                           const NotificationBehavior& aBehavior, nsIGlobalObject* aGlobal)
  : DOMEventTargetHelper(),
    mWorkerPrivate(nullptr), mObserver(nullptr),
    mID(aID), mTitle(aTitle), mBody(aBody), mDir(aDir), mLang(aLang),
    mTag(aTag), mIconUrl(aIconUrl), mBehavior(aBehavior), mIsClosed(false),
    mIsStored(false), mTaskCount(0)
{
  nsAutoString alertName;
  if (NS_IsMainThread()) {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal);
    MOZ_ASSERT(window);
    BindToOwner(window);

    DebugOnly<nsresult> rv = GetOrigin(window, alertName);
    MOZ_ASSERT(NS_SUCCEEDED(rv), "GetOrigin should not have failed");
  } else {
    mWorkerPrivate = GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(mWorkerPrivate);

    DebugOnly<nsresult> rv = GetOriginWorker(alertName);
    MOZ_ASSERT(NS_SUCCEEDED(rv), "GetOrigin should not have failed");
  }

  
  
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
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(aGlobal.GetAsSupports());
  nsRefPtr<Notification> notification = CreateInternal(global,
                                                       EmptyString(),
                                                       aTitle,
                                                       aOptions);
  
  JS::Rooted<JS::Value> data(aGlobal.Context(), aOptions.mData);
  notification->InitFromJSVal(aGlobal.Context(), data, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  auto ref = MakeUnique<NotificationRef>(notification);
  if (!ref->Initialized()) {
    aRv.Throw(NS_ERROR_DOM_ABORT_ERR);
    return nullptr;
  }

  
  nsCOMPtr<nsIRunnable> showNotificationTask =
    new NotificationTask(Move(ref), NotificationTask::eShow);
  nsresult rv = NS_DispatchToMainThread(showNotificationTask);
  if (NS_FAILED(rv)) {
    notification->DispatchTrustedEvent(NS_LITERAL_STRING("error"));
  }

  
  
  
  if (NS_IsMainThread()) {
    nsresult rv = notification->PersistNotification();
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not persist main thread Notification");
    }
  }

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
  if (mWorkerPrivate) {
    rv = GetOriginWorker(origin);
    MOZ_ASSERT(NS_SUCCEEDED(rv));
  } else {
    nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(GetOwner());
    MOZ_ASSERT(window);
    rv = GetOrigin(window, origin);
    if (NS_FAILED(rv)) {
      return rv;
    }
  }

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
                                behavior);

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
  if (mIsStored) {
    nsCOMPtr<nsINotificationStorage> notificationStorage =
      do_GetService(NS_NOTIFICATION_STORAGE_CONTRACTID);
    if (notificationStorage) {
      nsString origin;
      nsresult rv = GetOrigin(GetOwner(), origin);
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

  nsRefPtr<Notification> notification = new Notification(id,
                                                         aTitle,
                                                         aOptions.mBody,
                                                         aOptions.mDir,
                                                         aOptions.mLang,
                                                         aOptions.mTag,
                                                         aOptions.mIcon,
                                                         aOptions.mMozbehavior,
                                                         aGlobal);
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
      WorkerRunInternal() override
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
  {}

  void
  WorkerRunInternal() override
  {
    bool doDefaultAction = mNotification->DispatchClickEvent();
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
    if (!window || !window->IsCurrentInnerWindow()) {
      
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
  
  if (!notification) {
    return NS_ERROR_FAILURE;
  }

  MOZ_ASSERT(notification->mWorkerPrivate);

  nsRefPtr<WorkerRunnable> r;
  if (!strcmp("alertclickcallback", aTopic)) {
    nsCOMPtr<nsPIDOMWindow> window = notification->mWorkerPrivate->GetWindow();
    if (!window || !window->IsCurrentInnerWindow()) {
      
      return NS_ERROR_FAILURE;
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

  if (mWorkerPrivate) {
    
    
    nsresult rv = PersistNotification();
    if (NS_FAILED(rv)) {
      NS_WARNING("Could not persist worker Notification");
    }
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
  if (mWorkerPrivate) {
    
    
    mObserver = new WorkerNotificationObserver(Move(ownership));
    observer = mObserver;
  } else {
    observer = new NotificationObserver(Move(ownership));
  }

  
  
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
        ops.mId = mAlertName;
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
  alertService->ShowAlertNotification(iconUrl, mTitle, mBody, true,
                                      uniqueCookie, observer, mAlertName,
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
  if (NS_IsMainThread()) {
    return GetPermissionInternal(aGlobal.GetAsSupports(), aRv);
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
Notification::Get(const GlobalObject& aGlobal,
                  const GetNotificationOptions& aFilter,
                  ErrorResult& aRv)
{
  AssertIsOnMainThread();
  nsCOMPtr<nsIGlobalObject> global =
    do_QueryInterface(aGlobal.GetAsSupports());
  MOZ_ASSERT(global);
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(global);
  MOZ_ASSERT(window);
  nsIDocument* doc = window->GetExtantDoc();
  if (!doc) {
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }

  nsString origin;
  aRv = GetOrigin(window, origin);
  if (aRv.Failed()) {
    return nullptr;
  }

  nsresult rv;
  nsCOMPtr<nsINotificationStorage> notificationStorage =
    do_GetService(NS_NOTIFICATION_STORAGE_CONTRACTID, &rv);
  if (NS_FAILED(rv)) {
    aRv.Throw(rv);
    return nullptr;
  }

  nsRefPtr<Promise> promise = Promise::Create(global, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }
  nsCOMPtr<nsINotificationStorageCallback> callback =
    new NotificationStorageCallback(aGlobal, window, promise);
  nsString tag = aFilter.mTag.WasPassed() ?
                 aFilter.mTag.Value() :
                 EmptyString();
  aRv = notificationStorage->Get(origin, tag, callback);
  if (aRv.Failed()) {
    return nullptr;
  }

  return promise.forget();
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
  
  
  
  UniquePtr<NotificationRef> ownership;
  mozilla::Swap(ownership, mTempRef);

  UnpersistNotification();
  if (!mIsClosed) {
    nsCOMPtr<nsIAlertsService> alertService =
      do_GetService(NS_ALERTSERVICE_CONTRACTID);
    if (alertService) {
      alertService->CloseAlert(mAlertName, GetPrincipal());
    }
  }
}

nsresult
Notification::GetOrigin(nsPIDOMWindow* aWindow, nsString& aOrigin)
{
  if (!aWindow) {
    return NS_ERROR_FAILURE;
  }
  nsresult rv;
  nsIDocument* doc = aWindow->GetExtantDoc();
  NS_ENSURE_TRUE(doc, NS_ERROR_UNEXPECTED);
  nsIPrincipal* principal = doc->NodePrincipal();
  NS_ENSURE_TRUE(principal, NS_ERROR_UNEXPECTED);

  uint16_t appStatus = principal->GetAppStatus();
  uint32_t appId = principal->GetAppId();

  if (appStatus == nsIPrincipal::APP_STATUS_NOT_INSTALLED ||
      appId == nsIScriptSecurityManager::NO_APP_ID ||
      appId == nsIScriptSecurityManager::UNKNOWN_APP_ID) {
    rv = nsContentUtils::GetUTFOrigin(principal, aOrigin);
    NS_ENSURE_SUCCESS(rv, rv);
  } else {
    
    
    nsCOMPtr<nsIAppsService> appsService =
      do_GetService("@mozilla.org/AppsService;1", &rv);
    NS_ENSURE_SUCCESS(rv, rv);
    appsService->GetManifestURLByLocalId(appId, aOrigin);
  }

  return NS_OK;
}

nsresult
Notification::GetOriginWorker(nsString& aOrigin)
{
  MOZ_ASSERT(mWorkerPrivate);
  aOrigin = mWorkerPrivate->GetLocationInfo().mOrigin;
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
} 
} 

