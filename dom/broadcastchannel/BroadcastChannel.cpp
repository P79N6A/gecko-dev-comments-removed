




#include "BroadcastChannel.h"
#include "BroadcastChannelChild.h"
#include "mozilla/dom/BroadcastChannelBinding.h"
#include "mozilla/dom/Navigator.h"
#include "mozilla/ipc/BackgroundChild.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "mozilla/ipc/PBackgroundChild.h"
#include "mozilla/Preferences.h"
#include "WorkerPrivate.h"
#include "WorkerRunnable.h"

#include "nsIAppsService.h"
#include "nsIDocument.h"
#include "nsIScriptSecurityManager.h"
#include "nsServiceManagerUtils.h"
#include "nsISupportsPrimitives.h"

namespace mozilla {

using namespace ipc;

namespace dom {

using namespace workers;

namespace {

void
GetOrigin(nsIPrincipal* aPrincipal, nsAString& aOrigin, ErrorResult& aRv)
{
  MOZ_ASSERT(aPrincipal);

  uint16_t appStatus = aPrincipal->GetAppStatus();

  if (appStatus == nsIPrincipal::APP_STATUS_NOT_INSTALLED) {
    nsAutoString tmp;
    aRv = nsContentUtils::GetUTFOrigin(aPrincipal, tmp);
    if (NS_WARN_IF(aRv.Failed())) {
      return;
    }

    aOrigin = tmp;
    return;
  }

  uint32_t appId = aPrincipal->GetAppId();

  
  
  nsresult rv;
  nsCOMPtr<nsIAppsService> appsService =
    do_GetService("@mozilla.org/AppsService;1", &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    aRv.Throw(rv);
    return;
  }

  appsService->GetManifestURLByLocalId(appId, aOrigin);
}

nsIPrincipal*
GetPrincipalFromWorkerPrivate(WorkerPrivate* aWorkerPrivate)
{
  nsIPrincipal* principal = aWorkerPrivate->GetPrincipal();
  if (principal) {
    return principal;
  }

  
  WorkerPrivate* wp = aWorkerPrivate;
  while (wp->GetParent()) {
    wp = wp->GetParent();
  }

  return wp->GetPrincipal();
}

class InitializeRunnable MOZ_FINAL : public WorkerMainThreadRunnable
{
public:
  InitializeRunnable(WorkerPrivate* aWorkerPrivate, nsAString& aOrigin,
                     PrincipalInfo& aPrincipalInfo, ErrorResult& aRv)
    : WorkerMainThreadRunnable(aWorkerPrivate)
    , mWorkerPrivate(GetCurrentThreadWorkerPrivate())
    , mOrigin(aOrigin)
    , mPrincipalInfo(aPrincipalInfo)
    , mRv(aRv)
  {
    MOZ_ASSERT(mWorkerPrivate);
  }

  bool MainThreadRun() MOZ_OVERRIDE
  {
    MOZ_ASSERT(NS_IsMainThread());

    nsIPrincipal* principal = GetPrincipalFromWorkerPrivate(mWorkerPrivate);
    if (!principal) {
      mRv.Throw(NS_ERROR_FAILURE);
      return true;
    }

    bool isNullPrincipal;
    mRv = principal->GetIsNullPrincipal(&isNullPrincipal);
    if (NS_WARN_IF(mRv.Failed())) {
      return true;
    }

    if (NS_WARN_IF(isNullPrincipal)) {
      mRv.Throw(NS_ERROR_FAILURE);
      return true;
    }

    mRv = PrincipalToPrincipalInfo(principal, &mPrincipalInfo);
    if (NS_WARN_IF(mRv.Failed())) {
      return true;
    }

    GetOrigin(principal, mOrigin, mRv);
    if (NS_WARN_IF(mRv.Failed())) {
      return true;
    }

    
    WorkerPrivate* wp = mWorkerPrivate;
    while (wp->GetParent()) {
      wp = wp->GetParent();
    }

    
    nsPIDOMWindow* window = wp->GetWindow();
    if (!window) {
      return true;
    }

    nsIDocument* doc = window->GetExtantDoc();
    
    if (doc) {
      doc->DisallowBFCaching();
    }

    return true;
  }

private:
  WorkerPrivate* mWorkerPrivate;
  nsAString& mOrigin;
  PrincipalInfo& mPrincipalInfo;
  ErrorResult& mRv;
};

class PostMessageRunnable MOZ_FINAL : public nsICancelableRunnable
{
public:
  NS_DECL_ISUPPORTS

  PostMessageRunnable(BroadcastChannelChild* aActor,
                      const nsAString& aMessage)
    : mActor(aActor)
    , mMessage(aMessage)
  {
    MOZ_ASSERT(mActor);
  }

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(mActor);
    if (!mActor->IsActorDestroyed()) {
      mActor->SendPostMessage(mMessage);
    }
    return NS_OK;
  }

