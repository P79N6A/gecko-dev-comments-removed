







































#ifndef mozilla_net_HttpChannelChild_h
#define mozilla_net_HttpChannelChild_h

#include "mozilla/net/PHttpChannelChild.h"
#include "mozilla/net/NeckoCommon.h"

#include "nsHttpRequestHead.h"
#include "nsHashPropertyBag.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsIStreamListener.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "nsICachingChannel.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIEncodedChannel.h"
#include "nsIUploadChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIResumableChannel.h"
#include "nsISupportsPriority.h"
#include "nsIProxiedChannel.h"
#include "nsITraceableChannel.h"


namespace mozilla {
namespace net {


enum HttpChannelChildState {
  HCC_NEW,
  HCC_OPENED,
  HCC_ONSTART,
  HCC_ONDATA,
  HCC_ONSTOP
};


class HttpChannelChild : public PHttpChannelChild
                       , public nsIHttpChannel
                       , public nsHashPropertyBag
                       , public nsIHttpChannelInternal
                       , public nsICachingChannel
                       , public nsIUploadChannel
                       , public nsIUploadChannel2
                       , public nsIEncodedChannel
                       , public nsIResumableChannel
                       , public nsISupportsPriority
                       , public nsIProxiedChannel
                       , public nsITraceableChannel
                       , public nsIApplicationCacheChannel
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL
  NS_DECL_NSIHTTPCHANNEL
  NS_DECL_NSIHTTPCHANNELINTERNAL
  NS_DECL_NSICACHINGCHANNEL
  NS_DECL_NSIUPLOADCHANNEL
  NS_DECL_NSIUPLOADCHANNEL2
  NS_DECL_NSIENCODEDCHANNEL
  NS_DECL_NSIRESUMABLECHANNEL
  NS_DECL_NSISUPPORTSPRIORITY
  NS_DECL_NSIPROXIEDCHANNEL
  NS_DECL_NSITRACEABLECHANNEL
  NS_DECL_NSIAPPLICATIONCACHECONTAINER
  NS_DECL_NSIAPPLICATIONCACHECHANNEL

  HttpChannelChild();
  virtual ~HttpChannelChild();

  nsresult Init(nsIURI *uri);

protected:
  bool RecvOnStartRequest(const PRInt32& HACK_ContentLength,
                          const nsCString& HACK_ContentType,
                          const PRUint32& HACK_Status,
                          const nsCString& HACK_StatusText);
  bool RecvOnDataAvailable(const nsCString& data, 
                           const PRUint32& offset,
                           const PRUint32& count);
  bool RecvOnStopRequest(const nsresult& statusCode);

private:
  nsCOMPtr<nsIStreamListener>         mChildListener;
  nsCOMPtr<nsISupports>               mChildListenerContext;

  
  PRInt32                             mContentLength_HACK;
  nsCString                           mContentType_HACK;
  PRUint32                            mResponseStatus_HACK;
  nsCString                           mResponseStatusText_HACK;

  
  enum HttpChannelChildState mState;

  


  nsCOMPtr<nsIURI>                  mOriginalURI;
  nsCOMPtr<nsIURI>                  mURI;
  nsCOMPtr<nsIURI>                  mDocumentURI;

  nsCOMPtr<nsIInterfaceRequestor>   mCallbacks;
  nsCOMPtr<nsIProgressEventSink>    mProgressSink;

  nsHttpRequestHead                 mRequestHead;

  nsCString                         mSpec; 

  PRUint32                          mLoadFlags;
  PRUint32                          mStatus;

  
  PRUint32                          mIsPending                : 1;
  PRUint32                          mWasOpened                : 1;

};


} 
} 

#endif 
