










































#include <stdlib.h>

#include "nsICODecoder.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "imgIContainerObserver.h"

#include "nsIInterfaceRequestor.h"
#include "nsIInterfaceRequestorUtils.h"

#include "nsIProperties.h"
#include "nsISupportsPrimitives.h"

#include "nsAutoPtr.h"

NS_IMPL_ISUPPORTS1(nsICODecoder, imgIDecoder)

#define ICONCOUNTOFFSET 4
#define DIRENTRYOFFSET 6
#define BITMAPINFOSIZE 40
#define PREFICONSIZE 16





PRUint32 nsICODecoder::CalcAlphaRowSize()
{
  
  PRUint32 rowSize = (mDirEntry.mWidth + 31) / 32; 
  return rowSize * 4;        
}

nsICODecoder::nsICODecoder()
{
  mPos = mNumColors = mRowBytes = mImageOffset = mCurrIcon = mNumIcons = 0;
  mCurLine = 1; 
  mColors = nsnull;
  mRow = nsnull;
  mHaveAlphaData = mDecodingAndMask = PR_FALSE;
  mError = PR_FALSE;
}

nsICODecoder::~nsICODecoder()
{
}

NS_IMETHODIMP nsICODecoder::Init(imgIContainer *aImage,
                                 imgIDecoderObserver *aObserver,
                                 PRUint32 aFlags)
{
  
  mImage = aImage;
  mObserver = aObserver;
  mFlags = aFlags;

  
  if (!(mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) && mObserver)
    mObserver->OnStartDecode(nsnull);

  return NS_OK;
}

NS_IMETHODIMP nsICODecoder::Close(PRUint32 aFlags)
{
  nsresult rv = NS_OK;

  
  if (!(mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY) &&
      !mError && !(aFlags & CLOSE_FLAG_DONTNOTIFY)) {
    
    nsIntRect r(0, 0, mDirEntry.mWidth, mDirEntry.mHeight);
    rv = mImage->FrameUpdated(0, r);


    if (mObserver) {
      mObserver->OnDataAvailable(nsnull, PR_TRUE, &r);
      mObserver->OnStopFrame(nsnull, 0);
    }
    mImage->DecodingComplete();
    if (mObserver) {
      mObserver->OnStopContainer(nsnull, 0);
      mObserver->OnStopDecode(nsnull, NS_OK, nsnull);
    }
  }

  mPos = 0;

  delete[] mColors;

  mCurLine = 0;
  mRowBytes = 0;
  mImageOffset = 0;
  mCurrIcon = 0;
  mNumIcons = 0;

  if (mRow) {
    free(mRow);
    mRow = nsnull;
  }
  mDecodingAndMask = PR_FALSE;

  return rv;
}

NS_IMETHODIMP nsICODecoder::Flush()
{
  return NS_OK;
}


NS_METHOD nsICODecoder::ReadSegCb(nsIInputStream* aIn, void* aClosure,
                             const char* aFromRawSegment, PRUint32 aToOffset,
                             PRUint32 aCount, PRUint32 *aWriteCount) {
  nsICODecoder *decoder = reinterpret_cast<nsICODecoder*>(aClosure);

  
  *aWriteCount = aCount;

  
  nsresult rv = decoder->ProcessData(aFromRawSegment, aCount);

  
  if (NS_FAILED(rv))
    decoder->mError = PR_TRUE;
  return rv;
}

NS_IMETHODIMP nsICODecoder::WriteFrom(nsIInputStream *aInStr, PRUint32 aCount)
{
  
  nsresult rv = NS_OK;
  PRUint32 ignored;
  if (!mError)
    rv = aInStr->ReadSegments(ReadSegCb, this, aCount, &ignored);
  if (mError || NS_FAILED(rv))
    return NS_ERROR_FAILURE;
  return NS_OK;
}