  NS_IMETHODIMP Cancel()
  {
    mActor = nullptr;
    return NS_OK;
  }

private:
  ~PostMessageRunnable() {}

  nsRefPtr<BroadcastChannelChild> mActor;
  nsString mMessage;
};

NS_IMPL_ISUPPORTS(PostMessageRunnable, nsICancelableRunnable, nsIRunnable)

class TeardownRunnable MOZ_FINAL : public nsICancelableRunnable
{
public:
  NS_DECL_ISUPPORTS

  TeardownRunnable(BroadcastChannelChild* aActor)
    : mActor(aActor)
  {
    MOZ_ASSERT(mActor);
  }

  NS_IMETHODIMP Run()
  {
    MOZ_ASSERT(mActor);
    if (!mActor->IsActorDestroyed()) {
      mActor->SendClose();
    }
    return NS_OK;
  }

  NS_IMETHODIMP Cancel()
  {
    mActor = nullptr;
    return NS_OK;
  }

private:
  ~TeardownRunnable() {}

  nsRefPtr<BroadcastChannelChild> mActor;
};

NS_IMPL_ISUPPORTS(TeardownRunnable, nsICancelableRunnable, nsIRunnable)

class BroadcastChannelFeature MOZ_FINAL : public workers::WorkerFeature
{
  BroadcastChannel* mChannel;

public:
  explicit BroadcastChannelFeature(BroadcastChannel* aChannel)
    : mChannel(aChannel)
  {
    MOZ_COUNT_CTOR(BroadcastChannelFeature);
  }

  virtual bool Notify(JSContext* aCx, workers::Status aStatus) MOZ_OVERRIDE
  {
    if (aStatus >= Canceling) {
      WorkerPrivate* workerPrivate = GetWorkerPrivateFromContext(aCx);
      MOZ_ASSERT(workerPrivate);

      mChannel->Shutdown();
    }

    return true;
  }

private:
  ~BroadcastChannelFeature()
  {
    MOZ_COUNT_DTOR(BroadcastChannelFeature);
  }
};

bool
CheckPermission(nsIPrincipal* aPrincipal)
{
  
  bool enabled = false;
  Preferences::GetBool("dom.broadcastChannel.enabled", &enabled);
  if (!enabled) {
    return false;
  }

  if (!aPrincipal) {
    return false;
  }

  uint16_t status;
  if (NS_FAILED(aPrincipal->GetAppStatus(&status))) {
    return false;
  }

  
  return status == nsIPrincipal::APP_STATUS_CERTIFIED ||
         status == nsIPrincipal::APP_STATUS_NOT_INSTALLED;
}



class CheckPermissionRunnable MOZ_FINAL
  : public workers::WorkerMainThreadRunnable
{
public:
  bool mResult;

  CheckPermissionRunnable(workers::WorkerPrivate* aWorkerPrivate)
    : workers::WorkerMainThreadRunnable(aWorkerPrivate)
    , mResult(false)
  {
    MOZ_ASSERT(aWorkerPrivate);
    aWorkerPrivate->AssertIsOnWorkerThread();
  }

protected:
  virtual bool
  MainThreadRun() MOZ_OVERRIDE
  {
    workers::AssertIsOnMainThread();

    nsIPrincipal* principal = GetPrincipalFromWorkerPrivate(mWorkerPrivate);
    if (!principal) {
      return true;
    }

    mResult = CheckPermission(principal);
    return true;
  }
};

} 

 bool
BroadcastChannel::IsEnabled(JSContext* aCx, JSObject* aGlobal)
{
  if (NS_IsMainThread()) {
    JS::Rooted<JSObject*> global(aCx, aGlobal);

    nsCOMPtr<nsPIDOMWindow> win = Navigator::GetWindowFromGlobal(global);
    if (!win) {
      return false;
    }

    nsIDocument* doc = win->GetExtantDoc();
    if (!doc) {
      return false;
    }

    return CheckPermission(doc->NodePrincipal());
  }

  workers::WorkerPrivate* workerPrivate =
    workers::GetWorkerPrivateFromContext(aCx);
  workerPrivate->AssertIsOnWorkerThread();

  nsRefPtr<CheckPermissionRunnable> runnable =
    new CheckPermissionRunnable(workerPrivate);

  if (!runnable->Dispatch(aCx)) {
    JS_ClearPendingException(aCx);
    return false;
  }

  return runnable->mResult;
}

BroadcastChannel::BroadcastChannel(nsPIDOMWindow* aWindow,
                                   const PrincipalInfo& aPrincipalInfo,
                                   const nsAString& aOrigin,
                                   const nsAString& aChannel)
  : DOMEventTargetHelper(aWindow)
  , mWorkerFeature(nullptr)
  , mPrincipalInfo(aPrincipalInfo)
  , mOrigin(aOrigin)
  , mChannel(aChannel)
  , mIsKeptAlive(false)
  , mInnerID(0)
  , mState(StateActive)
{
  
}

BroadcastChannel::~BroadcastChannel()
{
  Shutdown();
  MOZ_ASSERT(!mWorkerFeature);
}

JSObject*
BroadcastChannel::WrapObject(JSContext* aCx)
{
  return BroadcastChannelBinding::Wrap(aCx, this);
}

