




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

  already_AddRefed<InternalResponse> Clone();

  static already_AddRefed<InternalResponse>
  NetworkError()
  {
    nsRefPtr<InternalResponse> response = new InternalResponse(0, EmptyCString());
    response->mType = ResponseType::Error;
    return response.forget();
  }

  static already_AddRefed<InternalResponse>
  OpaqueResponse()
  {
    nsRefPtr<InternalResponse> response = new InternalResponse(0, EmptyCString());
    response->mType = ResponseType::Opaque;
    return response.forget();
  }

  
  
  static already_AddRefed<InternalResponse>
  BasicResponse(InternalResponse* aInner);

  
  
  static already_AddRefed<InternalResponse>
  CORSResponse(InternalResponse* aInner);

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

  
  void
  GetUrl(nsCString& aURL) const
  {
    aURL.Assign(mURL);
  }

  void
  SetUrl(const nsACString& aURL)
  {
    mURL.Assign(aURL);
  }

  bool
  FinalURL() const
  {
    return mFinalURL;
  }

  void
  SetFinalURL(bool aFinalURL)
  {
    mFinalURL = aFinalURL;
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
    
    MOZ_ASSERT(!mBody);
    mBody = aBody;
  }

private:
  ~InternalResponse()
  { }

  
  
  explicit InternalResponse(const InternalResponse& aOther);

  ResponseType mType;
  nsCString mTerminationReason;
  nsCString mURL;
  bool mFinalURL;
  const uint16_t mStatus;
  const nsCString mStatusText;
  nsRefPtr<InternalHeaders> mHeaders;
  nsCOMPtr<nsIInputStream> mBody;
  nsCString mContentType;
};

} 
} 

#endif 