nsresult nsICODecoder::ProcessData(const char* aBuffer, PRUint32 aCount) {
  if (!aCount) 
    return NS_OK;

  while (aCount && (mPos < ICONCOUNTOFFSET)) { 
    if (mPos == 2) { 
      if ((*aBuffer != 1) && (*aBuffer != 2)) {
        return NS_ERROR_FAILURE;
      }
      mIsCursor = (*aBuffer == 2);
    }
    mPos++; aBuffer++; aCount--;
  }

  if (mPos == ICONCOUNTOFFSET && aCount >= 2) {
    mNumIcons = LITTLE_TO_NATIVE16(((PRUint16*)aBuffer)[0]);
    aBuffer += 2;
    mPos += 2;
    aCount -= 2;
  }

  if (mNumIcons == 0)
    return NS_OK; 

  PRUint16 colorDepth = 0;
  while (mCurrIcon < mNumIcons) {
    if (mPos >= DIRENTRYOFFSET + (mCurrIcon*sizeof(mDirEntryArray)) && 
        mPos < DIRENTRYOFFSET + ((mCurrIcon+1)*sizeof(mDirEntryArray))) {
      PRUint32 toCopy = sizeof(mDirEntryArray) - (mPos - DIRENTRYOFFSET - mCurrIcon*sizeof(mDirEntryArray));
      if (toCopy > aCount)
        toCopy = aCount;
      memcpy(mDirEntryArray + sizeof(mDirEntryArray) - toCopy, aBuffer, toCopy);
      mPos += toCopy;
      aCount -= toCopy;
      aBuffer += toCopy;
    }
    if (aCount == 0)
      return NS_OK; 

    IconDirEntry e;
    if (mPos == 22+mCurrIcon*sizeof(mDirEntryArray)) {
      mCurrIcon++;
      ProcessDirEntry(e);
      if ((e.mWidth == PREFICONSIZE && e.mHeight == PREFICONSIZE && e.mBitCount >= colorDepth)
           || (mCurrIcon == mNumIcons && mImageOffset == 0)) {
        mImageOffset = e.mImageOffset;

        
        PRUint32 minImageOffset = DIRENTRYOFFSET + mNumIcons*sizeof(mDirEntryArray);
        if (mImageOffset < minImageOffset)
          return NS_ERROR_FAILURE;

        colorDepth = e.mBitCount;
        memcpy(&mDirEntry, &e, sizeof(IconDirEntry));
      }
    }
  }

  if (mPos < mImageOffset) {
    
    PRUint32 toSkip = mImageOffset - mPos;
    if (toSkip > aCount)
      toSkip = aCount;

    mPos    += toSkip;
    aBuffer += toSkip;
    aCount  -= toSkip;
  }

  if (mCurrIcon == mNumIcons && mPos >= mImageOffset && mPos < mImageOffset + BITMAPINFOSIZE) {
    
    PRUint32 toCopy = sizeof(mBIHraw) - (mPos - mImageOffset);
    if (toCopy > aCount)
      toCopy = aCount;

    memcpy(mBIHraw + (mPos - mImageOffset), aBuffer, toCopy);
    mPos += toCopy;
    aCount -= toCopy;
    aBuffer += toCopy;
  }

  nsresult rv;

  if (mPos == mImageOffset + BITMAPINFOSIZE) {

    ProcessInfoHeader();
    rv = mImage->SetSize(mDirEntry.mWidth, mDirEntry.mHeight);
    NS_ENSURE_SUCCESS(rv, rv);
    if (mObserver) {
      rv = mObserver->OnStartContainer(nsnull, mImage);
      NS_ENSURE_SUCCESS(rv, rv);
    }
    if (mFlags & imgIDecoder::DECODER_FLAG_HEADERONLY)
      return NS_OK;

    if (mBIH.bpp <= 8) {
      switch (mBIH.bpp) {
        case 1:
          mNumColors = 2;
          break;
        case 4:
          mNumColors = 16;
          break;
        case 8:
          mNumColors = 256;
          break;
        default:
          return NS_ERROR_FAILURE;
      }

      mColors = new colorTable[mNumColors];
      if (!mColors)
        return NS_ERROR_OUT_OF_MEMORY;
    }

    if (mIsCursor) {
      nsCOMPtr<nsIProperties> props(do_QueryInterface(mImage));
      if (props) {
        nsCOMPtr<nsISupportsPRUint32> intwrapx = do_CreateInstance("@mozilla.org/supports-PRUint32;1");
        nsCOMPtr<nsISupportsPRUint32> intwrapy = do_CreateInstance("@mozilla.org/supports-PRUint32;1");

        if (intwrapx && intwrapy) {
          intwrapx->SetData(mDirEntry.mXHotspot);
          intwrapy->SetData(mDirEntry.mYHotspot);

          props->Set("hotspotX", intwrapx);
          props->Set("hotspotY", intwrapy);
        }
      }
    }

    mCurLine = mDirEntry.mHeight;
    mRow = (PRUint8*)malloc((mDirEntry.mWidth * mBIH.bpp)/8 + 4);
    
    
    
    if (!mRow)
      return NS_ERROR_OUT_OF_MEMORY;
    
    PRUint32 imageLength;
    rv = mImage->AppendFrame(0, 0, mDirEntry.mWidth, mDirEntry.mHeight,
                             gfxASurface::ImageFormatARGB32, (PRUint8**)&mImageData, &imageLength);
    NS_ENSURE_SUCCESS(rv, rv);

    if (mObserver) {
      mObserver->OnStartFrame(nsnull, 0);
      NS_ENSURE_SUCCESS(rv, rv);
    }
  }

  if (mColors && (mPos >= mImageOffset + BITMAPINFOSIZE) && 
                 (mPos < (mImageOffset + BITMAPINFOSIZE + mNumColors * 4))) {
    
    PRUint32 colorBytes = mPos - (mImageOffset + 40); 
    PRUint8 colorNum = colorBytes / 4; 
    PRUint8 at = colorBytes % 4;
    while (aCount && (mPos < (mImageOffset + BITMAPINFOSIZE + mNumColors * 4))) {
      switch (at) {
        case 0:
          mColors[colorNum].blue = *aBuffer;
          break;
        case 1:
          mColors[colorNum].green = *aBuffer;
          break;
        case 2:
          mColors[colorNum].red = *aBuffer;
          break;
        case 3:
          colorNum++; 
          break;
      }
      mPos++; aBuffer++; aCount--;
      at = (at + 1) % 4;
    }
  }

  if (!mDecodingAndMask && (mPos >= (mImageOffset + BITMAPINFOSIZE + mNumColors*4))) {
    if (mPos == (mImageOffset + BITMAPINFOSIZE + mNumColors*4)) {
      
      mPos++;
    }

    
    
    NS_ASSERTION(mRow, "mRow is null");
    NS_ASSERTION(mImageData, "mImageData is null");
    if (!mRow || !mImageData)
      return NS_ERROR_FAILURE;

    PRUint32 rowSize = (mBIH.bpp * mDirEntry.mWidth + 7) / 8; 
    if (rowSize % 4)
      rowSize += (4 - (rowSize % 4)); 
    PRUint32 toCopy;
    do {
        toCopy = rowSize - mRowBytes;
        if (toCopy) {
            if (toCopy > aCount)
                toCopy = aCount;
            memcpy(mRow + mRowBytes, aBuffer, toCopy);
            aCount -= toCopy;
            aBuffer += toCopy;
            mRowBytes += toCopy;
        }
        if (rowSize == mRowBytes) {
            mCurLine--;
            PRUint32* d = mImageData + (mCurLine * mDirEntry.mWidth);
            PRUint8* p = mRow;
            PRUint32 lpos = mDirEntry.mWidth;
            switch (mBIH.bpp) {
              case 1:
                while (lpos > 0) {
                  PRInt8 bit;
                  PRUint8 idx;
                  for (bit = 7; bit >= 0 && lpos > 0; bit--) {
                      idx = (*p >> bit) & 1;
                      SetPixel(d, idx, mColors);
                      --lpos;
                  }
                  ++p;
                }
                break;
              case 4:
                while (lpos > 0) {
                  Set4BitPixel(d, *p, lpos, mColors);
                  ++p;
                }
                break;
              case 8:
                while (lpos > 0) {
                  SetPixel(d, *p, mColors);
                  --lpos;
                  ++p;
                }
                break;
              case 16:
                while (lpos > 0) {
                  SetPixel(d,
                          (p[1] & 124) << 1,
                          ((p[1] & 3) << 6) | ((p[0] & 224) >> 2),
                          (p[0] & 31) << 3);

                  --lpos;
                  p+=2;
                }
                break;
              case 24:
                while (lpos > 0) {
                  SetPixel(d, p[2], p[1], p[0]);
                  p += 3;
                  --lpos;
                }
                break;
              case 32:
                
                
                
                
                while (lpos > 0) {
                  if (!mHaveAlphaData && p[3]) {
                    
                    memset(mImageData + mCurLine * mDirEntry.mWidth, 0, 
                           (mDirEntry.mHeight - mCurLine) * mDirEntry.mWidth * sizeof(PRUint32));
                    mHaveAlphaData = PR_TRUE;
                  }                        
                  SetPixel(d, p[2], p[1], p[0], mHaveAlphaData ? p[3] : 0xFF);
                  p += 4;
                  --lpos;
                }
                break;
              default:
                
                return NS_ERROR_FAILURE;
            }

            if (mCurLine == 0)
              mDecodingAndMask = PR_TRUE;
              
            mRowBytes = 0;
        }
    } while (!mDecodingAndMask && aCount > 0);

  }

  if (mDecodingAndMask && !mHaveAlphaData) {
    PRUint32 rowSize = CalcAlphaRowSize();

    if (mPos == (1 + mImageOffset + BITMAPINFOSIZE + mNumColors*4)) {
      mPos++;
      mRowBytes = 0;
      mCurLine = mDirEntry.mHeight;
      mRow = (PRUint8*)realloc(mRow, rowSize);
      if (!mRow)
        return NS_ERROR_OUT_OF_MEMORY;
    }

    
    NS_ASSERTION(mRow, "mRow is null");
    NS_ASSERTION(mImageData, "mImageData is null");
    if (!mRow || !mImageData)
      return NS_ERROR_FAILURE;

    while (mCurLine > 0 && aCount > 0) {
      PRUint32 toCopy = PR_MIN(rowSize - mRowBytes, aCount);
      if (toCopy) {
        memcpy(mRow + mRowBytes, aBuffer, toCopy);
        aCount -= toCopy;
        aBuffer += toCopy;
        mRowBytes += toCopy;
      }
      if (rowSize == mRowBytes) {
        mCurLine--;
        mRowBytes = 0;

        PRUint32* decoded = mImageData + mCurLine * mDirEntry.mWidth;
        PRUint32* decoded_end = decoded + mDirEntry.mWidth;
        PRUint8* p = mRow, *p_end = mRow + rowSize; 
        while (p < p_end) {
          PRUint8 idx = *p++;
          for (PRUint8 bit = 0x80; bit && decoded<decoded_end; bit >>= 1) {
            
            if (idx & bit) *decoded = 0;
            decoded ++;
          }
        }
      }
    }
  }

  return NS_OK;
}

