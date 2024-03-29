



#include "nsCRT.h"
#include "mozilla/Endian.h"
#include "nsBMPEncoder.h"
#include "nsPNGEncoder.h"
#include "nsICOEncoder.h"
#include "prprf.h"
#include "nsString.h"
#include "nsStreamUtils.h"
#include "nsTArray.h"

using namespace mozilla;
using namespace mozilla::image;

NS_IMPL_ISUPPORTS(nsICOEncoder, imgIEncoder, nsIInputStream,
                  nsIAsyncInputStream)

nsICOEncoder::nsICOEncoder() : mImageBufferStart(nullptr),
                               mImageBufferCurr(0),
                               mImageBufferSize(0),
                               mImageBufferReadPoint(0),
                               mFinished(false),
                               mUsePNG(true),
                               mNotifyThreshold(0)
{
}

nsICOEncoder::~nsICOEncoder()
{
  if (mImageBufferStart) {
    free(mImageBufferStart);
    mImageBufferStart = nullptr;
    mImageBufferCurr = nullptr;
  }
}





NS_IMETHODIMP
nsICOEncoder::InitFromData(const uint8_t* aData,
                           uint32_t aLength,
                           uint32_t aWidth,
                           uint32_t aHeight,
                           uint32_t aStride,
                           uint32_t aInputFormat,
                           const nsAString& aOutputOptions)
{
  
  if (aInputFormat != INPUT_FORMAT_RGB &&
      aInputFormat != INPUT_FORMAT_RGBA &&
      aInputFormat != INPUT_FORMAT_HOSTARGB) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if ((aInputFormat == INPUT_FORMAT_RGB &&
       aStride < aWidth * 3) ||
       ((aInputFormat == INPUT_FORMAT_RGBA ||
         aInputFormat == INPUT_FORMAT_HOSTARGB) &&
        aStride < aWidth * 4)) {
    NS_WARNING("Invalid stride for InitFromData");
    return NS_ERROR_INVALID_ARG;
  }

  nsresult rv;
  rv = StartImageEncode(aWidth, aHeight, aInputFormat, aOutputOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = AddImageFrame(aData, aLength, aWidth, aHeight, aStride,
                     aInputFormat, aOutputOptions);
  NS_ENSURE_SUCCESS(rv, rv);

  rv = EndImageEncode();
  return rv;
}



NS_IMETHODIMP
nsICOEncoder::GetImageBufferUsed(uint32_t* aOutputSize)
{
  NS_ENSURE_ARG_POINTER(aOutputSize);
  *aOutputSize = mImageBufferSize;
  return NS_OK;
}


NS_IMETHODIMP
nsICOEncoder::GetImageBuffer(char** aOutputBuffer)
{
  NS_ENSURE_ARG_POINTER(aOutputBuffer);
  *aOutputBuffer = reinterpret_cast<char*>(mImageBufferStart);
  return NS_OK;
}

NS_IMETHODIMP
nsICOEncoder::AddImageFrame(const uint8_t* aData,
                            uint32_t aLength,
                            uint32_t aWidth,
                            uint32_t aHeight,
                            uint32_t aStride,
                            uint32_t aInputFormat,
                            const nsAString& aFrameOptions)
{
  if (mUsePNG) {

    mContainedEncoder = new nsPNGEncoder();
    nsresult rv;
    nsAutoString noParams;
    rv = mContainedEncoder->InitFromData(aData, aLength, aWidth, aHeight,
                                         aStride, aInputFormat, noParams);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t PNGImageBufferSize;
    mContainedEncoder->GetImageBufferUsed(&PNGImageBufferSize);
    mImageBufferSize = ICONFILEHEADERSIZE + ICODIRENTRYSIZE +
                       PNGImageBufferSize;
    mImageBufferStart = static_cast<uint8_t*>(malloc(mImageBufferSize));
    if (!mImageBufferStart) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mImageBufferCurr = mImageBufferStart;
    mICODirEntry.mBytesInRes = PNGImageBufferSize;

    EncodeFileHeader();
    EncodeInfoHeader();

    char* imageBuffer;
    rv = mContainedEncoder->GetImageBuffer(&imageBuffer);
    NS_ENSURE_SUCCESS(rv, rv);
    memcpy(mImageBufferCurr, imageBuffer, PNGImageBufferSize);
    mImageBufferCurr += PNGImageBufferSize;
  } else {
    mContainedEncoder = new nsBMPEncoder();
    nsresult rv;

    nsAutoString params;
    params.AppendLiteral("bpp=");
    params.AppendInt(mICODirEntry.mBitCount);

    rv = mContainedEncoder->InitFromData(aData, aLength, aWidth, aHeight,
                                         aStride, aInputFormat, params);
    NS_ENSURE_SUCCESS(rv, rv);

    uint32_t andMaskSize = ((GetRealWidth() + 31) / 32) * 4 * 
                           GetRealHeight(); 

    uint32_t BMPImageBufferSize;
    mContainedEncoder->GetImageBufferUsed(&BMPImageBufferSize);
    mImageBufferSize = ICONFILEHEADERSIZE + ICODIRENTRYSIZE +
                       BMPImageBufferSize + andMaskSize;
    mImageBufferStart = static_cast<uint8_t*>(malloc(mImageBufferSize));
    if (!mImageBufferStart) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mImageBufferCurr = mImageBufferStart;

    
    
    
    mICODirEntry.mBytesInRes = BMPImageBufferSize - BFH_LENGTH + andMaskSize;

    
    EncodeFileHeader();
    EncodeInfoHeader();

    char* imageBuffer;
    rv = mContainedEncoder->GetImageBuffer(&imageBuffer);
    NS_ENSURE_SUCCESS(rv, rv);
    memcpy(mImageBufferCurr, imageBuffer + BFH_LENGTH,
           BMPImageBufferSize - BFH_LENGTH);
    
    uint32_t fixedHeight = GetRealHeight() * 2;
    NativeEndian::swapToLittleEndianInPlace(&fixedHeight, 1);
    
    memcpy(mImageBufferCurr + 8, &fixedHeight, sizeof(fixedHeight));
    mImageBufferCurr += BMPImageBufferSize - BFH_LENGTH;

    
    uint32_t rowSize = ((GetRealWidth() + 31) / 32) * 4; 
    int32_t currentLine = GetRealHeight();

    
    while (currentLine > 0) {
      currentLine--;
      uint8_t* encoded = mImageBufferCurr + currentLine * rowSize;
      uint8_t* encodedEnd = encoded + rowSize;
      while (encoded != encodedEnd) {
        *encoded = 0; 
        encoded++;
      }
    }

    mImageBufferCurr += andMaskSize;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsICOEncoder::StartImageEncode(uint32_t aWidth,
                               uint32_t aHeight,
                               uint32_t aInputFormat,
                               const nsAString& aOutputOptions)
{
  
  if (mImageBufferStart || mImageBufferCurr) {
    return NS_ERROR_ALREADY_INITIALIZED;
  }

  
  if (aInputFormat != INPUT_FORMAT_RGB &&
      aInputFormat != INPUT_FORMAT_RGBA &&
      aInputFormat != INPUT_FORMAT_HOSTARGB) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if (aWidth > 256 || aHeight > 256) {
    return NS_ERROR_INVALID_ARG;
  }

  
  uint32_t bpp = 24;
  bool usePNG = true;
  nsresult rv = ParseOptions(aOutputOptions, &bpp, &usePNG);
  NS_ENSURE_SUCCESS(rv, rv);

  mUsePNG = usePNG;

  InitFileHeader();
  
  InitInfoHeader(bpp, aWidth == 256 ? 0 : (uint8_t)aWidth,
                 aHeight == 256 ? 0 : (uint8_t)aHeight);

  return NS_OK;
}

NS_IMETHODIMP
nsICOEncoder::EndImageEncode()
{
  
  if (!mImageBufferStart || !mImageBufferCurr) {
    return NS_ERROR_NOT_INITIALIZED;
  }

  mFinished = true;
  NotifyListener();

  
  if (!mImageBufferStart || !mImageBufferCurr) {
    return NS_ERROR_OUT_OF_MEMORY;
  }

  return NS_OK;
}



nsresult
nsICOEncoder::ParseOptions(const nsAString& aOptions, uint32_t* bpp,
                           bool* usePNG)
{
  
  if (aOptions.Length() == 0) {
    if (usePNG) {
      *usePNG = true;
    }
    if (bpp) {
      *bpp = 24;
    }
  }

  
  
  
  nsTArray<nsCString> nameValuePairs;
  if (!ParseString(NS_ConvertUTF16toUTF8(aOptions), ';', nameValuePairs)) {
    return NS_ERROR_INVALID_ARG;
  }

  
  for (unsigned i = 0; i < nameValuePairs.Length(); ++i) {

    
    nsTArray<nsCString> nameValuePair;
    if (!ParseString(nameValuePairs[i], '=', nameValuePair)) {
      return NS_ERROR_INVALID_ARG;
    }
    if (nameValuePair.Length() != 2) {
      return NS_ERROR_INVALID_ARG;
    }

    
    if (nameValuePair[0].Equals("format",
                                nsCaseInsensitiveCStringComparator())) {
      if (nameValuePair[1].Equals("png",
                                  nsCaseInsensitiveCStringComparator())) {
        *usePNG = true;
      }
      else if (nameValuePair[1].Equals("bmp",
                                       nsCaseInsensitiveCStringComparator())) {
        *usePNG = false;
      }
      else {
        return NS_ERROR_INVALID_ARG;
      }
    }

    
    if (nameValuePair[0].Equals("bpp", nsCaseInsensitiveCStringComparator())) {
      if (nameValuePair[1].EqualsLiteral("24")) {
        *bpp = 24;
      }
      else if (nameValuePair[1].EqualsLiteral("32")) {
        *bpp = 32;
      }
      else {
        return NS_ERROR_INVALID_ARG;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP
nsICOEncoder::Close()
{
  if (mImageBufferStart) {
    free(mImageBufferStart);
    mImageBufferStart = nullptr;
    mImageBufferSize = 0;
    mImageBufferReadPoint = 0;
    mImageBufferCurr = nullptr;
  }

  return NS_OK;
}


NS_IMETHODIMP
nsICOEncoder::Available(uint64_t *_retval)
{
  if (!mImageBufferStart || !mImageBufferCurr) {
    return NS_BASE_STREAM_CLOSED;
  }

  *_retval = GetCurrentImageBufferOffset() - mImageBufferReadPoint;
  return NS_OK;
}


NS_IMETHODIMP
nsICOEncoder::Read(char* aBuf, uint32_t aCount, uint32_t* _retval)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, _retval);
}


NS_IMETHODIMP
nsICOEncoder::ReadSegments(nsWriteSegmentFun aWriter, void* aClosure,
                           uint32_t aCount, uint32_t* _retval)
{
  uint32_t maxCount = GetCurrentImageBufferOffset() - mImageBufferReadPoint;
  if (maxCount == 0) {
    *_retval = 0;
    return mFinished ? NS_OK : NS_BASE_STREAM_WOULD_BLOCK;
  }

  if (aCount > maxCount) {
    aCount = maxCount;
  }

  nsresult rv = aWriter(this, aClosure,
                        reinterpret_cast<const char*>(mImageBufferStart +
                                                      mImageBufferReadPoint),
                        0, aCount, _retval);
  if (NS_SUCCEEDED(rv)) {
    NS_ASSERTION(*_retval <= aCount, "bad write count");
    mImageBufferReadPoint += *_retval;
  }
  
  return NS_OK;
}

NS_IMETHODIMP
nsICOEncoder::IsNonBlocking(bool* _retval)
{
  *_retval = true;
  return NS_OK;
}

NS_IMETHODIMP
nsICOEncoder::AsyncWait(nsIInputStreamCallback* aCallback,
                        uint32_t aFlags,
                        uint32_t aRequestedCount,
                        nsIEventTarget* aTarget)
{
  if (aFlags != 0) {
    return NS_ERROR_NOT_IMPLEMENTED;
  }

  if (mCallback || mCallbackTarget) {
    return NS_ERROR_UNEXPECTED;
  }

  mCallbackTarget = aTarget;
  
  mNotifyThreshold = aRequestedCount;
  if (!aRequestedCount) {
    mNotifyThreshold = 1024; 
  }

  
  
  
  
  
  mCallback = aCallback;

  
  NotifyListener();
  return NS_OK;
}

NS_IMETHODIMP
nsICOEncoder::CloseWithStatus(nsresult aStatus)
{
  return Close();
}

void
nsICOEncoder::NotifyListener()
{
  if (mCallback &&
      (GetCurrentImageBufferOffset() -
         mImageBufferReadPoint >= mNotifyThreshold || mFinished)) {
    nsCOMPtr<nsIInputStreamCallback> callback;
    if (mCallbackTarget) {
      callback = NS_NewInputStreamReadyEvent(mCallback, mCallbackTarget);
    } else {
      callback = mCallback;
    }

    NS_ASSERTION(callback, "Shouldn't fail to make the callback");
    
    
    mCallback = nullptr;
    mCallbackTarget = nullptr;
    mNotifyThreshold = 0;

    callback->OnInputStreamReady(this);
  }
}


void
nsICOEncoder::InitFileHeader()
{
  memset(&mICOFileHeader, 0, sizeof(mICOFileHeader));
  mICOFileHeader.mReserved = 0;
  mICOFileHeader.mType = 1;
  mICOFileHeader.mCount = 1;
}


void
nsICOEncoder::InitInfoHeader(uint32_t aBPP, uint8_t aWidth, uint8_t aHeight)
{
  memset(&mICODirEntry, 0, sizeof(mICODirEntry));
  mICODirEntry.mBitCount = aBPP;
  mICODirEntry.mBytesInRes = 0;
  mICODirEntry.mColorCount = 0;
  mICODirEntry.mWidth = aWidth;
  mICODirEntry.mHeight = aHeight;
  mICODirEntry.mImageOffset = ICONFILEHEADERSIZE + ICODIRENTRYSIZE;
  mICODirEntry.mPlanes = 1;
  mICODirEntry.mReserved = 0;
}


void
nsICOEncoder::EncodeFileHeader()
{
  IconFileHeader littleEndianIFH = mICOFileHeader;
  NativeEndian::swapToLittleEndianInPlace(&littleEndianIFH.mReserved, 1);
  NativeEndian::swapToLittleEndianInPlace(&littleEndianIFH.mType, 1);
  NativeEndian::swapToLittleEndianInPlace(&littleEndianIFH.mCount, 1);

  memcpy(mImageBufferCurr, &littleEndianIFH.mReserved,
         sizeof(littleEndianIFH.mReserved));
  mImageBufferCurr += sizeof(littleEndianIFH.mReserved);
  memcpy(mImageBufferCurr, &littleEndianIFH.mType,
         sizeof(littleEndianIFH.mType));
  mImageBufferCurr += sizeof(littleEndianIFH.mType);
  memcpy(mImageBufferCurr, &littleEndianIFH.mCount,
         sizeof(littleEndianIFH.mCount));
  mImageBufferCurr += sizeof(littleEndianIFH.mCount);
}


void
nsICOEncoder::EncodeInfoHeader()
{
  IconDirEntry littleEndianmIDE = mICODirEntry;

  NativeEndian::swapToLittleEndianInPlace(&littleEndianmIDE.mPlanes, 1);
  NativeEndian::swapToLittleEndianInPlace(&littleEndianmIDE.mBitCount, 1);
  NativeEndian::swapToLittleEndianInPlace(&littleEndianmIDE.mBytesInRes, 1);
  NativeEndian::swapToLittleEndianInPlace(&littleEndianmIDE.mImageOffset, 1);

  memcpy(mImageBufferCurr, &littleEndianmIDE.mWidth,
         sizeof(littleEndianmIDE.mWidth));
  mImageBufferCurr += sizeof(littleEndianmIDE.mWidth);
  memcpy(mImageBufferCurr, &littleEndianmIDE.mHeight,
         sizeof(littleEndianmIDE.mHeight));
  mImageBufferCurr += sizeof(littleEndianmIDE.mHeight);
  memcpy(mImageBufferCurr, &littleEndianmIDE.mColorCount,
         sizeof(littleEndianmIDE.mColorCount));
  mImageBufferCurr += sizeof(littleEndianmIDE.mColorCount);
  memcpy(mImageBufferCurr, &littleEndianmIDE.mReserved,
         sizeof(littleEndianmIDE.mReserved));
  mImageBufferCurr += sizeof(littleEndianmIDE.mReserved);
  memcpy(mImageBufferCurr, &littleEndianmIDE.mPlanes,
         sizeof(littleEndianmIDE.mPlanes));
  mImageBufferCurr += sizeof(littleEndianmIDE.mPlanes);
  memcpy(mImageBufferCurr, &littleEndianmIDE.mBitCount,
         sizeof(littleEndianmIDE.mBitCount));
  mImageBufferCurr += sizeof(littleEndianmIDE.mBitCount);
  memcpy(mImageBufferCurr, &littleEndianmIDE.mBytesInRes,
         sizeof(littleEndianmIDE.mBytesInRes));
  mImageBufferCurr += sizeof(littleEndianmIDE.mBytesInRes);
  memcpy(mImageBufferCurr, &littleEndianmIDE.mImageOffset,
         sizeof(littleEndianmIDE.mImageOffset));
  mImageBufferCurr += sizeof(littleEndianmIDE.mImageOffset);
}
