





#ifndef InterceptedChannel_h
#define InterceptedChannel_h

#include "nsINetworkInterceptController.h"
#include "nsRefPtr.h"
#include "mozilla/Maybe.h"

class nsICacheEntry;
class nsInputStreamPump;
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
  nsresult DoSynthesizeStatus(uint16_t aStatus, const nsACString& aReason);
  nsresult DoSynthesizeHeader(const nsACString& aName, const nsACString& aValue);

  virtual ~InterceptedChannelBase();
public:
  InterceptedChannelBase(nsINetworkInterceptController* aController,
                         bool aIsNavigation);

  
  
  virtual void NotifyController() = 0;

  NS_DECL_ISUPPORTS

  NS_IMETHOD GetResponseBody(nsIOutputStream** aOutput) override;
  NS_IMETHOD GetIsNavigation(bool* aIsNavigation) override;
};

class InterceptedChannelChrome : public InterceptedChannelBase
{
  
  nsRefPtr<nsHttpChannel> mChannel;

  
  nsCOMPtr<nsICacheEntry> mSynthesizedCacheEntry;

  
  
  
  
  bool mOldApplyConversion;
public:
  InterceptedChannelChrome(nsHttpChannel* aChannel,
                           nsINetworkInterceptController* aController,
                           nsICacheEntry* aEntry);

  NS_IMETHOD ResetInterception() override;
  NS_IMETHOD FinishSynthesizedResponse() override;
  NS_IMETHOD GetChannel(nsIChannel** aChannel) override;
  NS_IMETHOD SynthesizeStatus(uint16_t aStatus, const nsACString& aReason) override;
  NS_IMETHOD SynthesizeHeader(const nsACString& aName, const nsACString& aValue) override;
  NS_IMETHOD Cancel() override;
  NS_IMETHOD SetSecurityInfo(nsISupports* aSecurityInfo) override;

  virtual void NotifyController() override;
};

class InterceptedChannelContent : public InterceptedChannelBase
{
  
  nsRefPtr<HttpChannelChild> mChannel;

  
  nsCOMPtr<nsIInputStream> mSynthesizedInput;

  
  
  nsCOMPtr<nsIStreamListener> mStreamListener;
public:
  InterceptedChannelContent(HttpChannelChild* aChannel,
                            nsINetworkInterceptController* aController,
                            nsIStreamListener* aListener);

  NS_IMETHOD ResetInterception() override;
  NS_IMETHOD FinishSynthesizedResponse() override;
  NS_IMETHOD GetChannel(nsIChannel** aChannel) override;
  NS_IMETHOD SynthesizeStatus(uint16_t aStatus, const nsACString& aReason) override;
  NS_IMETHOD SynthesizeHeader(const nsACString& aName, const nsACString& aValue) override;
  NS_IMETHOD Cancel() override;
  NS_IMETHOD SetSecurityInfo(nsISupports* aSecurityInfo) override;

  virtual void NotifyController() override;
};

} 
} 

#endif 
