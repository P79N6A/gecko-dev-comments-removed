




#include "InternalRequest.h"

#include "nsIContentPolicy.h"
#include "nsIDocument.h"
#include "nsStreamUtils.h"

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
  copy->SetUnsafeRequest();

  copy->mBodyStream = mBodyStream;
  copy->mForceOriginHeader = true;
  
  
  
  copy->mSameOriginDataURL = true;
  copy->mPreserveContentCodings = true;
  

  copy->mContentPolicyType = nsIContentPolicy::TYPE_FETCH;
  copy->mMode = mMode;
  copy->mCredentialsMode = mCredentialsMode;
  copy->mCacheMode = mCacheMode;
  return copy.forget();
}

already_AddRefed<InternalRequest>
InternalRequest::Clone()
{
  nsRefPtr<InternalRequest> clone = new InternalRequest(*this);

  if (!mBodyStream) {
    return clone.forget();
  }

  nsCOMPtr<nsIInputStream> clonedBody;
  nsCOMPtr<nsIInputStream> replacementBody;

  nsresult rv = NS_CloneInputStream(mBodyStream, getter_AddRefs(clonedBody),
                                    getter_AddRefs(replacementBody));
  if (NS_WARN_IF(NS_FAILED(rv))) { return nullptr; }

  clone->mBodyStream.swap(clonedBody);
  if (replacementBody) {
    mBodyStream.swap(replacementBody);
  }

  return clone.forget();
}

InternalRequest::InternalRequest(const InternalRequest& aOther)
  : mMethod(aOther.mMethod)
  , mURL(aOther.mURL)
  , mHeaders(new InternalHeaders(*aOther.mHeaders))
  , mContentPolicyType(aOther.mContentPolicyType)
  , mReferrer(aOther.mReferrer)
  , mMode(aOther.mMode)
  , mCredentialsMode(aOther.mCredentialsMode)
  , mResponseTainting(aOther.mResponseTainting)
  , mCacheMode(aOther.mCacheMode)
  , mAuthenticationFlag(aOther.mAuthenticationFlag)
  , mForceOriginHeader(aOther.mForceOriginHeader)
  , mPreserveContentCodings(aOther.mPreserveContentCodings)
  , mSameOriginDataURL(aOther.mSameOriginDataURL)
  , mSandboxedStorageAreaURLs(aOther.mSandboxedStorageAreaURLs)
  , mSkipServiceWorker(aOther.mSkipServiceWorker)
  , mSynchronous(aOther.mSynchronous)
  , mUnsafeRequest(aOther.mUnsafeRequest)
  , mUseURLCredentials(aOther.mUseURLCredentials)
{
  
}

InternalRequest::~InternalRequest()
{
}

} 
} 
