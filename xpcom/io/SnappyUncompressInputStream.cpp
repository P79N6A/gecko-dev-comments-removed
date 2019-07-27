





#include "mozilla/SnappyUncompressInputStream.h"

#include "nsIAsyncInputStream.h"
#include "nsStreamUtils.h"
#include "snappy/snappy.h"

namespace mozilla {

NS_IMPL_ISUPPORTS(SnappyUncompressInputStream,
                  nsIInputStream);

static size_t kCompressedBufferLength =
  detail::SnappyFrameUtils::MaxCompressedBufferLength(snappy::kBlockSize);

SnappyUncompressInputStream::SnappyUncompressInputStream(nsIInputStream* aBaseStream)
  : mBaseStream(aBaseStream)
  , mUncompressedBytes(0)
  , mNextByte(0)
  , mNextChunkType(Unknown)
  , mNextChunkDataLength(0)
  , mNeedFirstStreamIdentifier(true)
{
  
  
  
  
  
  
#ifdef DEBUG
  bool baseNonBlocking;
  nsresult rv = mBaseStream->IsNonBlocking(&baseNonBlocking);
  MOZ_ASSERT(NS_SUCCEEDED(rv));
  if (baseNonBlocking) {
    nsCOMPtr<nsIAsyncInputStream> async = do_QueryInterface(mBaseStream);
    MOZ_ASSERT(!async);
  }
#endif
}

NS_IMETHODIMP
SnappyUncompressInputStream::Close()
{
  if (!mBaseStream) {
    return NS_OK;
  }

  mBaseStream->Close();
  mBaseStream = nullptr;

  mUncompressedBuffer = nullptr;
  mCompressedBuffer = nullptr;

  return NS_OK;
}

NS_IMETHODIMP
SnappyUncompressInputStream::Available(uint64_t* aLengthOut)
{
  if (!mBaseStream) {
    return NS_BASE_STREAM_CLOSED;
  }

  
  *aLengthOut = UncompressedLength();
  if (*aLengthOut > 0) {
    return NS_OK;
  }

  
  
  
  uint32_t bytesRead;
  do {
    nsresult rv = ParseNextChunk(&bytesRead);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    *aLengthOut = UncompressedLength();
  } while(*aLengthOut == 0 && bytesRead);

  return NS_OK;
}

NS_IMETHODIMP
SnappyUncompressInputStream::Read(char* aBuf, uint32_t aCount,
                                  uint32_t* aBytesReadOut)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, aBytesReadOut);
}

