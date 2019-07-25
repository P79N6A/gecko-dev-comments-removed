




































#include "nsConverterInputStream.h"
#include "nsIInputStream.h"
#include "nsICharsetConverterManager.h"
#include "nsIServiceManager.h"

#define CONVERTER_BUFFER_SIZE 8192

NS_IMPL_ISUPPORTS3(nsConverterInputStream, nsIConverterInputStream,
                   nsIUnicharInputStream, nsIUnicharLineInputStream)
    
static NS_DEFINE_CID(kCharsetConverterManagerCID, NS_ICHARSETCONVERTERMANAGER_CID);

NS_IMETHODIMP
nsConverterInputStream::Init(nsIInputStream* aStream,
                             const char *aCharset,
                             PRInt32 aBufferSize,
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
 
    
    rv = NS_NewByteBuffer(getter_AddRefs(mByteData), nsnull, aBufferSize);
    if (NS_FAILED(rv)) return rv;

    rv = NS_NewUnicharBuffer(getter_AddRefs(mUnicharData), nsnull, aBufferSize);
    if (NS_FAILED(rv)) return rv;

    mInput = aStream;
    mReplacementChar = aReplacementChar;
    
    return NS_OK;
}

NS_IMETHODIMP
nsConverterInputStream::Close()
{
    nsresult rv = mInput ? mInput->Close() : NS_OK;
    PR_FREEIF(mLineBuffer);
    mInput = nsnull;
    mConverter = nsnull;
    mByteData = nsnull;
    mUnicharData = nsnull;
    return rv;
}

NS_IMETHODIMP
nsConverterInputStream::Read(PRUnichar* aBuf,
                             PRUint32 aCount,
                             PRUint32 *aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  PRUint32 readCount = mUnicharDataLength - mUnicharDataOffset;
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
                                     PRUint32 aCount, PRUint32 *aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  PRUint32 bytesToWrite = mUnicharDataLength - mUnicharDataOffset;
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
  
  PRUint32 bytesWritten;
  PRUint32 totalBytesWritten = 0;

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
nsConverterInputStream::ReadString(PRUint32 aCount, nsAString& aString,
                                   PRUint32* aReadCount)
{
  NS_ASSERTION(mUnicharDataLength >= mUnicharDataOffset, "unsigned madness");
  PRUint32 readCount = mUnicharDataLength - mUnicharDataOffset;
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

PRUint32
nsConverterInputStream::Fill(nsresult * aErrorCode)
{
  if (nsnull == mInput) {
    
    *aErrorCode = NS_BASE_STREAM_CLOSED;
    return 0;
  }

  if (NS_FAILED(mLastErrorCode)) {
    
    
    *aErrorCode = mLastErrorCode;
    return 0;
  }
  
  
  
  
  
  
  
  PRInt32 nb = mByteData->Fill(aErrorCode, mInput, mLeftOverBytes);
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

  NS_ASSERTION(PRUint32(nb) + mLeftOverBytes == mByteData->GetLength(),
               "mByteData is lying to us somewhere");
  
  
  mUnicharDataOffset = 0;
  mUnicharDataLength = 0;
  PRUint32 srcConsumed = 0;
  do {
    PRInt32 srcLen = mByteData->GetLength() - srcConsumed;
    PRInt32 dstLen = mUnicharData->GetBufferSize() - mUnicharDataLength;
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
      
      
      srcConsumed = NS_MAX<PRUint32>(srcConsumed, 0);
      mConverter->Reset();
    }
    NS_ASSERTION(srcConsumed <= mByteData->GetLength(),
                 "Whoa.  The converter should have returned NS_OK_UDEC_MOREINPUT before this point!");
  } while (mReplacementChar &&
           NS_FAILED(*aErrorCode) &&
           mUnicharData->GetBufferSize() > mUnicharDataLength);

  mLeftOverBytes = mByteData->GetLength() - srcConsumed;

  return mUnicharDataLength;
}

NS_IMETHODIMP
nsConverterInputStream::ReadLine(nsAString& aLine, PRBool* aResult)
{
  if (!mLineBuffer) {
    nsresult rv = NS_InitLineBuffer(&mLineBuffer);
    if (NS_FAILED(rv)) return rv;
  }
  return NS_ReadLine(this, mLineBuffer, aLine, aResult);
}
