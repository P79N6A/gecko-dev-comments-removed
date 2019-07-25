















































#include "nsAlgorithm.h"
#include "nsStorageStream.h"
#include "nsSegmentedBuffer.h"
#include "nsStreamUtils.h"
#include "nsCOMPtr.h"
#include "prbit.h"
#include "nsIInputStream.h"
#include "nsISeekableStream.h"
#include "prlog.h"

#if defined(PR_LOGGING)











static PRLogModuleInfo* sLog = PR_NewLogModule("nsStorageStream");
#endif
#define LOG(args) PR_LOG(sLog, PR_LOG_DEBUG, args)

nsStorageStream::nsStorageStream()
    : mSegmentedBuffer(0), mSegmentSize(0), mWriteInProgress(PR_FALSE),
      mLastSegmentNum(-1), mWriteCursor(0), mSegmentEnd(0), mLogicalLength(0)
{
    LOG(("Creating nsStorageStream [%p].\n", this));
}

nsStorageStream::~nsStorageStream()
{
    delete mSegmentedBuffer;
}

NS_IMPL_THREADSAFE_ISUPPORTS2(nsStorageStream,
                              nsIStorageStream,
                              nsIOutputStream)

NS_IMETHODIMP
nsStorageStream::Init(PRUint32 segmentSize, PRUint32 maxSize,
                      nsIMemory *segmentAllocator)
{
    mSegmentedBuffer = new nsSegmentedBuffer();
    if (!mSegmentedBuffer)
        return NS_ERROR_OUT_OF_MEMORY;
    
    mSegmentSize = segmentSize;
    mSegmentSizeLog2 = PR_FloorLog2(segmentSize);

    
    if (mSegmentSize != ((PRUint32)1 << mSegmentSizeLog2))
        return NS_ERROR_INVALID_ARG;

    return mSegmentedBuffer->Init(segmentSize, maxSize, segmentAllocator);
}

