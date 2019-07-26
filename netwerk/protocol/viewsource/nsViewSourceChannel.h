




#ifndef nsViewSourceChannel_h___
#define nsViewSourceChannel_h___

#include "nsString.h"
#include "nsCOMPtr.h"
#include "nsXPIDLString.h"
#include "nsIViewSourceChannel.h"
#include "nsIURI.h"
#include "nsIStreamListener.h"
#include "nsViewSourceHandler.h"
#include "nsNetCID.h"
#include "nsIHttpChannel.h"
#include "nsIHttpChannelInternal.h"
#include "nsICachingChannel.h"
#include "nsIApplicationCacheChannel.h"
#include "nsIUploadChannel.h"
#include "mozilla/Attributes.h"

class nsViewSourceChannel MOZ_FINAL : public nsIViewSourceChannel,
                                      public nsIStreamListener,
                                      public nsIHttpChannel,
                                      public nsIHttpChannelInternal,
                                      public nsICachingChannel,
                                      public nsIApplicationCacheChannel,
                                      public nsIUploadChannel
{

public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIVIEWSOURCECHANNEL
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSIHTTPCHANNEL
    NS_FORWARD_SAFE_NSICACHEINFOCHANNEL(mCachingChannel)
    NS_FORWARD_SAFE_NSICACHINGCHANNEL(mCachingChannel)
    NS_FORWARD_SAFE_NSIAPPLICATIONCACHECHANNEL(mApplicationCacheChannel)
    NS_FORWARD_SAFE_NSIAPPLICATIONCACHECONTAINER(mApplicationCacheChannel)
    NS_FORWARD_SAFE_NSIUPLOADCHANNEL(mUploadChannel)
    NS_FORWARD_SAFE_NSIHTTPCHANNELINTERNAL(mHttpChannelInternal)

    
    nsViewSourceChannel()
        : mIsDocument(false)
        , mOpened(false) {}

    NS_HIDDEN_(nsresult) Init(nsIURI* uri);

    NS_HIDDEN_(nsresult) InitSrcdoc(nsIURI* aURI, const nsAString &aSrcdoc);

protected:
    nsCOMPtr<nsIChannel>        mChannel;
    nsCOMPtr<nsIHttpChannel>    mHttpChannel;
    nsCOMPtr<nsIHttpChannelInternal>    mHttpChannelInternal;
    nsCOMPtr<nsICachingChannel> mCachingChannel;
    nsCOMPtr<nsIApplicationCacheChannel> mApplicationCacheChannel;
    nsCOMPtr<nsIUploadChannel>  mUploadChannel;
    nsCOMPtr<nsIStreamListener> mListener;
    nsCOMPtr<nsIURI>            mOriginalURI;
    nsCString                   mContentType;
    bool                        mIsDocument; 
    bool                        mOpened;
};

#endif 
