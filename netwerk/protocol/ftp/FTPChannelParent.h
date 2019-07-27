






#ifndef mozilla_net_FTPChannelParent_h
#define mozilla_net_FTPChannelParent_h

#include "ADivertableParentChannel.h"
#include "mozilla/net/PFTPChannelParent.h"
#include "mozilla/net/NeckoParent.h"
#include "nsIParentChannel.h"
#include "nsIInterfaceRequestor.h"

class nsFtpChannel;
class nsILoadContext;

namespace mozilla {
namespace net {

class FTPChannelParent : public PFTPChannelParent
                       , public nsIParentChannel
                       , public nsIInterfaceRequestor
                       , public ADivertableParentChannel
                       , public nsIChannelEventSink
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

  
  void DivertTo(nsIStreamListener *aListener) MOZ_OVERRIDE;
  nsresult SuspendForDiversion() MOZ_OVERRIDE;

  
  
  
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
                   const uint32_t& aSecurityFlags,
                   const uint32_t& aContentPolicyType);

  
  
  bool ConnectChannel(const uint32_t& channelId);

  virtual bool RecvCancel(const nsresult& status) MOZ_OVERRIDE;
  virtual bool RecvSuspend() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;
  virtual bool RecvDivertOnDataAvailable(const nsCString& data,
                                         const uint64_t& offset,
                                         const uint32_t& count) MOZ_OVERRIDE;
  virtual bool RecvDivertOnStopRequest(const nsresult& statusCode) MOZ_OVERRIDE;
  virtual bool RecvDivertComplete() MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  
  nsCOMPtr<nsIChannel> mChannel;

  bool mIPCClosed;

  nsCOMPtr<nsILoadContext> mLoadContext;

  PBOverrideStatus mPBOverride;

  
  
  nsCOMPtr<nsIStreamListener> mDivertToListener;
  
  nsresult mStatus;
  
  
  
  bool mDivertingFromChild;
  
  bool mDivertedOnStartRequest;

  
  
  bool mSuspendedForDiversion;
};

} 
} 

#endif 
