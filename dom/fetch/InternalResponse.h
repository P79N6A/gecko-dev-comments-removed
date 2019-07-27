




#ifndef mozilla_dom_InternalResponse_h
#define mozilla_dom_InternalResponse_h

#include "nsIInputStream.h"
#include "nsISupportsImpl.h"

#include "mozilla/dom/ResponseBinding.h"

namespace mozilla {
namespace dom {

class InternalHeaders;

class InternalResponse MOZ_FINAL
{
  friend class FetchDriver;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(InternalResponse)

  InternalResponse(uint16_t aStatus, const nsACString& aStatusText);

  explicit InternalResponse(const InternalResponse& aOther) MOZ_DELETE;

  static already_AddRefed<InternalResponse>
  NetworkError()
  {
    nsRefPtr<InternalResponse> response = new InternalResponse(0, EmptyCString());
    response->mType = ResponseType::Error;
    return response.forget();
  }

  ResponseType
  Type() const
  {
    return mType;
  }

  bool
  IsError() const
  {
    return Type() == ResponseType::Error;
  }

  
  nsCString&
  GetUrl()
  {
    return mURL;
  }

  uint16_t
  GetStatus() const
  {
    return mStatus;
  }

  const nsCString&
  GetStatusText() const
  {
    return mStatusText;
  }

  InternalHeaders*
  Headers()
  {
    return mHeaders;
  }

  void
  GetBody(nsIInputStream** aStream)
  {
    nsCOMPtr<nsIInputStream> stream = mBody;
    stream.forget(aStream);
  }

  void
  SetBody(nsIInputStream* aBody)
  {
    mBody = aBody;
  }

private:
  ~InternalResponse()
  { }

  ResponseType mType;
  nsCString mTerminationReason;
  nsCString mURL;
  const uint16_t mStatus;
  const nsCString mStatusText;
  nsRefPtr<InternalHeaders> mHeaders;
  nsCOMPtr<nsIInputStream> mBody;
  nsCString mContentType;
};

} 
} 

#endif 
