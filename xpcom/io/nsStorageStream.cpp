













#include "nsAlgorithm.h"
#include "nsStorageStream.h"
#include "nsSegmentedBuffer.h"
#include "nsStreamUtils.h"
#include "nsCOMPtr.h"
#include "nsICloneableInputStream.h"
#include "nsIInputStream.h"
#include "nsIIPCSerializableInputStream.h"
#include "nsISeekableStream.h"
#include "mozilla/Logging.h"
#include "mozilla/Attributes.h"
#include "mozilla/Likely.h"
#include "mozilla/MathAlgorithms.h"
#include "mozilla/ipc/InputStreamUtils.h"

using mozilla::ipc::InputStreamParams;
using mozilla::ipc::StringInputStreamParams;












static PRLogModuleInfo*
GetStorageStreamLog()
{
  static PRLogModuleInfo* sLog;
  if (!sLog) {
    sLog = PR_NewLogModule("nsStorageStream");
  }
  return sLog;
}
#ifdef LOG
#undef LOG
#endif
#define LOG(args) MOZ_LOG(GetStorageStreamLog(), mozilla::LogLevel::Debug, args)

nsStorageStream::nsStorageStream()
  : mSegmentedBuffer(0), mSegmentSize(0), mWriteInProgress(false),
    mLastSegmentNum(-1), mWriteCursor(0), mSegmentEnd(0), mLogicalLength(0)
{
  LOG(("Creating nsStorageStream [%p].\n", this));
}

nsStorageStream::~nsStorageStream()
{
  delete mSegmentedBuffer;
}

NS_IMPL_ISUPPORTS(nsStorageStream,
                  nsIStorageStream,
                  nsIOutputStream)

NS_IMETHODIMP
nsStorageStream::Init(uint32_t aSegmentSize, uint32_t aMaxSize)
{
  mSegmentedBuffer = new nsSegmentedBuffer();
  mSegmentSize = aSegmentSize;
  mSegmentSizeLog2 = mozilla::FloorLog2(aSegmentSize);

  
  if (mSegmentSize != ((uint32_t)1 << mSegmentSizeLog2)) {
    return NS_ERROR_INVALID_ARG;
  }

  return mSegmentedBuffer->Init(aSegmentSize, aMaxSize);
}