 already_AddRefed<BroadcastChannel>
BroadcastChannel::Constructor(const GlobalObject& aGlobal,
                              const nsAString& aChannel,
                              ErrorResult& aRv)
{
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  

  nsAutoString origin;
  PrincipalInfo principalInfo;
  WorkerPrivate* workerPrivate = nullptr;

  if (NS_IsMainThread()) {
    nsCOMPtr<nsIGlobalObject> incumbent = mozilla::dom::GetIncumbentGlobal();

    if (!incumbent) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    nsIPrincipal* principal = incumbent->PrincipalOrNull();
    if (!principal) {
      aRv.Throw(NS_ERROR_UNEXPECTED);
      return nullptr;
    }

    bool isNullPrincipal;
    aRv = principal->GetIsNullPrincipal(&isNullPrincipal);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }

    if (NS_WARN_IF(isNullPrincipal)) {
      aRv.Throw(NS_ERROR_FAILURE);
      return nullptr;
    }

    GetOrigin(principal, origin, aRv);

    aRv = PrincipalToPrincipalInfo(principal, &principalInfo);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }

    nsIDocument* doc = window->GetExtantDoc();
    
    if (doc) {
      doc->DisallowBFCaching();
    }
  } else {
    JSContext* cx = aGlobal.Context();
    workerPrivate = GetWorkerPrivateFromContext(cx);
    MOZ_ASSERT(workerPrivate);

    nsRefPtr<InitializeRunnable> runnable =
      new InitializeRunnable(workerPrivate, origin, principalInfo, aRv);
    runnable->Dispatch(cx);
  }

  if (aRv.Failed()) {
    return nullptr;
  }

  nsRefPtr<BroadcastChannel> bc =
    new BroadcastChannel(window, principalInfo, origin, aChannel);

  
  ipc::BackgroundChild::GetOrCreateForCurrentThread(bc);

  if (!workerPrivate) {
    MOZ_ASSERT(window);
    MOZ_ASSERT(window->IsInnerWindow());
    bc->mInnerID = window->WindowID();

    
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->AddObserver(bc, "inner-window-destroyed", false);
    }
  } else {
    bc->mWorkerFeature = new BroadcastChannelFeature(bc);
    JSContext* cx = workerPrivate->GetJSContext();
    if (NS_WARN_IF(!workerPrivate->AddFeature(cx, bc->mWorkerFeature))) {
      NS_WARNING("Failed to register the BroadcastChannel worker feature.");
      return nullptr;
    }
  }

  return bc.forget();
}

void
BroadcastChannel::PostMessage(const nsAString& aMessage, ErrorResult& aRv)
{
  if (mState != StateActive) {
    aRv.Throw(NS_ERROR_FAILURE);
    return;
  }

  PostMessageInternal(aMessage);
}

void
BroadcastChannel::PostMessageInternal(const nsAString& aMessage)
{
  if (mActor) {
    nsRefPtr<PostMessageRunnable> runnable =
      new PostMessageRunnable(mActor, aMessage);

    if (NS_FAILED(NS_DispatchToCurrentThread(runnable))) {
      NS_WARNING("Failed to dispatch to the current thread!");
    }

    return;
  }

  mPendingMessages.AppendElement(aMessage);
}

void
BroadcastChannel::Close()
{
  if (mState != StateActive) {
    return;
  }

  if (mPendingMessages.IsEmpty()) {
    Shutdown();
  } else {
    MOZ_ASSERT(!mActor);
    mState = StateClosing;
  }
}

void
BroadcastChannel::ActorFailed()
{
  MOZ_CRASH("Failed to create a PBackgroundChild actor!");
}

void
BroadcastChannel::ActorCreated(ipc::PBackgroundChild* aActor)
{
  MOZ_ASSERT(aActor);

  if (mState == StateClosed) {
    return;
  }

  PBroadcastChannelChild* actor =
    aActor->SendPBroadcastChannelConstructor(mPrincipalInfo, mOrigin, mChannel);

  mActor = static_cast<BroadcastChannelChild*>(actor);
  MOZ_ASSERT(mActor);

  mActor->SetParent(this);

  
  for (uint32_t i = 0; i < mPendingMessages.Length(); ++i) {
    PostMessageInternal(mPendingMessages[i]);
  }

  mPendingMessages.Clear();

  if (mState == StateClosing) {
    Shutdown();
  }
}

