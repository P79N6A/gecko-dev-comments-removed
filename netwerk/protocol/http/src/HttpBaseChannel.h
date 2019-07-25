







































#ifndef mozilla_net_HttpBaseChannel_h
#define mozilla_net_HttpBaseChannel_h

#include "nsHttp.h"
#include "nsAutoPtr.h"
#include "nsHashPropertyBag.h"
#include "nsProxyInfo.h"
#include "nsHttpRequestHead.h"
#include "nsHttpResponseHead.h"
#include "nsHttpConnectionInfo.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIProgressEventSink.h"
#include "nsIURI.h"
#include "nsISupportsPriority.h"

#define DIE_WITH_ASYNC_OPEN_MSG()                                              \
  do {                                                                         \
    fprintf(stderr,                                                            \
            "*&*&*&*&*&*&*&**&*&&*& FATAL ERROR: '%s' "                        \
            "called after AsyncOpen: %s +%d",                                  \
            __FUNCTION__, __FILE__, __LINE__);                                 \
    NS_ABORT();                                                                \
    return NS_ERROR_NOT_IMPLEMENTED;                                           \
  } while (0)

#define ENSURE_CALLED_BEFORE_ASYNC_OPEN()                                      \
  if (mIsPending)                                                              \
    DIE_WITH_ASYNC_OPEN_MSG();                                                 \
  if (mWasOpened)                                                              \
    DIE_WITH_ASYNC_OPEN_MSG();                                                 \
  NS_ENSURE_TRUE(!mIsPending, NS_ERROR_IN_PROGRESS);                           \
  NS_ENSURE_TRUE(!mWasOpened, NS_ERROR_ALREADY_OPENED);

namespace mozilla {
namespace net {








class HttpBaseChannel : public nsHashPropertyBag
                      , public nsIHttpChannel
                      , public nsIHttpChannelInternal
                      , public nsISupportsPriority
{
public:
  NS_DECL_ISUPPORTS_INHERITED

  HttpBaseChannel();
  virtual ~HttpBaseChannel();

  nsresult Init(nsIURI *aURI, PRUint8 aCaps, nsProxyInfo *aProxyInfo);

  
  NS_IMETHOD GetName(nsACString& aName);
  NS_IMETHOD IsPending(PRBool *aIsPending);
  NS_IMETHOD GetStatus(nsresult *aStatus);
  NS_IMETHOD GetLoadGroup(nsILoadGroup **aLoadGroup);
  NS_IMETHOD SetLoadGroup(nsILoadGroup *aLoadGroup);
  NS_IMETHOD GetLoadFlags(nsLoadFlags *aLoadFlags);
  NS_IMETHOD SetLoadFlags(nsLoadFlags aLoadFlags);

  
  NS_IMETHOD GetOriginalURI(nsIURI **aOriginalURI);
  NS_IMETHOD SetOriginalURI(nsIURI *aOriginalURI);
  NS_IMETHOD GetURI(nsIURI **aURI);
  NS_IMETHOD GetNotificationCallbacks(nsIInterfaceRequestor **aCallbacks);
  NS_IMETHOD SetNotificationCallbacks(nsIInterfaceRequestor *aCallbacks);
  NS_IMETHOD GetContentType(nsACString& aContentType);
  NS_IMETHOD SetContentType(const nsACString& aContentType);
  NS_IMETHOD GetContentCharset(nsACString& aContentCharset);
  NS_IMETHOD SetContentCharset(const nsACString& aContentCharset);
  NS_IMETHOD GetContentLength(PRInt32 *aContentLength);
  NS_IMETHOD SetContentLength(PRInt32 aContentLength);
  NS_IMETHOD Open(nsIInputStream **aResult);

  
  NS_IMETHOD GetRequestMethod(nsACString& aMethod);
  NS_IMETHOD SetRequestMethod(const nsACString& aMethod);
  NS_IMETHOD GetReferrer(nsIURI **referrer);
  NS_IMETHOD SetReferrer(nsIURI *referrer);
  NS_IMETHOD GetRequestHeader(const nsACString& aHeader, nsACString& aValue);
  NS_IMETHOD SetRequestHeader(const nsACString& aHeader, 
                              const nsACString& aValue, PRBool aMerge);
  NS_IMETHOD VisitRequestHeaders(nsIHttpHeaderVisitor *visitor);
  NS_IMETHOD GetResponseHeader(const nsACString &header, nsACString &value);
  NS_IMETHOD SetResponseHeader(const nsACString& header, 
                               const nsACString& value, PRBool merge);
  NS_IMETHOD VisitResponseHeaders(nsIHttpHeaderVisitor *visitor);
  NS_IMETHOD GetAllowPipelining(PRBool *value);
  NS_IMETHOD SetAllowPipelining(PRBool value);
  NS_IMETHOD GetRedirectionLimit(PRUint32 *value);
  NS_IMETHOD SetRedirectionLimit(PRUint32 value);
  NS_IMETHOD IsNoStoreResponse(PRBool *value);
  NS_IMETHOD IsNoCacheResponse(PRBool *value);
  NS_IMETHOD GetResponseStatus(PRUint32 *aValue);
  NS_IMETHOD GetResponseStatusText(nsACString& aValue);
  NS_IMETHOD GetRequestSucceeded(PRBool *aValue);

  
  NS_IMETHOD GetDocumentURI(nsIURI **aDocumentURI);
  NS_IMETHOD SetDocumentURI(nsIURI *aDocumentURI);
  NS_IMETHOD GetRequestVersion(PRUint32 *major, PRUint32 *minor);
  NS_IMETHOD GetResponseVersion(PRUint32 *major, PRUint32 *minor);
  NS_IMETHOD SetCookie(const char *aCookieHeader);
  NS_IMETHOD GetForceAllowThirdPartyCookie(PRBool *aForce);
  NS_IMETHOD SetForceAllowThirdPartyCookie(PRBool aForce);

  
  NS_IMETHOD GetPriority(PRInt32 *value);
  NS_IMETHOD AdjustPriority(PRInt32 delta);

protected:
  nsCOMPtr<nsIURI>                  mURI;
  nsCOMPtr<nsIURI>                  mOriginalURI;
  nsCOMPtr<nsIURI>                  mDocumentURI;
  nsCOMPtr<nsIStreamListener>       mListener;
  nsCOMPtr<nsISupports>             mListenerContext;
  nsCOMPtr<nsILoadGroup>            mLoadGroup;
  nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
  nsCOMPtr<nsIProgressEventSink>    mProgressSink;
  nsCOMPtr<nsIURI>                  mReferrer;

  nsHttpRequestHead                 mRequestHead;
  nsAutoPtr<nsHttpResponseHead>     mResponseHead;
  nsRefPtr<nsHttpConnectionInfo>    mConnectionInfo;

  nsCString                         mSpec; 
  nsCString                         mContentTypeHint;
  nsCString                         mContentCharsetHint;

  nsresult                          mStatus;
  PRUint32                          mLoadFlags;
  PRInt16                           mPriority;
  PRUint8                           mCaps;
  PRUint8                           mRedirectionLimit;

  PRUint8                           mIsPending                  : 1;
  PRUint8                           mWasOpened                  : 1;
  PRUint8                           mResponseHeadersModified    : 1;
  PRUint8                           mAllowPipelining            : 1;
  PRUint8                           mForceAllowThirdPartyCookie : 1;
};


} 
} 

#endif 
