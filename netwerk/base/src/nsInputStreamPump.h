




#ifndef nsInputStreamPump_h__
#define nsInputStreamPump_h__

#include "nsIInputStreamPump.h"
#include "nsIInputStream.h"
#include "nsIURI.h"
#include "nsILoadGroup.h"
#include "nsIStreamListener.h"
#include "nsIInterfaceRequestor.h"
#include "nsIProgressEventSink.h"
#include "nsIAsyncInputStream.h"
#include "nsIThread.h"
#include "nsCOMPtr.h"
#include "mozilla/Attributes.h"

class nsInputStreamPump MOZ_FINAL : public nsIInputStreamPump
                                  , public nsIInputStreamCallback
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSIINPUTSTREAMPUMP
    NS_DECL_NSIINPUTSTREAMCALLBACK

    nsInputStreamPump(); 
    ~nsInputStreamPump();

    static NS_HIDDEN_(nsresult)
                      Create(nsInputStreamPump  **result,
                             nsIInputStream      *stream,
                             int64_t              streamPos = -1,
                             int64_t              streamLen = -1,
                             uint32_t             segsize = 0,
                             uint32_t             segcount = 0,
                             bool                 closeWhenDone = false);

    typedef void (*PeekSegmentFun)(void *closure, const uint8_t *buf,
                                   uint32_t bufLen);
    









    NS_HIDDEN_(nsresult) PeekStream(PeekSegmentFun callback, void *closure);

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
    nsCOMPtr<nsIThread>           mTargetThread;
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
    bool                          mWaiting; 
    bool                          mCloseWhenDone;
};

#endif
