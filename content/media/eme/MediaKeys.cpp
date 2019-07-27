





#include "mozilla/dom/HTMLMediaElement.h"
#include "mozilla/dom/MediaKeys.h"
#include "mozilla/dom/MediaKeysBinding.h"
#include "mozilla/dom/MediaKeyMessageEvent.h"
#include "mozilla/dom/MediaKeyError.h"
#include "mozilla/dom/MediaKeySession.h"
#include "mozilla/dom/DOMException.h"
#include "mozilla/CDMProxy.h"
#include "mozilla/EMELog.h"
#include "nsContentUtils.h"
#include "nsIScriptObjectPrincipal.h"
#include "mozilla/Preferences.h"
#ifdef XP_WIN
#include "mozilla/WindowsVersion.h"
#endif

namespace mozilla {

namespace dom {

NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(MediaKeys,
                                      mParent,
                                      mKeySessions,
                                      mPromises,
                                      mPendingSessions);
NS_IMPL_CYCLE_COLLECTING_ADDREF(MediaKeys)
NS_IMPL_CYCLE_COLLECTING_RELEASE(MediaKeys)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(MediaKeys)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

MediaKeys::MediaKeys(nsPIDOMWindow* aParent, const nsAString& aKeySystem)
  : mParent(aParent)
  , mKeySystem(aKeySystem)
  , mCreatePromiseId(0)
{
  SetIsDOMBinding();
}

MediaKeys::~MediaKeys()
{
  if (mProxy) {
    mProxy->Shutdown();
    mProxy = nullptr;
  }
}

nsPIDOMWindow*
MediaKeys::GetParentObject() const
{
  return mParent;
}

JSObject*
MediaKeys::WrapObject(JSContext* aCx)
{
  return MediaKeysBinding::Wrap(aCx, this);
}

void
MediaKeys::GetKeySystem(nsString& retval) const
{
  retval = mKeySystem;
}

already_AddRefed<Promise>
MediaKeys::SetServerCertificate(const Uint8Array& aCert, ErrorResult& aRv)
{
  aCert.ComputeLengthAndData();
  nsRefPtr<Promise> promise(MakePromise(aRv));
  if (aRv.Failed()) {
    return nullptr;
  }
  mProxy->SetServerCertificate(StorePromise(promise), aCert);
  return promise.forget();
}

static bool
IsSupportedKeySystem(const nsAString& aKeySystem)
{
  return aKeySystem.EqualsASCII("org.w3.clearkey") ||
#ifdef XP_WIN
         (aKeySystem.EqualsASCII("com.adobe.access") &&
          IsVistaOrLater() &&
          Preferences::GetBool("media.eme.adobe-access.enabled", false)) ||
#endif
         false;
}


IsTypeSupportedResult
MediaKeys::IsTypeSupported(const GlobalObject& aGlobal,
                           const nsAString& aKeySystem,
                           const Optional<nsAString>& aInitDataType,
                           const Optional<nsAString>& aContentType,
                           const Optional<nsAString>& aCapability)
{
  
  
  return IsSupportedKeySystem(aKeySystem) ? IsTypeSupportedResult::Maybe
                                          : IsTypeSupportedResult::_empty;
}

already_AddRefed<Promise>
MediaKeys::MakePromise(ErrorResult& aRv)
{
  nsCOMPtr<nsIGlobalObject> global = do_QueryInterface(GetParentObject());
  if (!global) {
    NS_WARNING("Passed non-global to MediaKeys ctor!");
    aRv.Throw(NS_ERROR_UNEXPECTED);
    return nullptr;
  }
  return Promise::Create(global, aRv);
}

PromiseId
MediaKeys::StorePromise(Promise* aPromise)
{
  static uint32_t sEMEPromiseCount = 1;
  MOZ_ASSERT(aPromise);
  uint32_t id = sEMEPromiseCount++;
  mPromises.Put(id, aPromise);
  return id;
}

already_AddRefed<Promise>
MediaKeys::RetrievePromise(PromiseId aId)
{
  MOZ_ASSERT(mPromises.Contains(aId));
  nsRefPtr<Promise> promise;
  mPromises.Remove(aId, getter_AddRefs(promise));
  return promise.forget();
}

void
MediaKeys::RejectPromise(PromiseId aId, nsresult aExceptionCode)
{
  nsRefPtr<Promise> promise(RetrievePromise(aId));
  if (!promise) {
    NS_WARNING("MediaKeys tried to reject a non-existent promise");
    return;
  }
  if (mPendingSessions.Contains(aId)) {
    
    
    
    
    mPendingSessions.Remove(aId);
  }

  MOZ_ASSERT(NS_FAILED(aExceptionCode));
  promise->MaybeReject(aExceptionCode);

  if (mCreatePromiseId == aId) {
    
    Release();
  }
}

void
MediaKeys::ResolvePromise(PromiseId aId)
{
  nsRefPtr<Promise> promise(RetrievePromise(aId));
  if (!promise) {
    NS_WARNING("MediaKeys tried to resolve a non-existent promise");
    return;
  }
  if (mPendingSessions.Contains(aId)) {
    
    
    nsRefPtr<MediaKeySession> session;
    if (!mPendingSessions.Get(aId, getter_AddRefs(session)) ||
        !session ||
        session->GetSessionId().IsEmpty()) {
      NS_WARNING("Received activation for non-existent session!");
      promise->MaybeReject(NS_ERROR_DOM_INVALID_ACCESS_ERR);
      mPendingSessions.Remove(aId);
      return;
    }
    mPendingSessions.Remove(aId);
    mKeySessions.Put(session->GetSessionId(), session);
    promise->MaybeResolve(session);
  } else {
    promise->MaybeResolve(JS::UndefinedHandleValue);
  }
}


already_AddRefed<Promise>
MediaKeys::Create(const GlobalObject& aGlobal,
                  const nsAString& aKeySystem,
                  ErrorResult& aRv)
{
  
  
  nsCOMPtr<nsPIDOMWindow> window = do_QueryInterface(aGlobal.GetAsSupports());
  if (!window) {
    aRv.Throw(NS_ERROR_FAILURE);
    return nullptr;
  }

  nsRefPtr<MediaKeys> keys = new MediaKeys(window, aKeySystem);
  nsRefPtr<Promise> promise(keys->MakePromise(aRv));
  if (aRv.Failed()) {
    return nullptr;
  }

  if (!IsSupportedKeySystem(aKeySystem)) {
    aRv.Throw(NS_ERROR_DOM_NOT_SUPPORTED_ERR);
    return nullptr;
  }

  keys->mProxy = new CDMProxy(keys, aKeySystem);

  
  
  
  
  
  
  
  
  MOZ_ASSERT(!keys->mCreatePromiseId, "Should only be created once!");
  keys->mCreatePromiseId = keys->StorePromise(promise);
  keys->AddRef();
  keys->mProxy->Init(keys->mCreatePromiseId);

  return promise.forget();
}

void
MediaKeys::OnCDMCreated(PromiseId aId)
{
  nsRefPtr<Promise> promise(RetrievePromise(aId));
  if (!promise) {
    NS_WARNING("MediaKeys tried to resolve a non-existent promise");
    return;
  }
  nsRefPtr<MediaKeys> keys(this);
  promise->MaybeResolve(keys);
  if (mCreatePromiseId == aId) {
    Release();
  }
}

already_AddRefed<Promise>
MediaKeys::LoadSession(const nsAString& aSessionId, ErrorResult& aRv)
{
  nsRefPtr<Promise> promise(MakePromise(aRv));
  if (aRv.Failed()) {
    return nullptr;
  }

  if (aSessionId.IsEmpty()) {
    promise->MaybeReject(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    
    return promise.forget();
  }

  
  if (mKeySessions.Contains(aSessionId)) {
    promise->MaybeReject(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return promise.forget();
  }

  
  nsRefPtr<MediaKeySession> session(
    new MediaKeySession(GetParentObject(), this, mKeySystem, SessionType::Persistent, aRv));
  if (aRv.Failed()) {
    return nullptr;
  }

  session->Init(aSessionId);
  auto pid = StorePromise(promise);
  mPendingSessions.Put(pid, session);
  mProxy->LoadSession(pid, aSessionId);

  return promise.forget();
}

already_AddRefed<Promise>
MediaKeys::CreateSession(const nsAString& initDataType,
                         const Uint8Array& aInitData,
                         SessionType aSessionType,
                         ErrorResult& aRv)
{
  aInitData.ComputeLengthAndData();
  nsRefPtr<Promise> promise(MakePromise(aRv));
  if (aRv.Failed()) {
    return nullptr;
  }
  nsRefPtr<MediaKeySession> session = new MediaKeySession(GetParentObject(),
                                                          this,
                                                          mKeySystem,
                                                          aSessionType, aRv);
  if (aRv.Failed()) {
    return nullptr;
  }

  auto pid = StorePromise(promise);
  
  mPendingSessions.Put(pid, session);
  mProxy->CreateSession(aSessionType,
                        pid,
                        initDataType,
                        aInitData);

  return promise.forget();
}

void
MediaKeys::OnSessionCreated(PromiseId aId, const nsAString& aSessionId)
{
  nsRefPtr<Promise> promise(RetrievePromise(aId));
  if (!promise) {
    NS_WARNING("MediaKeys tried to resolve a non-existent promise");
    return;
  }
  MOZ_ASSERT(mPendingSessions.Contains(aId));

  nsRefPtr<MediaKeySession> session;
  bool gotSession = mPendingSessions.Get(aId, getter_AddRefs(session));
  
  
  
  mPendingSessions.Remove(aId);
  if (!gotSession || !session) {
    NS_WARNING("Received activation for non-existent session!");
    promise->MaybeReject(NS_ERROR_DOM_INVALID_ACCESS_ERR);
    return;
  }

  session->Init(aSessionId);
  mKeySessions.Put(aSessionId, session);
  promise->MaybeResolve(session);
}

void
MediaKeys::OnSessionClosed(MediaKeySession* aSession)
{
  nsAutoString id;
  aSession->GetSessionId(id);
  mKeySessions.Remove(id);
}

already_AddRefed<MediaKeySession>
MediaKeys::GetSession(const nsAString& aSessionId)
{
  nsRefPtr<MediaKeySession> session;
  mKeySessions.Get(aSessionId, getter_AddRefs(session));
  return session.forget();
}

nsresult
MediaKeys::GetOrigin(nsString& aOutOrigin)
{
  MOZ_ASSERT(NS_IsMainThread());
  

  nsIPrincipal* principal = nullptr;
  nsCOMPtr<nsPIDOMWindow> pWindow = do_QueryInterface(GetParentObject());
  nsCOMPtr<nsIScriptObjectPrincipal> scriptPrincipal =
    do_QueryInterface(pWindow);
  if (scriptPrincipal) {
    principal = scriptPrincipal->GetPrincipal();
  }
  NS_ENSURE_TRUE(principal, NS_ERROR_FAILURE);

  nsresult res = nsContentUtils::GetUTFOrigin(principal, aOutOrigin);

  EME_LOG("EME Origin = '%s'", NS_ConvertUTF16toUTF8(aOutOrigin).get());

  return res;
}

} 
} 
