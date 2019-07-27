





#ifndef RtspChannelChild_h
#define RtspChannelChild_h

#include "mozilla/net/PRtspChannelChild.h"
#include "mozilla/net/NeckoChild.h"
#include "nsBaseChannel.h"
#include "nsIChildChannel.h"
#include "nsIStreamingProtocolController.h"
#include "nsIStreamingProtocolService.h"

namespace mozilla {
namespace net {












class RtspChannelChild : public PRtspChannelChild
                       , public nsBaseChannel
                       , public nsIChildChannel
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSICHILDCHANNEL

  RtspChannelChild(nsIURI *aUri);

  
  NS_IMETHOD GetContentType(nsACString & aContentType) override final;
  NS_IMETHOD AsyncOpen(nsIStreamListener *listener, nsISupports *aContext)
                       override final;
  NS_IMETHOD AsyncOpen2(nsIStreamListener *listener) override final;

  
  NS_IMETHOD OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
                            override final;
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

  
  void AddIPDLReference();
  void ReleaseIPDLReference();

  
  nsIStreamingProtocolController* GetController();
  void ReleaseController();

protected:
  ~RtspChannelChild();

private:
  bool mIPCOpen;
  bool mCanceled;
  nsCOMPtr<nsIStreamingProtocolController> mMediaStreamController;
};

} 
} 

#endif 
