





#include "mozilla/CDMProxy.h"
#include "nsString.h"
#include "mozilla/dom/MediaKeys.h"
#include "mozilla/dom/MediaKeySession.h"
#include "mozIGeckoMediaPluginService.h"
#include "nsContentCID.h"
#include "nsServiceManagerUtils.h"
#include "MainThreadUtils.h"
#include "mozilla/EMELog.h"
#include "nsIConsoleService.h"
#include "prenv.h"
#include "mozilla/PodOperations.h"
#include "mozilla/CDMCallbackProxy.h"

namespace mozilla {

CDMProxy::CDMProxy(dom::MediaKeys* aKeys, const nsAString& aKeySystem)
  : mKeys(aKeys)
  , mKeySystem(aKeySystem)
  , mCDM(nullptr)
  , mDecryptionJobCount(0)
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

  nsresult rv = mKeys->GetOrigin(mOrigin);
  if (NS_FAILED(rv)) {
    RejectPromise(aPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  EME_LOG("Creating CDMProxy for origin='%s'",
          NS_ConvertUTF16toUTF8(GetOrigin()).get());

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

  nsRefPtr<nsIRunnable> task(NS_NewRunnableMethodWithArg<uint32_t>(this, &CDMProxy::gmp_Init, aPromiseId));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

#ifdef DEBUG
bool
CDMProxy::IsOnGMPThread()
{
  return NS_GetCurrentThread() == mGMPThread;
}
#endif

void
CDMProxy::gmp_Init(uint32_t aPromiseId)
{
  MOZ_ASSERT(IsOnGMPThread());

  nsCOMPtr<mozIGeckoMediaPluginService> mps =
    do_GetService("@mozilla.org/gecko-media-plugin-service;1");
  if (!mps) {
    RejectPromise(aPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  nsTArray<nsCString> tags;
  tags.AppendElement(NS_ConvertUTF16toUTF8(mKeySystem));
  nsresult rv = mps->GetGMPDecryptor(&tags, GetOrigin(), &mCDM);
  if (NS_FAILED(rv) || !mCDM) {
    RejectPromise(aPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
  } else {
    mCallback = new CDMCallbackProxy(this);
    mCDM->Init(mCallback);
    nsRefPtr<nsIRunnable> task(NS_NewRunnableMethodWithArg<uint32_t>(this, &CDMProxy::OnCDMCreated, aPromiseId));
    NS_DispatchToMainThread(task);
  }
}

void
CDMProxy::OnCDMCreated(uint32_t aPromiseId)
{
  MOZ_ASSERT(NS_IsMainThread());
  if (!mKeys.IsNull()) {
    mKeys->OnCDMCreated(aPromiseId);
  } else {
    NS_WARNING("CDMProxy unable to reject promise!");
  }
}

void
CDMProxy::CreateSession(dom::SessionType aSessionType,
                        PromiseId aPromiseId,
                        const nsAString& aInitDataType,
                        const Uint8Array& aInitData)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);

  nsAutoPtr<CreateSessionData> data(new CreateSessionData());
  data->mSessionType = aSessionType;
  data->mPromiseId = aPromiseId;
  data->mInitDataType = NS_ConvertUTF16toUTF8(aInitDataType);
  data->mInitData.AppendElements(aInitData.Data(), aInitData.Length());

  nsRefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<nsAutoPtr<CreateSessionData>>(this, &CDMProxy::gmp_CreateSession, data));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

GMPSessionType
ToGMPSessionType(dom::SessionType aSessionType) {
  switch (aSessionType) {
    case dom::SessionType::Temporary: return kGMPTemporySession;
    case dom::SessionType::Persistent: return kGMPPersistentSession;
    default: return kGMPTemporySession;
  };
};

void
CDMProxy::gmp_CreateSession(nsAutoPtr<CreateSessionData> aData)
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mCDM) {
    RejectPromise(aData->mPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mCDM->CreateSession(aData->mPromiseId,
                      aData->mInitDataType,
                      aData->mInitData,
                      ToGMPSessionType(aData->mSessionType));
}

void
CDMProxy::LoadSession(PromiseId aPromiseId,
                      const nsAString& aSessionId)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);

  nsAutoPtr<SessionOpData> data(new SessionOpData());
  data->mPromiseId = aPromiseId;
  data->mSessionId = NS_ConvertUTF16toUTF8(aSessionId);
  nsRefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<nsAutoPtr<SessionOpData>>(this, &CDMProxy::gmp_LoadSession, data));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

void
CDMProxy::gmp_LoadSession(nsAutoPtr<SessionOpData> aData)
{
  MOZ_ASSERT(IsOnGMPThread());

  if (!mCDM) {
    RejectPromise(aData->mPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mCDM->LoadSession(aData->mPromiseId, aData->mSessionId);
}

void
CDMProxy::SetServerCertificate(PromiseId aPromiseId,
                               const Uint8Array& aCert)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);

  nsAutoPtr<SetServerCertificateData> data;
  data->mPromiseId = aPromiseId;
  data->mCert.AppendElements(aCert.Data(), aCert.Length());
  nsRefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<nsAutoPtr<SetServerCertificateData>>(this, &CDMProxy::gmp_SetServerCertificate, data));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

void
CDMProxy::gmp_SetServerCertificate(nsAutoPtr<SetServerCertificateData> aData)
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mCDM) {
    RejectPromise(aData->mPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mCDM->SetServerCertificate(aData->mPromiseId, aData->mCert);
}

void
CDMProxy::UpdateSession(const nsAString& aSessionId,
                        PromiseId aPromiseId,
                        const Uint8Array& aResponse)
{
  MOZ_ASSERT(NS_IsMainThread());
  MOZ_ASSERT(mGMPThread);
  NS_ENSURE_TRUE_VOID(!mKeys.IsNull());

  nsAutoPtr<UpdateSessionData> data(new UpdateSessionData());
  data->mPromiseId = aPromiseId;
  data->mSessionId = NS_ConvertUTF16toUTF8(aSessionId);
  data->mResponse.AppendElements(aResponse.Data(), aResponse.Length());
  nsRefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<nsAutoPtr<UpdateSessionData>>(this, &CDMProxy::gmp_UpdateSession, data));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

void
CDMProxy::gmp_UpdateSession(nsAutoPtr<UpdateSessionData> aData)
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mCDM) {
    RejectPromise(aData->mPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mCDM->UpdateSession(aData->mPromiseId,
                      aData->mSessionId,
                      aData->mResponse);
}

void
CDMProxy::CloseSession(const nsAString& aSessionId,
                       PromiseId aPromiseId)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE_VOID(!mKeys.IsNull());

  {
    CDMCaps::AutoLock caps(Capabilites());
    caps.DropKeysForSession(aSessionId);
  }
  nsAutoPtr<SessionOpData> data(new SessionOpData());
  data->mPromiseId = aPromiseId;
  data->mSessionId = NS_ConvertUTF16toUTF8(aSessionId);
  nsRefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<nsAutoPtr<SessionOpData>>(this, &CDMProxy::gmp_CloseSession, data));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

void
CDMProxy::gmp_CloseSession(nsAutoPtr<SessionOpData> aData)
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mCDM) {
    RejectPromise(aData->mPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mCDM->CloseSession(aData->mPromiseId, aData->mSessionId);
}

void
CDMProxy::RemoveSession(const nsAString& aSessionId,
                        PromiseId aPromiseId)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_ENSURE_TRUE_VOID(!mKeys.IsNull());

  {
    CDMCaps::AutoLock caps(Capabilites());
    caps.DropKeysForSession(aSessionId);
  }
  nsAutoPtr<SessionOpData> data(new SessionOpData());
  data->mPromiseId = aPromiseId;
  data->mSessionId = NS_ConvertUTF16toUTF8(aSessionId);
  nsRefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<nsAutoPtr<SessionOpData>>(this, &CDMProxy::gmp_RemoveSession, data));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

