










































#include "nsMultiplexInputStream.h"
#include "nsIMultiplexInputStream.h"
#include "nsISeekableStream.h"
#include "nsCOMPtr.h"
#include "nsCOMArray.h"
#include "nsInt64.h"

class nsMultiplexInputStream : public nsIMultiplexInputStream,
                               public nsISeekableStream
{
public:
    nsMultiplexInputStream();

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSIMULTIPLEXINPUTSTREAM
    NS_DECL_NSISEEKABLESTREAM

private:
    ~nsMultiplexInputStream() {}

    struct ReadSegmentsState {
        nsIInputStream* mThisStream;
        PRUint32 mOffset;
        nsWriteSegmentFun mWriter;
        void* mClosure;
        PRBool mDone;
    };

    static NS_METHOD ReadSegCb(nsIInputStream* aIn, void* aClosure,
                               const char* aFromRawSegment, PRUint32 aToOffset,
                               PRUint32 aCount, PRUint32 *aWriteCount);
    
    nsCOMArray<nsIInputStream> mStreams;
    PRUint32 mCurrentStream;
    PRBool mStartedReadingCurrent;
    nsresult mStatus;
};


NS_IMPL_THREADSAFE_ISUPPORTS3(nsMultiplexInputStream,
                              nsIMultiplexInputStream,
                              nsIInputStream,
                              nsISeekableStream)

nsMultiplexInputStream::nsMultiplexInputStream()
    : mCurrentStream(0),
      mStartedReadingCurrent(PR_FALSE),
      mStatus(NS_OK)
{
}


