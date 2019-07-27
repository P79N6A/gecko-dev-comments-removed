






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
#include "nsINetworkInterceptController.h"

class nsICacheEntry;
class nsIAssociatedContentSecurity;

namespace mozilla {

namespace dom{
class TabParent;
class PBrowserOrId;
}

namespace net {

class HttpChannelParentListener;

class HttpChannelParent final : public PHttpChannelParent
                              , public nsIParentRedirectingChannel
                              , public nsIProgressEventSink
                              , public nsIInterfaceRequestor
                              , public ADivertableParentChannel
                              , public nsIAuthPromptProvider
                              , public nsINetworkInterceptController
                              , public DisconnectableParent
                              , public HttpChannelSecurityWarningReporter
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
  NS_DECL_NSINETWORKINTERCEPTCONTROLLER

  HttpChannelParent(const dom::PBrowserOrId& iframeEmbedding,
                    nsILoadContext* aLoadContext,
                    PBOverrideStatus aStatus);

  bool Init(const HttpChannelCreationArgs& aOpenArgs);

  
  void DivertTo(nsIStreamListener *aListener) override;
  nsresult SuspendForDiversion() override;

  
  
  
  void StartDiversion();

  
  
  void NotifyDiversionFailed(nsresult aErrorCode, bool aSkipResume = true);

  
  void SetApplyConversion(bool aApplyConversion) {
    if (mChannel) {
      mChannel->SetApplyConversion(aApplyConversion);
    }
  }

protected:
  
  
  bool ConnectChannel(const uint32_t& channelId, const bool& shouldIntercept);

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
                   const bool&                allowAltSvc,
                   const OptionalFileDescriptorSet& aFds,
                   const ipc::PrincipalInfo&  aRequestingPrincipalInfo,
                   const ipc::PrincipalInfo&  aTriggeringPrincipalInfo,
                   const uint32_t&            aSecurityFlags,
                   const uint32_t&            aContentPolicyType,
                   const uint32_t&            aInnerWindowID,
                   const OptionalHttpResponseHead& aSynthesizedResponseHead,
                   const uint32_t&            aCacheKey);

  virtual bool RecvSetPriority(const uint16_t& priority) override;
  virtual bool RecvSetClassOfService(const uint32_t& cos) override;
  virtual bool RecvSetCacheTokenCachedCharset(const nsCString& charset) override;
  virtual bool RecvSuspend() override;
  virtual bool RecvResume() override;
  virtual bool RecvCancel(const nsresult& status) override;
  virtual bool RecvRedirect2Verify(const nsresult& result,
                                   const RequestHeaderTuples& changedHeaders,
                                   const OptionalURIParams& apiRedirectUri) override;
  virtual bool RecvUpdateAssociatedContentSecurity(const int32_t& broken,
                                                   const int32_t& no) override;
  virtual bool RecvDocumentChannelCleanup() override;
  virtual bool RecvMarkOfflineCacheEntryAsForeign() override;
  virtual bool RecvDivertOnDataAvailable(const nsCString& data,
                                         const uint64_t& offset,
                                         const uint32_t& count) override;
  virtual bool RecvDivertOnStopRequest(const nsresult& statusCode) override;
  virtual bool RecvDivertComplete() override;
  virtual void ActorDestroy(ActorDestroyReason why) override;

  
  nsresult ResumeForDiversion();

  
  void FailDiversion(nsresult aErrorCode, bool aSkipResume = true);

  friend class HttpChannelParentListener;
  nsRefPtr<mozilla::dom::TabParent> mTabParent;

  void OfflineDisconnect() override;
  uint32_t GetAppId() override;

  nsresult ReportSecurityMessage(const nsAString& aMessageTag,
                                 const nsAString& aMessageCategory) override;

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

  nsAutoPtr<nsHttpResponseHead> mSynthesizedResponseHead;

  nsRefPtr<HttpChannelParentListener> mParentListener;
  
  nsCOMPtr<nsIStreamListener> mDivertListener;
  
  nsresult mStatus;
  
  
  
  bool mDivertingFromChild;

  
  bool mDivertedOnStartRequest;

  bool mSuspendedForDiversion;

  
  bool mShouldIntercept : 1;
  
  bool mShouldSuspendIntercept : 1;

  dom::TabId mNestedFrameId;

  
  nsCOMPtr<nsIInterceptedChannel> mInterceptedChannel;
};

} 
} 

#endif 
