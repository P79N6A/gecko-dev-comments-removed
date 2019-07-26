






#ifndef mozilla_net_FTPChannelParent_h
#define mozilla_net_FTPChannelParent_h

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
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIPARENTCHANNEL
  NS_DECL_NSIINTERFACEREQUESTOR

  FTPChannelParent(nsILoadContext* aLoadContext, PBOverrideStatus aOverrideStatus);
  virtual ~FTPChannelParent();

  bool Init(const FTPChannelCreationArgs& aOpenArgs);

protected:
  bool DoAsyncOpen(const URIParams& aURI, const uint64_t& aStartPos,
                   const nsCString& aEntityID,
                   const OptionalInputStreamParams& aUploadStream);

  
  
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

  nsRefPtr<nsFtpChannel> mChannel;

  bool mIPCClosed;

  nsCOMPtr<nsILoadContext> mLoadContext;

  PBOverrideStatus mPBOverride;
};

} 
} 

#endif 
