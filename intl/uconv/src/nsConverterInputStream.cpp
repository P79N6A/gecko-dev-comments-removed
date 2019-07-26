




#include "nsConverterInputStream.h"
#include "nsIInputStream.h"
#include "nsICharsetConverterManager.h"
#include "nsIServiceManager.h"
#include "nsReadLine.h"
#include <algorithm>

#define CONVERTER_BUFFER_SIZE 8192

NS_IMPL_ISUPPORTS3(nsConverterInputStream, nsIConverterInputStream,
                   nsIUnicharInputStream, nsIUnicharLineInputStream)
    
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

NS_IMETHODIMP
nsConverterInputStream::Init(nsIInputStream* aStream,
                             const char *aCharset,
                             int32_t aBufferSize,
                             PRUnichar aReplacementChar)
{
    if (!aCharset)
        aCharset = "UTF-8";

    nsresult rv;

    if (aBufferSize <=0) aBufferSize=CONVERTER_BUFFER_SIZE;
    
    
    nsCOMPtr<nsICharsetConverterManager> ccm =
        do_GetService(kCharsetConverterManagerCID, &rv);
    if (NS_FAILED(rv)) return rv;

    rv = ccm->GetUnicodeDecoder(aCharset ? aCharset : "ISO-8859-1", getter_AddRefs(mConverter));
    if (NS_FAILED(rv)) return rv;
 
    
    rv = NS_NewByteBuffer(getter_AddRefs(mByteData), nullptr, aBufferSize);
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewUnicharBuffer(getter_AddRefs(mUnicharData), nullptr, aBufferSize);
    if (NS_FAILED(rv)) return rv;

    mInput = aStream;
    mReplacementChar = aReplacementChar;
    if (!aReplacementChar ||
        aReplacementChar != mConverter->GetCharacterForUnMapped()) {
        mConverter->SetInputErrorBehavior(nsIUnicodeDecoder::kOnError_Signal);
    }

    return NS_OK;
}

NS_IMETHODIMP
nsConverterInputStream::Close()
{
    nsresult rv = mInput ? mInput->Close() : NS_OK;
    mLineBuffer = nullptr;
    mInput = nullptr;
    mConverter = nullptr;
    mByteData = nullptr;
    mUnicharData = nullptr;
    return rv;
}

NS_IMETHODIMP
nsConverterInputStream::Read(PRUnichar* aBuf,
                             uint32_t aCount,
                             uint32_t *aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  uint32_t readCount = mUnicharDataLength - mUnicharDataOffset;
  if (0 == readCount) {
    
    readCount = Fill(&mLastErrorCode);
    if (readCount == 0) {
      *aReadCount = 0;
      return mLastErrorCode;
    }
  }
  if (readCount > aCount) {
    readCount = aCount;
  }
  memcpy(aBuf, mUnicharData->GetBuffer() + mUnicharDataOffset,
         readCount * sizeof(PRUnichar));
  mUnicharDataOffset += readCount;
  *aReadCount = readCount;
  return NS_OK;
}

NS_IMETHODIMP
nsConverterInputStream::ReadSegments(nsWriteUnicharSegmentFun aWriter,
                                     void* aClosure,
                                     uint32_t aCount, uint32_t *aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  uint32_t bytesToWrite = mUnicharDataLength - mUnicharDataOffset;
  nsresult rv;
  if (0 == bytesToWrite) {
    
    bytesToWrite = Fill(&rv);
    if (bytesToWrite <= 0) {
      *aReadCount = 0;
      return rv;
    }
  }
  
  if (bytesToWrite > aCount)
    bytesToWrite = aCount;
  
  uint32_t bytesWritten;
  uint32_t totalBytesWritten = 0;

  while (bytesToWrite) {
    rv = aWriter(this, aClosure,
                 mUnicharData->GetBuffer() + mUnicharDataOffset,
                 totalBytesWritten, bytesToWrite, &bytesWritten);
    if (NS_FAILED(rv)) {
      
      break;
    }
    
    bytesToWrite -= bytesWritten;
    totalBytesWritten += bytesWritten;
    mUnicharDataOffset += bytesWritten;
    
  }

  *aReadCount = totalBytesWritten;

  return NS_OK;
}

