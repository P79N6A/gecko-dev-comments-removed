



#ifndef nsAsyncStreamCopier_h__
#define nsAsyncStreamCopier_h__

#include "nsIAsyncStreamCopier.h"
#include "mozilla/Mutex.h"
#include "nsStreamUtils.h"
#include "nsCOMPtr.h"

class nsIRequestObserver;



class nsAsyncStreamCopier : public nsIAsyncStreamCopier
{
public:
    NS_DECL_THREADSAFE_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSIASYNCSTREAMCOPIER

    nsAsyncStreamCopier();
    virtual ~nsAsyncStreamCopier();

    
    

    bool IsComplete(nsresult *status = nullptr);
    void   Complete(nsresult status);

private:

    static void OnAsyncCopyComplete(void *, nsresult);

    nsCOMPtr<nsIInputStream>       mSource;
    nsCOMPtr<nsIOutputStream>      mSink;

    nsCOMPtr<nsIRequestObserver>   mObserver;

    nsCOMPtr<nsIEventTarget>       mTarget;

    nsCOMPtr<nsISupports>          mCopierCtx;

    mozilla::Mutex                 mLock;

    nsAsyncCopyMode                mMode;
    uint32_t                       mChunkSize;
    nsresult                       mStatus;
    bool                           mIsPending;
    bool                           mCloseSource;
    bool                           mCloseSink;
};

#endif 
