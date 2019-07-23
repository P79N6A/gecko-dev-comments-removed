





































#include "pratom.h"
#include "nsIComponentManager.h"
#include "nsICharRepresentable.h"
#include "nsUCSupport.h"
#include "nsUnicodeDecodeHelper.h"
#include "nsUnicodeEncodeHelper.h"

#define DEFAULT_BUFFER_CAPACITY 16






nsBasicDecoderSupport::nsBasicDecoderSupport()
  : mErrBehavior(kOnError_Recover)
{
}

nsBasicDecoderSupport::~nsBasicDecoderSupport()
{
}




NS_IMPL_ADDREF(nsBasicDecoderSupport)
NS_IMPL_RELEASE(nsBasicDecoderSupport)
#ifdef NS_DEBUG
NS_IMPL_QUERY_INTERFACE2(nsBasicDecoderSupport, nsIUnicodeDecoder, nsIBasicDecoder)
#else
NS_IMPL_QUERY_INTERFACE1(nsBasicDecoderSupport, nsIUnicodeDecoder)
#endif




void
nsBasicDecoderSupport::SetInputErrorBehavior(PRInt32 aBehavior)
{
  NS_ABORT_IF_FALSE(aBehavior == kOnError_Recover || aBehavior == kOnError_Signal,
                    "Unknown behavior for SetInputErrorBehavior");
  mErrBehavior = aBehavior;
}

PRUnichar
nsBasicDecoderSupport::GetCharacterForUnMapped()
{
  return PRUnichar(0xfffd); 
}




nsBufferDecoderSupport::nsBufferDecoderSupport(PRUint32 aMaxLengthFactor)
  : nsBasicDecoderSupport(),
    mMaxLengthFactor(aMaxLengthFactor)
{
  mBufferCapacity = DEFAULT_BUFFER_CAPACITY;
  mBuffer = new char[mBufferCapacity];

  Reset();
}

nsBufferDecoderSupport::~nsBufferDecoderSupport()
{
  delete [] mBuffer;
}

void nsBufferDecoderSupport::FillBuffer(const char ** aSrc, PRInt32 aSrcLength)
{
  PRInt32 bcr = PR_MIN(mBufferCapacity - mBufferLength, aSrcLength);
  memcpy(mBuffer + mBufferLength, *aSrc, bcr);
  mBufferLength += bcr;
  (*aSrc) += bcr;
}

void nsBufferDecoderSupport::DoubleBuffer()
{
  mBufferCapacity *= 2;
  char * newBuffer = new char [mBufferCapacity];
  if (mBufferLength > 0) memcpy(newBuffer, mBuffer, mBufferLength);
  delete [] mBuffer;
  mBuffer = newBuffer;
}




NS_IMETHODIMP nsBufferDecoderSupport::Convert(const char * aSrc,
                                              PRInt32 * aSrcLength,
                                              PRUnichar * aDest,
                                              PRInt32 * aDestLength)
{
  
  const char * src = aSrc;
  const char * srcEnd = aSrc + *aSrcLength;
  PRUnichar * dest = aDest;
  PRUnichar * destEnd = aDest + *aDestLength;

  PRInt32 bcr, bcw; 
  nsresult res = NS_OK;

  
  if (mBufferLength > 0) {
    if (dest == destEnd) {
      res = NS_OK_UDEC_MOREOUTPUT;
    } else {
      for (;;) {
        
        if (src == srcEnd) {
          res = NS_OK_UDEC_MOREINPUT;
          break;
        }

        
        PRInt32 buffLen = mBufferLength;  
        FillBuffer(&src, srcEnd - src);

        
        bcr = mBufferLength;
        bcw = destEnd - dest;
        res = ConvertNoBuff(mBuffer, &bcr, dest, &bcw);
        dest += bcw;

        
        if (res == NS_ERROR_ILLEGAL_INPUT && mErrBehavior == kOnError_Signal) {
          break;
        }

        if ((res == NS_OK_UDEC_MOREINPUT) && (bcw == 0)) {
          res = NS_ERROR_UNEXPECTED;
#if defined(DEBUG_yokoyama) || defined(DEBUG_ftang)
          NS_ERROR("This should not happen. Internal buffer may be corrupted.");
#endif
          break;
        } else {
          if (bcr < buffLen) {
            
            src -= mBufferLength - buffLen;
            mBufferLength = buffLen;
#if defined(DEBUG_yokoyama) || defined(DEBUG_ftang)
            NS_ERROR("This should not happen. Internal buffer may be corrupted.");
#endif
          } else {
            
            src -= mBufferLength - bcr;
            mBufferLength = 0;
            res = NS_OK;
          }
          break;
        }
      }
    }
  }

  if (res == NS_OK) {
    bcr = srcEnd - src;
    bcw = destEnd - dest;
    res = ConvertNoBuff(src, &bcr, dest, &bcw);
    src += bcr;
    dest += bcw;

    
    if (res == NS_OK_UDEC_MOREINPUT) {
      bcr = srcEnd - src;
      
      if (bcr > mBufferCapacity) {
          
          
          res = NS_ERROR_UNEXPECTED;
      } else {
          FillBuffer(&src, bcr);
      }
    }
  }

  *aSrcLength   -= srcEnd - src;
  *aDestLength  -= destEnd - dest;
  return res;
}