NS_IMETHODIMP
nsConverterInputStream::ReadString(uint32_t aCount, nsAString& aString,
                                   uint32_t* aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  uint32_t readCount = mUnicharDataLength - mUnicharDataOffset;
  if (0 == readCount) {
    
    readCount = Fill(&mLastErrorCode);
    if (readCount == 0) {
      *aReadCount = 0;
      return mLastErrorCode;
    }
  }
  if (readCount > aCount) {
    readCount = aCount;
  }
  const PRUnichar* buf = reinterpret_cast<const PRUnichar*>(mUnicharData->GetBuffer() +
                                             mUnicharDataOffset);
  aString.Assign(buf, readCount);
  mUnicharDataOffset += readCount;
  *aReadCount = readCount;
  return NS_OK;
}

uint32_t
nsConverterInputStream::Fill(nsresult * aErrorCode)
{
  if (nullptr == mInput) {
    
    *aErrorCode = NS_BASE_STREAM_CLOSED;
    return 0;
  }

  if (NS_FAILED(mLastErrorCode)) {
    
    
    *aErrorCode = mLastErrorCode;
    return 0;
  }
  
  
  
  
  
  
  
  int32_t nb = mByteData->Fill(aErrorCode, mInput, mLeftOverBytes);
#if defined(DEBUG_bzbarsky) && 0
  for (unsigned int foo = 0; foo < mByteData->GetLength(); ++foo) {
    fprintf(stderr, "%c", mByteData->GetBuffer()[foo]);
  }
  fprintf(stderr, "\n");
#endif
  if (nb <= 0 && mLeftOverBytes == 0) {
    
    *aErrorCode = NS_OK;
    return 0;
  }

  NS_ASSERTION(uint32_t(nb) + mLeftOverBytes == mByteData->GetLength(),
               "mByteData is lying to us somewhere");
  
  
  mUnicharDataOffset = 0;
  mUnicharDataLength = 0;
  uint32_t srcConsumed = 0;
  do {
    int32_t srcLen = mByteData->GetLength() - srcConsumed;
    int32_t dstLen = mUnicharData->GetBufferSize() - mUnicharDataLength;
    *aErrorCode = mConverter->Convert(mByteData->GetBuffer()+srcConsumed,
                                      &srcLen,
                                      mUnicharData->GetBuffer()+mUnicharDataLength,
                                      &dstLen);
    mUnicharDataLength += dstLen;
    
    
    
    srcConsumed += srcLen;
    if (NS_FAILED(*aErrorCode) && mReplacementChar) {
      NS_ASSERTION(0 < mUnicharData->GetBufferSize() - mUnicharDataLength,
                   "Decoder returned an error but filled the output buffer! "
                   "Should not happen.");
      mUnicharData->GetBuffer()[mUnicharDataLength++] = mReplacementChar;
      ++srcConsumed;
      
      
      srcConsumed = std::max<uint32_t>(srcConsumed, 0);
      mConverter->Reset();
    }
    NS_ASSERTION(srcConsumed <= mByteData->GetLength(),
                 "Whoa.  The converter should have returned NS_OK_UDEC_MOREINPUT before this point!");
  } while (mReplacementChar &&
           NS_FAILED(*aErrorCode) &&
           uint32_t(mUnicharData->GetBufferSize()) > mUnicharDataLength);

  mLeftOverBytes = mByteData->GetLength() - srcConsumed;

  return mUnicharDataLength;
}

NS_IMETHODIMP
nsConverterInputStream::ReadLine(nsAString& aLine, bool* aResult)
{
  if (!mLineBuffer) {
    mLineBuffer = new nsLineBuffer<PRUnichar>;
  }
  return NS_ReadLine(this, mLineBuffer.get(), aLine, aResult);
}
