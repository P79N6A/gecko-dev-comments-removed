



#ifndef mozilla_net_NullHttpChannel_h
#define mozilla_net_NullHttpChannel_h

#include "nsINullChannel.h"
#include "nsIHttpChannel.h"
#include "nsITimedChannel.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "mozilla/TimeStamp.h"
#include "nsString.h"
#include "prtime.h"

class nsProxyInfo;
class nsHttpChannel;

namespace mozilla {
namespace net {

class NullHttpChannel final
  : public nsINullChannel
  , public nsIHttpChannel
  , public nsITimedChannel
{
  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSINULLCHANNEL
  NS_DECL_NSIHTTPCHANNEL
  NS_DECL_NSITIMEDCHANNEL
  NS_DECL_NSIREQUEST
  NS_DECL_NSICHANNEL

  NullHttpChannel();

  
  
  explicit NullHttpChannel(nsIHttpChannel * chan);

  
  nsresult Init(nsIURI *aURI, uint32_t aCaps, nsProxyInfo *aProxyInfo,
                        uint32_t aProxyResolveFlags,
                        nsIURI *aProxyURI);
private:
  ~NullHttpChannel() { }

protected:
  nsCOMPtr<nsIURI> mURI;
  nsCOMPtr<nsIURI> mOriginalURI;

  nsString  mInitiatorType;
  PRTime    mChannelCreationTime;
  TimeStamp mAsyncOpenTime;
  TimeStamp mChannelCreationTimestamp;
  nsCOMPtr<nsIPrincipal> mResourcePrincipal;
  nsCString mTimingAllowOriginHeader;
  bool      mAllRedirectsSameOrigin;
  bool      mAllRedirectsPassTimingAllowCheck;
};

} 
} 



#endif 
