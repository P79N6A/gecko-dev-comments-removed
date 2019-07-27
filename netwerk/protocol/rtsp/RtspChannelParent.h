





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

  
  NS_IMETHOD GetContentType(nsACString & aContentType) override final;
  NS_IMETHOD AsyncOpen(nsIStreamListener *listener,
                       nsISupports *aContext) override final;
  NS_IMETHOD AsyncOpen2(nsIStreamListener *listener) override final;

  
  NS_IMETHOD OnStartRequest(nsIRequest *aRequest,
                            nsISupports *aContext) override final;
  NS_IMETHOD OnStopRequest(nsIRequest *aRequest,
                           nsISupports *aContext,
                           nsresult aStatusCode) override final;

  
  NS_IMETHOD OnDataAvailable(nsIRequest *aRequest,
                             nsISupports *aContext,
                             nsIInputStream *aInputStream,
                             uint64_t aOffset,
                             uint32_t aCount) override final;

  
  NS_IMETHOD Cancel(nsresult status) override final;
  NS_IMETHOD Suspend() override final;
  NS_IMETHOD Resume() override final;

  
  NS_IMETHOD OpenContentStream(bool aAsync,
                               nsIInputStream **aStream,
                               nsIChannel **aChannel) override final;

  
  bool Init(const RtspChannelConnectArgs& aArgs);

protected:
  ~RtspChannelParent();

  
  
  bool ConnectChannel(const uint32_t& channelId);

private:
  bool mIPCClosed;
  virtual void ActorDestroy(ActorDestroyReason why) override;
};

} 
} 

#endif 
