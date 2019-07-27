




#ifndef mozilla_dom_InternalRequest_h
#define mozilla_dom_InternalRequest_h

#include "mozilla/dom/HeadersBinding.h"
#include "mozilla/dom/InternalHeaders.h"
#include "mozilla/dom/RequestBinding.h"

#include "nsIContentPolicy.h"
#include "nsIInputStream.h"
#include "nsISupportsImpl.h"

class nsIDocument;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class FetchBodyStream;
class Request;

class InternalRequest MOZ_FINAL
{
  friend class Request;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(InternalRequest)

  enum ContextFrameType
  {
    FRAMETYPE_AUXILIARY = 0,
    FRAMETYPE_TOP_LEVEL,
    FRAMETYPE_NESTED,
    FRAMETYPE_NONE,
  };

  
  enum ReferrerType
  {
    REFERRER_NONE = 0,
    REFERRER_CLIENT,
    REFERRER_URL,
  };

  enum ResponseTainting
  {
    RESPONSETAINT_BASIC,
    RESPONSETAINT_CORS,
    RESPONSETAINT_OPAQUE,
  };

  explicit InternalRequest()
    : mMethod("GET")
    , mHeaders(new InternalHeaders(HeadersGuardEnum::None))
    , mContextFrameType(FRAMETYPE_NONE)
    , mReferrerType(REFERRER_CLIENT)
    , mMode(RequestMode::No_cors)
    , mCredentialsMode(RequestCredentials::Omit)
    , mResponseTainting(RESPONSETAINT_BASIC)
    , mCacheMode(RequestCache::Default)
    , mRedirectCount(0)
    , mAuthenticationFlag(false)
    , mForceOriginHeader(false)
    , mManualRedirect(false)
    , mPreserveContentCodings(false)
      
      
      
      
    , mSameOriginDataURL(true)
    , mSkipServiceWorker(false)
    , mSynchronous(false)
    , mUnsafeRequest(false)
    , mUseURLCredentials(false)
  {
  }

  explicit InternalRequest(const InternalRequest& aOther)
    : mMethod(aOther.mMethod)
    , mURL(aOther.mURL)
    , mHeaders(aOther.mHeaders)
    , mBodyStream(aOther.mBodyStream)
    , mContext(aOther.mContext)
    , mContextFrameType(aOther.mContextFrameType)
    , mReferrerType(aOther.mReferrerType)
    , mReferrerURL(aOther.mReferrerURL)
    , mMode(aOther.mMode)
    , mCredentialsMode(aOther.mCredentialsMode)
    , mResponseTainting(aOther.mResponseTainting)
    , mCacheMode(aOther.mCacheMode)
    , mRedirectCount(aOther.mRedirectCount)
    , mAuthenticationFlag(aOther.mAuthenticationFlag)
    , mForceOriginHeader(aOther.mForceOriginHeader)
    , mManualRedirect(aOther.mManualRedirect)
    , mPreserveContentCodings(aOther.mPreserveContentCodings)
    , mSameOriginDataURL(aOther.mSameOriginDataURL)
    , mSandboxedStorageAreaURLs(aOther.mSandboxedStorageAreaURLs)
    , mSkipServiceWorker(aOther.mSkipServiceWorker)
    , mSynchronous(aOther.mSynchronous)
    , mUnsafeRequest(aOther.mUnsafeRequest)
    , mUseURLCredentials(aOther.mUseURLCredentials)
  {
  }

  void
  GetMethod(nsCString& aMethod) const
  {
    aMethod.Assign(mMethod);
  }

  void
  SetMethod(const nsACString& aMethod)
  {
    mMethod.Assign(aMethod);
  }

  bool
  HasSimpleMethod() const
  {
    return mMethod.LowerCaseEqualsASCII("get") ||
           mMethod.LowerCaseEqualsASCII("post") ||
           mMethod.LowerCaseEqualsASCII("head");
  }

  void
  GetURL(nsCString& aURL) const
  {
    aURL.Assign(mURL);
  }

  bool
  ReferrerIsNone() const
  {
    return mReferrerType == REFERRER_NONE;
  }

  bool
  ReferrerIsURL() const
  {
    return mReferrerType == REFERRER_URL;
  }

  bool
  ReferrerIsClient() const
  {
    return mReferrerType == REFERRER_CLIENT;
  }

  nsCString
  ReferrerAsURL() const
  {
    MOZ_ASSERT(ReferrerIsURL());
    return mReferrerURL;
  }

  void
  SetReferrer(const nsACString& aReferrer)
  {
    
    MOZ_ASSERT(!ReferrerIsNone());
    mReferrerType = REFERRER_URL;
    mReferrerURL.Assign(aReferrer);
  }

  bool
  IsSynchronous() const
  {
    return mSynchronous;
  }

  RequestMode
  Mode() const
  {
    return mMode;
  }

  void
  SetMode(RequestMode aMode)
  {
    mMode = aMode;
  }

  void
  SetCredentialsMode(RequestCredentials aCredentialsMode)
  {
    mCredentialsMode = aCredentialsMode;
  }

  ResponseTainting
  GetResponseTainting() const
  {
    return mResponseTainting;
  }

  void
  SetResponseTainting(ResponseTainting aTainting)
  {
    mResponseTainting = aTainting;
  }

  RequestCache
  GetCacheMode() const
  {
    return mCacheMode;
  }

  nsContentPolicyType
  GetContext() const
  {
    return mContext;
  }

  bool
  UnsafeRequest() const
  {
    return mUnsafeRequest;
  }

  InternalHeaders*
  Headers()
  {
    return mHeaders;
  }

  bool
  ForceOriginHeader()
  {
    return mForceOriginHeader;
  }

  bool
  SameOriginDataURL() const
  {
    return mSameOriginDataURL;
  }

  void
  SetBody(nsIInputStream* aStream)
  {
    
    MOZ_ASSERT(!mBodyStream);
    mBodyStream = aStream;
  }

  
  
  void
  GetBody(nsIInputStream** aStream)
  {
    nsCOMPtr<nsIInputStream> s = mBodyStream;
    s.forget(aStream);
  }

  
  already_AddRefed<InternalRequest>
  GetRequestConstructorCopy(nsIGlobalObject* aGlobal, ErrorResult& aRv) const;

private:
  ~InternalRequest();

  void
  SetURL(const nsACString& aURL)
  {
    mURL.Assign(aURL);
  }

  nsCString mMethod;
  nsCString mURL;
  nsRefPtr<InternalHeaders> mHeaders;
  nsCOMPtr<nsIInputStream> mBodyStream;

  
  
  nsContentPolicyType mContext;

  ContextFrameType mContextFrameType;
  ReferrerType mReferrerType;

  
  nsCString mReferrerURL;

  RequestMode mMode;
  RequestCredentials mCredentialsMode;
  ResponseTainting mResponseTainting;
  RequestCache mCacheMode;

  uint32_t mRedirectCount;

  bool mAuthenticationFlag;
  bool mForceOriginHeader;
  bool mManualRedirect;
  bool mPreserveContentCodings;
  bool mSameOriginDataURL;
  bool mSandboxedStorageAreaURLs;
  bool mSkipServiceWorker;
  bool mSynchronous;
  bool mUnsafeRequest;
  bool mUseURLCredentials;
};

} 
} 

#endif 
