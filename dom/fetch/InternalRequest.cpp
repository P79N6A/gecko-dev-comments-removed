




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

  copy->mContext = nsIContentPolicy::TYPE_FETCH;
  copy->mMode = mMode;
  copy->mCredentialsMode = mCredentialsMode;
  return copy.forget();
}

InternalRequest::~InternalRequest()
{
}

} 
} 
