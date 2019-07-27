




#include "nsUCSupport.h"
#include "nsUnicodeDecodeHelper.h"
#include "nsUnicodeEncodeHelper.h"
#include <algorithm>

#define DEFAULT_BUFFER_CAPACITY 16






nsBasicDecoderSupport::nsBasicDecoderSupport()
  : mErrBehavior(kOnError_Recover)
{
}

nsBasicDecoderSupport::~nsBasicDecoderSupport()
{
}




#ifdef DEBUG
NS_IMPL_ISUPPORTS(nsBasicDecoderSupport,
                  nsIUnicodeDecoder,
                  nsIBasicDecoder)
#else
NS_IMPL_ISUPPORTS(nsBasicDecoderSupport, nsIUnicodeDecoder)
#endif




void
nsBasicDecoderSupport::SetInputErrorBehavior(int32_t aBehavior)
{
  NS_ABORT_IF_FALSE(aBehavior == kOnError_Recover || aBehavior == kOnError_Signal,
                    "Unknown behavior for SetInputErrorBehavior");
  mErrBehavior = aBehavior;
}

char16_t
nsBasicDecoderSupport::GetCharacterForUnMapped()
{
  return char16_t(0xfffd); 
}




nsBufferDecoderSupport::nsBufferDecoderSupport(uint32_t aMaxLengthFactor)
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

void nsBufferDecoderSupport::FillBuffer(const char ** aSrc, int32_t aSrcLength)
{
  int32_t bcr = std::min(mBufferCapacity - mBufferLength, aSrcLength);
  memcpy(mBuffer + mBufferLength, *aSrc, bcr);
  mBufferLength += bcr;
  (*aSrc) += bcr;
}




NS_IMETHODIMP nsBufferDecoderSupport::Convert(const char * aSrc,
                                              int32_t * aSrcLength,
                                              char16_t * aDest,
                                              int32_t * aDestLength)
{
  
  const char * src = aSrc;
  const char * srcEnd = aSrc + *aSrcLength;
  char16_t * dest = aDest;
  char16_t * destEnd = aDest + *aDestLength;

  int32_t bcr, bcw; 
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

        
        int32_t buffLen = mBufferLength;  
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
                                                   int32_t aSrcLength,
                                                   int32_t* aDestLength)
{
  NS_ASSERTION(mMaxLengthFactor != 0, "Must override GetMaxLength!");
  *aDestLength = aSrcLength * mMaxLengthFactor;
  return NS_OK;
}




nsTableDecoderSupport::nsTableDecoderSupport(uScanClassID aScanClass,
                                             uShiftInTable * aShiftInTable,
                                             uMappingTable  * aMappingTable,
                                             uint32_t aMaxLengthFactor)
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
                                                   int32_t * aSrcLength,
                                                   char16_t * aDest,
                                                   int32_t * aDestLength)
{
  return nsUnicodeDecodeHelper::ConvertByTable(aSrc, aSrcLength,
                                               aDest, aDestLength,
                                               mScanClass,
                                               mShiftInTable, mMappingTable,
                                               mErrBehavior == kOnError_Signal);
}




nsMultiTableDecoderSupport::nsMultiTableDecoderSupport(
                            int32_t aTableCount,
                            const uRange * aRangeArray,
                            uScanClassID * aScanClassArray,
                            uMappingTable ** aMappingTable,
                            uint32_t aMaxLengthFactor)
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
                                                        int32_t * aSrcLength,
                                                        char16_t * aDest,
                                                        int32_t * aDestLength)
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
  , mMappingTable(aMappingTable)
  , mFastTableCreated(false)
  , mFastTableMutex("nsOneByteDecoderSupport mFastTableMutex")
{
}

nsOneByteDecoderSupport::~nsOneByteDecoderSupport()
{
}




NS_IMETHODIMP nsOneByteDecoderSupport::Convert(const char * aSrc,
                                              int32_t * aSrcLength,
                                              char16_t * aDest,
                                              int32_t * aDestLength)
{
  if (!mFastTableCreated) {
    
    mozilla::MutexAutoLock autoLock(mFastTableMutex);
    if (!mFastTableCreated) {
      nsresult res = nsUnicodeDecodeHelper::CreateFastTable(
                         mMappingTable, mFastTable, ONE_BYTE_TABLE_SIZE);
      if (NS_FAILED(res)) return res;
      mFastTableCreated = true;
    }
  }

  return nsUnicodeDecodeHelper::ConvertByFastTable(aSrc, aSrcLength,
                                                   aDest, aDestLength,
                                                   mFastTable,
                                                   ONE_BYTE_TABLE_SIZE,
                                                   mErrBehavior == kOnError_Signal);
}

