




#ifndef nsJARChannel_h__
#define nsJARChannel_h__

#include "mozilla/net/MemoryDownloader.h"
#include "nsIJARChannel.h"
#include "nsIJARURI.h"
#include "nsIInputStreamPump.h"
#include "InterceptedJARChannel.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsIStreamListener.h"
#include "nsIRemoteOpenFileListener.h"
#include "nsIZipReader.h"
#include "nsILoadGroup.h"
#include "nsILoadInfo.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsIThreadRetargetableStreamListener.h"
#include "nsHashPropertyBag.h"
#include "nsIFile.h"
#include "nsIURI.h"
#include "nsCOMPtr.h"
#include "nsString.h"
#include "mozilla/Logging.h"

class nsJARInputThunk;
class nsInputStreamPump;

namespace mozilla {
namespace net {
  class InterceptedJARChannel;
} 
} 



class nsJARChannel final : public nsIJARChannel
                         , public mozilla::net::MemoryDownloader::IObserver
                         , public nsIStreamListener
                         , public nsIRemoteOpenFileListener
                         , public nsIThreadRetargetableRequest
                         , public nsIThreadRetargetableStreamListener
                         , public nsHashPropertyBag
{
public:
    NS_DECL_ISUPPORTS_INHERITED
    NS_DECL_NSIREQUEST
    NS_DECL_NSICHANNEL
    NS_DECL_NSIJARCHANNEL
    NS_DECL_NSIREQUESTOBSERVER
    NS_DECL_NSISTREAMLISTENER
    NS_DECL_NSIREMOTEOPENFILELISTENER
    NS_DECL_NSITHREADRETARGETABLEREQUEST
    NS_DECL_NSITHREADRETARGETABLESTREAMLISTENER

    nsJARChannel();

    nsresult Init(nsIURI *uri);

    nsresult OverrideSecurityInfo(nsISupports* aSecurityInfo);
    void OverrideURI(nsIURI* aRedirectedURI);

private:
    virtual ~nsJARChannel();

    nsresult CreateJarInput(nsIZipReaderCache *, nsJARInputThunk **);
    nsresult LookupFile(bool aAllowAsync);
    nsresult OpenLocalFile();
    void NotifyError(nsresult aError);
    void FireOnProgress(uint64_t aProgress);
    nsresult SetRemoteNSPRFileDesc(PRFileDesc *fd);
    virtual void OnDownloadComplete(mozilla::net::MemoryDownloader* aDownloader,
                                    nsIRequest* aRequest,
                                    nsISupports* aCtxt,
                                    nsresult aStatus,
                                    mozilla::net::MemoryDownloader::Data aData)
        override;

    
    
    bool ShouldIntercept();

    nsresult ContinueAsyncOpen();
    void FinishAsyncOpen();

    
    
    void ResetInterception();
    
    
    void OverrideWithSynthesizedResponse(nsIInputStream* aSynthesizedInput,
                                         const nsACString& aContentType);

    nsCString                       mSpec;

    bool                            mOpened;

    nsCOMPtr<nsIJARURI>             mJarURI;
    nsCOMPtr<nsIURI>                mOriginalURI;
    nsCOMPtr<nsIURI>                mAppURI;
    nsCOMPtr<nsISupports>           mOwner;
    nsCOMPtr<nsILoadInfo>           mLoadInfo;
    nsCOMPtr<nsIInterfaceRequestor> mCallbacks;
    nsCOMPtr<nsISupports>           mSecurityInfo;
    nsCOMPtr<nsIProgressEventSink>  mProgressSink;
    nsCOMPtr<nsILoadGroup>          mLoadGroup;
    nsCOMPtr<nsIStreamListener>     mListener;
    nsCOMPtr<nsISupports>           mListenerContext;
    nsCString                       mContentType;
    nsCString                       mContentCharset;
    nsCString                       mContentDispositionHeader;
    

    uint32_t                        mContentDisposition;
    int64_t                         mContentLength;
    uint32_t                        mLoadFlags;
    nsresult                        mStatus;
    bool                            mIsPending;
    bool                            mIsUnsafe;
    bool                            mOpeningRemote;
    bool                            mEnsureChildFd;

    mozilla::net::MemoryDownloader::Data mTempMem;
    nsCOMPtr<nsIInputStreamPump>    mPump;
    
    
    nsCOMPtr<nsIRequest>            mRequest;
    nsCOMPtr<nsIFile>               mJarFile;
    nsCOMPtr<nsIURI>                mJarBaseURI;
    nsCString                       mJarEntry;
    nsCString                       mInnerJarEntry;

    nsRefPtr<nsInputStreamPump> mSynthesizedResponsePump;
    int64_t mSynthesizedStreamLength;

    friend class mozilla::net::InterceptedJARChannel;
};

#endif 
