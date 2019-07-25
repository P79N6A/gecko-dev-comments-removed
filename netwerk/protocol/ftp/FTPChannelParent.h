






#ifndef mozilla_net_FTPChannelParent_h
#define mozilla_net_FTPChannelParent_h

#include "mozilla/net/PFTPChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIParentChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsILoadContext.h"

class nsFtpChannel;

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

  FTPChannelParent();
  virtual ~FTPChannelParent();

protected:
  virtual bool RecvAsyncOpen(const URIParams& uri,
                             const uint64_t& startPos,
                             const nsCString& entityID,
                             const OptionalInputStreamParams& uploadStream,
                             const IPC::SerializedLoadContext& loadContext) MOZ_OVERRIDE;
  virtual bool RecvConnectChannel(const uint32_t& channelId) MOZ_OVERRIDE;
  virtual bool RecvCancel(const nsresult& status) MOZ_OVERRIDE;
  virtual bool RecvSuspend() MOZ_OVERRIDE;
  virtual bool RecvResume() MOZ_OVERRIDE;

  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;

  nsRefPtr<nsFtpChannel> mChannel;

  bool mIPCClosed;

  nsCOMPtr<nsILoadContext> mLoadContext;
};

} 
} 

#endif 