NS_IMETHODIMP nsBufferDecoderSupport::Reset()
{
  mBufferLength = 0;
  return NS_OK;
}

NS_IMETHODIMP nsBufferDecoderSupport::GetMaxLength(const char* aSrc,
                                                   PRInt32 aSrcLength,
                                                   PRInt32* aDestLength)
{
  NS_ASSERTION(mMaxLengthFactor != 0, "Must override GetMaxLength!");
  *aDestLength = aSrcLength * mMaxLengthFactor;
  return NS_OK;
}




nsTableDecoderSupport::nsTableDecoderSupport(uScanClassID aScanClass,
                                             uShiftInTable * aShiftInTable,
                                             uMappingTable  * aMappingTable,
                                             PRUint32 aMaxLengthFactor)
: nsBufferDecoderSupport(aMaxLengthFactor)
{
  mScanClass = aScanClass;
  mShiftInTable = aShiftInTable;
  mMappingTable = aMappingTable;
}

nsTableDecoderSupport::~nsTableDecoderSupport()
{
}




NS_IMETHODIMP nsTableDecoderSupport::ConvertNoBuff(const char * aSrc,
                                                   PRInt32 * aSrcLength,
                                                   PRUnichar * aDest,
                                                   PRInt32 * aDestLength)
{
  return nsUnicodeDecodeHelper::ConvertByTable(aSrc, aSrcLength,
                                               aDest, aDestLength,
                                               mScanClass,
                                               mShiftInTable, mMappingTable,
                                               mErrBehavior == kOnError_Signal);
}




nsMultiTableDecoderSupport::nsMultiTableDecoderSupport(
                            PRInt32 aTableCount,
                            const uRange * aRangeArray,
                            uScanClassID * aScanClassArray,
                            uMappingTable ** aMappingTable,
                            PRUint32 aMaxLengthFactor)
: nsBufferDecoderSupport(aMaxLengthFactor)
{
  mTableCount = aTableCount;
  mRangeArray = aRangeArray;
  mScanClassArray = aScanClassArray;
  mMappingTable = aMappingTable;
}

nsMultiTableDecoderSupport::~nsMultiTableDecoderSupport()
{
}




NS_IMETHODIMP nsMultiTableDecoderSupport::ConvertNoBuff(const char * aSrc,
                                                        PRInt32 * aSrcLength,
                                                        PRUnichar * aDest,
                                                        PRInt32 * aDestLength)
{
  return nsUnicodeDecodeHelper::ConvertByMultiTable(aSrc, aSrcLength,
                                                    aDest, aDestLength,
                                                    mTableCount, mRangeArray,
                                                    mScanClassArray,
                                                    mMappingTable,
                                                    mErrBehavior == kOnError_Signal);
}




nsOneByteDecoderSupport::nsOneByteDecoderSupport(
                         uMappingTable  * aMappingTable)
: nsBasicDecoderSupport()
{
  mMappingTable = aMappingTable;
  mFastTableCreated = PR_FALSE;
}

nsOneByteDecoderSupport::~nsOneByteDecoderSupport()
{
}




NS_IMETHODIMP nsOneByteDecoderSupport::Convert(const char * aSrc,
                                              PRInt32 * aSrcLength,
                                              PRUnichar * aDest,
                                              PRInt32 * aDestLength)
{
  if (!mFastTableCreated) {
    nsresult res = nsUnicodeDecodeHelper::CreateFastTable(
                       mMappingTable, mFastTable, ONE_BYTE_TABLE_SIZE);
    if (NS_FAILED(res)) return res;
    mFastTableCreated = PR_TRUE;
  }

  return nsUnicodeDecodeHelper::ConvertByFastTable(aSrc, aSrcLength,
                                                   aDest, aDestLength,
                                                   mFastTable,
                                                   ONE_BYTE_TABLE_SIZE,
                                                   mErrBehavior == kOnError_Signal);
}

NS_IMETHODIMP nsOneByteDecoderSupport::GetMaxLength(const char * aSrc,
                                                    PRInt32 aSrcLength,
                                                    PRInt32 * aDestLength)
{
  
  *aDestLength = aSrcLength;
  return NS_OK_UDEC_EXACTLENGTH;
}

NS_IMETHODIMP nsOneByteDecoderSupport::Reset()
{
  
  return NS_OK;
}