void
nsICODecoder::ProcessDirEntry(IconDirEntry& aTarget)
{
  memset(&aTarget, 0, sizeof(aTarget));
  memcpy(&aTarget.mWidth, mDirEntryArray, sizeof(aTarget.mWidth));
  memcpy(&aTarget.mHeight, mDirEntryArray+1, sizeof(aTarget.mHeight));
  memcpy(&aTarget.mColorCount, mDirEntryArray+2, sizeof(aTarget.mColorCount));
  memcpy(&aTarget.mReserved, mDirEntryArray+3, sizeof(aTarget.mReserved));
  
  memcpy(&aTarget.mPlanes, mDirEntryArray+4, sizeof(aTarget.mPlanes));
  aTarget.mPlanes = LITTLE_TO_NATIVE16(aTarget.mPlanes);

  memcpy(&aTarget.mBitCount, mDirEntryArray+6, sizeof(aTarget.mBitCount));
  aTarget.mBitCount = LITTLE_TO_NATIVE16(aTarget.mBitCount);

  memcpy(&aTarget.mBytesInRes, mDirEntryArray+8, sizeof(aTarget.mBytesInRes));
  aTarget.mBytesInRes = LITTLE_TO_NATIVE32(aTarget.mBytesInRes);

  memcpy(&aTarget.mImageOffset, mDirEntryArray+12, sizeof(aTarget.mImageOffset));
  aTarget.mImageOffset = LITTLE_TO_NATIVE32(aTarget.mImageOffset);
}