NS_IMETHODIMP
SnappyUncompressInputStream::ReadSegments(nsWriteSegmentFun aWriter,
                                          void* aClosure, uint32_t aCount,
                                          uint32_t* aBytesReadOut)
{
  *aBytesReadOut = 0;

  if (!mBaseStream) {
    return NS_BASE_STREAM_CLOSED;
  }

  nsresult rv;

  
  
  
  

  while (aCount > 0) {
    
    
    if (mUncompressedBytes > 0) {
      MOZ_ASSERT(mUncompressedBuffer);
      uint32_t remaining = UncompressedLength();
      uint32_t numToWrite = std::min(aCount, remaining);
      uint32_t numWritten;
      rv = aWriter(this, aClosure, &mUncompressedBuffer[mNextByte], *aBytesReadOut,
                   numToWrite, &numWritten);

      
      if (NS_FAILED(rv)) {
        return NS_OK;
      }

      
      if (numWritten == 0) {
        return NS_OK;
      }

      *aBytesReadOut += numWritten;
      mNextByte += numWritten;
      MOZ_ASSERT(mNextByte <= mUncompressedBytes);

      if (mNextByte == mUncompressedBytes) {
        mNextByte = 0;
        mUncompressedBytes = 0;
      }

      aCount -= numWritten;

      continue;
    }

    
    
    uint32_t bytesRead;
    rv = ParseNextChunk(&bytesRead);
    if (NS_FAILED(rv)) { return rv; }

    
    
    if (bytesRead == 0 && mUncompressedBytes == 0) {
      return NS_OK;
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
SnappyUncompressInputStream::IsNonBlocking(bool* aNonBlockingOut)
{
  *aNonBlockingOut = false;
  return NS_OK;
}

SnappyUncompressInputStream::~SnappyUncompressInputStream()
{
  Close();
}

nsresult
SnappyUncompressInputStream::ParseNextChunk(uint32_t* aBytesReadOut)
{
  
  MOZ_ASSERT(mUncompressedBytes == 0);
  MOZ_ASSERT(mNextByte == 0);

  nsresult rv;
  *aBytesReadOut = 0;

  
  
  
  if (!mUncompressedBuffer) {
    mUncompressedBuffer.reset(new ((fallible_t())) char[snappy::kBlockSize]);
    if (NS_WARN_IF(!mUncompressedBuffer)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  if (!mCompressedBuffer) {
    mCompressedBuffer.reset(new ((fallible_t())) char[kCompressedBufferLength]);
    if (NS_WARN_IF(!mCompressedBuffer)) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
  }

  
  
  
  if (mNeedFirstStreamIdentifier) {
    const uint32_t firstReadLength = kHeaderLength +
                                     kStreamIdentifierDataLength +
                                     kHeaderLength;
    MOZ_ASSERT(firstReadLength <= kCompressedBufferLength);

    rv = ReadAll(mCompressedBuffer.get(), firstReadLength, firstReadLength,
                 aBytesReadOut);
    if (NS_WARN_IF(NS_FAILED(rv)) || *aBytesReadOut == 0) { return rv; }

    rv = ParseHeader(mCompressedBuffer.get(), kHeaderLength,
                     &mNextChunkType, &mNextChunkDataLength);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    if (NS_WARN_IF(mNextChunkType != StreamIdentifier ||
                   mNextChunkDataLength != kStreamIdentifierDataLength)) {
      return NS_ERROR_CORRUPTED_CONTENT;
    }
    size_t offset = kHeaderLength;

    mNeedFirstStreamIdentifier = false;

    size_t numRead;
    size_t numWritten;
    rv = ParseData(mUncompressedBuffer.get(), snappy::kBlockSize, mNextChunkType,
                   &mCompressedBuffer[offset],
                   mNextChunkDataLength, &numWritten, &numRead);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
    MOZ_ASSERT(numWritten == 0);
    MOZ_ASSERT(numRead == mNextChunkDataLength);
    offset += numRead;

    rv = ParseHeader(&mCompressedBuffer[offset], *aBytesReadOut - offset,
                     &mNextChunkType, &mNextChunkDataLength);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return NS_OK;
  }

  
  
  
  
  if (mNextChunkType == Unknown) {
    rv = ReadAll(mCompressedBuffer.get(), kHeaderLength, kHeaderLength,
                 aBytesReadOut);
    if (NS_WARN_IF(NS_FAILED(rv)) || *aBytesReadOut == 0) { return rv; }

    rv = ParseHeader(mCompressedBuffer.get(), kHeaderLength,
                     &mNextChunkType, &mNextChunkDataLength);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    return NS_OK;
  }

  
  
  uint32_t readLength = mNextChunkDataLength;
  MOZ_ASSERT(readLength <= kCompressedBufferLength);

  
  
  uint64_t avail;
  rv = mBaseStream->Available(&avail);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  if (avail >= (readLength + kHeaderLength)) {
    readLength += kHeaderLength;
    MOZ_ASSERT(readLength <= kCompressedBufferLength);
  }

  rv = ReadAll(mCompressedBuffer.get(), readLength, mNextChunkDataLength,
               aBytesReadOut);
  if (NS_WARN_IF(NS_FAILED(rv)) || *aBytesReadOut == 0) { return rv; }

  size_t numRead;
  size_t numWritten;
  rv = ParseData(mUncompressedBuffer.get(), snappy::kBlockSize, mNextChunkType,
                 mCompressedBuffer.get(), mNextChunkDataLength,
                 &numWritten, &numRead);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }
  MOZ_ASSERT(numRead == mNextChunkDataLength);

  mUncompressedBytes = numWritten;

  
  
  
  if (*aBytesReadOut <= mNextChunkDataLength) {
    mNextChunkType = Unknown;
    mNextChunkDataLength = 0;
    return NS_OK;
  }

  
  
  rv = ParseHeader(&mCompressedBuffer[numRead], *aBytesReadOut - numRead,
                   &mNextChunkType, &mNextChunkDataLength);
  if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

  return NS_OK;
}

nsresult
SnappyUncompressInputStream::ReadAll(char* aBuf, uint32_t aCount,
                                     uint32_t aMinValidCount,
                                     uint32_t* aBytesReadOut)
{
  MOZ_ASSERT(aCount >= aMinValidCount);

  *aBytesReadOut = 0;

  if (!mBaseStream) {
    return NS_BASE_STREAM_CLOSED;
  }

  uint32_t offset = 0;
  while (aCount > 0) {
    uint32_t bytesRead = 0;
    nsresult rv = mBaseStream->Read(aBuf + offset, aCount, &bytesRead);
    if (NS_WARN_IF(NS_FAILED(rv))) { return rv; }

    
    
    if (bytesRead == 0) {
      break;
    }

    *aBytesReadOut += bytesRead;
    offset += bytesRead;
    aCount -= bytesRead;
  }

  
  
  if (*aBytesReadOut != 0 && *aBytesReadOut < aMinValidCount) {
    return NS_ERROR_CORRUPTED_CONTENT;
  }

  return NS_OK;
}

size_t
SnappyUncompressInputStream::UncompressedLength() const
{
  MOZ_ASSERT(mNextByte <= mUncompressedBytes);
  return mUncompressedBytes - mNextByte;
}

} 