nsBasicEncoder::nsBasicEncoder()
{
}

nsBasicEncoder::~nsBasicEncoder()
{
}




NS_IMPL_ADDREF(nsBasicEncoder)
NS_IMPL_RELEASE(nsBasicEncoder)
#ifdef NS_DEBUG
NS_IMPL_QUERY_INTERFACE3(nsBasicEncoder,
                         nsIUnicodeEncoder,
                         nsICharRepresentable, nsIBasicEncoder)
#else
NS_IMPL_QUERY_INTERFACE2(nsBasicEncoder,
                         nsIUnicodeEncoder,
                         nsICharRepresentable)
#endif



nsEncoderSupport::nsEncoderSupport(PRUint32 aMaxLengthFactor) :
  mMaxLengthFactor(aMaxLengthFactor)
{
  mBufferCapacity = DEFAULT_BUFFER_CAPACITY;
  mBuffer = new char[mBufferCapacity];

  mErrBehavior = kOnError_Signal;
  mErrChar = 0;

  Reset();
}

nsEncoderSupport::~nsEncoderSupport()
{
  delete [] mBuffer;
}

NS_IMETHODIMP nsEncoderSupport::ConvertNoBuff(const PRUnichar * aSrc,
                                              PRInt32 * aSrcLength,
                                              char * aDest,
                                              PRInt32 * aDestLength)
{
  
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;

  PRInt32 bcr, bcw; 
  nsresult res;

  for (;;) {
    bcr = srcEnd - src;
    bcw = destEnd - dest;
    res = ConvertNoBuffNoErr(src, &bcr, dest, &bcw);
    src += bcr;
    dest += bcw;

    if (res == NS_ERROR_UENC_NOMAPPING) {
      if (mErrBehavior == kOnError_Replace) {
        const PRUnichar buff[] = {mErrChar};
        bcr = 1;
        bcw = destEnd - dest;
        src--; 
        res = ConvertNoBuffNoErr(buff, &bcr, dest, &bcw);
        src += bcr;
        dest += bcw;
        if (res != NS_OK) break;
      } else if (mErrBehavior == kOnError_CallBack) {
        bcw = destEnd - dest;
        src--;
        res = mErrEncoder->Convert(*src, dest, &bcw);
        dest += bcw;
        
        if (res != NS_OK_UENC_MOREOUTPUT) src++;
        if (res != NS_OK) break;
      } else break;
    }
    else break;
  }

  *aSrcLength   -= srcEnd - src;
  *aDestLength  -= destEnd - dest;
  return res;
}

NS_IMETHODIMP nsEncoderSupport::FinishNoBuff(char * aDest,
                                             PRInt32 * aDestLength)
{
  *aDestLength = 0;
  return NS_OK;
}

nsresult nsEncoderSupport::FlushBuffer(char ** aDest, const char * aDestEnd)
{
  PRInt32 bcr, bcw; 
  nsresult res = NS_OK;
  char * dest = *aDest;

  if (mBufferStart < mBufferEnd) {
    bcr = mBufferEnd - mBufferStart;
    bcw = aDestEnd - dest;
    if (bcw < bcr) bcr = bcw;
    memcpy(dest, mBufferStart, bcr);
    dest += bcr;
    mBufferStart += bcr;

    if (mBufferStart < mBufferEnd) res = NS_OK_UENC_MOREOUTPUT;
  }

  *aDest = dest;
  return res;
}





NS_IMETHODIMP nsEncoderSupport::Convert(const PRUnichar * aSrc,
                                        PRInt32 * aSrcLength,
                                        char * aDest,
                                        PRInt32 * aDestLength)
{
  
  const PRUnichar * src = aSrc;
  const PRUnichar * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;

  PRInt32 bcr, bcw; 
  nsresult res;

  res = FlushBuffer(&dest, destEnd);
  if (res == NS_OK_UENC_MOREOUTPUT) goto final;

  bcr = srcEnd - src;
  bcw = destEnd - dest;
  res = ConvertNoBuff(src, &bcr, dest, &bcw);
  src += bcr;
  dest += bcw;
  if ((res == NS_OK_UENC_MOREOUTPUT) && (dest < destEnd)) {
    
    
    for (;;) {
      bcr = 1;
      bcw = mBufferCapacity;
      res = ConvertNoBuff(src, &bcr, mBuffer, &bcw);

      if (res == NS_OK_UENC_MOREOUTPUT) {
        delete [] mBuffer;
        mBufferCapacity *= 2;
        mBuffer = new char [mBufferCapacity];
      } else {
        src += bcr;
        mBufferStart = mBufferEnd = mBuffer;
        mBufferEnd += bcw;
        break;
      }
    }

    res = FlushBuffer(&dest, destEnd);
  }

final:
  *aSrcLength   -= srcEnd - src;
  *aDestLength  -= destEnd - dest;
  return res;
}

