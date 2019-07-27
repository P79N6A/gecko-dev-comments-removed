




#ifndef mozilla_dom_InternalRequest_h
#define mozilla_dom_InternalRequest_h

#include "mozilla/dom/HeadersBinding.h"
#include "mozilla/dom/InternalHeaders.h"
#include "mozilla/dom/RequestBinding.h"

#include "nsIContentPolicy.h"
#include "nsIInputStream.h"
#include "nsISupportsImpl.h"
#ifdef DEBUG
#include "nsIURLParser.h"
#include "nsNetCID.h"
#include "nsServiceManagerUtils.h"
#endif

namespace mozilla {
namespace dom {




























































class Request;

#define kFETCH_CLIENT_REFERRER_STR "about:client"

class InternalRequest final
{
  friend class Request;

public:
  NS_INLINE_DECL_THREADSAFE_REFCOUNTING(InternalRequest)

  enum ResponseTainting
  {
    RESPONSETAINT_BASIC,
    RESPONSETAINT_CORS,
    RESPONSETAINT_OPAQUE,
  };

  explicit InternalRequest()
    : mMethod("GET")
    , mHeaders(new InternalHeaders(HeadersGuardEnum::None))
    , mReferrer(NS_LITERAL_STRING(kFETCH_CLIENT_REFERRER_STR))
    , mMode(RequestMode::No_cors)
    , mCredentialsMode(RequestCredentials::Omit)
    , mResponseTainting(RESPONSETAINT_BASIC)
    , mCacheMode(RequestCache::Default)
    , mAuthenticationFlag(false)
    , mForceOriginHeader(false)
    , mPreserveContentCodings(false)
      
      
      
      
    , mSameOriginDataURL(true)
    , mSkipServiceWorker(false)
    , mSynchronous(false)
    , mUnsafeRequest(false)
    , mUseURLCredentials(false)
  {
  }

  already_AddRefed<InternalRequest> Clone();

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

  void
  SetURL(const nsACString& aURL)
  {
    mURL.Assign(aURL);
  }

  void
  GetReferrer(nsAString& aReferrer) const
  {
    aReferrer.Assign(mReferrer);
  }

  void
  SetReferrer(const nsAString& aReferrer)
  {
#ifdef DEBUG
    bool validReferrer = false;
    if (aReferrer.IsEmpty() ||
        aReferrer.EqualsLiteral(kFETCH_CLIENT_REFERRER_STR)) {
      validReferrer = true;
    } else {
      nsCOMPtr<nsIURLParser> parser = do_GetService(NS_STDURLPARSER_CONTRACTID);
      if (!parser) {
        NS_WARNING("Could not get parser to validate URL!");
      } else {
        uint32_t schemePos;
        int32_t schemeLen;
        uint32_t authorityPos;
        int32_t authorityLen;
        uint32_t pathPos;
        int32_t pathLen;

        NS_ConvertUTF16toUTF8 ref(aReferrer);
        nsresult rv = parser->ParseURL(ref.get(), ref.Length(),
                                       &schemePos, &schemeLen,
                                       &authorityPos, &authorityLen,
                                       &pathPos, &pathLen);
        if (NS_FAILED(rv)) {
          NS_WARNING("Invalid referrer URL!");
        } else if (schemeLen < 0 || authorityLen < 0) {
          NS_WARNING("Invalid referrer URL!");
        } else {
          validReferrer = true;
        }
      }
    }

    MOZ_ASSERT(validReferrer);
#endif

    mReferrer.Assign(aReferrer);
  }

  bool
  SkipServiceWorker() const
  {
    return mSkipServiceWorker;
  }

  void
  SetSkipServiceWorker()
  {
    mSkipServiceWorker = true;
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

  RequestCredentials
  GetCredentialsMode() const
  {
    return mCredentialsMode;
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

  void
  SetCacheMode(RequestCache aCacheMode)
  {
    mCacheMode = aCacheMode;
  }

  nsContentPolicyType
  ContentPolicyType() const
  {
    return mContentPolicyType;
  }

  void
  SetContentPolicyType(nsContentPolicyType aContentPolicyType);

  RequestContext
  Context() const
  {
    return mContext;
  }

  void
  SetContext(RequestContext aContext)
  {
    mContext = aContext;
  }

  bool
  UnsafeRequest() const
  {
    return mUnsafeRequest;
  }

  void
  SetUnsafeRequest()
  {
    mUnsafeRequest = true;
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
  UnsetSameOriginDataURL()
  {
    mSameOriginDataURL = false;
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

  bool
  WasCreatedByFetchEvent() const
  {
    return mCreatedByFetchEvent;
  }

  void
  SetCreatedByFetchEvent()
  {
    mCreatedByFetchEvent = true;
  }

  void
  ClearCreatedByFetchEvent()
  {
    mCreatedByFetchEvent = false;
  }

private:
  
  explicit InternalRequest(const InternalRequest& aOther);

  ~InternalRequest();

  nsCString mMethod;
  nsCString mURL;
  nsRefPtr<InternalHeaders> mHeaders;
  nsCOMPtr<nsIInputStream> mBodyStream;

  nsContentPolicyType mContentPolicyType;
  RequestContext mContext;

  
  
  
  nsString mReferrer;

  RequestMode mMode;
  RequestCredentials mCredentialsMode;
  ResponseTainting mResponseTainting;
  RequestCache mCacheMode;

  bool mAuthenticationFlag;
  bool mForceOriginHeader;
  bool mPreserveContentCodings;
  bool mSameOriginDataURL;
  bool mSandboxedStorageAreaURLs;
  bool mSkipServiceWorker;
  bool mSynchronous;
  bool mUnsafeRequest;
  bool mUseURLCredentials;
  
  
  
  bool mCreatedByFetchEvent = false;
};

} 
} 

#endif 
