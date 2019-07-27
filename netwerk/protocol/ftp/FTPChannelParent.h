






#ifndef mozilla_net_FTPChannelParent_h
#define mozilla_net_FTPChannelParent_h

#include "ADivertableParentChannel.h"
#include "mozilla/net/PFTPChannelParent.h"
#include "mozilla/net/NeckoParent.h"
#include "nsIParentChannel.h"
#include "nsIInterfaceRequestor.h"
#include "OfflineObserver.h"

class nsILoadContext;

namespace mozilla {
namespace net {

class FTPChannelParent final : public PFTPChannelParent
                             , public nsIParentChannel
                             , public nsIInterfaceRequestor
                             , public ADivertableParentChannel
                             , public nsIChannelEventSink
                             , public DisconnectableParent
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIPARENTCHANNEL
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSICHANNELEVENTSINK

  FTPChannelParent(nsILoadContext* aLoadContext, PBOverrideStatus aOverrideStatus);

  bool Init(const FTPChannelCreationArgs& aOpenArgs);

  
  void DivertTo(nsIStreamListener *aListener) override;
  nsresult SuspendForDiversion() override;

  
  
  
  void StartDiversion();

  
  
  void NotifyDiversionFailed(nsresult aErrorCode, bool aSkipResume = true);

protected:
  virtual ~FTPChannelParent();

  
  nsresult ResumeForDiversion();

  
  void FailDiversion(nsresult aErrorCode, bool aSkipResume = true);

  bool DoAsyncOpen(const URIParams& aURI, const uint64_t& aStartPos,
                   const nsCString& aEntityID,
                   const OptionalInputStreamParams& aUploadStream,
                   const ipc::PrincipalInfo& aRequestingPrincipalInfo,
                   const ipc::PrincipalInfo& aTriggeringPrincipalInfo,
                   const uint32_t& aSecurityFlags,
                   const uint32_t& aContentPolicyType,
                   const uint32_t& aInnerWindowID);

  
  
  bool ConnectChannel(const uint32_t& channelId);

  virtual bool RecvCancel(const nsresult& status) override;
  virtual bool RecvSuspend() override;
  virtual bool RecvResume() override;
  virtual bool RecvDivertOnDataAvailable(const nsCString& data,
                                         const uint64_t& offset,
                                         const uint32_t& count) override;
  virtual bool RecvDivertOnStopRequest(const nsresult& statusCode) override;
  virtual bool RecvDivertComplete() override;

  virtual void ActorDestroy(ActorDestroyReason why) override;

  void OfflineDisconnect() override;
  uint32_t GetAppId() override;

  
  nsCOMPtr<nsIChannel> mChannel;

  bool mIPCClosed;

  nsCOMPtr<nsILoadContext> mLoadContext;

  PBOverrideStatus mPBOverride;

  
  
  nsCOMPtr<nsIStreamListener> mDivertToListener;
  
  nsresult mStatus;
  
  
  
  bool mDivertingFromChild;
  
  bool mDivertedOnStartRequest;

  
  
  bool mSuspendedForDiversion;
  nsRefPtr<OfflineObserver> mObserver;
};

} 
} 

#endif 
