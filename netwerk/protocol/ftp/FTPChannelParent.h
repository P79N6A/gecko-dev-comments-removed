






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
                       , public nsILoadContext
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIREQUESTOBSERVER
  NS_DECL_NSISTREAMLISTENER
  NS_DECL_NSIPARENTCHANNEL
  NS_DECL_NSIINTERFACEREQUESTOR
  NS_DECL_NSILOADCONTEXT

  FTPChannelParent();
  virtual ~FTPChannelParent();

protected:
  NS_OVERRIDE virtual bool RecvAsyncOpen(const IPC::URI& uri,
                                         const PRUint64& startPos,
                                         const nsCString& entityID,
                                         const IPC::InputStream& uploadStream,
                                         const bool& haveLoadContext,
                                         const bool& isContent,
                                         const bool& usingPrivateBrowsing,
                                         const bool& isInBrowserElement,
                                         const PRUint32& appId,
                                         const nsCString& extendedOrigin);
  NS_OVERRIDE virtual bool RecvConnectChannel(const PRUint32& channelId);
  NS_OVERRIDE virtual bool RecvCancel(const nsresult& status);
  NS_OVERRIDE virtual bool RecvSuspend();
  NS_OVERRIDE virtual bool RecvResume();

  NS_OVERRIDE virtual void ActorDestroy(ActorDestroyReason why);

  nsRefPtr<nsFtpChannel> mChannel;

  bool mIPCClosed;

  
  bool mHaveLoadContext       : 1;
  bool mIsContent             : 1;
  bool mUsePrivateBrowsing    : 1;
  bool mIsInBrowserElement    : 1;

  PRUint32 mAppId;
  nsCString mExtendedOrigin;
};

} 
} 

#endif 
