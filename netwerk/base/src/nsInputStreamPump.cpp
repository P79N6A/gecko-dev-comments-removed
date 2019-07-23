





































#include "nsInputStreamPump.h"
#include "nsIServiceManager.h"
#include "nsIStreamTransportService.h"
#include "nsIInterfaceRequestorUtils.h"
#include "nsISeekableStream.h"
#include "nsITransport.h"
#include "nsNetUtil.h"
#include "nsThreadUtils.h"
#include "nsNetSegmentUtils.h"
#include "nsCOMPtr.h"
#include "prlog.h"

static NS_DEFINE_CID(kStreamTransportServiceCID, NS_STREAMTRANSPORTSERVICE_CID);

#if defined(PR_LOGGING)



static PRLogModuleInfo *gStreamPumpLog = nsnull;
#endif
#define LOG(args) PR_LOG(gStreamPumpLog, PR_LOG_DEBUG, args)





nsInputStreamPump::nsInputStreamPump()
    : mState(STATE_IDLE)
    , mStreamOffset(0)
    , mStreamLength(LL_MaxUint())
    , mStatus(NS_OK)
    , mSuspendCount(0)
    , mLoadFlags(LOAD_NORMAL)
    , mWaiting(PR_FALSE)
    , mCloseWhenDone(PR_FALSE)
{
#if defined(PR_LOGGING)
    if (!gStreamPumpLog)
        gStreamPumpLog = PR_NewLogModule("nsStreamPump");
#endif
}

nsInputStreamPump::~nsInputStreamPump()
{
}

nsresult
nsInputStreamPump::Create(nsInputStreamPump  **result,
                          nsIInputStream      *stream,
                          PRInt64              streamPos,
                          PRInt64              streamLen,
                          PRUint32             segsize,
                          PRUint32             segcount,
                          PRBool               closeWhenDone)
{
    nsresult rv = NS_ERROR_OUT_OF_MEMORY;
    nsRefPtr<nsInputStreamPump> pump = new nsInputStreamPump();
    if (pump) {
        rv = pump->Init(stream, streamPos, streamLen,
                        segsize, segcount, closeWhenDone);
        if (NS_SUCCEEDED(rv)) {
            *result = nsnull;
            pump.swap(*result);
        }
    }
    return rv;
}

struct PeekData {
  PeekData(nsInputStreamPump::PeekSegmentFun fun, void* closure)
    : mFunc(fun), mClosure(closure) {}

  nsInputStreamPump::PeekSegmentFun mFunc;
  void* mClosure;
};

static NS_METHOD
CallPeekFunc(nsIInputStream *aInStream, void *aClosure,
             const char *aFromSegment, PRUint32 aToOffset, PRUint32 aCount,
             PRUint32 *aWriteCount)
{
  NS_ASSERTION(aToOffset == 0, "Called more than once?");
  NS_ASSERTION(aCount > 0, "Called without data?");

  PeekData* data = NS_STATIC_CAST(PeekData*, aClosure);
  data->mFunc(data->mClosure,
              NS_REINTERPRET_CAST(const PRUint8*, aFromSegment), aCount);
  return NS_BINDING_ABORTED;
}

void
nsInputStreamPump::PeekStream(PeekSegmentFun callback, void* closure)
{
  NS_ASSERTION(mAsyncStream, "PeekStream called without stream");
  PeekData data(callback, closure);
  PRUint32 read;
  mAsyncStream->ReadSegments(CallPeekFunc, &data, NET_DEFAULT_SEGMENT_SIZE,
                             &read);
}

nsresult
nsInputStreamPump::EnsureWaiting()
{
    
    

    if (!mWaiting) {
        nsresult rv = mAsyncStream->AsyncWait(this, 0, 0, mTargetThread);
        if (NS_FAILED(rv)) {
            NS_ERROR("AsyncWait failed");
            return rv;
        }
        mWaiting = PR_TRUE;
    }
    return NS_OK;
}








NS_IMPL_THREADSAFE_ISUPPORTS3(nsInputStreamPump,
                              nsIRequest,
                              nsIInputStreamCallback,
                              nsIInputStreamPump)





