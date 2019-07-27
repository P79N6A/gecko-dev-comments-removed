







#include <stdlib.h>

#include "mozilla/Endian.h"
#include "nsICODecoder.h"

#include "RasterImage.h"

namespace mozilla {
namespace image {

#define ICONCOUNTOFFSET 4
#define DIRENTRYOFFSET 6
#define BITMAPINFOSIZE 40
#define PREFICONSIZE 16





uint32_t
nsICODecoder::CalcAlphaRowSize() 
{
  
  uint32_t rowSize = (GetRealWidth() + 31) / 32; 
  return rowSize * 4; 
}


uint16_t
nsICODecoder::GetNumColors() 
{
  uint16_t numColors = 0;
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
      numColors = (uint16_t)-1;
    }
  }
  return numColors;
}


nsICODecoder::nsICODecoder(RasterImage &aImage)
 : Decoder(aImage)
{
  mPos = mImageOffset = mCurrIcon = mNumIcons = mBPP = mRowBytes = 0;
  mIsPNG = false;
  mRow = nullptr;
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







bool nsICODecoder::FillBitmapFileHeaderBuffer(int8_t *bfh) 
{
  memset(bfh, 0, 14);
  bfh[0] = 'B';
  bfh[1] = 'M';
  int32_t dataOffset = 0;
  int32_t fileSize = 0;
  dataOffset = BFH_LENGTH + BITMAPINFOSIZE;

  
  if (mDirEntry.mBitCount <= 8) {
    uint16_t numColors = GetNumColors();
    if (numColors == (uint16_t)-1) {
      return false;
    }
    dataOffset += 4 * numColors;
    fileSize = dataOffset + GetRealWidth() * GetRealHeight();
  } else {
    fileSize = dataOffset + (mDirEntry.mBitCount * GetRealWidth() * 
                             GetRealHeight()) / 8;
  }

  NativeEndian::swapToLittleEndianInPlace(&fileSize, 1);
  memcpy(bfh + 2, &fileSize, sizeof(fileSize));
  NativeEndian::swapToLittleEndianInPlace(&dataOffset, 1);
  memcpy(bfh + 10, &dataOffset, sizeof(dataOffset));
  return true;
}




bool
nsICODecoder::FixBitmapHeight(int8_t *bih) 
{
  
  int32_t height;
  memcpy(&height, bih + 8, sizeof(height));
  NativeEndian::swapFromLittleEndianInPlace(&height, 1);
  
  height = abs(height);

  
  
  height /= 2;

  if (height > 256) {
    return false;
  }

  
  
  if (height == 256) {
    mDirEntry.mHeight = 0;
  } else {
    mDirEntry.mHeight = (int8_t)height;
  }

  
  NativeEndian::swapToLittleEndianInPlace(&height, 1);
  memcpy(bih + 8, &height, sizeof(height));
  return true;
}



bool
nsICODecoder::FixBitmapWidth(int8_t *bih) 
{
  
  int32_t width;
  memcpy(&width, bih + 4, sizeof(width));
  NativeEndian::swapFromLittleEndianInPlace(&width, 1);
  if (width > 256) {
    return false;
  }

  
  
  if (width == 256) {
    mDirEntry.mWidth = 0;
  } else {
    mDirEntry.mWidth = (int8_t)width;
  }
  return true;
}



int32_t 
nsICODecoder::ExtractBPPFromBitmap(int8_t *bih)
{
  int32_t bitsPerPixel;
  memcpy(&bitsPerPixel, bih + 14, sizeof(bitsPerPixel));
  NativeEndian::swapFromLittleEndianInPlace(&bitsPerPixel, 1);
  return bitsPerPixel;
}

int32_t 
nsICODecoder::ExtractBIHSizeFromBitmap(int8_t *bih)
{
  int32_t headerSize;
  memcpy(&headerSize, bih, sizeof(headerSize));
  NativeEndian::swapFromLittleEndianInPlace(&headerSize, 1);
  return headerSize;
}

void
nsICODecoder::SetHotSpotIfCursor() {
  if (!mIsCursor) {
    return;
  }

  mImageMetadata.SetHotspot(mDirEntry.mXHotspot, mDirEntry.mYHotspot);
}

void
nsICODecoder::WriteInternal(const char* aBuffer, uint32_t aCount, DecodeStrategy aStrategy)
{
  NS_ABORT_IF_FALSE(!HasError(), "Shouldn't call WriteInternal after error!");

  if (!aCount) {
    if (mContainedDecoder) {
      WriteToContainedDecoder(aBuffer, aCount, aStrategy);
    }
    return;
  }

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
    mNumIcons = LittleEndian::readUint16(reinterpret_cast<const uint16_t*>(aBuffer));
    aBuffer += 2;
    mPos += 2;
    aCount -= 2;
  }

  if (mNumIcons == 0)
    return; 

  uint16_t colorDepth = 0;
  nsIntSize prefSize = mImage.GetRequestedResolution();
  if (prefSize.width == 0 && prefSize.height == 0) {
    prefSize.SizeTo(PREFICONSIZE, PREFICONSIZE);
  }

  
  
  
  
  int32_t diff = INT_MIN;

  
  while (mCurrIcon < mNumIcons) { 
    if (mPos >= DIRENTRYOFFSET + (mCurrIcon * sizeof(mDirEntryArray)) && 
        mPos < DIRENTRYOFFSET + ((mCurrIcon + 1) * sizeof(mDirEntryArray))) {
      uint32_t toCopy = sizeof(mDirEntryArray) - 
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
      
      
      
      
      
      int32_t delta = (e.mWidth == 0 ? 256 : e.mWidth) - prefSize.width +
                      (e.mHeight == 0 ? 256 : e.mHeight) - prefSize.height;
      if (e.mBitCount >= colorDepth &&
          ((diff < 0 && delta >= diff) || (delta >= 0 && delta <= diff))) {
        diff = delta;
        mImageOffset = e.mImageOffset;

        
        uint32_t minImageOffset = DIRENTRYOFFSET + 
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
    
    uint32_t toSkip = mImageOffset - mPos;
    if (toSkip > aCount)
      toSkip = aCount;

    mPos    += toSkip;
    aBuffer += toSkip;
    aCount  -= toSkip;
  }

  
  
  
  if (mCurrIcon == mNumIcons && mPos >= mImageOffset && 
      mPos < mImageOffset + PNGSIGNATURESIZE)
  {
    uint32_t toCopy = PNGSIGNATURESIZE - (mPos - mImageOffset);
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
      mContainedDecoder = new nsPNGDecoder(mImage);
      mContainedDecoder->SetObserver(mObserver);
      mContainedDecoder->SetSizeDecode(IsSizeDecode());
      mContainedDecoder->InitSharedDecoder(mImageData, mImageDataLength,
                                           mColormap, mColormapSize,
                                           mCurrentFrame);
      if (!WriteToContainedDecoder(mSignature, PNGSIGNATURESIZE, aStrategy)) {
        return;
      }
    }
  }

  
  if (mIsPNG && mContainedDecoder && mPos >= mImageOffset + PNGSIGNATURESIZE) {
    if (!WriteToContainedDecoder(aBuffer, aCount, aStrategy)) {
      return;
    }

    if (!HasSize() && mContainedDecoder->HasSize()) {
      PostSize(mContainedDecoder->GetImageMetadata().GetWidth(),
               mContainedDecoder->GetImageMetadata().GetHeight());
    }

    mPos += aCount;
    aBuffer += aCount;
    aCount = 0;

    
    
    if (!IsSizeDecode() &&
        !static_cast<nsPNGDecoder*>(mContainedDecoder.get())->IsValidICO()) {
      PostDataError();
    }
    return;
  }

  
  
  if (!mIsPNG && mCurrIcon == mNumIcons && mPos >= mImageOffset && 
      mPos >= mImageOffset + PNGSIGNATURESIZE && 
      mPos < mImageOffset + BITMAPINFOSIZE) {

    
    
    
    
    memcpy(mBIHraw, mSignature, PNGSIGNATURESIZE);

    
    uint32_t toCopy = sizeof(mBIHraw) - (mPos - mImageOffset);
    if (toCopy > aCount)
      toCopy = aCount;

    memcpy(mBIHraw + (mPos - mImageOffset), aBuffer, toCopy);
    mPos += toCopy;
    aCount -= toCopy;
    aBuffer += toCopy;
  }

  
  if (!mIsPNG && mPos == mImageOffset + BITMAPINFOSIZE) {

    
    int32_t bihSize = ExtractBIHSizeFromBitmap(reinterpret_cast<int8_t*>(mBIHraw));
    if (bihSize != BITMAPINFOSIZE) {
      PostDataError();
      return;
    }
    
    
    mBPP = ExtractBPPFromBitmap(reinterpret_cast<int8_t*>(mBIHraw));
    
    
    
    
    nsBMPDecoder *bmpDecoder = new nsBMPDecoder(mImage);
    mContainedDecoder = bmpDecoder;
    bmpDecoder->SetUseAlphaData(true);
    mContainedDecoder->SetObserver(mObserver);
    mContainedDecoder->SetSizeDecode(IsSizeDecode());
    mContainedDecoder->InitSharedDecoder(mImageData, mImageDataLength,
                                         mColormap, mColormapSize,
                                         mCurrentFrame);

    
    
    
    int8_t bfhBuffer[BMPFILEHEADERSIZE];
    if (!FillBitmapFileHeaderBuffer(bfhBuffer)) {
      PostDataError();
      return;
    }
    if (!WriteToContainedDecoder((const char*)bfhBuffer, sizeof(bfhBuffer), aStrategy)) {
      return;
    }

    
    SetHotSpotIfCursor();

    
    
    if (!FixBitmapHeight(reinterpret_cast<int8_t*>(mBIHraw))) {
      PostDataError();
      return;
    }

    
    if (!FixBitmapWidth(reinterpret_cast<int8_t*>(mBIHraw))) {
      PostDataError();
      return;
    }

    
    if (!WriteToContainedDecoder(mBIHraw, sizeof(mBIHraw), aStrategy)) {
      return;
    }

    PostSize(mContainedDecoder->GetImageMetadata().GetWidth(),
             mContainedDecoder->GetImageMetadata().GetHeight());

    
    
    if (IsSizeDecode())
      return;

    
    
    
    mBPP = bmpDecoder->GetBitsPerPixel();

    
    uint16_t numColors = GetNumColors();
    if (numColors == (uint16_t)-1) {
      PostDataError();
      return;
    }
  }

  
  if (!mIsPNG && mContainedDecoder && mPos >= mImageOffset + BITMAPINFOSIZE) {
    uint16_t numColors = GetNumColors();
    if (numColors == (uint16_t)-1) {
      PostDataError();
      return;
    }
    
    uint32_t bmpDataOffset = mDirEntry.mImageOffset + BITMAPINFOSIZE;
    uint32_t bmpDataEnd = mDirEntry.mImageOffset + BITMAPINFOSIZE + 
                          static_cast<nsBMPDecoder*>(mContainedDecoder.get())->GetCompressedImageSize() +
                          4 * numColors;

    
    
    if (mPos >= bmpDataOffset && mPos < bmpDataEnd) {

      
      uint32_t toFeed = bmpDataEnd - mPos;
      if (toFeed > aCount) {
        toFeed = aCount;
      }

      if (!WriteToContainedDecoder(aBuffer, toFeed, aStrategy)) {
        return;
      }

      mPos += toFeed;
      aCount -= toFeed;
      aBuffer += toFeed;
    }
  
    
    
    if (!mIsPNG && mPos >= bmpDataEnd) {
      
      
      
      
      if (static_cast<nsBMPDecoder*>(mContainedDecoder.get())->GetBitsPerPixel() != 32 || 
          !static_cast<nsBMPDecoder*>(mContainedDecoder.get())->HasAlphaData()) {
        uint32_t rowSize = ((GetRealWidth() + 31) / 32) * 4; 
        if (mPos == bmpDataEnd) {
          mPos++;
          mRowBytes = 0;
          mCurLine = GetRealHeight();
          mRow = (uint8_t*)moz_realloc(mRow, rowSize);
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
          uint32_t toCopy = std::min(rowSize - mRowBytes, aCount);
          if (toCopy) {
            memcpy(mRow + mRowBytes, aBuffer, toCopy);
            aCount -= toCopy;
            aBuffer += toCopy;
            mRowBytes += toCopy;
          }
          if (rowSize == mRowBytes) {
            mCurLine--;
            mRowBytes = 0;

            uint32_t* imageData = 
              static_cast<nsBMPDecoder*>(mContainedDecoder.get())->GetImageData();
            if (!imageData) {
              PostDataError();
              return;
            }
            uint32_t* decoded = imageData + mCurLine * GetRealWidth();
            uint32_t* decoded_end = decoded + GetRealWidth();
            uint8_t* p = mRow;
            uint8_t* p_end = mRow + rowSize;
            while (p < p_end) {
              uint8_t idx = *p++;
              for (uint8_t bit = 0x80; bit && decoded<decoded_end; bit >>= 1) {
                
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
nsICODecoder::WriteToContainedDecoder(const char* aBuffer, uint32_t aCount, DecodeStrategy aStrategy)
{
  mContainedDecoder->Write(aBuffer, aCount, aStrategy);
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
  aTarget.mPlanes = LittleEndian::readUint16(&aTarget.mPlanes);
  memcpy(&aTarget.mBitCount, mDirEntryArray + 6, sizeof(aTarget.mBitCount));
  aTarget.mBitCount = LittleEndian::readUint16(&aTarget.mBitCount);
  memcpy(&aTarget.mBytesInRes, mDirEntryArray + 8, sizeof(aTarget.mBytesInRes));
  aTarget.mBytesInRes = LittleEndian::readUint32(&aTarget.mBytesInRes);
  memcpy(&aTarget.mImageOffset, mDirEntryArray + 12, 
         sizeof(aTarget.mImageOffset));
  aTarget.mImageOffset = LittleEndian::readUint32(&aTarget.mImageOffset);
}

bool
nsICODecoder::NeedsNewFrame() const
{
  if (mContainedDecoder) {
    return mContainedDecoder->NeedsNewFrame();
  }

  return Decoder::NeedsNewFrame();
}

nsresult
nsICODecoder::AllocateFrame()
{
  if (mContainedDecoder) {
    nsresult rv = mContainedDecoder->AllocateFrame();
    mCurrentFrame = mContainedDecoder->GetCurrentFrame();
    return rv;
  }

  return Decoder::AllocateFrame();
}

} 
} 