NS_IMETHODIMP
nsStorageStream::GetOutputStream(int32_t aStartingOffset,
                                 nsIOutputStream** aOutputStream)
{
  if (NS_WARN_IF(!aOutputStream)) {
    return NS_ERROR_INVALID_ARG;
  }
  if (NS_WARN_IF(!mSegmentedBuffer)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (mWriteInProgress) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  nsresult rv = Seek(aStartingOffset);
  if (NS_FAILED(rv)) {
    return rv;
  }

  
  
  
  if (mLastSegmentNum >= 0)
    if (mSegmentedBuffer->ReallocLastSegment(mSegmentSize)) {
      
      rv = Seek(aStartingOffset);
      if (NS_FAILED(rv)) {
        return rv;
      }
    }

  NS_ADDREF(this);
  *aOutputStream = static_cast<nsIOutputStream*>(this);
  mWriteInProgress = true;
  return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::Close()
{
  if (NS_WARN_IF(!mSegmentedBuffer)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  mWriteInProgress = false;

  int32_t segmentOffset = SegOffset(mLogicalLength);

  
  
  if (segmentOffset) {
    mSegmentedBuffer->ReallocLastSegment(segmentOffset);
  }

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
nsStorageStream::Write(const char* aBuffer, uint32_t aCount,
                       uint32_t* aNumWritten)
{
  if (NS_WARN_IF(!aNumWritten) || NS_WARN_IF(!aBuffer)) {
    return NS_ERROR_INVALID_ARG;
  }
  if (NS_WARN_IF(!mSegmentedBuffer)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  const char* readCursor;
  uint32_t count, availableInSegment, remaining;
  nsresult rv = NS_OK;

  LOG(("nsStorageStream [%p] Write mWriteCursor=%x mSegmentEnd=%x aCount=%d\n",
       this, mWriteCursor, mSegmentEnd, aCount));

  remaining = aCount;
  readCursor = aBuffer;
  
  
  
  
  
  
  bool firstTime = mSegmentedBuffer->GetSegmentCount() == 0;
  while (remaining || MOZ_UNLIKELY(firstTime)) {
    firstTime = false;
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

    count = XPCOM_MIN(availableInSegment, remaining);
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
nsStorageStream::WriteFrom(nsIInputStream* aInStr, uint32_t aCount,
                           uint32_t* aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStorageStream::WriteSegments(nsReadSegmentFun aReader, void* aClosure,
                               uint32_t aCount, uint32_t* aResult)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
nsStorageStream::IsNonBlocking(bool* aNonBlocking)
{
  *aNonBlocking = false;
  return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::GetLength(uint32_t* aLength)
{
  *aLength = mLogicalLength;
  return NS_OK;
}


NS_IMETHODIMP
nsStorageStream::SetLength(uint32_t aLength)
{
  if (NS_WARN_IF(!mSegmentedBuffer)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  if (mWriteInProgress) {
    return NS_ERROR_NOT_AVAILABLE;
  }

  if (aLength > mLogicalLength) {
    return NS_ERROR_INVALID_ARG;
  }

  int32_t newLastSegmentNum = SegNum(aLength);
  int32_t segmentOffset = SegOffset(aLength);
  if (segmentOffset == 0) {
    newLastSegmentNum--;
  }

  while (newLastSegmentNum < mLastSegmentNum) {
    mSegmentedBuffer->DeleteLastSegment();
    mLastSegmentNum--;
  }

  mLogicalLength = aLength;
  return NS_OK;
}

NS_IMETHODIMP
nsStorageStream::GetWriteInProgress(bool* aWriteInProgress)
{
  *aWriteInProgress = mWriteInProgress;
  return NS_OK;
}

NS_METHOD
nsStorageStream::Seek(int32_t aPosition)
{
  if (NS_WARN_IF(!mSegmentedBuffer)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  
  if (aPosition == -1) {
    aPosition = mLogicalLength;
  }

  
  if ((uint32_t)aPosition > mLogicalLength) {
    return NS_ERROR_INVALID_ARG;
  }

  
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

  
  
  
  int32_t segmentOffset = SegOffset(aPosition);
  if (segmentOffset == 0 && (SegNum(aPosition) > (uint32_t) mLastSegmentNum)) {
    mWriteCursor = mSegmentEnd;
  } else {
    mWriteCursor += segmentOffset;
  }

  LOG(("nsStorageStream [%p] Seek mWriteCursor=%x mSegmentEnd=%x\n",
       this, mWriteCursor, mSegmentEnd));
  return NS_OK;
}




class nsStorageInputStream final
  : public nsIInputStream
  , public nsISeekableStream
  , public nsIIPCSerializableInputStream
  , public nsICloneableInputStream
{
public:
  nsStorageInputStream(nsStorageStream* aStorageStream, uint32_t aSegmentSize)
    : mStorageStream(aStorageStream), mReadCursor(0),
      mSegmentEnd(0), mSegmentNum(0),
      mSegmentSize(aSegmentSize), mLogicalCursor(0),
      mStatus(NS_OK)
  {
  }

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIINPUTSTREAM
  NS_DECL_NSISEEKABLESTREAM
  NS_DECL_NSIIPCSERIALIZABLEINPUTSTREAM
  NS_DECL_NSICLONEABLEINPUTSTREAM

private:
  ~nsStorageInputStream()
  {
  }

protected:
  NS_METHOD Seek(uint32_t aPosition);

  friend class nsStorageStream;

private:
  nsRefPtr<nsStorageStream> mStorageStream;
  uint32_t         mReadCursor;    
  uint32_t         mSegmentEnd;    
  uint32_t         mSegmentNum;    
  uint32_t         mSegmentSize;   
  uint32_t         mLogicalCursor; 
  nsresult         mStatus;

  uint32_t SegNum(uint32_t aPosition)
  {
    return aPosition >> mStorageStream->mSegmentSizeLog2;
  }
  uint32_t SegOffset(uint32_t aPosition)
  {
    return aPosition & (mSegmentSize - 1);
  }
};

NS_IMPL_ISUPPORTS(nsStorageInputStream,
                  nsIInputStream,
                  nsISeekableStream,
                  nsIIPCSerializableInputStream,
                  nsICloneableInputStream)

NS_IMETHODIMP
nsStorageStream::NewInputStream(int32_t aStartingOffset,
                                nsIInputStream** aInputStream)
{
  if (NS_WARN_IF(!mSegmentedBuffer)) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  nsStorageInputStream* inputStream =
    new nsStorageInputStream(this, mSegmentSize);
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
nsStorageInputStream::Available(uint64_t* aAvailable)
{
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  *aAvailable = mStorageStream->mLogicalLength - mLogicalCursor;
  return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Read(char* aBuffer, uint32_t aCount, uint32_t* aNumRead)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuffer, aCount, aNumRead);
}

NS_IMETHODIMP
nsStorageInputStream::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                                   uint32_t aCount, uint32_t* aNumRead)
{
  *aNumRead = 0;
  if (mStatus == NS_BASE_STREAM_CLOSED) {
    return NS_OK;
  }
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  uint32_t count, availableInSegment, remainingCapacity, bytesConsumed;
  nsresult rv;

  remainingCapacity = aCount;
  while (remainingCapacity) {
    availableInSegment = mSegmentEnd - mReadCursor;
    if (!availableInSegment) {
      uint32_t available = mStorageStream->mLogicalLength - mLogicalCursor;
      if (!available) {
        goto out;
      }

      mSegmentNum++;
      mReadCursor = 0;
      mSegmentEnd = XPCOM_MIN(mSegmentSize, available);
      availableInSegment = mSegmentEnd;
    }
    const char* cur = mStorageStream->mSegmentedBuffer->GetSegment(mSegmentNum);

    count = XPCOM_MIN(availableInSegment, remainingCapacity);
    rv = aWriter(this, aClosure, cur + mReadCursor, aCount - remainingCapacity,
                 count, &bytesConsumed);
    if (NS_FAILED(rv) || (bytesConsumed == 0)) {
      break;
    }
    remainingCapacity -= bytesConsumed;
    mReadCursor += bytesConsumed;
    mLogicalCursor += bytesConsumed;
  };

out:
  *aNumRead = aCount - remainingCapacity;

  bool isWriteInProgress = false;
  if (NS_FAILED(mStorageStream->GetWriteInProgress(&isWriteInProgress))) {
    isWriteInProgress = false;
  }

  if (*aNumRead == 0 && isWriteInProgress) {
    return NS_BASE_STREAM_WOULD_BLOCK;
  }

  return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::IsNonBlocking(bool* aNonBlocking)
{
  
  

  *aNonBlocking = true;
  return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Seek(int32_t aWhence, int64_t aOffset)
{
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  int64_t pos = aOffset;

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
  if (pos == int64_t(mLogicalCursor)) {
    return NS_OK;
  }

  return Seek(pos);
}

NS_IMETHODIMP
nsStorageInputStream::Tell(int64_t* aResult)
{
  if (NS_FAILED(mStatus)) {
    return mStatus;
  }

  *aResult = mLogicalCursor;
  return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::SetEOF()
{
  NS_NOTREACHED("nsStorageInputStream::SetEOF");
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_METHOD
nsStorageInputStream::Seek(uint32_t aPosition)
{
  uint32_t length = mStorageStream->mLogicalLength;
  if (aPosition > length) {
    return NS_ERROR_INVALID_ARG;
  }

  if (length == 0) {
    return NS_OK;
  }

  mSegmentNum = SegNum(aPosition);
  mReadCursor = SegOffset(aPosition);
  uint32_t available = length - aPosition;
  mSegmentEnd = mReadCursor + XPCOM_MIN(mSegmentSize - mReadCursor, available);
  mLogicalCursor = aPosition;
  return NS_OK;
}

void
nsStorageInputStream::Serialize(InputStreamParams& aParams, FileDescriptorArray&)
{
  nsCString combined;
  int64_t offset;
  mozilla::DebugOnly<nsresult> rv = Tell(&offset);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  uint64_t remaining;
  rv = Available(&remaining);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  combined.SetCapacity(remaining);
  uint32_t numRead = 0;

  rv = Read(combined.BeginWriting(), remaining, &numRead);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  MOZ_ASSERT(numRead == remaining);
  combined.SetLength(numRead);

  rv = Seek(NS_SEEK_SET, offset);
  MOZ_ASSERT(NS_SUCCEEDED(rv));

  StringInputStreamParams params;
  params.data() = combined;
  aParams = params;
}

bool
nsStorageInputStream::Deserialize(const InputStreamParams& aParams,
                                  const FileDescriptorArray&)
{
  NS_NOTREACHED("We should never attempt to deserialize a storage input stream.");
  return false;
}

NS_IMETHODIMP
nsStorageInputStream::GetCloneable(bool* aCloneableOut)
{
  *aCloneableOut = true;
  return NS_OK;
}

NS_IMETHODIMP
nsStorageInputStream::Clone(nsIInputStream** aCloneOut)
{
  return mStorageStream->NewInputStream(mLogicalCursor, aCloneOut);
}

nsresult
NS_NewStorageStream(uint32_t aSegmentSize, uint32_t aMaxSize,
                    nsIStorageStream** aResult)
{
  nsStorageStream* storageStream = new nsStorageStream();
  NS_ADDREF(storageStream);
  nsresult rv = storageStream->Init(aSegmentSize, aMaxSize);
  if (NS_FAILED(rv)) {
    NS_RELEASE(storageStream);
    return rv;
  }
  *aResult = storageStream;
  return NS_OK;
}



#undef LOG