void
CDMProxy::gmp_RemoveSession(nsAutoPtr<SessionOpData> aData)
{
  MOZ_ASSERT(IsOnGMPThread());
  if (!mCDM) {
    RejectPromise(aData->mPromiseId, NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }
  mCDM->RemoveSession(aData->mPromiseId, aData->mSessionId);
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

const nsAString&
CDMProxy::GetOrigin() const
{
  return mOrigin;
}

void
CDMProxy::OnResolveNewSessionPromise(uint32_t aPromiseId,
                                     const nsAString& aSessionId)
{
  MOZ_ASSERT(NS_IsMainThread());
  mKeys->OnSessionCreated(aPromiseId, aSessionId);
}

void
CDMProxy::OnSessionMessage(const nsAString& aSessionId,
                           nsTArray<uint8_t>& aMessage,
                           const nsAString& aDestinationURL)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsRefPtr<dom::MediaKeySession> session(mKeys->GetSession(aSessionId));
  if (session) {
    session->DispatchKeyMessage(aMessage, aDestinationURL);
  }
}

void
CDMProxy::OnExpirationChange(const nsAString& aSessionId,
                             GMPTimestamp aExpiryTime)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_WARNING("CDMProxy::OnExpirationChange() not implemented");
}

void
CDMProxy::OnSessionClosed(const nsAString& aSessionId)
{
  MOZ_ASSERT(NS_IsMainThread());
  NS_WARNING("CDMProxy::OnSessionClosed() not implemented");
}

