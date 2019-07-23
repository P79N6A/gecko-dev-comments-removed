





































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
#include "nsICachingChannel.h"
#include "nsIUploadChannel.h"

class nsViewSourceChannel : public nsIViewSourceChannel,
                            public nsIStreamListener,
                            public nsIHttpChannel,
                            public nsICachingChannel,
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
    NS_FORWARD_SAFE_NSICACHINGCHANNEL(mCachingChannel)
    NS_FORWARD_SAFE_NSIUPLOADCHANNEL(mUploadChannel)

    
    nsViewSourceChannel()
        : mIsDocument(PR_FALSE)
        , mOpened(PR_FALSE) {}

    NS_HIDDEN_(nsresult) Init(nsIURI* uri);

protected:
    nsCOMPtr<nsIChannel>        mChannel;
    nsCOMPtr<nsIHttpChannel>    mHttpChannel;
    nsCOMPtr<nsICachingChannel> mCachingChannel;
    nsCOMPtr<nsIUploadChannel>  mUploadChannel;
    nsCOMPtr<nsIStreamListener> mListener;
    nsCOMPtr<nsIURI>            mOriginalURI;
    nsCString                   mContentType;
    PRBool                      mIsDocument; 
    PRBool                      mOpened;
};

#endif 
