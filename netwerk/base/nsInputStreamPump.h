




#ifndef nsInputStreamPump_h__
#define nsInputStreamPump_h__

#include "nsIInputStreamPump.h"
#include "nsIAsyncInputStream.h"
#include "nsIThreadRetargetableRequest.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"
#include "mozilla/ReentrantMonitor.h"

class nsIInputStream;
class nsILoadGroup;
class nsIStreamListener;

class nsInputStreamPump final : public nsIInputStreamPump
                                  , public nsIInputStreamCallback
                                  , public nsIThreadRetargetableRequest
{
    ~nsInputStreamPump();

public:
    typedef mozilla::ReentrantMonitorAutoEnter ReentrantMonitorAutoEnter;
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSIINPUTSTREAMPUMP
    NS_DECL_NSIINPUTSTREAMCALLBACK
    NS_DECL_NSITHREADRETARGETABLEREQUEST

    nsInputStreamPump(); 

    static nsresult
                      Create(nsInputStreamPump  **result,
                             nsIInputStream      *stream,
                             int64_t              streamPos = -1,
                             int64_t              streamLen = -1,
                             uint32_t             segsize = 0,
                             uint32_t             segcount = 0,
                             bool                 closeWhenDone = false);

    typedef void (*PeekSegmentFun)(void *closure, const uint8_t *buf,
                                   uint32_t bufLen);
    









    nsresult PeekStream(PeekSegmentFun callback, void *closure);

    



    nsresult CallOnStateStop();

protected:

    enum {
        STATE_IDLE,
        STATE_START,
        STATE_TRANSFER,
        STATE_STOP
    };

    nsresult EnsureWaiting();
    uint32_t OnStateStart();
    uint32_t OnStateTransfer();
    uint32_t OnStateStop();

    uint32_t                      mState;
    nsCOMPtr<nsILoadGroup>        mLoadGroup;
    nsCOMPtr<nsIStreamListener>   mListener;
    nsCOMPtr<nsISupports>         mListenerContext;
    nsCOMPtr<nsIEventTarget>      mTargetThread;
    nsCOMPtr<nsIInputStream>      mStream;
    nsCOMPtr<nsIAsyncInputStream> mAsyncStream;
    uint64_t                      mStreamOffset;
    uint64_t                      mStreamLength;
    uint32_t                      mSegSize;
    uint32_t                      mSegCount;
    nsresult                      mStatus;
    uint32_t                      mSuspendCount;
    uint32_t                      mLoadFlags;
    bool                          mIsPending;
    
    
    bool                          mProcessingCallbacks;
    
    bool                          mWaitingForInputStreamReady;
    bool                          mCloseWhenDone;
    bool                          mRetargeting;
    
    mozilla::ReentrantMonitor     mMonitor;
};

#endif
