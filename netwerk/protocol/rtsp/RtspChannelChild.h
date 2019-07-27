





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

  
  NS_IMETHOD GetContentType(nsACString & aContentType) MOZ_OVERRIDE MOZ_FINAL;
  NS_IMETHOD AsyncOpen(nsIStreamListener *listener, nsISupports *aContext)
                       MOZ_OVERRIDE MOZ_FINAL;

  
  NS_IMETHOD OnStartRequest(nsIRequest *aRequest, nsISupports *aContext)
                            MOZ_OVERRIDE MOZ_FINAL;
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
