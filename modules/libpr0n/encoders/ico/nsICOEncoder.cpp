




































#include "nsCRT.h"
#include "EndianMacros.h"
#include "nsBMPEncoder.h"
#include "nsPNGEncoder.h"
#include "nsICOEncoder.h"
#include "prmem.h"
#include "prprf.h"
#include "nsString.h"
#include "nsStreamUtils.h"

using namespace mozilla;
using namespace mozilla::imagelib;

NS_IMPL_THREADSAFE_ISUPPORTS3(nsICOEncoder, imgIEncoder, nsIInputStream, nsIAsyncInputStream)

nsICOEncoder::nsICOEncoder() : mFinished(false),
                               mImageBufferStart(nsnull), 
                               mImageBufferCurr(0),
                               mImageBufferSize(0), 
                               mImageBufferReadPoint(0), 
                               mCallback(nsnull),
                               mCallbackTarget(nsnull), 
                               mNotifyThreshold(0),
                               mUsePNG(true)
{
}

nsICOEncoder::~nsICOEncoder()
{
  if (mImageBufferStart) {
    moz_free(mImageBufferStart);
    mImageBufferStart = nsnull;
    mImageBufferCurr = nsnull;
  }
}





NS_IMETHODIMP nsICOEncoder::InitFromData(const PRUint8* aData,
                                         PRUint32 aLength,
                                         PRUint32 aWidth,
                                         PRUint32 aHeight,
                                         PRUint32 aStride,
                                         PRUint32 aInputFormat,
                                         const nsAString& aOutputOptions)
{
  
  if (aInputFormat != INPUT_FORMAT_RGB &&
      aInputFormat != INPUT_FORMAT_RGBA &&
      aInputFormat != INPUT_FORMAT_HOSTARGB) {
    return NS_ERROR_INVALID_ARG;
  }

  
  if ((aInputFormat == INPUT_FORMAT_RGB &&
       aStride < aWidth * 3) ||
      ((aInputFormat == INPUT_FORMAT_RGBA || aInputFormat == INPUT_FORMAT_HOSTARGB) &&
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
nsICOEncoder::GetImageBufferSize(PRUint32 *aOutputSize)
{
  NS_ENSURE_ARG_POINTER(aOutputSize);
  *aOutputSize = mImageBufferSize;
  return NS_OK;
}


NS_IMETHODIMP 
nsICOEncoder::GetImageBuffer(char **aOutputBuffer)
{
  NS_ENSURE_ARG_POINTER(aOutputBuffer);
  *aOutputBuffer = reinterpret_cast<char*>(mImageBufferStart);
  return NS_OK;
}

NS_IMETHODIMP 
nsICOEncoder::AddImageFrame(const PRUint8* aData,
                            PRUint32 aLength,
                            PRUint32 aWidth,
                            PRUint32 aHeight,
                            PRUint32 aStride,
                            PRUint32 aInputFormat, 
                            const nsAString& aFrameOptions)
{
  if (mUsePNG) {

    mContainedEncoder = new nsPNGEncoder();
    nsresult rv;
    nsAutoString noParams;
    rv = mContainedEncoder->InitFromData(aData, aLength, aWidth, aHeight,
                                         aStride, aInputFormat, noParams);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 imageBufferSize;
    mContainedEncoder->GetImageBufferSize(&imageBufferSize);
    mImageBufferSize = ICONFILEHEADERSIZE + ICODIRENTRYSIZE + 
                       imageBufferSize;
    mImageBufferStart = static_cast<PRUint8*>(moz_malloc(mImageBufferSize));
    if (!mImageBufferStart) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mImageBufferCurr = mImageBufferStart;
    mICODirEntry.mBytesInRes = imageBufferSize;

    EncodeFileHeader();
    EncodeInfoHeader();

    char *imageBuffer;
    rv = mContainedEncoder->GetImageBuffer(&imageBuffer);
    NS_ENSURE_SUCCESS(rv, rv);
    memcpy(mImageBufferCurr, imageBuffer, imageBufferSize);
    mImageBufferCurr += imageBufferSize;
  } else {
    mContainedEncoder = new nsBMPEncoder();
    nsresult rv;

    nsAutoString params;
    params.AppendASCII("bpp=");
    params.AppendInt(mICODirEntry.mBitCount);

    rv = mContainedEncoder->InitFromData(aData, aLength, aWidth, aHeight,
                                         aStride, aInputFormat, params);
    NS_ENSURE_SUCCESS(rv, rv);

    PRUint32 andMaskSize = ((GetRealWidth() + 31) / 32) * 4 * 
                           GetRealHeight(); 

    PRUint32 imageBufferSize;
    mContainedEncoder->GetImageBufferSize(&imageBufferSize);
    mImageBufferSize = ICONFILEHEADERSIZE + ICODIRENTRYSIZE + 
                       imageBufferSize + andMaskSize;
    mImageBufferStart = static_cast<PRUint8*>(moz_malloc(mImageBufferSize));
    if (!mImageBufferStart) {
      return NS_ERROR_OUT_OF_MEMORY;
    }
    mImageBufferCurr = mImageBufferStart;

    
    mICODirEntry.mBytesInRes = imageBufferSize - BFH_LENGTH + andMaskSize;

    
    EncodeFileHeader();
    EncodeInfoHeader();

    char *imageBuffer;
    rv = mContainedEncoder->GetImageBuffer(&imageBuffer);
    NS_ENSURE_SUCCESS(rv, rv);
    memcpy(mImageBufferCurr, imageBuffer + BFH_LENGTH, 
           imageBufferSize - BFH_LENGTH);
    
    PRUint32 fixedHeight = GetRealHeight() * 2;
    fixedHeight = NATIVE32_TO_LITTLE(fixedHeight);
    
    memcpy(mImageBufferCurr + 8, &fixedHeight, sizeof(fixedHeight));
    mImageBufferCurr += imageBufferSize - BFH_LENGTH;

    
    PRUint32 rowSize = ((GetRealWidth() + 31) / 32) * 4; 
    PRInt32 currentLine = GetRealHeight();
    
    
    while (currentLine > 0) {
      currentLine--;
      PRUint8* encoded = mImageBufferCurr + currentLine * rowSize;
      PRUint8* encodedEnd = encoded + rowSize;
      while (encoded != encodedEnd) {
        *encoded = 0; 
        encoded++;
      }
    }

    mImageBufferCurr += andMaskSize;
  }

  return NS_OK;
}


NS_IMETHODIMP nsICOEncoder::StartImageEncode(PRUint32 aWidth,
                                             PRUint32 aHeight,
                                             PRUint32 aInputFormat,
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

  
  PRUint32 bpp = 24;
  bool usePNG = true;
  nsresult rv = ParseOptions(aOutputOptions, &bpp, &usePNG);
  NS_ENSURE_SUCCESS(rv, rv);

  mUsePNG = usePNG;

  InitFileHeader();
  
  InitInfoHeader(bpp, aWidth == 256 ? 0 : (PRUint8)aWidth, 
                 aHeight == 256 ? 0 : (PRUint8)aHeight);

  return NS_OK;
}

NS_IMETHODIMP nsICOEncoder::EndImageEncode()
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
nsICOEncoder::ParseOptions(const nsAString& aOptions, PRUint32* bpp, 
                           bool *usePNG)
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

  
  for (int i = 0; i < nameValuePairs.Length(); ++i) {

    
    nsTArray<nsCString> nameValuePair;
    if (!ParseString(nameValuePairs[i], '=', nameValuePair)) {
      return NS_ERROR_INVALID_ARG;
    }
    if (nameValuePair.Length() != 2) {
      return NS_ERROR_INVALID_ARG;
    }

    
    if (nameValuePair[0].Equals("format", nsCaseInsensitiveCStringComparator())) {
      if (nameValuePair[1].Equals("png", nsCaseInsensitiveCStringComparator())) {
        *usePNG = true;
      }
      else if (nameValuePair[1].Equals("bmp", nsCaseInsensitiveCStringComparator())) {
        *usePNG = false;
      }
      else {
        return NS_ERROR_INVALID_ARG;
      }
    }

    
    if (nameValuePair[0].Equals("bpp", nsCaseInsensitiveCStringComparator())) {
      if (nameValuePair[1].Equals("24")) {
        *bpp = 24;
      }
      else if (nameValuePair[1].Equals("32")) {
        *bpp = 32;
      }
      else {
        return NS_ERROR_INVALID_ARG;
      }
    }
  }

  return NS_OK;
}

NS_IMETHODIMP nsICOEncoder::Close()
{
  if (mImageBufferStart) {
    moz_free(mImageBufferStart);
    mImageBufferStart = nsnull;
    mImageBufferSize = 0;
    mImageBufferReadPoint = 0;
    mImageBufferCurr = nsnull;
  }

  return NS_OK;
}


NS_IMETHODIMP nsICOEncoder::Available(PRUint32 *_retval)
{
  if (!mImageBufferStart || !mImageBufferCurr) {
    return NS_BASE_STREAM_CLOSED;
  }

  *_retval = GetCurrentImageBufferOffset() - mImageBufferReadPoint;
  return NS_OK;
}


NS_IMETHODIMP nsICOEncoder::Read(char *aBuf, PRUint32 aCount,
                                 PRUint32 *_retval)
{
  return ReadSegments(NS_CopySegmentToBuffer, aBuf, aCount, _retval);
}


NS_IMETHODIMP nsICOEncoder::ReadSegments(nsWriteSegmentFun aWriter,
                                         void *aClosure, PRUint32 aCount,
                                         PRUint32 *_retval)
{
  PRUint32 maxCount = GetCurrentImageBufferOffset() - mImageBufferReadPoint;
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
nsICOEncoder::IsNonBlocking(bool *_retval)
{
  *_retval = true;
  return NS_OK;
}

NS_IMETHODIMP 
nsICOEncoder::AsyncWait(nsIInputStreamCallback *aCallback,
                        PRUint32 aFlags,
                        PRUint32 aRequestedCount,
                        nsIEventTarget *aTarget)
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

NS_IMETHODIMP nsICOEncoder::CloseWithStatus(nsresult aStatus)
{
  return Close();
}

void
nsICOEncoder::NotifyListener()
{
  if (mCallback &&
      (GetCurrentImageBufferOffset() - mImageBufferReadPoint >= mNotifyThreshold ||
       mFinished)) {
    nsCOMPtr<nsIInputStreamCallback> callback;
    if (mCallbackTarget) {
      NS_NewInputStreamReadyEvent(getter_AddRefs(callback),
                                  mCallback,
                                  mCallbackTarget);
    } else {
      callback = mCallback;
    }

    NS_ASSERTION(callback, "Shouldn't fail to make the callback");
    
    
    mCallback = nsnull;
    mCallbackTarget = nsnull;
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
nsICOEncoder::InitInfoHeader(PRUint32 aBPP, PRUint8 aWidth, PRUint8 aHeight)
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
  littleEndianIFH.mReserved = NATIVE16_TO_LITTLE(littleEndianIFH.mReserved);
  littleEndianIFH.mType = NATIVE16_TO_LITTLE(littleEndianIFH.mType);
  littleEndianIFH.mCount = NATIVE16_TO_LITTLE(littleEndianIFH.mCount);

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

  littleEndianmIDE.mPlanes = NATIVE16_TO_LITTLE(littleEndianmIDE.mPlanes);
  littleEndianmIDE.mBitCount = NATIVE16_TO_LITTLE(littleEndianmIDE.mBitCount);
  littleEndianmIDE.mBytesInRes = 
    NATIVE32_TO_LITTLE(littleEndianmIDE.mBytesInRes);
  littleEndianmIDE.mImageOffset  = 
    NATIVE32_TO_LITTLE(littleEndianmIDE.mImageOffset);

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
