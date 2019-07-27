




#include "InternalResponse.h"

#include "nsIDOMFile.h"

#include "mozilla/dom/Headers.h"

namespace mozilla {
namespace dom {

InternalResponse::InternalResponse(uint16_t aStatus, const nsACString& aStatusText)
  : mType(ResponseType::Default)
  , mStatus(aStatus)
  , mStatusText(aStatusText)
  , mHeaders(new Headers(nullptr, HeadersGuardEnum::Response))
{
}

} 
} 
