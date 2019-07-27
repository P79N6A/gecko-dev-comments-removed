




#ifndef nsSecCheckWrapChannel_h__
#define nsSecCheckWrapChannel_h__

#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsISecCheckWrapChannel.h"
#include "nsIWyciwygChannel.h"
#include "mozilla/LoadInfo.h"





























class nsSecCheckWrapChannelBase : public nsIHttpChannel
                                , public nsIHttpChannelInternal
                                , public nsISecCheckWrapChannel
{
public:
  NS_FORWARD_NSIHTTPCHANNEL(mHttpChannel->)
  NS_FORWARD_NSIHTTPCHANNELINTERNAL(mHttpChannelInternal->)
  NS_FORWARD_NSICHANNEL(mChannel->)
  NS_FORWARD_NSIREQUEST(mRequest->)
  NS_DECL_NSISECCHECKWRAPCHANNEL
  NS_DECL_ISUPPORTS

  explicit nsSecCheckWrapChannelBase(nsIChannel* aChannel);

protected:
  virtual ~nsSecCheckWrapChannelBase();

  nsCOMPtr<nsIChannel>             mChannel;
  
  nsCOMPtr<nsIHttpChannel>         mHttpChannel;
  nsCOMPtr<nsIHttpChannelInternal> mHttpChannelInternal;
  nsCOMPtr<nsIRequest>             mRequest;
};





class nsSecCheckWrapChannel : public nsSecCheckWrapChannelBase
{
public:
  NS_IMETHOD GetLoadInfo(nsILoadInfo **aLoadInfo);
  NS_IMETHOD SetLoadInfo(nsILoadInfo *aLoadInfo);

  nsSecCheckWrapChannel(nsIChannel* aChannel, nsILoadInfo* aLoadInfo);

protected:
  virtual ~nsSecCheckWrapChannel();

  nsCOMPtr<nsILoadInfo> mLoadInfo;
};

#endif 