NS_IMETHODIMP
nsInputStreamPump::GetName(nsACString &result)
{
    result.Truncate();
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::IsPending(PRBool *result)
{
    *result = (mState != STATE_IDLE);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::GetStatus(nsresult *status)
{
    *status = mStatus;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::Cancel(nsresult status)
{
    LOG(("nsInputStreamPump::Cancel [this=%x status=%x]\n",
        this, status));

    if (NS_FAILED(mStatus)) {
        LOG(("  already canceled\n"));
        return NS_OK;
    }

    NS_ASSERTION(NS_FAILED(status), "cancel with non-failure status code");
    mStatus = status;

    
    if (mAsyncStream) {
        mAsyncStream->CloseWithStatus(status);
        if (mSuspendCount == 0)
            EnsureWaiting();
        
        
        
        
    }
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::Suspend()
{
    LOG(("nsInputStreamPump::Suspend [this=%x]\n", this));
    NS_ENSURE_TRUE(mState != STATE_IDLE, NS_ERROR_UNEXPECTED);
    ++mSuspendCount;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::Resume()
{
    LOG(("nsInputStreamPump::Resume [this=%x]\n", this));
    NS_ENSURE_TRUE(mSuspendCount > 0, NS_ERROR_UNEXPECTED);
    NS_ENSURE_TRUE(mState != STATE_IDLE, NS_ERROR_UNEXPECTED);

    if (--mSuspendCount == 0)
        EnsureWaiting();
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::GetLoadFlags(nsLoadFlags *aLoadFlags)
{
    *aLoadFlags = mLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::SetLoadFlags(nsLoadFlags aLoadFlags)
{
    mLoadFlags = aLoadFlags;
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::GetLoadGroup(nsILoadGroup **aLoadGroup)
{
    NS_IF_ADDREF(*aLoadGroup = mLoadGroup);
    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::SetLoadGroup(nsILoadGroup *aLoadGroup)
{
    mLoadGroup = aLoadGroup;
    return NS_OK;
}





NS_IMETHODIMP
nsInputStreamPump::Init(nsIInputStream *stream,
                        PRInt64 streamPos, PRInt64 streamLen,
                        PRUint32 segsize, PRUint32 segcount,
                        PRBool closeWhenDone)
{
    NS_ENSURE_TRUE(mState == STATE_IDLE, NS_ERROR_IN_PROGRESS);

    mStreamOffset = PRUint64(streamPos);
    if (nsInt64(streamLen) >= nsInt64(0))
        mStreamLength = PRUint64(streamLen);
    mStream = stream;
    mSegSize = segsize;
    mSegCount = segcount;
    mCloseWhenDone = closeWhenDone;

    return NS_OK;
}

NS_IMETHODIMP
nsInputStreamPump::AsyncRead(nsIStreamListener *listener, nsISupports *ctxt)
{
    NS_ENSURE_TRUE(mState == STATE_IDLE, NS_ERROR_IN_PROGRESS);
    NS_ENSURE_ARG_POINTER(listener);

    
    
    
    
    
    

    PRBool nonBlocking;
    nsresult rv = mStream->IsNonBlocking(&nonBlocking);
    if (NS_FAILED(rv)) return rv;

    if (nonBlocking) {
        mAsyncStream = do_QueryInterface(mStream);
        
        
        
        
        
        
        if (mAsyncStream && (mStreamOffset != LL_MAXUINT)) {
            nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mStream);
            if (seekable)
                seekable->Seek(nsISeekableStream::NS_SEEK_SET, mStreamOffset);
        }
    }

    if (!mAsyncStream) {
        
        nsCOMPtr<nsIStreamTransportService> sts =
            do_GetService(kStreamTransportServiceCID, &rv);
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsITransport> transport;
        rv = sts->CreateInputTransport(mStream, mStreamOffset, mStreamLength,
                                       mCloseWhenDone, getter_AddRefs(transport));
        if (NS_FAILED(rv)) return rv;

        nsCOMPtr<nsIInputStream> wrapper;
        rv = transport->OpenInputStream(0, mSegSize, mSegCount, getter_AddRefs(wrapper));
        if (NS_FAILED(rv)) return rv;

        mAsyncStream = do_QueryInterface(wrapper, &rv);
        if (NS_FAILED(rv)) return rv;
    }

    
    
    mStream = 0;

    
    
    mStreamOffset = 0;

    
    
    mTargetThread = do_GetCurrentThread();
    NS_ENSURE_STATE(mTargetThread);

    rv = EnsureWaiting();
    if (NS_FAILED(rv)) return rv;

    if (mLoadGroup)
        mLoadGroup->AddRequest(this, nsnull);

    mState = STATE_START;
    mListener = listener;
    mListenerContext = ctxt;
    return NS_OK;
}





NS_IMETHODIMP
nsInputStreamPump::OnInputStreamReady(nsIAsyncInputStream *stream)
{
    LOG(("nsInputStreamPump::OnInputStreamReady [this=%x]\n", this));

    
    

    for (;;) {
        if (mSuspendCount || mState == STATE_IDLE) {
            mWaiting = PR_FALSE;
            break;
        }

        PRUint32 nextState;
        switch (mState) {
        case STATE_START:
            nextState = OnStateStart();
            break;
        case STATE_TRANSFER:
            nextState = OnStateTransfer();
            break;
        case STATE_STOP:
            nextState = OnStateStop();
            break;
        }

        if (mState == nextState && !mSuspendCount) {
            NS_ASSERTION(mState == STATE_TRANSFER, "unexpected state");
            NS_ASSERTION(NS_SUCCEEDED(mStatus), "unexpected status");

            mWaiting = PR_FALSE;
            mStatus = EnsureWaiting();
            if (NS_SUCCEEDED(mStatus))
                break;
            
            nextState = STATE_STOP;
        }

        mState = nextState;
    }
    return NS_OK;
}

PRUint32
nsInputStreamPump::OnStateStart()
{
    LOG(("  OnStateStart [this=%x]\n", this));

    nsresult rv;

    
    
    
    if (NS_SUCCEEDED(mStatus)) {
        PRUint32 avail;
        rv = mAsyncStream->Available(&avail);
        if (NS_FAILED(rv) && rv != NS_BASE_STREAM_CLOSED)
            mStatus = rv;
    }

    rv = mListener->OnStartRequest(this, mListenerContext);

    
    
    if (NS_FAILED(rv) && NS_SUCCEEDED(mStatus))
        mStatus = rv;

    return NS_SUCCEEDED(mStatus) ? STATE_TRANSFER : STATE_STOP;
}

PRUint32
nsInputStreamPump::OnStateTransfer()
{
    LOG(("  OnStateTransfer [this=%x]\n", this));

    
    if (NS_FAILED(mStatus))
        return STATE_STOP;

    nsresult rv;

    PRUint32 avail;
    rv = mAsyncStream->Available(&avail);
    LOG(("  Available returned [stream=%x rv=%x avail=%u]\n", mAsyncStream.get(), rv, avail));

    if (rv == NS_BASE_STREAM_CLOSED) {
        rv = NS_OK;
        avail = 0;
    }
    else if (NS_SUCCEEDED(rv) && avail) {
        
        if (PRUint64(avail) + mStreamOffset > mStreamLength)
            avail = PRUint32(mStreamLength - mStreamOffset);

        if (avail) {
            
            
            
            
            
            
            
            
            
            
            
            

            
            
            PRInt64 offsetBefore;
            nsCOMPtr<nsISeekableStream> seekable = do_QueryInterface(mAsyncStream);
            if (seekable && NS_FAILED(seekable->Tell(&offsetBefore))) {
                NS_NOTREACHED("Tell failed on readable stream");
                offsetBefore = 0;
            }

            
            
            
            
            PRUint32 odaOffset =
                mStreamOffset > PR_UINT32_MAX ?
                PR_UINT32_MAX : PRUint32(mStreamOffset);

            LOG(("  calling OnDataAvailable [offset=%lld(%u) count=%u]\n",
                mStreamOffset, odaOffset, avail));

            rv = mListener->OnDataAvailable(this, mListenerContext, mAsyncStream,
                                            odaOffset, avail);

            
            if (NS_SUCCEEDED(rv) && NS_SUCCEEDED(mStatus)) {
                
                if (seekable) {
                    
                    
                    PRInt64 offsetAfter;
                    if (NS_FAILED(seekable->Tell(&offsetAfter)))
                        offsetAfter = offsetBefore + avail;
                    if (offsetAfter > offsetBefore)
                        mStreamOffset += (offsetAfter - offsetBefore);
                    else if (mSuspendCount == 0) {
                        
                        
                        
                        
                        
                        
                        
                        NS_ERROR("OnDataAvailable implementation consumed no data");
                        mStatus = NS_ERROR_UNEXPECTED;
                    }
                }
                else
                    mStreamOffset += avail; 
            }
        }
    }

    
    

    if (NS_SUCCEEDED(mStatus)) {
        if (NS_FAILED(rv))
            mStatus = rv;
        else if (avail) {
            
            
            
            
            rv = mAsyncStream->Available(&avail);
            if (NS_SUCCEEDED(rv))
                return STATE_TRANSFER;
        }
    }
    return STATE_STOP;
}

PRUint32
nsInputStreamPump::OnStateStop()
{
    LOG(("  OnStateStop [this=%x status=%x]\n", this, mStatus));

    
    
    

    if (NS_FAILED(mStatus))
        mAsyncStream->CloseWithStatus(mStatus);
    else if (mCloseWhenDone)
        mAsyncStream->Close();

    mAsyncStream = 0;
    mTargetThread = 0;
    mIsPending = PR_FALSE;

    mListener->OnStopRequest(this, mListenerContext, mStatus);
    mListener = 0;
    mListenerContext = 0;

    if (mLoadGroup)
        mLoadGroup->RemoveRequest(this, nsnull, mStatus);

    return STATE_IDLE;
}