NS_IMETHODIMP nsOneByteDecoderSupport::GetMaxLength(const char * aSrc,
                                                    int32_t aSrcLength,
                                                    int32_t * aDestLength)
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
#ifdef DEBUG
NS_IMPL_QUERY_INTERFACE(nsBasicEncoder,
                        nsIUnicodeEncoder,
                        nsIBasicEncoder)
#else
NS_IMPL_QUERY_INTERFACE(nsBasicEncoder,
                        nsIUnicodeEncoder)
#endif



nsEncoderSupport::nsEncoderSupport(uint32_t aMaxLengthFactor) :
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

NS_IMETHODIMP nsEncoderSupport::ConvertNoBuff(const char16_t * aSrc,
                                              int32_t * aSrcLength,
                                              char * aDest,
                                              int32_t * aDestLength)
{
  
  const char16_t * src = aSrc;
  const char16_t * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;

  int32_t bcr, bcw; 
  nsresult res;

  for (;;) {
    bcr = srcEnd - src;
    bcw = destEnd - dest;
    res = ConvertNoBuffNoErr(src, &bcr, dest, &bcw);
    src += bcr;
    dest += bcw;

    if (res == NS_ERROR_UENC_NOMAPPING) {
      if (mErrBehavior == kOnError_Replace) {
        const char16_t buff[] = {mErrChar};
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
                                             int32_t * aDestLength)
{
  *aDestLength = 0;
  return NS_OK;
}

nsresult nsEncoderSupport::FlushBuffer(char ** aDest, const char * aDestEnd)
{
  int32_t bcr, bcw; 
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





NS_IMETHODIMP nsEncoderSupport::Convert(const char16_t * aSrc,
                                        int32_t * aSrcLength,
                                        char * aDest,
                                        int32_t * aDestLength)
{
  
  const char16_t * src = aSrc;
  const char16_t * srcEnd = aSrc + *aSrcLength;
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;

  int32_t bcr, bcw; 
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

NS_IMETHODIMP nsEncoderSupport::Finish(char * aDest, int32_t * aDestLength)
{
  
  char * dest = aDest;
  char * destEnd = aDest + *aDestLength;

  int32_t bcw; 
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
                                int32_t aBehavior,
                                nsIUnicharEncoder * aEncoder,
                                char16_t aChar)
{
  if (aBehavior == kOnError_CallBack && !aEncoder)
    return NS_ERROR_NULL_POINTER;

  mErrEncoder = aEncoder;
  mErrBehavior = aBehavior;
  mErrChar = aChar;
  return NS_OK;
}

NS_IMETHODIMP
nsEncoderSupport::GetMaxLength(const char16_t * aSrc,
                               int32_t aSrcLength,
                               int32_t * aDestLength)
{
  *aDestLength = aSrcLength * mMaxLengthFactor;
  return NS_OK;
}





nsTableEncoderSupport::nsTableEncoderSupport(uScanClassID aScanClass,
                                             uShiftOutTable * aShiftOutTable,
                                             uMappingTable  * aMappingTable,
                                             uint32_t aMaxLengthFactor)
: nsEncoderSupport(aMaxLengthFactor)
{
  mScanClass = aScanClass;
  mShiftOutTable = aShiftOutTable,
  mMappingTable = aMappingTable;
}

nsTableEncoderSupport::nsTableEncoderSupport(uScanClassID aScanClass,
                                             uMappingTable  * aMappingTable,
                                             uint32_t aMaxLengthFactor)
: nsEncoderSupport(aMaxLengthFactor)
{
  mScanClass = aScanClass;
  mShiftOutTable = nullptr;
  mMappingTable = aMappingTable;
}

nsTableEncoderSupport::~nsTableEncoderSupport()
{
}




NS_IMETHODIMP nsTableEncoderSupport::ConvertNoBuffNoErr(
                                     const char16_t * aSrc,
                                     int32_t * aSrcLength,
                                     char * aDest,
                                     int32_t * aDestLength)
{
  return nsUnicodeEncodeHelper::ConvertByTable(aSrc, aSrcLength,
                                               aDest, aDestLength,
                                               mScanClass,
                                               mShiftOutTable, mMappingTable);
}




nsMultiTableEncoderSupport::nsMultiTableEncoderSupport(
                            int32_t aTableCount,
                            uScanClassID * aScanClassArray,
                            uShiftOutTable ** aShiftOutTable,
                            uMappingTable  ** aMappingTable,
                            uint32_t aMaxLengthFactor)
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




NS_IMETHODIMP nsMultiTableEncoderSupport::ConvertNoBuffNoErr(
                                          const char16_t * aSrc,
                                          int32_t * aSrcLength,
                                          char * aDest,
                                          int32_t * aDestLength)
{
  return nsUnicodeEncodeHelper::ConvertByMultiTable(aSrc, aSrcLength,
                                                    aDest, aDestLength,
                                                    mTableCount,
                                                    mScanClassArray,
                                                    mShiftOutTable,
                                                    mMappingTable);
}
