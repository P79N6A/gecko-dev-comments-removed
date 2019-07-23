





































#ifndef nsWyciwygChannel_h___
#define nsWyciwygChannel_h___

#include "nsWyciwygProtocolHandler.h"
#include "nsXPIDLString.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "prlog.h"

#include "nsIWyciwygChannel.h"
#include "nsILoadGroup.h"
#include "nsIOutputStream.h"
#include "nsIInputStream.h"
#include "nsIInputStreamPump.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsIStreamListener.h"
#include "nsICacheListener.h"
#include "nsICacheEntryDescriptor.h"
#include "nsIURI.h"

extern PRLogModuleInfo * gWyciwygLog;



class nsWyciwygChannel: public nsIWyciwygChannel,
                        public nsIStreamListener,
                        public nsICacheListener
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIWYCIWYGCHANNEL
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSICACHELISTENER

    
    nsWyciwygChannel();
    virtual ~nsWyciwygChannel();

    nsresult Init(nsIURI *uri);

protected:
    nsresult ReadFromCache();
    nsresult OpenCacheEntry(const nsACString & aCacheKey, nsCacheAccessMode aWriteAccess, PRBool * aDelayFlag = nsnull);
       
    nsresult                            mStatus;
    PRBool                              mIsPending;
    PRInt32                             mContentLength;
    PRUint32                            mLoadFlags;
    nsCOMPtr<nsIURI>                    mURI;
    nsCOMPtr<nsIURI>                    mOriginalURI;
    nsCOMPtr<nsISupports>               mOwner;
    nsCOMPtr<nsIInterfaceRequestor>     mCallbacks;
    nsCOMPtr<nsIProgressEventSink>      mProgressSink;
    nsCOMPtr<nsILoadGroup>              mLoadGroup;
    nsCOMPtr<nsIStreamListener>         mListener;
    nsCOMPtr<nsISupports>               mListenerContext;

    
    nsCOMPtr<nsIInputStreamPump>        mPump;
    
    
    nsCOMPtr<nsICacheEntryDescriptor>   mCacheEntry;
    nsCOMPtr<nsIOutputStream>           mCacheOutputStream;
    nsCOMPtr<nsIInputStream>            mCacheInputStream;

    nsCOMPtr<nsISupports>               mSecurityInfo;
};

#endif 
