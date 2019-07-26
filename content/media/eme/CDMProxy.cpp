





#include "mozilla/CDMProxy.h"
#include "nsString.h"
#include "mozilla/dom/MediaKeys.h"
#include "mozilla/dom/MediaKeySession.h"
#include "mozIGeckoMediaPluginService.h"
#include "nsContentCID.h"
#include "nsServiceManagerUtils.h"
#include "MainThreadUtils.h"





namespace mozilla {

CDMProxy::CDMProxy(dom::MediaKeys* aKeys, const nsAString& aKeySystem)
  : mKeys(aKeys)
  , mKeySystem(aKeySystem)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_COUNT_CTOR(CDMProxy);
}

CDMProxy::~CDMProxy()
{
  MOZ_COUNT_DTOR(CDMProxy);
}

void
CDMProxy::Init(PromiseId aPromiseId)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mGMPThread) {
    nsCOMPtr<mozIGeckoMediaPluginService> mps =
      do_GetService("@mozilla.org/gecko-media-plugin-service;1");
    if (!mps) {
      RejectPromise(aPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
      return;
    }
    mps->GetThread(getter_AddRefs(mGMPThread));
    if (!mGMPThread) {
      RejectPromise(aPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
      return;
    }
  }

  

  mKeys->OnCDMCreated(aPromiseId);
}

static int sFakeSessionIdNum = 0;

void
CDMProxy::CreateSession(dom::SessionType aSessionType,
                        PromiseId aPromiseId,
                        const nsAString& aInitDataType,
                        const Uint8Array& aInitData)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);

  

  
  nsAutoString id;
  id.AppendASCII("FakeSessionId_");
  id.AppendInt(sFakeSessionIdNum++);

  mKeys->OnSessionActivated(aPromiseId, id);
}

void
CDMProxy::LoadSession(PromiseId aPromiseId,
                      const nsAString& aSessionId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);

  
  

  mKeys->OnSessionActivated(aPromiseId, aSessionId);
}

void
CDMProxy::SetServerCertificate(PromiseId aPromiseId,
                               const Uint8Array& aCertData)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);

  

  ResolvePromise(aPromiseId);
}

static int sUpdateCount = 0;

void
CDMProxy::UpdateSession(const nsAString& aSessionId,
                        PromiseId aPromiseId,
                        const Uint8Array& aResponse)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);
  NS_ENSURE_TRUE_VOID(!mKeys.IsNull());

  

  nsRefPtr<dom::MediaKeySession> session(mKeys->GetSession(aSessionId));
  nsAutoCString str(NS_LITERAL_CSTRING("Update_"));
  str.AppendInt(sUpdateCount++);
  nsTArray<uint8_t> msg;
  msg.AppendElements(str.get(), str.Length());
  session->DispatchKeyMessage(msg, NS_LITERAL_STRING("http://bogus.url"));
  ResolvePromise(aPromiseId);
}

void
CDMProxy::CloseSession(const nsAString& aSessionId,
                       PromiseId aPromiseId)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE_VOID(!mKeys.IsNull());

  

  nsRefPtr<dom::MediaKeySession> session(mKeys->GetSession(aSessionId));

  
  
  
  session->OnClosed();

  
  ResolvePromise(aPromiseId);
}

void
CDMProxy::RemoveSession(const nsAString& aSessionId,
                        PromiseId aPromiseId)
{
  MOZ_ASSERT(NS_IsMainThread());

  

  
  
  CloseSession(aSessionId, aPromiseId);
}

void
CDMProxy::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread());
  mKeys.Clear();
}

void
CDMProxy::RejectPromise(PromiseId aId, nsresult aCode)
{
  if (NS_IsMainThread()) {
    if (!mKeys.IsNull()) {
      mKeys->RejectPromise(aId, aCode);
    } else {
      NS_WARNING("CDMProxy unable to reject promise!");
    }
  } else {
    nsRefPtr<nsIRunnable> task(new RejectPromiseTask(this, aId, aCode));
    NS_DispatchToMainThread(task);
  }
}

void
CDMProxy::ResolvePromise(PromiseId aId)
{
  if (NS_IsMainThread()) {
    if (!mKeys.IsNull()) {
      mKeys->ResolvePromise(aId);
    } else {
      NS_WARNING("CDMProxy unable to resolve promise!");
    }
  } else {
    nsRefPtr<nsIRunnable> task;
    task = NS_NewRunnableMethodWithArg<PromiseId>(this,
                                                  &CDMProxy::ResolvePromise,
                                                  aId);
    NS_DispatchToMainThread(task);
  }
}

} 