NS_IMETHODIMP nsEncoderSupport::Finish(char * aDest, PRInt32 * aDestLength)
{
  
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;

  PRInt32 bcw; 
  nsresult res;

  res = FlushBuffer(&dest, destEnd);
  if (res == NS_OK_UENC_MOREOUTPUT) goto final;

  
  for (;;) {
    bcw = mBufferCapacity;
    res = FinishNoBuff(mBuffer, &bcw);

    if (res == NS_OK_UENC_MOREOUTPUT) {
      delete [] mBuffer;
      mBufferCapacity *= 2;
      mBuffer = new char [mBufferCapacity];
    } else {
      mBufferStart = mBufferEnd = mBuffer;
      mBufferEnd += bcw;
      break;
    }
  }

  res = FlushBuffer(&dest, destEnd);

final:
  *aDestLength  -= destEnd - dest;
  return res;
}

NS_IMETHODIMP nsEncoderSupport::Reset()
{
  mBufferStart = mBufferEnd = mBuffer;
  return NS_OK;
}

NS_IMETHODIMP nsEncoderSupport::SetOutputErrorBehavior(
                                PRInt32 aBehavior,
                                nsIUnicharEncoder * aEncoder,
                                PRUnichar aChar)
{
  if (aBehavior == kOnError_CallBack && aEncoder == nsnull)
    return NS_ERROR_NULL_POINTER;

  mErrEncoder = aEncoder;
  mErrBehavior = aBehavior;
  mErrChar = aChar;
  return NS_OK;
}

NS_IMETHODIMP
nsEncoderSupport::GetMaxLength(const PRUnichar * aSrc,
                               PRInt32 aSrcLength,
                               PRInt32 * aDestLength)
{
  *aDestLength = aSrcLength * mMaxLengthFactor;
  return NS_OK;
}





nsTableEncoderSupport::nsTableEncoderSupport(uScanClassID aScanClass,
                                             uShiftOutTable * aShiftOutTable,
                                             uMappingTable  * aMappingTable,
                                             PRUint32 aMaxLengthFactor)
: nsEncoderSupport(aMaxLengthFactor)
{
  mScanClass = aScanClass;
  mShiftOutTable = aShiftOutTable,
  mMappingTable = aMappingTable;
}

nsTableEncoderSupport::nsTableEncoderSupport(uScanClassID aScanClass,
                                             uMappingTable  * aMappingTable,
                                             PRUint32 aMaxLengthFactor)
: nsEncoderSupport(aMaxLengthFactor)
{
  mScanClass = aScanClass;
  mShiftOutTable = nsnull;
  mMappingTable = aMappingTable;
}

nsTableEncoderSupport::~nsTableEncoderSupport()
{
}

NS_IMETHODIMP nsTableEncoderSupport::FillInfo(PRUint32 *aInfo)
{
  return nsUnicodeEncodeHelper::FillInfo(aInfo, mMappingTable);
}



NS_IMETHODIMP nsTableEncoderSupport::ConvertNoBuffNoErr(
                                     const PRUnichar * aSrc,
                                     PRInt32 * aSrcLength,
                                     char * aDest,
                                     PRInt32 * aDestLength)
{
  return nsUnicodeEncodeHelper::ConvertByTable(aSrc, aSrcLength,
                                               aDest, aDestLength,
                                               mScanClass,
                                               mShiftOutTable, mMappingTable);
}




nsMultiTableEncoderSupport::nsMultiTableEncoderSupport(
                            PRInt32 aTableCount,
                            uScanClassID * aScanClassArray,
                            uShiftOutTable ** aShiftOutTable,
                            uMappingTable  ** aMappingTable,
                            PRUint32 aMaxLengthFactor)
: nsEncoderSupport(aMaxLengthFactor)
{
  mTableCount = aTableCount;
  mScanClassArray = aScanClassArray;
  mShiftOutTable = aShiftOutTable;
  mMappingTable = aMappingTable;
}

nsMultiTableEncoderSupport::~nsMultiTableEncoderSupport()
{
}

NS_IMETHODIMP nsMultiTableEncoderSupport::FillInfo(PRUint32 *aInfo)
{
  return nsUnicodeEncodeHelper::FillInfo(aInfo,mTableCount, mMappingTable);
}



NS_IMETHODIMP nsMultiTableEncoderSupport::ConvertNoBuffNoErr(
                                          const PRUnichar * aSrc,
                                          PRInt32 * aSrcLength,
                                          char * aDest,
                                          PRInt32 * aDestLength)
{
  return nsUnicodeEncodeHelper::ConvertByMultiTable(aSrc, aSrcLength,
                                                    aDest, aDestLength,
                                                    mTableCount,
                                                    mScanClassArray,
                                                    mShiftOutTable,
                                                    mMappingTable);
}
