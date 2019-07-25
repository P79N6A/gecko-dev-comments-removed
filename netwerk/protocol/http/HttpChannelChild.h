








































#ifndef mozilla_net_HttpChannelChild_h
#define mozilla_net_HttpChannelChild_h

#include "mozilla/net/HttpBaseChannel.h"
#include "mozilla/net/PHttpChannelChild.h"

#include "nsIStreamListener.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "nsICacheInfoChannel.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIEncodedChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIResumableChannel.h"
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
                       , public HttpBaseChannel
                       , public nsICacheInfoChannel
                       , public nsIEncodedChannel
                       , public nsIResumableChannel
                       , public nsIProxiedChannel
                       , public nsITraceableChannel
                       , public nsIApplicationCacheChannel
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICACHEINFOCHANNEL
  NS_DECL_NSIENCODEDCHANNEL
  NS_DECL_NSIRESUMABLECHANNEL
  NS_DECL_NSIPROXIEDCHANNEL
  NS_DECL_NSITRACEABLECHANNEL
  NS_DECL_NSIAPPLICATIONCACHECONTAINER
  NS_DECL_NSIAPPLICATIONCACHECHANNEL

  HttpChannelChild();
  virtual ~HttpChannelChild();

  
  
  
  NS_IMETHOD Cancel(nsresult status);
  NS_IMETHOD Suspend();
  NS_IMETHOD Resume();
  
  NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo);
  NS_IMETHOD AsyncOpen(nsIStreamListener *listener, nsISupports *aContext);
  
  NS_IMETHOD SetRequestHeader(const nsACString& aHeader, 
                              const nsACString& aValue, 
                              PRBool aMerge);
  
  NS_IMETHOD SetupFallbackChannel(const char *aFallbackKey);
  
  NS_IMETHOD SetPriority(PRInt32 value);

  
  
  
  void AddIPDLReference();
  void ReleaseIPDLReference();

protected:
  bool RecvOnStartRequest(const nsHttpResponseHead& responseHead,
                          const PRBool& useResponseHead,
                          const PRBool& isFromCache,
                          const PRBool& cacheEntryAvailable,
                          const PRUint32& cacheExpirationTime,
                          const nsCString& cachedCharset);
  bool RecvOnDataAvailable(const nsCString& data, 
                           const PRUint32& offset,
                           const PRUint32& count);
  bool RecvOnStopRequest(const nsresult& statusCode);
  bool RecvOnProgress(const PRUint64& progress, const PRUint64& progressMax);
  bool RecvOnStatus(const nsresult& status, const nsString& statusArg);

private:
  RequestHeaderTuples mRequestHeaders;

  PRPackedBool mIsFromCache;
  PRPackedBool mCacheEntryAvailable;
  PRUint32     mCacheExpirationTime;
  nsCString    mCachedCharset;

  
  enum HttpChannelChildState mState;
  bool mIPCOpen;
};

} 
} 

#endif 
