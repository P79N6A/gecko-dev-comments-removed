




#include "InternalRequest.h"

#include "nsIContentPolicy.h"
#include "nsIDocument.h"

#include "mozilla/ErrorResult.h"
#include "mozilla/dom/ScriptSettings.h"
#include "mozilla/dom/workers/Workers.h"

#include "WorkerPrivate.h"

namespace mozilla {
namespace dom {


already_AddRefed<InternalRequest>
InternalRequest::GetRequestConstructorCopy(nsIGlobalObject* aGlobal, ErrorResult& aRv) const
{
  nsRefPtr<InternalRequest> copy = new InternalRequest();
  copy->mURL.Assign(mURL);
  copy->SetMethod(mMethod);
  copy->mHeaders = new InternalHeaders(*mHeaders);

  copy->mBodyStream = mBodyStream;
  copy->mPreserveContentCodings = true;

  if (NS_IsMainThread()) {
    nsIPrincipal* principal = aGlobal->PrincipalOrNull();
    MOZ_ASSERT(principal);
    aRv = nsContentUtils::GetASCIIOrigin(principal, copy->mOrigin);
    if (NS_WARN_IF(aRv.Failed())) {
      return nullptr;
    }
  } else {
    workers::WorkerPrivate* worker = workers::GetCurrentThreadWorkerPrivate();
    MOZ_ASSERT(worker);
    worker->AssertIsOnWorkerThread();

    workers::WorkerPrivate::LocationInfo& location = worker->GetLocationInfo();
    copy->mOrigin = NS_ConvertUTF16toUTF8(location.mOrigin);
  }

  copy->mMode = mMode;
  copy->mCredentialsMode = mCredentialsMode;
  
  
  return copy.forget();
}

InternalRequest::~InternalRequest()
{
}

} 
} 
