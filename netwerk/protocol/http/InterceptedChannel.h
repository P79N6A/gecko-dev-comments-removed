





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

  
  nsCOMPtr<nsIOutputStream> mResponseBody;

  
  Maybe<nsAutoPtr<nsHttpResponseHead>> mSynthesizedResponseHead;

  
  bool mIsNavigation;

  void EnsureSynthesizedResponse();
  void DoNotifyController();
  nsresult DoSynthesizeHeader(const nsACString& aName, const nsACString& aValue);

  virtual ~InterceptedChannelBase();
public:
  InterceptedChannelBase(nsINetworkInterceptController* aController,
                         bool aIsNavigation);

  
  
  virtual void NotifyController() = 0;

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetResponseBody(nsIOutputStream** aOutput) MOZ_OVERRIDE;
  NS_IMETHOD GetIsNavigation(bool* aIsNavigation) MOZ_OVERRIDE;
};

class InterceptedChannelChrome : public InterceptedChannelBase
{
  
  nsRefPtr<nsHttpChannel> mChannel;

  
  nsCOMPtr<nsICacheEntry> mSynthesizedCacheEntry;
public:
  InterceptedChannelChrome(nsHttpChannel* aChannel,
                           nsINetworkInterceptController* aController,
                           nsICacheEntry* aEntry);

  NS_IMETHOD ResetInterception() MOZ_OVERRIDE;
  NS_IMETHOD FinishSynthesizedResponse() MOZ_OVERRIDE;
  NS_IMETHOD GetChannel(nsIChannel** aChannel) MOZ_OVERRIDE;
  NS_IMETHOD SynthesizeHeader(const nsACString& aName, const nsACString& aValue) MOZ_OVERRIDE;
  NS_IMETHOD Cancel() MOZ_OVERRIDE;

  virtual void NotifyController() MOZ_OVERRIDE;
};

class InterceptedChannelContent : public InterceptedChannelBase
{
  
  nsRefPtr<HttpChannelChild> mChannel;

  
  nsCOMPtr<nsIInputStream> mSynthesizedInput;

  
  nsRefPtr<nsInputStreamPump> mStoragePump;

  
  
  nsCOMPtr<nsIStreamListener> mStreamListener;
public:
  InterceptedChannelContent(HttpChannelChild* aChannel,
                            nsINetworkInterceptController* aController,
                            nsIStreamListener* aListener);

  NS_IMETHOD ResetInterception() MOZ_OVERRIDE;
  NS_IMETHOD FinishSynthesizedResponse() MOZ_OVERRIDE;
  NS_IMETHOD GetChannel(nsIChannel** aChannel) MOZ_OVERRIDE;
  NS_IMETHOD SynthesizeHeader(const nsACString& aName, const nsACString& aValue) MOZ_OVERRIDE;
  NS_IMETHOD Cancel() MOZ_OVERRIDE;

  virtual void NotifyController() MOZ_OVERRIDE;
};

} 
} 

#endif 
