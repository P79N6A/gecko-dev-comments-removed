





#include "mozilla/dom/cache/PrincipalVerifier.h"

#include "mozilla/AppProcessChecker.h"
#include "mozilla/dom/ContentParent.h"
#include "mozilla/dom/cache/ManagerId.h"
#include "mozilla/ipc/BackgroundParent.h"
#include "mozilla/ipc/PBackgroundParent.h"
#include "mozilla/ipc/BackgroundUtils.h"
#include "nsIPrincipal.h"
#include "nsIScriptSecurityManager.h"
#include "nsNetUtil.h"

namespace mozilla {
namespace dom {
namespace cache {

using mozilla::ipc::AssertIsOnBackgroundThread;
using mozilla::ipc::BackgroundParent;
using mozilla::ipc::PBackgroundParent;
using mozilla::ipc::PrincipalInfo;
using mozilla::ipc::PrincipalInfoToPrincipal;


already_AddRefed<PrincipalVerifier>
PrincipalVerifier::CreateAndDispatch(Listener* aListener,
                                     PBackgroundParent* aActor,
                                     const PrincipalInfo& aPrincipalInfo)
{
  
  
  AssertIsOnBackgroundThread();

  nsRefPtr<PrincipalVerifier> verifier = new PrincipalVerifier(aListener,
                                                               aActor,
                                                               aPrincipalInfo);

  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(verifier)));

  return verifier.forget();
}

void
PrincipalVerifier::AddListener(Listener* aListener)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aListener);
  MOZ_ASSERT(!mListenerList.Contains(aListener));
  mListenerList.AppendElement(aListener);
}

void
PrincipalVerifier::RemoveListener(Listener* aListener)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(aListener);
  MOZ_ALWAYS_TRUE(mListenerList.RemoveElement(aListener));
}

PrincipalVerifier::PrincipalVerifier(Listener* aListener,
                                     PBackgroundParent* aActor,
                                     const PrincipalInfo& aPrincipalInfo)
  : mActor(BackgroundParent::GetContentParent(aActor))
  , mPrincipalInfo(aPrincipalInfo)
  , mInitiatingThread(NS_GetCurrentThread())
  , mResult(NS_OK)
{
  AssertIsOnBackgroundThread();
  MOZ_ASSERT(mInitiatingThread);
  MOZ_ASSERT(aListener);

  mListenerList.AppendElement(aListener);
}

PrincipalVerifier::~PrincipalVerifier()
{
  
  
  

  MOZ_ASSERT(mListenerList.IsEmpty());

  
  
  MOZ_ASSERT(!mActor);
}

NS_IMETHODIMP
PrincipalVerifier::Run()
{
  
  

  if (NS_IsMainThread()) {
    VerifyOnMainThread();
    return NS_OK;
  }

  CompleteOnInitiatingThread();
  return NS_OK;
}

void
PrincipalVerifier::VerifyOnMainThread()
{
  MOZ_ASSERT(NS_IsMainThread());

  
  
  nsRefPtr<ContentParent> actor;
  actor.swap(mActor);

  nsresult rv;
  nsRefPtr<nsIPrincipal> principal = PrincipalInfoToPrincipal(mPrincipalInfo,
                                                              &rv);
  if (NS_WARN_IF(NS_FAILED(rv))) {
    DispatchToInitiatingThread(rv);
    return;
  }

  
  
  if (NS_WARN_IF(principal->GetIsNullPrincipal() ||
                 principal->GetUnknownAppId())) {
    DispatchToInitiatingThread(NS_ERROR_FAILURE);
    return;
  }

  
  if (NS_WARN_IF(actor && !AssertAppPrincipal(actor, principal))) {
    DispatchToInitiatingThread(NS_ERROR_FAILURE);
    return;
  }
  actor = nullptr;

  nsCOMPtr<nsIScriptSecurityManager> ssm = nsContentUtils::GetSecurityManager();
  if (NS_WARN_IF(!ssm)) {
    DispatchToInitiatingThread(NS_ERROR_ILLEGAL_DURING_SHUTDOWN);
    return;
  }

#ifdef DEBUG
  
  
  
  if (!ssm->IsSystemPrincipal(principal)) {
    nsAutoCString origin;
    rv = principal->GetOrigin(getter_Copies(origin));
    if (NS_WARN_IF(NS_FAILED(rv))) {
      DispatchToInitiatingThread(rv);
      return;
    }
    nsCOMPtr<nsIURI> uri;
    rv = NS_NewURI(getter_AddRefs(uri), origin);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      DispatchToInitiatingThread(rv);
      return;
    }
    rv = principal->CheckMayLoad(uri, false, false);
    if (NS_WARN_IF(NS_FAILED(rv))) {
      DispatchToInitiatingThread(rv);
      return;
    }
  }
#endif

  rv = ManagerId::Create(principal, getter_AddRefs(mManagerId));
  if (NS_WARN_IF(NS_FAILED(rv))) {
    DispatchToInitiatingThread(rv);
    return;
  }

  DispatchToInitiatingThread(NS_OK);
}

void
PrincipalVerifier::CompleteOnInitiatingThread()
{
  AssertIsOnBackgroundThread();
  ListenerList::ForwardIterator iter(mListenerList);
  while (iter.HasMore()) {
    iter.GetNext()->OnPrincipalVerified(mResult, mManagerId);
  }

  
  MOZ_ASSERT(mListenerList.IsEmpty());
}

void
PrincipalVerifier::DispatchToInitiatingThread(nsresult aRv)
{
  MOZ_ASSERT(NS_IsMainThread());

  mResult = aRv;

  
  
  
  
  
  nsresult rv = mInitiatingThread->Dispatch(this, nsIThread::DISPATCH_NORMAL);
  if (NS_FAILED(rv)) {
    NS_WARNING("Cache unable to complete principal verification due to shutdown.");
  }
}

} 
} 
} 