NS_IMETHODIMP
nsMultiplexInputStream::GetCount(PRUint32 *aCount)
{
    *aCount = mStreams.Count();
    return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::AppendStream(nsIInputStream *aStream)
{
    return mStreams.AppendObject(aStream) ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}


NS_IMETHODIMP
nsMultiplexInputStream::InsertStream(nsIInputStream *aStream, PRUint32 aIndex)
{
    PRBool result = mStreams.InsertObjectAt(aStream, aIndex);
    NS_ENSURE_TRUE(result, NS_ERROR_OUT_OF_MEMORY);
    if (mCurrentStream > aIndex ||
        (mCurrentStream == aIndex && mStartedReadingCurrent))
        ++mCurrentStream;
    return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::RemoveStream(PRUint32 aIndex)
{
    PRBool result = mStreams.RemoveObjectAt(aIndex);
    NS_ENSURE_TRUE(result, NS_ERROR_NOT_AVAILABLE);
    if (mCurrentStream > aIndex)
        --mCurrentStream;
    else if (mCurrentStream == aIndex)
        mStartedReadingCurrent = PR_FALSE;

    return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::GetStream(PRUint32 aIndex, nsIInputStream **_retval)
{
    *_retval = mStreams.SafeObjectAt(aIndex);
    NS_ENSURE_TRUE(*_retval, NS_ERROR_NOT_AVAILABLE);

    NS_ADDREF(*_retval);
    return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::Close()
{
    mStatus = NS_BASE_STREAM_CLOSED;

    nsresult rv = NS_OK;

    PRUint32 len = mStreams.Count();
    for (PRUint32 i = 0; i < len; ++i) {
        nsresult rv2 = mStreams[i]->Close();
        
        if (NS_FAILED(rv2))
            rv = rv2;
    }
    return rv;
}


NS_IMETHODIMP
nsMultiplexInputStream::Available(PRUint32 *_retval)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    nsresult rv;
    PRUint32 avail = 0;

    PRUint32 len = mStreams.Count();
    for (PRUint32 i = mCurrentStream; i < len; i++) {
        PRUint32 streamAvail;
        rv = mStreams[i]->Available(&streamAvail);
        NS_ENSURE_SUCCESS(rv, rv);
        avail += streamAvail;
    }
    *_retval = avail;
    return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::Read(char * aBuf, PRUint32 aCount, PRUint32 *_retval)
{
    
    
    
 
    *_retval = 0;

    if (mStatus == NS_BASE_STREAM_CLOSED)
        return NS_OK;
    if (NS_FAILED(mStatus))
        return mStatus;
 
    nsresult rv = NS_OK;

    PRUint32 len = mStreams.Count();
    while (mCurrentStream < len && aCount) {
        PRUint32 read;
        rv = mStreams[mCurrentStream]->Read(aBuf, aCount, &read);

        
        
        if (rv == NS_BASE_STREAM_CLOSED) {
            NS_NOTREACHED("Input stream's Read method returned NS_BASE_STREAM_CLOSED");
            rv = NS_OK;
            read = 0;
        }
        else if (NS_FAILED(rv))
            break;

        if (read == 0) {
            ++mCurrentStream;
            mStartedReadingCurrent = PR_FALSE;
        }
        else {
            NS_ASSERTION(aCount >= read, "Read more than requested");
            *_retval += read;
            aCount -= read;
            aBuf += read;
            mStartedReadingCurrent = PR_TRUE;
        }
    }
    return *_retval ? NS_OK : rv;
}




NS_IMETHODIMP
nsMultiplexInputStream::ReadSegments(nsWriteSegmentFun aWriter, void *aClosure,
                                     PRUint32 aCount, PRUint32 *_retval)
{
    if (mStatus == NS_BASE_STREAM_CLOSED) {
        *_retval = 0;
        return NS_OK;
    }
    if (NS_FAILED(mStatus))
        return mStatus;

    NS_ASSERTION(aWriter, "missing aWriter");

    nsresult rv = NS_OK;
    ReadSegmentsState state;
    state.mThisStream = this;
    state.mOffset = 0;
    state.mWriter = aWriter;
    state.mClosure = aClosure;
    state.mDone = PR_FALSE;
    
    PRUint32 len = mStreams.Count();
    while (mCurrentStream < len && aCount) {
        PRUint32 read;
        rv = mStreams[mCurrentStream]->ReadSegments(ReadSegCb, &state, aCount, &read);

        
        
        if (rv == NS_BASE_STREAM_CLOSED) {
            NS_NOTREACHED("Input stream's Read method returned NS_BASE_STREAM_CLOSED");
            rv = NS_OK;
            read = 0;
        }

        
        if (state.mDone || NS_FAILED(rv))
            break;

        
        if (read == 0) {
            ++mCurrentStream;
            mStartedReadingCurrent = PR_FALSE;
        }
        else {
            NS_ASSERTION(aCount >= read, "Read more than requested");
            state.mOffset += read;
            aCount -= read;
            mStartedReadingCurrent = PR_TRUE;
        }
    }

    
    *_retval = state.mOffset;
    return state.mOffset ? NS_OK : rv;
}

NS_METHOD
nsMultiplexInputStream::ReadSegCb(nsIInputStream* aIn, void* aClosure,
                                  const char* aFromRawSegment,
                                  PRUint32 aToOffset, PRUint32 aCount,
                                  PRUint32 *aWriteCount)
{
    nsresult rv;
    ReadSegmentsState* state = (ReadSegmentsState*)aClosure;
    rv = (state->mWriter)(state->mThisStream,
                          state->mClosure,
                          aFromRawSegment,
                          aToOffset + state->mOffset,
                          aCount,
                          aWriteCount);
    if (NS_FAILED(rv))
        state->mDone = PR_TRUE;
    return rv;
}


NS_IMETHODIMP
nsMultiplexInputStream::IsNonBlocking(PRBool *aNonBlocking)
{
    PRUint32 len = mStreams.Count();
    for (PRUint32 i = 0; i < len; ++i) {
        nsresult rv = mStreams[i]->IsNonBlocking(aNonBlocking);
        NS_ENSURE_SUCCESS(rv, rv);
        
        
        
        if (*aNonBlocking)
            return NS_OK;
    }
    return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    nsresult rv;

    
    if (aWhence == NS_SEEK_SET && aOffset == 0)
    {
        PRUint32 i, last;
        last = mStartedReadingCurrent ? mCurrentStream+1 : mCurrentStream;
        for (i = 0; i < last; ++i) {
            nsCOMPtr<nsISeekableStream> stream = do_QueryInterface(mStreams[i]);
            NS_ENSURE_TRUE(stream, NS_ERROR_NO_INTERFACE);

            rv = stream->Seek(NS_SEEK_SET, 0);
            NS_ENSURE_SUCCESS(rv, rv);
        }
        mCurrentStream = 0;
        mStartedReadingCurrent = PR_FALSE;
        return NS_OK;
    }

    
    return NS_ERROR_NOT_IMPLEMENTED;
}


NS_IMETHODIMP
nsMultiplexInputStream::Tell(PRInt64 *_retval)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    nsresult rv;
    nsInt64 ret64 = 0;
    PRUint32 i, last;
    last = mStartedReadingCurrent ? mCurrentStream+1 : mCurrentStream;
    for (i = 0; i < last; ++i) {
        nsCOMPtr<nsISeekableStream> stream = do_QueryInterface(mStreams[i]);
        NS_ENSURE_TRUE(stream, NS_ERROR_NO_INTERFACE);

        PRInt64 pos;
        rv = stream->Tell(&pos);
        NS_ENSURE_SUCCESS(rv, rv);
        ret64 += pos;
    }
    *_retval =  ret64;

    return NS_OK;
}


NS_IMETHODIMP
nsMultiplexInputStream::SetEOF()
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsMultiplexInputStreamConstructor(nsISupports *outer,
                                  REFNSIID iid,
                                  void **result)
{
    *result = nsnull;

    if (outer)
        return NS_ERROR_NO_AGGREGATION;

    nsMultiplexInputStream *inst;
    NS_NEWXPCOM(inst, nsMultiplexInputStream);
    if (!inst)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(inst);
    nsresult rv = inst->QueryInterface(iid, result);
    NS_RELEASE(inst);

    return rv;
}
