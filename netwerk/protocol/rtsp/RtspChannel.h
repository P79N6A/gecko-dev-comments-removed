





#ifndef RtspChannel_h
#define RtspChannel_h

#include "nsBaseChannel.h"

namespace mozilla {
namespace net {










class RtspChannel : public nsBaseChannel
                  , public nsIStreamListener
{
public:
  NS_DECL_ISUPPORTS

  RtspChannel() { }

  ~RtspChannel() { }

  
  NS_IMETHOD AsyncOpen(nsIStreamListener *listener,
                       nsISupports *aContext) MOZ_OVERRIDE MOZ_FINAL;
  
  NS_IMETHOD Init(nsIURI* uri);
  
  NS_IMETHOD GetContentType(nsACString & aContentType) MOZ_OVERRIDE MOZ_FINAL;

  NS_IMETHOD OpenContentStream(bool aAsync,
                               nsIInputStream **aStream,
                               nsIChannel **aChannel) MOZ_OVERRIDE MOZ_FINAL
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  NS_IMETHOD OnStartRequest(nsIRequest *aRequest,
                            nsISupports *aContext) MOZ_OVERRIDE MOZ_FINAL
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  NS_IMETHOD OnStopRequest(nsIRequest *aRequest,
                           nsISupports *aContext,
                           nsresult aStatusCode) MOZ_OVERRIDE MOZ_FINAL
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  
  NS_IMETHOD OnDataAvailable(nsIRequest *aRequest,
                             nsISupports *aContext,
                             nsIInputStream *aInputStream,
                             uint64_t aOffset,
                             uint32_t aCount) MOZ_OVERRIDE MOZ_FINAL
  {
    return NS_ERROR_NOT_IMPLEMENTED;
  }
};

}
} 
#endif 
