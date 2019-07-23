




































#ifndef nsAsyncStreamCopier_h__
#define nsAsyncStreamCopier_h__

#include "nsIAsyncStreamCopier.h"
#include "nsIAsyncInputStream.h"
#include "nsIAsyncOutputStream.h"
#include "nsIRequestObserver.h"
#include "nsStreamUtils.h"
#include "nsCOMPtr.h"
#include "prlock.h"



class nsAsyncStreamCopier : public nsIAsyncStreamCopier
{
public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSIREQUEST
    NS_DECL_NSIASYNCSTREAMCOPIER

    nsAsyncStreamCopier();
    virtual ~nsAsyncStreamCopier();

    
    

    PRBool IsComplete(nsresult *status = nsnull);
    void   Complete(nsresult status);

private:

    static void OnAsyncCopyComplete(void *, nsresult);

    nsCOMPtr<nsIInputStream>       mSource;
    nsCOMPtr<nsIOutputStream>      mSink;

    nsCOMPtr<nsIRequestObserver>   mObserver;
    nsCOMPtr<nsISupports>          mObserverContext;

    nsCOMPtr<nsIEventTarget>       mTarget;

    PRLock                        *mLock;

    nsAsyncCopyMode                mMode;
    PRUint32                       mChunkSize;
    nsresult                       mStatus;
    PRPackedBool                   mIsPending;
};

#endif 
