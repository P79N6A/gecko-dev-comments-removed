





#include "ServiceWorker.h"

#include "nsIDocument.h"
#include "nsPIDOMWindow.h"
#include "ServiceWorkerClient.h"
#include "ServiceWorkerManager.h"
#include "SharedWorker.h"
#include "WorkerPrivate.h"

#include "mozilla/Preferences.h"
#include "mozilla/dom/Promise.h"
#include "mozilla/dom/ServiceWorkerGlobalScopeBinding.h"

#ifdef XP_WIN
#undef PostMessage
#endif

using mozilla::ErrorResult;
using namespace mozilla::dom;

namespace mozilla {
namespace dom {
namespace workers {

bool
ServiceWorkerVisible(JSContext* aCx, JSObject* aObj)
{
  if (NS_IsMainThread()) {
    return Preferences::GetBool("dom.serviceWorkers.enabled", false);
  }

  ServiceWorkerGlobalScope* scope = nullptr;
  nsresult rv = UnwrapObject<prototypes::id::ServiceWorkerGlobalScope_workers,
                             mozilla::dom::ServiceWorkerGlobalScopeBinding_workers::NativeType>(aObj, scope);
  return NS_SUCCEEDED(rv);
}

ServiceWorker::ServiceWorker(nsPIDOMWindow* aWindow,
                             ServiceWorkerInfo* aInfo,
                             SharedWorker* aSharedWorker)
  : DOMEventTargetHelper(aWindow),
    mInfo(aInfo),
    mSharedWorker(aSharedWorker)
{
  AssertIsOnMainThread();
  MOZ_ASSERT(aInfo);
  MOZ_ASSERT(mSharedWorker);

  if (aWindow) {
    mDocument = aWindow->GetExtantDoc();
    mWindow = aWindow->GetOuterWindow();
  }

  
  mInfo->AppendWorker(this);
}

ServiceWorker::~ServiceWorker()
{
  AssertIsOnMainThread();
  mInfo->RemoveWorker(this);
}

NS_IMPL_ADDREF_INHERITED(ServiceWorker, DOMEventTargetHelper)
NS_IMPL_RELEASE_INHERITED(ServiceWorker, DOMEventTargetHelper)

NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION_INHERITED(ServiceWorker)
NS_INTERFACE_MAP_END_INHERITING(DOMEventTargetHelper)

NS_IMPL_CYCLE_COLLECTION_INHERITED(ServiceWorker, DOMEventTargetHelper,
                                   mSharedWorker, mDocument, mWindow)

JSObject*
ServiceWorker::WrapObject(JSContext* aCx, JS::Handle<JSObject*> aGivenProto)
{
  AssertIsOnMainThread();

  return ServiceWorkerBinding::Wrap(aCx, this, aGivenProto);
}

void
ServiceWorker::GetScriptURL(nsString& aURL) const
{
  CopyUTF8toUTF16(mInfo->ScriptSpec(), aURL);
}

void
ServiceWorker::PostMessage(JSContext* aCx, JS::Handle<JS::Value> aMessage,
                           const Optional<Sequence<JS::Value>>& aTransferable,
                           ErrorResult& aRv)
{
  WorkerPrivate* workerPrivate = GetWorkerPrivate();
  MOZ_ASSERT(workerPrivate);

  if (State() == ServiceWorkerState::Redundant) {
    aRv.Throw(NS_ERROR_DOM_INVALID_STATE_ERR);
    return;
  }

  MOZ_ASSERT(mDocument && mWindow,
             "Cannot call PostMessage on a ServiceWorker object that doesn't "
             "have a window");

  nsAutoPtr<ServiceWorkerClientInfo> clientInfo(
    new ServiceWorkerClientInfo(mDocument, mWindow));

  workerPrivate->PostMessageToServiceWorker(aCx, aMessage, aTransferable,
                                            clientInfo, aRv);
}

WorkerPrivate*
ServiceWorker::GetWorkerPrivate() const
{
  
  
  
  MOZ_ASSERT(mSharedWorker);
  return mSharedWorker->GetWorkerPrivate();
}

void
ServiceWorker::QueueStateChangeEvent(ServiceWorkerState aState)
{
  nsCOMPtr<nsIRunnable> r =
    NS_NewRunnableMethodWithArg<ServiceWorkerState>(this,
                                                    &ServiceWorker::DispatchStateChange,
                                                    aState);
  MOZ_ALWAYS_TRUE(NS_SUCCEEDED(NS_DispatchToMainThread(r)));
}

} 
} 
} 
