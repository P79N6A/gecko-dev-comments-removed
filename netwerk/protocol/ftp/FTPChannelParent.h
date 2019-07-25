








































#ifndef mozilla_net_FTPChannelParent_h
#define mozilla_net_FTPChannelParent_h

#include "mozilla/net/PFTPChannelParent.h"
#include "mozilla/net/NeckoCommon.h"
#include "nsIParentChannel.h"
#include "nsIInterfaceRequestor.h"

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
  NS_OVERRIDE virtual bool RecvAsyncOpen(const IPC::URI& uri,
                                         const PRUint64& startPos,
                                         const nsCString& entityID,
                                         const IPC::InputStream& uploadStream);
  NS_OVERRIDE virtual bool RecvConnectChannel(const PRUint32& channelId);
  NS_OVERRIDE virtual bool RecvCancel(const nsresult& status);
  NS_OVERRIDE virtual bool RecvSuspend();
  NS_OVERRIDE virtual bool RecvResume();

  NS_OVERRIDE virtual void ActorDestroy(ActorDestroyReason why);

  nsRefPtr<nsFtpChannel> mChannel;

  bool mIPCClosed;
};

} 
} 

#endif 
