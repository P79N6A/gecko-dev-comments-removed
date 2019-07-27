






#ifndef mozilla_net_HttpChannelParent_h
#define mozilla_net_HttpChannelParent_h

#include "ADivertableParentChannel.h"
#include "nsHttp.h"
#include "mozilla/net/PHttpChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "mozilla/net/NeckoParent.h"
#include "OfflineObserver.h"
#include "nsIObserver.h"
#include "nsIParentRedirectingChannel.h"
#include "nsIProgressEventSink.h"
#include "nsHttpChannel.h"
#include "nsIAuthPromptProvider.h"
#include "mozilla/dom/ipc/IdType.h"

class nsICacheEntry;
class nsIAssociatedContentSecurity;

namespace mozilla {

namespace dom{
class TabParent;
class PBrowserOrId;
}

namespace net {

class HttpChannelParentListener;

class HttpChannelParent : public PHttpChannelParent
                        , public nsIParentRedirectingChannel
                        , public nsIProgressEventSink
                        , public nsIInterfaceRequestor
                        , public ADivertableParentChannel
                        , public nsIAuthPromptProvider
                        , public DisconnectableParent
{
  virtual ~HttpChannelParent();

public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIPARENTCHANNEL
  NS_DECL_NSIPARENTREDIRECTINGCHANNEL
  NS_DECL_NSIPROGRESSEVENTSINK
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSIAUTHPROMPTPROVIDER

  HttpChannelParent(const dom::PBrowserOrId& iframeEmbedding,
                    nsILoadContext* aLoadContext,
                    PBOverrideStatus aStatus);

  bool Init(const HttpChannelCreationArgs& aOpenArgs);

  
  void DivertTo(nsIStreamListener *aListener) MOZ_OVERRIDE;
  nsresult SuspendForDiversion() MOZ_OVERRIDE;

  
  
  
  void StartDiversion();

  
  
  void NotifyDiversionFailed(nsresult aErrorCode, bool aSkipResume = true);

  
  void SetApplyConversion(bool aApplyConversion) {
    if (mChannel) {
      mChannel->SetApplyConversion(aApplyConversion);
    }
  }

protected:
  
  
  bool ConnectChannel(const uint32_t& channelId);

  bool DoAsyncOpen(const URIParams&           uri,
                   const OptionalURIParams&   originalUri,
                   const OptionalURIParams&   docUri,
                   const OptionalURIParams&   referrerUri,
                   const uint32_t&            referrerPolicy,
                   const OptionalURIParams&   internalRedirectUri,
                   const OptionalURIParams&   topWindowUri,
                   const uint32_t&            loadFlags,
                   const RequestHeaderTuples& requestHeaders,
                   const nsCString&           requestMethod,
                   const OptionalInputStreamParams& uploadStream,
                   const bool&                uploadStreamHasHeaders,
                   const uint16_t&            priority,
                   const uint32_t&            classOfService,
                   const uint8_t&             redirectionLimit,
                   const bool&                allowPipelining,
                   const bool&                allowSTS,
                   const uint32_t&            thirdPartyFlags,
                   const bool&                doResumeAt,
                   const uint64_t&            startPos,
                   const nsCString&           entityID,
                   const bool&                chooseApplicationCache,
                   const nsCString&           appCacheClientID,
                   const bool&                allowSpdy,
                   const OptionalFileDescriptorSet& aFds,
                   const ipc::PrincipalInfo&  aRequestingPrincipalInfo,
                   const ipc::PrincipalInfo&  aTriggeringPrincipalInfo,
                   const uint32_t&            aSecurityFlags,
                   const uint32_t&            aContentPolicyType,
                   const uint32_t&            aInnerWindowID);

  virtual bool RecvSetPriority(const uint16_t& priority) MOZ_OVERRIDE;
  virtual bool RecvSetClassOfService(const uint32_t& cos) MOZ_OVERRIDE;
  virtual bool RecvSetCacheTokenCachedCharset(const nsCString& charset) MOZ_OVERRIDE;
  virtual bool RecvSuspend() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;
  virtual bool RecvCancel(const nsresult& status) MOZ_OVERRIDE;
  virtual bool RecvRedirect2Verify(const nsresult& result,
                                   const RequestHeaderTuples& changedHeaders,
                                   const OptionalURIParams& apiRedirectUri) MOZ_OVERRIDE;
  virtual bool RecvUpdateAssociatedContentSecurity(const int32_t& broken,
                                                   const int32_t& no) MOZ_OVERRIDE;
  virtual bool RecvDocumentChannelCleanup() MOZ_OVERRIDE;
  virtual bool RecvMarkOfflineCacheEntryAsForeign() MOZ_OVERRIDE;
  virtual bool RecvDivertOnDataAvailable(const nsCString& data,
                                         const uint64_t& offset,
                                         const uint32_t& count) MOZ_OVERRIDE;
  virtual bool RecvDivertOnStopRequest(const nsresult& statusCode) MOZ_OVERRIDE;
  virtual bool RecvDivertComplete() MOZ_OVERRIDE;
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  
  nsresult ResumeForDiversion();

  
  void FailDiversion(nsresult aErrorCode, bool aSkipResume = true);

  friend class HttpChannelParentListener;
  nsRefPtr<mozilla::dom::TabParent> mTabParent;

  void OfflineDisconnect() MOZ_OVERRIDE;
  uint32_t GetAppId() MOZ_OVERRIDE;

private:
  nsRefPtr<nsHttpChannel>       mChannel;
  nsCOMPtr<nsICacheEntry>       mCacheEntry;
  nsCOMPtr<nsIAssociatedContentSecurity>  mAssociatedContentSecurity;
  bool mIPCClosed;                

  nsCOMPtr<nsIChannel> mRedirectChannel;
  nsCOMPtr<nsIAsyncVerifyRedirectCallback> mRedirectCallback;

  nsAutoPtr<class nsHttpChannel::OfflineCacheEntryAsForeignMarker> mOfflineForeignMarker;

  
  
  nsresult mStoredStatus;
  int64_t mStoredProgress;
  int64_t mStoredProgressMax;

  bool mSentRedirect1Begin          : 1;
  bool mSentRedirect1BeginFailed    : 1;
  bool mReceivedRedirect2Verify     : 1;

  nsRefPtr<OfflineObserver> mObserver;

  PBOverrideStatus mPBOverride;

  nsCOMPtr<nsILoadContext> mLoadContext;
  nsRefPtr<nsHttpHandler>  mHttpHandler;

  nsRefPtr<HttpChannelParentListener> mParentListener;
  
  nsCOMPtr<nsIStreamListener> mDivertListener;
  
  nsresult mStatus;
  
  
  
  bool mDivertingFromChild;

  
  bool mDivertedOnStartRequest;

  bool mSuspendedForDiversion;

  dom::TabId mNestedFrameId;
};

} 
} 

#endif 
