





#ifndef InterceptedJARChannel_h
#define InterceptedJARChannel_h

#include "nsJAR.h"
#include "nsJARChannel.h"
#include "nsIInputStream.h"
#include "nsIInputStreamPump.h"
#include "nsINetworkInterceptController.h"
#include "nsIOutputStream.h"
#include "nsRefPtr.h"

#include "mozilla/Maybe.h"

class nsIStreamListener;
class nsJARChannel;

namespace mozilla {
namespace net {




class InterceptedJARChannel : public nsIInterceptedChannel
{
  
  
  nsCOMPtr<nsINetworkInterceptController> mController;

  
  nsRefPtr<nsJARChannel> mChannel;

  
  nsCOMPtr<nsIInputStream> mSynthesizedInput;

  
  nsCOMPtr<nsIOutputStream> mResponseBody;

  
  nsCString mContentType;

  
  bool mIsNavigation;

  virtual ~InterceptedJARChannel() {};
public:
  InterceptedJARChannel(nsJARChannel* aChannel,
                        nsINetworkInterceptController* aController,
                        bool aIsNavigation);

  NS_DECL_ISUPPORTS
  NS_DECL_NSIINTERCEPTEDCHANNEL

  void NotifyController();
};

} 
} 

#endif 
