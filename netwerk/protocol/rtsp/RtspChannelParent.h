





#ifndef RtspChannelParent_h
#define RtspChannelParent_h

#include "mozilla/net/PRtspChannelParent.h"
#include "mozilla/net/NeckoParent.h"
#include "nsBaseChannel.h"
#include "nsIParentChannel.h"

namespace mozilla {
namespace net {











class RtspChannelParent : public PRtspChannelParent
                        , public nsBaseChannel
                        , public nsIParentChannel
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIPARENTCHANNEL

  RtspChannelParent(nsIURI *aUri);
  ~RtspChannelParent();

  
  NS_IMETHOD GetContentType(nsACString & aContentType) MOZ_OVERRIDE MOZ_FINAL;
  NS_IMETHOD AsyncOpen(nsIStreamListener *listener,
                       nsISupports *aContext) MOZ_OVERRIDE MOZ_FINAL;

  
  NS_IMETHOD OnStartRequest(nsIRequest *aRequest,
                            nsISupports *aContext) MOZ_OVERRIDE MOZ_FINAL;
  NS_IMETHOD OnStopRequest(nsIRequest *aRequest,
                           nsISupports *aContext,
                           nsresult aStatusCode) MOZ_OVERRIDE MOZ_FINAL;

  
  NS_IMETHOD OnDataAvailable(nsIRequest *aRequest,
                             nsISupports *aContext,
                             nsIInputStream *aInputStream,
                             uint64_t aOffset,
                             uint32_t aCount) MOZ_OVERRIDE MOZ_FINAL;

  
  NS_IMETHOD Cancel(nsresult status) MOZ_OVERRIDE MOZ_FINAL;
  NS_IMETHOD Suspend() MOZ_OVERRIDE MOZ_FINAL;
  NS_IMETHOD Resume() MOZ_OVERRIDE MOZ_FINAL;

  
  NS_IMETHOD OpenContentStream(bool aAsync,
                               nsIInputStream **aStream,
                               nsIChannel **aChannel) MOZ_OVERRIDE MOZ_FINAL;

  
  bool Init(const RtspChannelConnectArgs& aArgs);

protected:
  
  
  bool ConnectChannel(const uint32_t& channelId);

private:
  bool mIPCClosed;
  virtual void ActorDestroy(ActorDestroyReason why) MOZ_OVERRIDE;
};

} 
} 

#endif 
