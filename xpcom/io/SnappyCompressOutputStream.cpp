





#include "mozilla/SnappyCompressOutputStream.h"

#include <algorithm>
#include "nsStreamUtils.h"
#include "snappy/snappy.h"

namespace mozilla {

NS_IMPL_ISUPPORTS(SnappyCompressOutputStream, nsIOutputStream);


const size_t
SnappyCompressOutputStream::kMaxBlockSize = snappy::kBlockSize;

SnappyCompressOutputStream::SnappyCompressOutputStream(nsIOutputStream* aBaseStream,
                                                       size_t aBlockSize)
 : mBaseStream(aBaseStream)
 , mBlockSize(std::min(aBlockSize, kMaxBlockSize))
 , mNextByte(0)
 , mCompressedBufferLength(0)
 , mStreamIdentifierWritten(false)
{
  MOZ_ASSERT(mBlockSize > 0);

  
  
  
  
#ifdef DEBUG
  bool baseNonBlocking;
  nsresult rv = mBaseStream->IsNonBlocking(&baseNonBlocking);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  MOZ_ASSERT(!baseNonBlocking);
#endif
}

size_t
SnappyCompressOutputStream::BlockSize() const
{
  return mBlockSize;
}

NS_IMETHODIMP
SnappyCompressOutputStream::Close()
{
  if (!mBaseStream) {
    return NS_OK;
  }

  nsresult rv = Flush();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  mBaseStream->Close();
  mBaseStream = nullptr;

  mBuffer = nullptr;
  mCompressedBuffer = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
SnappyCompressOutputStream::Flush()
{
  if (!mBaseStream) {
    return NS_BASE_STREAM_CLOSED;
  }

  nsresult rv = FlushToBaseStream();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  mBaseStream->Flush();

  return NS_OK;
}

NS_IMETHODIMP
SnappyCompressOutputStream::Write(const char* aBuf, uint32_t aCount,
                                  uint32_t* aResultOut)
{
  return WriteSegments(NS_CopySegmentToBuffer, const_cast<char*>(aBuf), aCount,
                       aResultOut);
}

NS_IMETHODIMP
SnappyCompressOutputStream::WriteFrom(nsIInputStream*, uint32_t, uint32_t*)
{
  return NS_ERROR_NOT_IMPLEMENTED;
}

NS_IMETHODIMP
SnappyCompressOutputStream::WriteSegments(nsReadSegmentFun aReader,
                                          void* aClosure,
                                          uint32_t aCount,
                                          uint32_t* aBytesWrittenOut)
{
  *aBytesWrittenOut = 0;

  if (!mBaseStream) {
    return NS_BASE_STREAM_CLOSED;
  }

  if (!mBuffer) {
    mBuffer.reset(new ((fallible_t())) char[mBlockSize]);
    if (NS_WARN_IF(!mBuffer)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  while (aCount > 0) {
    
    MOZ_ASSERT(mNextByte <= mBlockSize);
    uint32_t remaining = mBlockSize - mNextByte;

    
    if (remaining == 0) {
      nsresult rv = FlushToBaseStream();
      if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

      
      MOZ_ASSERT(!mNextByte);
      remaining = mBlockSize;
    }

    uint32_t numToRead = std::min(remaining, aCount);
    uint32_t numRead = 0;

    nsresult rv = aReader(this, aClosure, &mBuffer[mNextByte],
                          *aBytesWrittenOut, numToRead, &numRead);

    
    if (NS_FAILED(rv)) {
      return NS_OK;
    }

    
    if (numRead == 0) {
      return NS_OK;
    }

    mNextByte += numRead;
    *aBytesWrittenOut += numRead;
    aCount -= numRead;
  }

  return NS_OK;
}

NS_IMETHODIMP
SnappyCompressOutputStream::IsNonBlocking(bool* aNonBlockingOut)
{
  *aNonBlockingOut = false;
  return NS_OK;
}

SnappyCompressOutputStream::~SnappyCompressOutputStream()
{
  Close();
}

nsresult
SnappyCompressOutputStream::FlushToBaseStream()
{
  MOZ_ASSERT(mBaseStream);

  
  
  
  if (!mCompressedBuffer) {
    mCompressedBufferLength = MaxCompressedBufferLength(mBlockSize);
    mCompressedBuffer.reset(new ((fallible_t())) char[mCompressedBufferLength]);
    if (NS_WARN_IF(!mCompressedBuffer)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  nsresult rv = MaybeFlushStreamIdentifier();
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  size_t compressedLength;
  rv = WriteCompressedData(mCompressedBuffer.get(), mCompressedBufferLength,
                           mBuffer.get(), mNextByte, &compressedLength);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  MOZ_ASSERT(compressedLength > 0);

  mNextByte = 0;

  
  uint32_t numWritten = 0;
  rv = WriteAll(mCompressedBuffer.get(), compressedLength, &numWritten);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  MOZ_ASSERT(compressedLength == numWritten);

  return NS_OK;
}

nsresult
SnappyCompressOutputStream::MaybeFlushStreamIdentifier()
{
  MOZ_ASSERT(mCompressedBuffer);

  if (mStreamIdentifierWritten) {
    return NS_OK;
  }

  
  size_t compressedLength;
  nsresult rv = WriteStreamIdentifier(mCompressedBuffer.get(),
                                      mCompressedBufferLength,
                                      &compressedLength);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  
  uint32_t numWritten = 0;
  rv = WriteAll(mCompressedBuffer.get(), compressedLength, &numWritten);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  MOZ_ASSERT(compressedLength == numWritten);

  mStreamIdentifierWritten = true;

  return NS_OK;
}

nsresult
SnappyCompressOutputStream::WriteAll(const char* aBuf, uint32_t aCount,
                                     uint32_t* aBytesWrittenOut)
{
  *aBytesWrittenOut = 0;

  if (!mBaseStream) {
    return NS_BASE_STREAM_CLOSED;
  }

  uint32_t offset = 0;
  while (aCount > 0) {
    uint32_t numWritten = 0;
    nsresult rv = mBaseStream->Write(aBuf + offset, aCount, &numWritten);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    offset += numWritten;
    aCount -= numWritten;
    *aBytesWrittenOut += numWritten;
  }

  return NS_OK;
}

} 
