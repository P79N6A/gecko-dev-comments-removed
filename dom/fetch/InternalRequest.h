




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

class nsIDocument;
class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class FetchBodyStream;
class Request;

#define kFETCH_CLIENT_REFERRER_STR "about:client"

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

  explicit InternalRequest(const InternalRequest& aOther)
    : mMethod(aOther.mMethod)
    , mURL(aOther.mURL)
    , mHeaders(aOther.mHeaders)
    , mBodyStream(aOther.mBodyStream)
    , mContext(aOther.mContext)
    , mContextFrameType(aOther.mContextFrameType)
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

private:
  ~InternalRequest();

  nsCString mMethod;
  nsCString mURL;
  nsRefPtr<InternalHeaders> mHeaders;
  nsCOMPtr<nsIInputStream> mBodyStream;

  
  
  nsContentPolicyType mContext;

  ContextFrameType mContextFrameType;

  
  
  
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
};

} 
} 

#endif 
