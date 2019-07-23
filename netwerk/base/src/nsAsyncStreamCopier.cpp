




































#include "nsAsyncStreamCopier.h"
#include "nsIEventTarget.h"
#include "nsStreamUtils.h"
#include "nsNetSegmentUtils.h"
#include "nsNetUtil.h"
#include "nsAutoLock.h"
#include "prlog.h"

#if defined(PR_LOGGING)



static PRLogModuleInfo *gStreamCopierLog = nsnull;
#endif
#define LOG(args) PR_LOG(gStreamCopierLog, PR_LOG_DEBUG, args)



nsAsyncStreamCopier::nsAsyncStreamCopier()
    : mLock(nsnull)
    , mMode(NS_ASYNCCOPY_VIA_READSEGMENTS)
    , mChunkSize(NET_DEFAULT_SEGMENT_SIZE)
    , mStatus(NS_OK)
    , mIsPending(PR_FALSE)
{
#if defined(PR_LOGGING)
    if (!gStreamCopierLog)
        gStreamCopierLog = PR_NewLogModule("nsStreamCopier");
#endif
    LOG(("Creating nsAsyncStreamCopier @%x\n", this));
}

nsAsyncStreamCopier::~nsAsyncStreamCopier()
{
    LOG(("Destroying nsAsyncStreamCopier @%x\n", this));
    if (mLock)
        PR_DestroyLock(mLock);
}

PRBool
nsAsyncStreamCopier::IsComplete(nsresult *status)
{
    nsAutoLock lock(mLock);
    if (status)
        *status = mStatus;
    return !mIsPending;
}

void
nsAsyncStreamCopier::Complete(nsresult status)
{
    LOG(("nsAsyncStreamCopier::Complete [this=%x status=%x]\n", this, status));

    nsCOMPtr<nsIRequestObserver> observer;
    nsCOMPtr<nsISupports> ctx;
    {
        nsAutoLock lock(mLock);
        mCopierCtx = nsnull;

        if (mIsPending) {
            mIsPending = PR_FALSE;
            mStatus = status;

            
            observer = mObserver;
            ctx = mObserverContext;
            mObserver = nsnull;
            mObserverContext = nsnull;
        }
    }

    if (observer) {
        LOG(("  calling OnStopRequest [status=%x]\n", status));
        observer->OnStopRequest(this, ctx, status);
    }
}

void
nsAsyncStreamCopier::OnAsyncCopyComplete(void *closure, nsresult status)
{
    nsAsyncStreamCopier *self = (nsAsyncStreamCopier *) closure;
    self->Complete(status);
    NS_RELEASE(self); 
}




NS_IMPL_THREADSAFE_ISUPPORTS2(nsAsyncStreamCopier,
                              nsIRequest,
                              nsIAsyncStreamCopier)




NS_IMETHODIMP
nsAsyncStreamCopier::GetName(nsACString &name)
{
    name.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::IsPending(PRBool *result)
{
    *result = !IsComplete();
    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::GetStatus(nsresult *status)
{
    IsComplete(status);
    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::Cancel(nsresult status)
{
    nsCOMPtr<nsISupports> copierCtx;
    {
        nsAutoLock lock(mLock);
        if (!mIsPending)
            return NS_OK;
        copierCtx.swap(mCopierCtx);
    }

    if (NS_SUCCEEDED(status)) {
        NS_WARNING("cancel with non-failure status code");
        status = NS_BASE_STREAM_CLOSED;
    }

    if (copierCtx)
        NS_CancelAsyncCopy(copierCtx, status);

    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::Suspend()
{
    NS_NOTREACHED("nsAsyncStreamCopier::Suspend");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsAsyncStreamCopier::Resume()
{
    NS_NOTREACHED("nsAsyncStreamCopier::Resume");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsAsyncStreamCopier::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = LOAD_NORMAL;
    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    *aLoadGroup = nsnull;
    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    return NS_OK;
}




NS_IMETHODIMP
nsAsyncStreamCopier::Init(nsIInputStream *source,
                          nsIOutputStream *sink,
                          nsIEventTarget *target,
                          PRBool sourceBuffered,
                          PRBool sinkBuffered,
                          PRUint32 chunkSize,
                          PRBool closeSource,
                          PRBool closeSink)
{
    NS_ASSERTION(sourceBuffered || sinkBuffered, "at least one stream must be buffered");

    NS_ASSERTION(!mLock, "already initialized");
    mLock = PR_NewLock();
    if (!mLock)
        return NS_ERROR_OUT_OF_MEMORY;

    if (chunkSize == 0)
        chunkSize = NET_DEFAULT_SEGMENT_SIZE;
    mChunkSize = chunkSize;

    mSource = source;
    mSink = sink;
    mCloseSource = closeSource;
    mCloseSink = closeSink;

    mMode = sourceBuffered ? NS_ASYNCCOPY_VIA_READSEGMENTS
                           : NS_ASYNCCOPY_VIA_WRITESEGMENTS;
    if (target)
        mTarget = target;
    else {
        nsresult rv;
        mTarget = do_GetService(NS_STREAMTRANSPORTSERVICE_CONTRACTID, &rv);
        if (NS_FAILED(rv)) return rv;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsAsyncStreamCopier::AsyncCopy(nsIRequestObserver *observer, nsISupports *ctx)
{
    LOG(("nsAsyncStreamCopier::AsyncCopy [this=%x observer=%x]\n", this, observer));

    NS_ASSERTION(mSource && mSink, "not initialized");
    nsresult rv;

    if (observer) {
        
        rv = NS_NewRequestObserverProxy(getter_AddRefs(mObserver), observer);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    mIsPending = PR_TRUE;

    mObserverContext = ctx;
    if (mObserver) {
        rv = mObserver->OnStartRequest(this, mObserverContext);
        if (NS_FAILED(rv))
            Cancel(rv);
    }
    
    
    
    NS_ADDREF_THIS();
    rv = NS_AsyncCopy(mSource, mSink, mTarget, mMode, mChunkSize,
                      OnAsyncCopyComplete, this, mCloseSource, mCloseSink,
                      getter_AddRefs(mCopierCtx));
    if (NS_FAILED(rv)) {
        NS_RELEASE_THIS();
        Cancel(rv);
    }

    return NS_OK;
}
