




#include "InternalResponse.h"

#include "nsIDOMFile.h"

#include "mozilla/dom/InternalHeaders.h"

namespace mozilla {
namespace dom {

InternalResponse::InternalResponse(uint16_t aStatus, const nsACString& aStatusText)
  : mType(ResponseType::Default)
  , mFinalURL(false)
  , mStatus(aStatus)
  , mStatusText(aStatusText)
  , mHeaders(new InternalHeaders(HeadersGuardEnum::Response))
{
}



InternalResponse::InternalResponse(const InternalResponse& aOther)
  : mType(aOther.mType)
  , mTerminationReason(aOther.mTerminationReason)
  , mURL(aOther.mURL)
  , mFinalURL(aOther.mFinalURL)
  , mStatus(aOther.mStatus)
  , mStatusText(aOther.mStatusText)
  , mBody(aOther.mBody)
  , mContentType(aOther.mContentType)
{
}


already_AddRefed<InternalResponse>
InternalResponse::BasicResponse(InternalResponse* aInner)
{
  MOZ_ASSERT(aInner);
  nsRefPtr<InternalResponse> basic = new InternalResponse(*aInner);
  basic->mType = ResponseType::Basic;
  basic->mHeaders = InternalHeaders::BasicHeaders(aInner->mHeaders);
  return basic.forget();
}


already_AddRefed<InternalResponse>
InternalResponse::CORSResponse(InternalResponse* aInner)
{
  MOZ_ASSERT(aInner);
  nsRefPtr<InternalResponse> cors = new InternalResponse(*aInner);
  cors->mType = ResponseType::Cors;
  cors->mHeaders = InternalHeaders::CORSHeaders(aInner->mHeaders);
  return cors.forget();
}

} 
} 
