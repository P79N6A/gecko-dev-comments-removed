






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
#include "nsIDivertableChannel.h"
#include "mozilla/net/DNS.h"

namespace mozilla {
namespace net {

class HttpChannelChild MOZ_FINAL : public PHttpChannelChild
                                 , public HttpBaseChannel
                                 , public HttpAsyncAborter<HttpChannelChild>
                                 , public nsICacheInfoChannel
                                 , public nsIProxiedChannel
                                 , public nsIApplicationCacheChannel
                                 , public nsIAsyncVerifyRedirectCallback
                                 , public nsIAssociatedContentSecurity
                                 , public nsIChildChannel
                                 , public nsIHttpChannelChild
                                 , public nsIDivertableChannel
{
  virtual ~HttpChannelChild();
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
  NS_DECL_NSIDIVERTABLECHANNEL

  HttpChannelChild();

  
  
  
  NS_IMETHOD Cancel(nsresult status);
  NS_IMETHOD Suspend();
  NS_IMETHOD Resume();
  
  NS_IMETHOD GetSecurityInfo(nsISupports **aSecurityInfo);
  NS_IMETHOD AsyncOpen(nsIStreamListener *listener, nsISupports *aContext);
  
  NS_IMETHOD SetRequestHeader(const nsACString& aHeader,
                              const nsACString& aValue,
                              bool aMerge);
  NS_IMETHOD RedirectTo(nsIURI *newURI);
  
  NS_IMETHOD SetupFallbackChannel(const char *aFallbackKey);
  NS_IMETHOD GetLocalAddress(nsACString& addr);
  NS_IMETHOD GetLocalPort(int32_t* port);
  NS_IMETHOD GetRemoteAddress(nsACString& addr);
  NS_IMETHOD GetRemotePort(int32_t* port);
  
  NS_IMETHOD SetPriority(int32_t value);
  
  NS_IMETHOD ResumeAt(uint64_t startPos, const nsACString& entityID);

  
  
  
  void AddIPDLReference();
  void ReleaseIPDLReference();

  bool IsSuspended();

  void FlushedForDiversion();

protected:
  bool RecvOnStartRequest(const nsresult& channelStatus,
                          const nsHttpResponseHead& responseHead,
                          const bool& useResponseHead,
                          const nsHttpHeaderArray& requestHeaders,
                          const bool& isFromCache,
                          const bool& cacheEntryAvailable,
                          const uint32_t& cacheExpirationTime,
                          const nsCString& cachedCharset,
                          const nsCString& securityInfoSerialization,
                          const NetAddr& selfAddr,
                          const NetAddr& peerAddr,
                          const int16_t& redirectCount) MOZ_OVERRIDE;
  bool RecvOnTransportAndData(const nsresult& channelStatus,
                              const nsresult& status,
                              const uint64_t& progress,
                              const uint64_t& progressMax,
                              const nsCString& data,
                              const uint64_t& offset,
                              const uint32_t& count) MOZ_OVERRIDE;
  bool RecvOnStopRequest(const nsresult& statusCode);
  bool RecvOnProgress(const uint64_t& progress, const uint64_t& progressMax) MOZ_OVERRIDE;
  bool RecvOnStatus(const nsresult& status) MOZ_OVERRIDE;
  bool RecvFailedAsyncOpen(const nsresult& status) MOZ_OVERRIDE;
  bool RecvRedirect1Begin(const uint32_t& newChannel,
                          const URIParams& newURI,
                          const uint32_t& redirectFlags,
                          const nsHttpResponseHead& responseHead) MOZ_OVERRIDE;
  bool RecvRedirect3Complete() MOZ_OVERRIDE;
  bool RecvAssociateApplicationCache(const nsCString& groupID,
                                     const nsCString& clientID) MOZ_OVERRIDE;
  bool RecvFlushedForDiversion() MOZ_OVERRIDE;
  bool RecvDivertMessages() MOZ_OVERRIDE;
  bool RecvDeleteSelf() MOZ_OVERRIDE;

  bool GetAssociatedContentSecurity(nsIAssociatedContentSecurity** res = nullptr);
  virtual void DoNotifyListenerCleanup();

private:
  RequestHeaderTuples mClientSetRequestHeaders;
  nsCOMPtr<nsIChildChannel> mRedirectChannelChild;
  nsCOMPtr<nsISupports> mSecurityInfo;

  bool mIsFromCache;
  bool mCacheEntryAvailable;
  uint32_t     mCacheExpirationTime;
  nsCString    mCachedCharset;

  
  bool mSendResumeAt;

  bool mIPCOpen;
  bool mKeptAlive;            
  nsRefPtr<ChannelEventQueue> mEventQ;

  
  bool mDivertingToParent;
  
  
  bool mFlushedForDiversion;
  
  
  bool mSuspendSent;

  
  bool RemoteChannelExists() { return mIPCOpen && !mKeptAlive; }

  void AssociateApplicationCache(const nsCString &groupID,
                                 const nsCString &clientID);
  void OnStartRequest(const nsresult& channelStatus,
                      const nsHttpResponseHead& responseHead,
                      const bool& useResponseHead,
                      const nsHttpHeaderArray& requestHeaders,
                      const bool& isFromCache,
                      const bool& cacheEntryAvailable,
                      const uint32_t& cacheExpirationTime,
                      const nsCString& cachedCharset,
                      const nsCString& securityInfoSerialization,
                      const NetAddr& selfAddr,
                      const NetAddr& peerAddr);
  void OnTransportAndData(const nsresult& channelStatus,
                          const nsresult& status,
                          const uint64_t progress,
                          const uint64_t& progressMax,
                          const nsCString& data,
                          const uint64_t& offset,
                          const uint32_t& count);
  void OnStopRequest(const nsresult& channelStatus);
  void OnProgress(const uint64_t& progress, const uint64_t& progressMax);
  void OnStatus(const nsresult& status);
  void FailedAsyncOpen(const nsresult& status);
  void HandleAsyncAbort();
  void Redirect1Begin(const uint32_t& newChannelId,
                      const URIParams& newUri,
                      const uint32_t& redirectFlags,
                      const nsHttpResponseHead& responseHead);
  void Redirect3Complete();
  void DeleteSelf();

  friend class AssociateApplicationCacheEvent;
  friend class StartRequestEvent;
  friend class StopRequestEvent;
  friend class TransportAndDataEvent;
  friend class ProgressEvent;
  friend class StatusEvent;
  friend class FailedAsyncOpenEvent;
  friend class Redirect1Event;
  friend class Redirect3Event;
  friend class DeleteSelfEvent;
  friend class HttpAsyncAborter<HttpChannelChild>;
};





inline bool
HttpChannelChild::IsSuspended()
{
  return mSuspendCount != 0;
}

} 
} 

#endif 