static void
LogToConsole(const nsAString& aMsg)
{
  nsCOMPtr<nsIConsoleService> console(
    do_GetService("@mozilla.org/consoleservice;1"));
  if (!console) {
    NS_WARNING("Failed to log message to console.");
    return;
  }
  nsAutoString msg(aMsg);
  console->LogStringMessage(msg.get());
}

void
CDMProxy::OnSessionError(const nsAString& aSessionId,
                         nsresult aException,
                         uint32_t aSystemCode,
                         const nsAString& aMsg)
{
  MOZ_ASSERT(NS_IsMainThread());
  nsRefPtr<dom::MediaKeySession> session(mKeys->GetSession(aSessionId));
  if (session) {
    session->DispatchKeyError(aSystemCode);
  }
  LogToConsole(aMsg);
}

void
CDMProxy::OnRejectPromise(uint32_t aPromiseId,
                          nsresult aDOMException,
                          const nsAString& aMsg)
{
  MOZ_ASSERT(NS_IsMainThread());
  RejectPromise(aPromiseId, aDOMException);
  LogToConsole(aMsg);
}

const nsString&
CDMProxy::KeySystem() const
{
  return mKeySystem;
}

CDMCaps&
CDMProxy::Capabilites() {
  return mCapabilites;
}

void
CDMProxy::Decrypt(mp4_demuxer::MP4Sample* aSample,
                  DecryptionClient* aClient)
{
  nsAutoPtr<DecryptJob> job(new DecryptJob(aSample, aClient));
  nsRefPtr<nsIRunnable> task(
    NS_NewRunnableMethodWithArg<nsAutoPtr<DecryptJob>>(this, &CDMProxy::gmp_Decrypt, job));
  mGMPThread->Dispatch(task, NS_DISPATCH_NORMAL);
}

void
CDMProxy::gmp_Decrypt(nsAutoPtr<DecryptJob> aJob)
{
  MOZ_ASSERT(IsOnGMPThread());
  MOZ_ASSERT(aJob->mClient);
  MOZ_ASSERT(aJob->mSample);

  if (!mCDM) {
    aJob->mClient->Decrypted(NS_ERROR_FAILURE, nullptr);
    return;
  }

  aJob->mId = ++mDecryptionJobCount;
  nsTArray<uint8_t> data;
  data.AppendElements(aJob->mSample->data, aJob->mSample->size);
  mCDM->Decrypt(aJob->mId, aJob->mSample->crypto, data);
  mDecryptionJobs.AppendElement(aJob.forget());
}

void
CDMProxy::gmp_Decrypted(uint32_t aId,
                        GMPErr aResult,
                        const nsTArray<uint8_t>& aDecryptedData)
{
  MOZ_ASSERT(IsOnGMPThread());
  for (size_t i = 0; i < mDecryptionJobs.Length(); i++) {
    DecryptJob* job = mDecryptionJobs[i];
    if (job->mId == aId) {
      if (aDecryptedData.Length() != job->mSample->size) {
        NS_WARNING("CDM returned incorrect number of decrypted bytes");
      }
      PodCopy(job->mSample->data,
              aDecryptedData.Elements(),
              std::min<size_t>(aDecryptedData.Length(), job->mSample->size));
      nsresult rv = GMP_SUCCEEDED(aResult) ? NS_OK : NS_ERROR_FAILURE;
      job->mClient->Decrypted(rv, job->mSample.forget());
      mDecryptionJobs.RemoveElementAt(i);
      return;
    } else {
      NS_WARNING("GMPDecryptorChild returned incorrect job ID");
    }
  }
}

void
CDMProxy::gmp_Terminated()
{
  MOZ_ASSERT(IsOnGMPThread());
  EME_LOG("CDM terminated");
  if (mCDM) {
    mCDM->Close();
    mCDM = nullptr;
  }
}

} 
