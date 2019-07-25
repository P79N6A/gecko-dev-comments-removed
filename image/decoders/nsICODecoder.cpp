







#include <stdlib.h>

#include "EndianMacros.h"
#include "nsICODecoder.h"

#include "nsIInputStream.h"
#include "nsIComponentManager.h"
#include "RasterImage.h"
#include "imgIContainerObserver.h"

#include "nsIProperties.h"
#include "nsISupportsPrimitives.h"

namespace mozilla {
namespace image {

#define ICONCOUNTOFFSET 4
#define DIRENTRYOFFSET 6
#define BITMAPINFOSIZE 40
#define PREFICONSIZE 16





PRUint32
nsICODecoder::CalcAlphaRowSize() 
{
  
  PRUint32 rowSize = (GetRealWidth() + 31) / 32; 
  return rowSize * 4; 
}


PRUint16
nsICODecoder::GetNumColors() 
{
  PRUint16 numColors = 0;
  if (mBPP <= 8) {
    switch (mBPP) {
    case 1:
      numColors = 2;
      break;
    case 4:
      numColors = 16;
      break;
    case 8:
      numColors = 256;
      break;
    default:
      numColors = (PRUint16)-1;
    }
  }
  return numColors;
}


nsICODecoder::nsICODecoder(RasterImage &aImage, imgIDecoderObserver* aObserver)
 : Decoder(aImage, aObserver)
{
  mPos = mImageOffset = mCurrIcon = mNumIcons = mBPP = mRowBytes = 0;
  mIsPNG = false;
  mRow = nsnull;
  mOldLine = mCurLine = 1; 
}

nsICODecoder::~nsICODecoder()
{
  if (mRow) {
    moz_free(mRow);
  }
}

void
nsICODecoder::FinishInternal()
{
  
  NS_ABORT_IF_FALSE(!HasError(), "Shouldn't call FinishInternal after error!");

  
  if (mContainedDecoder) {
    mContainedDecoder->FinishSharedDecoder();
    mDecodeDone = mContainedDecoder->GetDecodeDone();
  }
}







bool nsICODecoder::FillBitmapFileHeaderBuffer(PRInt8 *bfh) 
{
  memset(bfh, 0, 14);
  bfh[0] = 'B';
  bfh[1] = 'M';
  PRInt32 dataOffset = 0;
  PRInt32 fileSize = 0;
  dataOffset = BFH_LENGTH + BITMAPINFOSIZE;

  
  if (mDirEntry.mBitCount <= 8) {
    PRUint16 numColors = GetNumColors();
    if (numColors == (PRUint16)-1) {
      return false;
    }
    dataOffset += 4 * numColors;
    fileSize = dataOffset + GetRealWidth() * GetRealHeight();
  } else {
    fileSize = dataOffset + (mDirEntry.mBitCount * GetRealWidth() * 
                             GetRealHeight()) / 8;
  }

  fileSize = NATIVE32_TO_LITTLE(fileSize);
  memcpy(bfh + 2, &fileSize, sizeof(fileSize));
  dataOffset = NATIVE32_TO_LITTLE(dataOffset);
  memcpy(bfh + 10, &dataOffset, sizeof(dataOffset));
  return true;
}




bool
nsICODecoder::FixBitmapHeight(PRInt8 *bih) 
{
  
  PRInt32 height;
  memcpy(&height, bih + 8, sizeof(height));
  height = LITTLE_TO_NATIVE32(height);
  
  height = abs(height);

  
  
  height /= 2;

  if (height > 256) {
    return false;
  }

  
  
  if (height == 256) {
    mDirEntry.mHeight = 0;
  } else {
    mDirEntry.mHeight = (PRInt8)height;
  }

  
  height = NATIVE32_TO_LITTLE(height);
  memcpy(bih + 8, &height, sizeof(height));
  return true;
}



bool
nsICODecoder::FixBitmapWidth(PRInt8 *bih) 
{
  
  PRInt32 width;
  memcpy(&width, bih + 4, sizeof(width));
  width = LITTLE_TO_NATIVE32(width);
  if (width > 256) {
    return false;
  }

  
  
  if (width == 256) {
    mDirEntry.mWidth = 0;
  } else {
    mDirEntry.mWidth = (PRInt8)width;
  }
  return true;
}



PRInt32 
nsICODecoder::ExtractBPPFromBitmap(PRInt8 *bih)
{
  PRInt32 bitsPerPixel;
  memcpy(&bitsPerPixel, bih + 14, sizeof(bitsPerPixel));
  bitsPerPixel = LITTLE_TO_NATIVE32(bitsPerPixel);
  return bitsPerPixel;
}

PRInt32 
nsICODecoder::ExtractBIHSizeFromBitmap(PRInt8 *bih)
{
  PRInt32 headerSize;
  memcpy(&headerSize, bih, sizeof(headerSize));
  headerSize = LITTLE_TO_NATIVE32(headerSize);
  return headerSize;
}

void
nsICODecoder::SetHotSpotIfCursor() {
  if (!mIsCursor) {
    return;
  }

  nsCOMPtr<nsISupportsPRUint32> intwrapx = 
    do_CreateInstance("@mozilla.org/supports-PRUint32;1");
  nsCOMPtr<nsISupportsPRUint32> intwrapy = 
    do_CreateInstance("@mozilla.org/supports-PRUint32;1");

  if (intwrapx && intwrapy) {
    intwrapx->SetData(mDirEntry.mXHotspot);
    intwrapy->SetData(mDirEntry.mYHotspot);

    mImage.Set("hotspotX", intwrapx);
    mImage.Set("hotspotY", intwrapy);
  }
}

void
nsICODecoder::WriteInternal(const char* aBuffer, PRUint32 aCount)
{
  NS_ABORT_IF_FALSE(!HasError(), "Shouldn't call WriteInternal after error!");

  if (!aCount) 
    return;

  while (aCount && (mPos < ICONCOUNTOFFSET)) { 
    if (mPos == 2) { 
      if ((*aBuffer != 1) && (*aBuffer != 2)) {
        PostDataError();
        return;
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
    return; 

  PRUint16 colorDepth = 0;
  
  while (mCurrIcon < mNumIcons) { 
    if (mPos >= DIRENTRYOFFSET + (mCurrIcon * sizeof(mDirEntryArray)) && 
        mPos < DIRENTRYOFFSET + ((mCurrIcon + 1) * sizeof(mDirEntryArray))) {
      PRUint32 toCopy = sizeof(mDirEntryArray) - 
                        (mPos - DIRENTRYOFFSET - mCurrIcon * sizeof(mDirEntryArray));
      if (toCopy > aCount) {
        toCopy = aCount;
      }
      memcpy(mDirEntryArray + sizeof(mDirEntryArray) - toCopy, aBuffer, toCopy);
      mPos += toCopy;
      aCount -= toCopy;
      aBuffer += toCopy;
    }
    if (aCount == 0)
      return; 

    IconDirEntry e;
    if (mPos == (DIRENTRYOFFSET + ICODIRENTRYSIZE) + 
                (mCurrIcon * sizeof(mDirEntryArray))) {
      mCurrIcon++;
      ProcessDirEntry(e);
      
      
      if (((e.mWidth == 0 ? 256 : e.mWidth) == PREFICONSIZE && 
           (e.mHeight == 0 ? 256 : e.mHeight) == PREFICONSIZE && 
           (e.mBitCount >= colorDepth)) ||
          (mCurrIcon == mNumIcons && mImageOffset == 0)) {
        mImageOffset = e.mImageOffset;

        
        PRUint32 minImageOffset = DIRENTRYOFFSET + 
                                  mNumIcons * sizeof(mDirEntryArray);
        if (mImageOffset < minImageOffset) {
          PostDataError();
          return;
        }

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

  
  
  
  if (mCurrIcon == mNumIcons && mPos >= mImageOffset && 
      mPos < mImageOffset + PNGSIGNATURESIZE)
  {
    PRUint32 toCopy = PNGSIGNATURESIZE - (mPos - mImageOffset);
    if (toCopy > aCount) {
      toCopy = aCount;
    }

    memcpy(mSignature + (mPos - mImageOffset), aBuffer, toCopy);
    mPos += toCopy;
    aCount -= toCopy;
    aBuffer += toCopy;

    mIsPNG = !memcmp(mSignature, nsPNGDecoder::pngSignatureBytes, 
                     PNGSIGNATURESIZE);
    if (mIsPNG) {
      mContainedDecoder = new nsPNGDecoder(mImage, mObserver);
      mContainedDecoder->InitSharedDecoder();
      if (!WriteToContainedDecoder(mSignature, PNGSIGNATURESIZE)) {
        return;
      }
    }
  }

  
  if (mIsPNG && mContainedDecoder && mPos >= mImageOffset + PNGSIGNATURESIZE) {
    if (!WriteToContainedDecoder(aBuffer, aCount)) {
      return;
    }
    mPos += aCount;
    aBuffer += aCount;
    aCount = 0;

    
    
    if (!static_cast<nsPNGDecoder*>(mContainedDecoder.get())->IsValidICO()) {
      PostDataError();
    }
    return;
  }

  
  
  if (!mIsPNG && mCurrIcon == mNumIcons && mPos >= mImageOffset && 
      mPos >= mImageOffset + PNGSIGNATURESIZE && 
      mPos < mImageOffset + BITMAPINFOSIZE) {

    
    
    
    
    memcpy(mBIHraw, mSignature, PNGSIGNATURESIZE);

    
    PRUint32 toCopy = sizeof(mBIHraw) - (mPos - mImageOffset);
    if (toCopy > aCount)
      toCopy = aCount;

    memcpy(mBIHraw + (mPos - mImageOffset), aBuffer, toCopy);
    mPos += toCopy;
    aCount -= toCopy;
    aBuffer += toCopy;
  }

  
  if (!mIsPNG && mPos == mImageOffset + BITMAPINFOSIZE) {

    
    PRInt32 bihSize = ExtractBIHSizeFromBitmap(reinterpret_cast<PRInt8*>(mBIHraw));
    if (bihSize != BITMAPINFOSIZE) {
      PostDataError();
      return;
    }
    
    
    mBPP = ExtractBPPFromBitmap(reinterpret_cast<PRInt8*>(mBIHraw));
    
    
    
    
    nsBMPDecoder *bmpDecoder = new nsBMPDecoder(mImage, mObserver); 
    mContainedDecoder = bmpDecoder;
    bmpDecoder->SetUseAlphaData(true);
    mContainedDecoder->SetSizeDecode(IsSizeDecode());
    mContainedDecoder->InitSharedDecoder();

    
    
    
    PRInt8 bfhBuffer[BMPFILEHEADERSIZE];
    if (!FillBitmapFileHeaderBuffer(bfhBuffer)) {
      PostDataError();
      return;
    }
    if (!WriteToContainedDecoder((const char*)bfhBuffer, sizeof(bfhBuffer))) {
      return;
    }

    
    SetHotSpotIfCursor();

    
    
    if (!FixBitmapHeight(reinterpret_cast<PRInt8*>(mBIHraw))) {
      PostDataError();
      return;
    }

    
    if (!FixBitmapWidth(reinterpret_cast<PRInt8*>(mBIHraw))) {
      PostDataError();
      return;
    }

    
    if (!WriteToContainedDecoder(mBIHraw, sizeof(mBIHraw))) {
      return;
    }

    
    
    if (IsSizeDecode())
      return;

    
    
    
    mBPP = bmpDecoder->GetBitsPerPixel();

    
    PRUint16 numColors = GetNumColors();
    if (numColors == (PRUint16)-1) {
      PostDataError();
      return;
    }
  }

  
  if (!mIsPNG && mContainedDecoder && mPos >= mImageOffset + BITMAPINFOSIZE) {
    PRUint16 numColors = GetNumColors();
    if (numColors == (PRUint16)-1) {
      PostDataError();
      return;
    }
    
    PRInt32 bmpDataOffset = mDirEntry.mImageOffset + BITMAPINFOSIZE;
    PRInt32 bmpDataEnd = mDirEntry.mImageOffset + BITMAPINFOSIZE + 
                         static_cast<nsBMPDecoder*>(mContainedDecoder.get())->GetCompressedImageSize() +
                         4 * numColors;

    
    
    if (mPos >= bmpDataOffset && mPos < bmpDataEnd) {

      
      PRUint32 toFeed = bmpDataEnd - mPos;
      if (toFeed > aCount) {
        toFeed = aCount;
      }

      if (!WriteToContainedDecoder(aBuffer, toFeed)) {
        return;
      }

      mPos += toFeed;
      aCount -= toFeed;
      aBuffer += toFeed;
    }
  
    
    
    if (!mIsPNG && mPos >= bmpDataEnd) {
      
      
      
      
      if (static_cast<nsBMPDecoder*>(mContainedDecoder.get())->GetBitsPerPixel() != 32 || 
          !static_cast<nsBMPDecoder*>(mContainedDecoder.get())->HasAlphaData()) {
        PRUint32 rowSize = ((GetRealWidth() + 31) / 32) * 4; 
        if (mPos == bmpDataEnd) {
          mPos++;
          mRowBytes = 0;
          mCurLine = GetRealHeight();
          mRow = (PRUint8*)moz_realloc(mRow, rowSize);
          if (!mRow) {
            PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
            return;
          }
        }

        
        NS_ABORT_IF_FALSE(mRow, "mRow is null");
        if (!mRow) {
          PostDataError();
          return;
        }

        while (mCurLine > 0 && aCount > 0) {
          PRUint32 toCopy = NS_MIN(rowSize - mRowBytes, aCount);
          if (toCopy) {
            memcpy(mRow + mRowBytes, aBuffer, toCopy);
            aCount -= toCopy;
            aBuffer += toCopy;
            mRowBytes += toCopy;
          }
          if (rowSize == mRowBytes) {
            mCurLine--;
            mRowBytes = 0;

            PRUint32* imageData = 
              static_cast<nsBMPDecoder*>(mContainedDecoder.get())->GetImageData();
            if (!imageData) {
              PostDataError();
              return;
            }
            PRUint32* decoded = imageData + mCurLine * GetRealWidth();
            PRUint32* decoded_end = decoded + GetRealWidth();
            PRUint8* p = mRow, *p_end = mRow + rowSize; 
            while (p < p_end) {
              PRUint8 idx = *p++;
              for (PRUint8 bit = 0x80; bit && decoded<decoded_end; bit >>= 1) {
                
                if (idx & bit) {
                  *decoded = 0;
                }
                decoded++;
              }
            }
          }
        }
      }
    }
  }
}

bool
nsICODecoder::WriteToContainedDecoder(const char* aBuffer, PRUint32 aCount)
{
  mContainedDecoder->Write(aBuffer, aCount);
  if (mContainedDecoder->HasDataError()) {
    mDataError = mContainedDecoder->HasDataError();
  }
  if (mContainedDecoder->HasDecoderError()) {
    PostDecoderError(mContainedDecoder->GetDecoderError());
  }
  return !HasError();
}

void
nsICODecoder::ProcessDirEntry(IconDirEntry& aTarget)
{
  memset(&aTarget, 0, sizeof(aTarget));
  memcpy(&aTarget.mWidth, mDirEntryArray, sizeof(aTarget.mWidth));
  memcpy(&aTarget.mHeight, mDirEntryArray + 1, sizeof(aTarget.mHeight));
  memcpy(&aTarget.mColorCount, mDirEntryArray + 2, sizeof(aTarget.mColorCount));
  memcpy(&aTarget.mReserved, mDirEntryArray + 3, sizeof(aTarget.mReserved));
  memcpy(&aTarget.mPlanes, mDirEntryArray + 4, sizeof(aTarget.mPlanes));
  aTarget.mPlanes = LITTLE_TO_NATIVE16(aTarget.mPlanes);
  memcpy(&aTarget.mBitCount, mDirEntryArray + 6, sizeof(aTarget.mBitCount));
  aTarget.mBitCount = LITTLE_TO_NATIVE16(aTarget.mBitCount);
  memcpy(&aTarget.mBytesInRes, mDirEntryArray + 8, sizeof(aTarget.mBytesInRes));
  aTarget.mBytesInRes = LITTLE_TO_NATIVE32(aTarget.mBytesInRes);
  memcpy(&aTarget.mImageOffset, mDirEntryArray + 12, 
         sizeof(aTarget.mImageOffset));
  aTarget.mImageOffset = LITTLE_TO_NATIVE32(aTarget.mImageOffset);
}

} 
} 