void
BroadcastChannel::Shutdown()
{
  mState = StateClosed;

  
  
  if (mIsKeptAlive) {
    mIsKeptAlive = false;
    Release();
  }

  if (mWorkerFeature) {
    WorkerPrivate* workerPrivate = GetCurrentThreadWorkerPrivate();
    workerPrivate->RemoveFeature(workerPrivate->GetJSContext(), mWorkerFeature);
    mWorkerFeature = nullptr;
  }

  if (mActor) {
    mActor->SetParent(nullptr);

    nsRefPtr<TeardownRunnable> runnable = new TeardownRunnable(mActor);
    NS_DispatchToCurrentThread(runnable);

    mActor = nullptr;
  }
}

EventHandlerNonNull*
BroadcastChannel::GetOnmessage()
{
  if (NS_IsMainThread()) {
    return GetEventHandler(nsGkAtoms::onmessage, EmptyString());
  }
  return GetEventHandler(nullptr, NS_LITERAL_STRING("message"));
}

void
BroadcastChannel::SetOnmessage(EventHandlerNonNull* aCallback)
{
  if (NS_IsMainThread()) {
    SetEventHandler(nsGkAtoms::onmessage, EmptyString(), aCallback);
  } else {
    SetEventHandler(nullptr, NS_LITERAL_STRING("message"), aCallback);
  }

  UpdateMustKeepAlive();
}

void
BroadcastChannel::AddEventListener(const nsAString& aType,
                                   EventListener* aCallback,
                                   bool aCapture,
                                   const dom::Nullable<bool>& aWantsUntrusted,
                                   ErrorResult& aRv)
{
  DOMEventTargetHelper::AddEventListener(aType, aCallback, aCapture,
                                         aWantsUntrusted, aRv);

  if (aRv.Failed()) {
    return;
  }

  UpdateMustKeepAlive();
}

void
BroadcastChannel::RemoveEventListener(const nsAString& aType,
                                      EventListener* aCallback,
                                      bool aCapture,
                                      ErrorResult& aRv)
{
  DOMEventTargetHelper::RemoveEventListener(aType, aCallback, aCapture, aRv);

  if (aRv.Failed()) {
    return;
  }

  UpdateMustKeepAlive();
}

void
BroadcastChannel::UpdateMustKeepAlive()
{
  bool toKeepAlive = HasListenersFor(NS_LITERAL_STRING("message"));
  if (toKeepAlive == mIsKeptAlive) {
    return;
  }

  mIsKeptAlive = toKeepAlive;

  if (toKeepAlive) {
    AddRef();
  } else {
    Release();
  }
}

NS_IMETHODIMP
BroadcastChannel::Observe(nsISupports* aSubject, const char* aTopic,
                          const char16_t* aData)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(!strcmp(aTopic, "inner-window-destroyed"));

  
  
  if (!mIsKeptAlive) {
    return NS_OK;
  }

  nsCOMPtr<nsISupportsPRUint64> wrapper = do_QueryInterface(aSubject);
  NS_ENSURE_TRUE(wrapper, NS_ERROR_FAILURE);

  uint64_t innerID;
  nsresult rv = wrapper->GetData(&innerID);
  NS_ENSURE_SUCCESS(rv, rv);

  if (innerID == mInnerID) {
    nsCOMPtr<nsIObserverService> obs = mozilla::services::GetObserverService();
    if (obs) {
      obs->RemoveObserver(this, "inner-window-destroyed");
    }

    Shutdown();
  }

  return NS_OK;
}

NS_IMPL_CYCLE_COLLECTION_CLASS(BroadcastChannel)

NS_IMPL_CYCLE_COLLECTION_TRAVERSE_BEGIN_INHERITED(BroadcastChannel,
                                                  DOMEventTargetHelper)
NS_IMPL_CYCLE_COLLECTION_TRAVERSE_END

NS_IMPL_CYCLE_COLLECTION_UNLINK_BEGIN_INHERITED(BroadcastChannel,
                                                DOMEventTargetHelper)
  tmp->Shutdown();
NS_IMPL_CYCLE_COLLECTION_UNLINK_END

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(BroadcastChannel)
  NS_INTERFACE_MAP_ENTRY(nsIIPCBackgroundChildCreateCallback)
  NS_INTERFACE_MAP_ENTRY(nsIObserver)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_ADDREF_INHERITED(BroadcastChannel, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(BroadcastChannel, DOMEventTargetHelper)

} 
} 