NS_IMETHODIMP
nsStorageStream::GetOutputStream(PRInt32 aStartingOffset, 
                                 nsIOutputStream * *aOutputStream)
{
    NS_ENSURE_ARG(aOutputStream);
    NS_ENSURE_TRUE(mSegmentedBuffer, NS_ERROR_NOT_INITIALIZED);
    
    if (mWriteInProgress)
        return NS_ERROR_NOT_AVAILABLE;

    nsresult rv = Seek(aStartingOffset);
    if (NS_FAILED(rv)) return rv;

    
    
    
    if (mLastSegmentNum >= 0)
        mSegmentedBuffer->ReallocLastSegment(mSegmentSize);

    
    rv = Seek(aStartingOffset);
    if (NS_FAILED(rv)) return rv;

    NS_ADDREF(this);
    *aOutputStream = static_cast<nsIOutputStream*>(this);
    mWriteInProgress = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::Close()
{
    NS_ENSURE_TRUE(mSegmentedBuffer, NS_ERROR_NOT_INITIALIZED);
    
    mWriteInProgress = PR_FALSE;
    
    PRInt32 segmentOffset = SegOffset(mLogicalLength);

    
    
    if (segmentOffset)
        mSegmentedBuffer->ReallocLastSegment(segmentOffset);
    
    mWriteCursor = 0;
    mSegmentEnd = 0;

    LOG(("nsStorageStream [%p] Close mWriteCursor=%x mSegmentEnd=%x\n",
        this, mWriteCursor, mSegmentEnd));

    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::Flush()
{
    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::Write(const char *aBuffer, PRUint32 aCount, PRUint32 *aNumWritten)
{
    NS_ENSURE_TRUE(mSegmentedBuffer, NS_ERROR_NOT_INITIALIZED);
    
    const char* readCursor;
    PRUint32 count, availableInSegment, remaining;
    nsresult rv = NS_OK;

    NS_ENSURE_ARG_POINTER(aNumWritten);
    NS_ENSURE_ARG(aBuffer);

    LOG(("nsStorageStream [%p] Write mWriteCursor=%x mSegmentEnd=%x aCount=%d\n",
        this, mWriteCursor, mSegmentEnd, aCount));

    remaining = aCount;
    readCursor = aBuffer;
    
    
    
    
    
    
    PRBool firstTime = mSegmentedBuffer->GetSegmentCount() == 0;
    while (remaining || NS_UNLIKELY(firstTime)) {
        firstTime = PR_FALSE;
        availableInSegment = mSegmentEnd - mWriteCursor;
        if (!availableInSegment) {
            mWriteCursor = mSegmentedBuffer->AppendNewSegment();
            if (!mWriteCursor) {
                mSegmentEnd = 0;
                rv = NS_ERROR_OUT_OF_MEMORY;
                goto out;
            }
            mLastSegmentNum++;
            mSegmentEnd = mWriteCursor + mSegmentSize;
            availableInSegment = mSegmentEnd - mWriteCursor;
            LOG(("nsStorageStream [%p] Write (new seg) mWriteCursor=%x mSegmentEnd=%x\n",
                this, mWriteCursor, mSegmentEnd));
        }
	
        count = NS_MIN(availableInSegment, remaining);
        memcpy(mWriteCursor, readCursor, count);
        remaining -= count;
        readCursor += count;
        mWriteCursor += count;
        LOG(("nsStorageStream [%p] Writing mWriteCursor=%x mSegmentEnd=%x count=%d\n",
            this, mWriteCursor, mSegmentEnd, count));
    };

 out:
    *aNumWritten = aCount - remaining;
    mLogicalLength += *aNumWritten;

    LOG(("nsStorageStream [%p] Wrote mWriteCursor=%x mSegmentEnd=%x numWritten=%d\n",
        this, mWriteCursor, mSegmentEnd, *aNumWritten));
    return rv;
}

NS_IMETHODIMP 
nsStorageStream::WriteFrom(nsIInputStream *inStr, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsStorageStream::WriteSegments(nsReadSegmentFun reader, void * closure, PRUint32 count, PRUint32 *_retval)
{
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP 
nsStorageStream::IsNonBlocking(PRBool *aNonBlocking)
{
    *aNonBlocking = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::GetLength(PRUint32 *aLength)
{
    NS_ENSURE_ARG(aLength);
    *aLength = mLogicalLength;
    return NS_OK;
}


NS_IMETHODIMP
nsStorageStream::SetLength(PRUint32 aLength)
{
    NS_ENSURE_TRUE(mSegmentedBuffer, NS_ERROR_NOT_INITIALIZED);
    
    if (mWriteInProgress)
        return NS_ERROR_NOT_AVAILABLE;

    if (aLength > mLogicalLength)
        return NS_ERROR_INVALID_ARG;

    PRInt32 newLastSegmentNum = SegNum(aLength);
    PRInt32 segmentOffset = SegOffset(aLength);
    if (segmentOffset == 0)
        newLastSegmentNum--;

    while (newLastSegmentNum < mLastSegmentNum) {
        mSegmentedBuffer->DeleteLastSegment();
        mLastSegmentNum--;
    }

    mLogicalLength = aLength;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::GetWriteInProgress(PRBool *aWriteInProgress)
{
    NS_ENSURE_ARG(aWriteInProgress);

    *aWriteInProgress = mWriteInProgress;
    return NS_OK;
}

NS_METHOD
nsStorageStream::Seek(PRInt32 aPosition)
{
    NS_ENSURE_TRUE(mSegmentedBuffer, NS_ERROR_NOT_INITIALIZED);
    
    
    if (aPosition == -1)
        aPosition = mLogicalLength;

    
    if ((PRUint32)aPosition > mLogicalLength)
        return NS_ERROR_INVALID_ARG;

    
    SetLength(aPosition);

    
    if (aPosition == 0) {
        mWriteCursor = 0;
        mSegmentEnd = 0;
        LOG(("nsStorageStream [%p] Seek mWriteCursor=%x mSegmentEnd=%x\n",
            this, mWriteCursor, mSegmentEnd));
        return NS_OK;
    }

    
    mWriteCursor = mSegmentedBuffer->GetSegment(mLastSegmentNum);
    NS_ASSERTION(mWriteCursor, "null mWriteCursor");
    mSegmentEnd = mWriteCursor + mSegmentSize;

    
    
    
    PRInt32 segmentOffset = SegOffset(aPosition);
    if (segmentOffset == 0 && (SegNum(aPosition) > (PRUint32) mLastSegmentNum))
        mWriteCursor = mSegmentEnd;
    else
        mWriteCursor += segmentOffset;
    
    LOG(("nsStorageStream [%p] Seek mWriteCursor=%x mSegmentEnd=%x\n",
        this, mWriteCursor, mSegmentEnd));
    return NS_OK;
}




class nsStorageInputStream : public nsIInputStream
                           , public nsISeekableStream
{
public:
    nsStorageInputStream(nsStorageStream *aStorageStream, PRUint32 aSegmentSize)
        : mStorageStream(aStorageStream), mReadCursor(0),
          mSegmentEnd(0), mSegmentNum(0),
          mSegmentSize(aSegmentSize), mLogicalCursor(0),
          mStatus(NS_OK)
	{
        NS_ADDREF(mStorageStream);
	}

    NS_DECL_ISUPPORTS
    NS_DECL_NSIINPUTSTREAM
    NS_DECL_NSISEEKABLESTREAM

private:
    ~nsStorageInputStream()
    {
        NS_IF_RELEASE(mStorageStream);
    }

protected:
    NS_METHOD Seek(PRUint32 aPosition);

    friend class nsStorageStream;

private:
    nsStorageStream* mStorageStream;
    const char*      mReadCursor;    
    const char*      mSegmentEnd;    
    PRUint32         mSegmentNum;    
    PRUint32         mSegmentSize;   
    PRUint32         mLogicalCursor; 
    nsresult         mStatus;

    PRUint32 SegNum(PRUint32 aPosition)    {return aPosition >> mStorageStream->mSegmentSizeLog2;}
    PRUint32 SegOffset(PRUint32 aPosition) {return aPosition & (mSegmentSize - 1);}
};

NS_IMPL_THREADSAFE_ISUPPORTS2(nsStorageInputStream,
                              nsIInputStream,
                              nsISeekableStream)

NS_IMETHODIMP
nsStorageStream::NewInputStream(PRInt32 aStartingOffset, nsIInputStream* *aInputStream)
{
    NS_ENSURE_TRUE(mSegmentedBuffer, NS_ERROR_NOT_INITIALIZED);

    nsStorageInputStream *inputStream = new nsStorageInputStream(this, mSegmentSize);
    if (!inputStream)
        return NS_ERROR_OUT_OF_MEMORY;

    NS_ADDREF(inputStream);

    nsresult rv = inputStream->Seek(aStartingOffset);
    if (NS_FAILED(rv)) {
        NS_RELEASE(inputStream);
        return rv;
    }

    *aInputStream = inputStream;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Close()
{
    mStatus = NS_BASE_STREAM_CLOSED;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Available(PRUint32 *aAvailable)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    *aAvailable = mStorageStream->mLogicalLength - mLogicalCursor;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Read(char* aBuffer, PRUint32 aCount, PRUint32 *aNumRead)
{
    return ReadSegments(NS_CopySegmentToBuffer, aBuffer, aCount, aNumRead);
}

NS_IMETHODIMP 
nsStorageInputStream::ReadSegments(nsWriteSegmentFun writer, void * closure, PRUint32 aCount, PRUint32 *aNumRead)
{
    *aNumRead = 0;
    if (mStatus == NS_BASE_STREAM_CLOSED)
        return NS_OK;
    if (NS_FAILED(mStatus))
        return mStatus;

    PRUint32 count, availableInSegment, remainingCapacity, bytesConsumed;
    nsresult rv;

    remainingCapacity = aCount;
    while (remainingCapacity) {
        availableInSegment = mSegmentEnd - mReadCursor;
        if (!availableInSegment) {
            PRUint32 available = mStorageStream->mLogicalLength - mLogicalCursor;
            if (!available)
                goto out;

            mReadCursor = mStorageStream->mSegmentedBuffer->GetSegment(++mSegmentNum);
            mSegmentEnd = mReadCursor + NS_MIN(mSegmentSize, available);
            availableInSegment = mSegmentEnd - mReadCursor;
        }
	
        count = NS_MIN(availableInSegment, remainingCapacity);
        rv = writer(this, closure, mReadCursor, aCount - remainingCapacity,
                    count, &bytesConsumed);
        if (NS_FAILED(rv) || (bytesConsumed == 0))
          break;
        remainingCapacity -= bytesConsumed;
        mReadCursor += bytesConsumed;
        mLogicalCursor += bytesConsumed;
    };

 out:
    *aNumRead = aCount - remainingCapacity;

    PRBool isWriteInProgress = PR_FALSE;
    if (NS_FAILED(mStorageStream->GetWriteInProgress(&isWriteInProgress)))
        isWriteInProgress = PR_FALSE;

    if (*aNumRead == 0 && isWriteInProgress)
        return NS_BASE_STREAM_WOULD_BLOCK;

    return NS_OK;
}

NS_IMETHODIMP 
nsStorageInputStream::IsNonBlocking(PRBool *aNonBlocking)
{
    
    
 
    *aNonBlocking = PR_TRUE;
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Seek(PRInt32 aWhence, PRInt64 aOffset)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    PRInt64 pos = aOffset;

    switch (aWhence) {
    case NS_SEEK_SET:
        break;
    case NS_SEEK_CUR:
        pos += mLogicalCursor;
        break;
    case NS_SEEK_END:
        pos += mStorageStream->mLogicalLength;
        break;
    default:
        NS_NOTREACHED("unexpected whence value");
        return NS_ERROR_UNEXPECTED;
    }
    if (pos == PRInt64(mLogicalCursor))
        return NS_OK;

    return Seek(pos);
}

NS_IMETHODIMP
nsStorageInputStream::Tell(PRInt64 *aResult)
{
    if (NS_FAILED(mStatus))
        return mStatus;

    LL_UI2L(*aResult, mLogicalCursor);
    return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::SetEOF()
{
    NS_NOTREACHED("nsStorageInputStream::SetEOF");
    return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsStorageInputStream::Seek(PRUint32 aPosition)
{
    PRUint32 length = mStorageStream->mLogicalLength;
    if (aPosition > length)
        return NS_ERROR_INVALID_ARG;

    if (length == 0)
        return NS_OK;

    mSegmentNum = SegNum(aPosition);
    PRUint32 segmentOffset = SegOffset(aPosition);
    mReadCursor = mStorageStream->mSegmentedBuffer->GetSegment(mSegmentNum) +
        segmentOffset;
    PRUint32 available = length - aPosition;
    mSegmentEnd = mReadCursor + NS_MIN(mSegmentSize - segmentOffset, available);
    mLogicalCursor = aPosition;
    return NS_OK;
}

NS_COM nsresult
NS_NewStorageStream(PRUint32 segmentSize, PRUint32 maxSize, nsIStorageStream **result)
{
    NS_ENSURE_ARG(result);

    nsStorageStream* storageStream = new nsStorageStream();
    if (!storageStream) return NS_ERROR_OUT_OF_MEMORY;
    
    NS_ADDREF(storageStream);
    nsresult rv = storageStream->Init(segmentSize, maxSize, nsnull);
    if (NS_FAILED(rv)) {
        NS_RELEASE(storageStream);
        return rv;
    }
    *result = storageStream;
    return NS_OK;
}
