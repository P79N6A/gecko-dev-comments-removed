



































#include "nsIOService.h"
#include "nsSyncStreamListener.h"
#include "nsIPipe.h"

nsresult
nsSyncStreamListener::Init()
{
    return NS_NewPipe(getter_AddRefs(mPipeIn),
                      getter_AddRefs(mPipeOut),
                      nsIOService::gDefaultSegmentSize,
                      PR_UINT32_MAX, 
                      PR_FALSE,
                      PR_FALSE);
}

nsresult
nsSyncStreamListener::WaitForData()
{
    mKeepWaiting = PR_TRUE;

    while (mKeepWaiting)
        NS_ENSURE_STATE(NS_ProcessNextEvent(NS_GetCurrentThread()));

    return NS_OK;
}





NS_IMPL_ISUPPORTS4(nsSyncStreamListener,
                   nsIStreamListener,
                   nsIRequestObserver,
                   nsIInputStream,
                   nsISyncStreamListener)





NS_IMETHODIMP
nsSyncStreamListener::GetInputStream(nsIInputStream **result)
{
    NS_ADDREF(*result = this);
    return NS_OK;
}





NS_IMETHODIMP
nsSyncStreamListener::OnStartRequest(nsIRequest  *request,
                                     nsISupports *context)
{
    return NS_OK;
}

NS_IMETHODIMP
nsSyncStreamListener::OnDataAvailable(nsIRequest     *request,
                                      nsISupports    *context,
                                      nsIInputStream *stream,
                                      PRUint32        offset,
                                      PRUint32        count)
{
    PRUint32 bytesWritten;

    nsresult rv = mPipeOut->WriteFrom(stream, count, &bytesWritten);

    
    
    
    
    if (NS_FAILED(rv))
        return rv;

    
    
    NS_ASSERTION(bytesWritten == count, "did not write all data"); 

    mKeepWaiting = PR_FALSE; 
    return NS_OK;
}

NS_IMETHODIMP
nsSyncStreamListener::OnStopRequest(nsIRequest  *request,
                                    nsISupports *context,
                                    nsresult     status)
{
    mStatus = status;
    mKeepWaiting = PR_FALSE; 
    mDone = PR_TRUE;
    return NS_OK;
}





NS_IMETHODIMP
nsSyncStreamListener::Close()
{
    mStatus = NS_BASE_STREAM_CLOSED;
    mDone = PR_TRUE;

    
    
    
    if (mPipeIn) {
        mPipeIn->Close();
        mPipeIn = nsnull;
    }
    return NS_OK;
}

NS_IMETHODIMP
nsSyncStreamListener::Available(PRUint32 *result)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    mStatus = mPipeIn->Available(result);
    if (NS_SUCCEEDED(mStatus) && (*result == 0) && !mDone) {
        mStatus = WaitForData();
        if (NS_SUCCEEDED(mStatus))
            mStatus = mPipeIn->Available(result);
    }
    return mStatus;
}

NS_IMETHODIMP
nsSyncStreamListener::Read(char     *buf,
                           PRUint32  bufLen,
                           PRUint32 *result)
{
    if (mStatus == NS_BASE_STREAM_CLOSED) {
        *result = 0;
        return NS_OK;
    }

    PRUint32 avail;
    if (NS_FAILED(Available(&avail)))
        return mStatus;

    avail = NS_MIN(avail, bufLen);
    mStatus = mPipeIn->Read(buf, avail, result);
    return mStatus;
}

NS_IMETHODIMP
nsSyncStreamListener::ReadSegments(nsWriteSegmentFun  writer,
                                   void              *closure,
                                   PRUint32           count,
                                   PRUint32          *result)
{
    if (mStatus == NS_BASE_STREAM_CLOSED) {
        *result = 0;
        return NS_OK;
    }

    PRUint32 avail;
    if (NS_FAILED(Available(&avail)))
        return mStatus;

    avail = NS_MIN(avail, count);
    mStatus = mPipeIn->ReadSegments(writer, closure, avail, result);
    return mStatus;
}

NS_IMETHODIMP
nsSyncStreamListener::IsNonBlocking(PRBool *result)
{
    *result = PR_FALSE;
    return NS_OK;
}
