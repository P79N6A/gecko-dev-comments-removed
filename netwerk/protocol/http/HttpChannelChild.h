









































#ifndef mozilla_net_HttpChannelChild_h
#define mozilla_net_HttpChannelChild_h

#include "mozilla/net/HttpBaseChannel.h"
#include "mozilla/net/PHttpChannelChild.h"
#include "mozilla/net/ChannelEventQueue.h"

#include "nsIStreamListener.h"
#include "nsILoadGroup.h"
#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsIProgressEventSink.h"
#include "nsICacheInfoChannel.h"
#include "nsIApplicationCache.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIUploadChannel2.h"
#include "nsIResumableChannel.h"
#include "nsIProxiedChannel.h"
#include "nsIAsyncVerifyRedirectCallback.h"
#include "nsIAssociatedContentSecurity.h"
#include "nsIChildChannel.h"
#include "nsIHttpChannelChild.h"

namespace mozilla {
namespace net {

class HttpChannelChild : public PHttpChannelChild
                       , public HttpBaseChannel
                       , public nsICacheInfoChannel
                       , public nsIProxiedChannel
                       , public nsIApplicationCacheChannel
                       , public nsIAsyncVerifyRedirectCallback
                       , public nsIAssociatedContentSecurity
                       , public nsIChildChannel
                       , public nsIHttpChannelChild
                       , public ChannelEventQueue<HttpChannelChild>
{
public:
  NS_DECL_ISUPPORTS_INHERITED
  NS_DECL_NSICACHEINFOCHANNEL
  NS_DECL_NSIPROXIEDCHANNEL
  NS_DECL_NSIAPPLICATIONCACHECONTAINER
  NS_DECL_NSIAPPLICATIONCACHECHANNEL
  NS_DECL_NSIASYNCVERIFYREDIRECTCALLBACK
  NS_DECL_NSIASSOCIATEDCONTENTSECURITY
  NS_DECL_NSICHILDCHANNEL
  NS_DECL_NSIHTTPCHANNELCHILD

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
  NS_IMETHOD GetLocalAddress(nsACString& addr);
  NS_IMETHOD GetLocalPort(PRInt32* port);
  NS_IMETHOD GetRemoteAddress(nsACString& addr);
  NS_IMETHOD GetRemotePort(PRInt32* port);
  
  NS_IMETHOD SetPriority(PRInt32 value);
  
  NS_IMETHOD ResumeAt(PRUint64 startPos, const nsACString& entityID);

  
  
  
  void AddIPDLReference();
  void ReleaseIPDLReference();

  bool IsSuspended();

protected:
  bool RecvOnStartRequest(const nsHttpResponseHead& responseHead,
                          const PRBool& useResponseHead,
                          const RequestHeaderTuples& requestHeaders,
                          const PRBool& isFromCache,
                          const PRBool& cacheEntryAvailable,
                          const PRUint32& cacheExpirationTime,
                          const nsCString& cachedCharset,
                          const nsCString& securityInfoSerialization,
                          const PRNetAddr& selfAddr,
                          const PRNetAddr& peerAddr);
  bool RecvOnTransportAndData(const nsresult& status,
                              const PRUint64& progress,
                              const PRUint64& progressMax,
                              const nsCString& data,
                              const PRUint32& offset,
                              const PRUint32& count);
  bool RecvOnStopRequest(const nsresult& statusCode);
  bool RecvOnProgress(const PRUint64& progress, const PRUint64& progressMax);
  bool RecvOnStatus(const nsresult& status);
  bool RecvCancelEarly(const nsresult& status);
  bool RecvRedirect1Begin(const PRUint32& newChannel,
                          const URI& newURI,
                          const PRUint32& redirectFlags,
                          const nsHttpResponseHead& responseHead);
  bool RecvRedirect3Complete();
  bool RecvAssociateApplicationCache(const nsCString& groupID,
                                     const nsCString& clientID);
  bool RecvDeleteSelf();

  bool GetAssociatedContentSecurity(nsIAssociatedContentSecurity** res = nsnull);

private:
  RequestHeaderTuples mRequestHeaders;
  nsCOMPtr<nsIChildChannel> mRedirectChannelChild;
  nsCOMPtr<nsIURI> mRedirectOriginalURI;
  nsCOMPtr<nsISupports> mSecurityInfo;

  PRPackedBool mIsFromCache;
  PRPackedBool mCacheEntryAvailable;
  PRUint32     mCacheExpirationTime;
  nsCString    mCachedCharset;

  
  bool mSendResumeAt;
  
  PRUint32 mSuspendCount;

  bool mIPCOpen;
  bool mKeptAlive;

  void OnStartRequest(const nsHttpResponseHead& responseHead,
                      const PRBool& useResponseHead,
                      const RequestHeaderTuples& requestHeaders,
                      const PRBool& isFromCache,
                      const PRBool& cacheEntryAvailable,
                      const PRUint32& cacheExpirationTime,
                      const nsCString& cachedCharset,
                      const nsCString& securityInfoSerialization,
                      const PRNetAddr& selfAddr,
                      const PRNetAddr& peerAddr);
  void OnTransportAndData(const nsresult& status,
                          const PRUint64 progress,
                          const PRUint64& progressMax,
                          const nsCString& data,
                          const PRUint32& offset,
                          const PRUint32& count);
  void OnStopRequest(const nsresult& statusCode);
  void OnProgress(const PRUint64& progress, const PRUint64& progressMax);
  void OnStatus(const nsresult& status);
  void OnCancel(const nsresult& status);
  void Redirect1Begin(const PRUint32& newChannelId,
                      const URI& newUri,
                      const PRUint32& redirectFlags,
                      const nsHttpResponseHead& responseHead);
  void Redirect3Complete();
  void DeleteSelf();

  friend class StartRequestEvent;
  friend class StopRequestEvent;
  friend class TransportAndDataEvent;
  friend class ProgressEvent;
  friend class StatusEvent;
  friend class CancelEvent;
  friend class Redirect1Event;
  friend class Redirect3Event;
  friend class DeleteSelfEvent;
};





inline bool
HttpChannelChild::IsSuspended()
{
  return mSuspendCount != 0;
}

} 
} 

#endif 