void nsICODecoder::ProcessInfoHeader() {
  memset(&mBIH, 0, sizeof(mBIH));
  

  memcpy(&mBIH.width, mBIHraw + 4, sizeof(mBIH.width));
  memcpy(&mBIH.height, mBIHraw + 8, sizeof(mBIH.height));
  memcpy(&mBIH.planes, mBIHraw + 12, sizeof(mBIH.planes));
  memcpy(&mBIH.bpp, mBIHraw + 14, sizeof(mBIH.bpp));
  memcpy(&mBIH.compression, mBIHraw + 16, sizeof(mBIH.compression));
  memcpy(&mBIH.image_size, mBIHraw + 20, sizeof(mBIH.image_size));
  memcpy(&mBIH.xppm, mBIHraw + 24, sizeof(mBIH.xppm));
  memcpy(&mBIH.yppm, mBIHraw + 28, sizeof(mBIH.yppm));
  memcpy(&mBIH.colors, mBIHraw + 32, sizeof(mBIH.colors));
  memcpy(&mBIH.important_colors, mBIHraw + 36, sizeof(mBIH.important_colors));

  
  mBIH.width = LITTLE_TO_NATIVE32(mBIH.width);
  mBIH.height = LITTLE_TO_NATIVE32(mBIH.height);
  mBIH.planes = LITTLE_TO_NATIVE16(mBIH.planes);
  mBIH.bpp = LITTLE_TO_NATIVE16(mBIH.bpp);

  mBIH.compression = LITTLE_TO_NATIVE32(mBIH.compression);
  mBIH.image_size = LITTLE_TO_NATIVE32(mBIH.image_size);
  mBIH.xppm = LITTLE_TO_NATIVE32(mBIH.xppm);
  mBIH.yppm = LITTLE_TO_NATIVE32(mBIH.yppm);
  mBIH.colors = LITTLE_TO_NATIVE32(mBIH.colors);
  mBIH.important_colors = LITTLE_TO_NATIVE32(mBIH.important_colors);
}
