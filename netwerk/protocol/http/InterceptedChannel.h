





#ifndef InterceptedChannel_h
#define InterceptedChannel_h

#include "nsINetworkInterceptController.h"
#include "nsRefPtr.h"
#include "mozilla/Maybe.h"

class nsICacheEntry;
class nsInputStreamPump;
class nsIStorageStream;
class nsIStreamListener;

namespace mozilla {
namespace net {

class nsHttpChannel;
class HttpChannelChild;
class nsHttpResponseHead;



class InterceptedChannelBase : public nsIInterceptedChannel {
protected:
  
  nsCOMPtr<nsINetworkInterceptController> mController;

  
  Maybe<nsHttpResponseHead> mSynthesizedResponseHead;

  void EnsureSynthesizedResponse();
  void DoNotifyController(nsIOutputStream* aOut);
  nsresult DoSynthesizeHeader(const nsACString& aName, const nsACString& aValue);

  virtual ~InterceptedChannelBase();
public:
  explicit InterceptedChannelBase(nsINetworkInterceptController* aController);

  
  
  virtual void NotifyController() = 0;

  NS_DECL_ISUPPORTS
};

class InterceptedChannelChrome : public InterceptedChannelBase
{
  
  nsRefPtr<nsHttpChannel> mChannel;

  
  nsCOMPtr<nsICacheEntry> mSynthesizedCacheEntry;
public:
  InterceptedChannelChrome(nsHttpChannel* aChannel,
                           nsINetworkInterceptController* aController,
                           nsICacheEntry* aEntry);

  NS_DECL_NSIINTERCEPTEDCHANNEL

  virtual void NotifyController() MOZ_OVERRIDE;
};

class InterceptedChannelContent : public InterceptedChannelBase
{
  
  nsRefPtr<HttpChannelChild> mChannel;

  
  nsCOMPtr<nsIOutputStream> mSynthesizedOutput;
  nsCOMPtr<nsIInputStream> mSynthesizedInput;

  
  nsRefPtr<nsInputStreamPump> mStoragePump;

  
  
  nsCOMPtr<nsIStreamListener> mStreamListener;
public:
  InterceptedChannelContent(HttpChannelChild* aChannel,
                            nsINetworkInterceptController* aController,
                            nsIStreamListener* aListener);

  NS_DECL_NSIINTERCEPTEDCHANNEL

  virtual void NotifyController() MOZ_OVERRIDE;
};

} 
} 

#endif
