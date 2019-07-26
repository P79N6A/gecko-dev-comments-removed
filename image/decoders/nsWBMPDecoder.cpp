





#include "nsWBMPDecoder.h"
#include "RasterImage.h"
#include "nspr.h"
#include "nsRect.h"
#include "gfxPlatform.h"

#include "nsError.h"

namespace mozilla {
namespace image {

static inline void SetPixel(uint32_t*& aDecoded, bool aPixelWhite)
{
  uint8_t pixelValue = aPixelWhite ? 255 : 0;

  *aDecoded++ = gfxPackedPixel(0xFF, pixelValue, pixelValue, pixelValue);
}















static WbmpIntDecodeStatus DecodeEncodedInt (uint32_t& aField, const char*& aBuffer, uint32_t& aCount)
{
  while (aCount > 0) {
    
    
    if (aField & 0xFE000000) {
      
      return IntParseFailed;
    }

    
    char encodedByte = *aBuffer;

    
    aBuffer++;
    aCount--;

    
    aField = (aField << 7) + (uint32_t)(encodedByte & 0x7F);

    if (!(encodedByte & 0x80)) {
      
      return IntParseSucceeded;
    }
  }

  
  return IntParseInProgress;
}

nsWBMPDecoder::nsWBMPDecoder(RasterImage &aImage)
 : Decoder(aImage),
   mWidth(0),
   mHeight(0),
   mRow(nullptr),
   mRowBytes(0),
   mCurLine(0),
   mState(WbmpStateStart)
{
  
}

nsWBMPDecoder::~nsWBMPDecoder()
{
  moz_free(mRow);
}

void
nsWBMPDecoder::WriteInternal(const char *aBuffer, uint32_t aCount)
{
  NS_ABORT_IF_FALSE(!HasError(), "Shouldn't call WriteInternal after error!");

  
  while (aCount > 0) {
    switch (mState) {
      case WbmpStateStart:
      {
        
        
        if (*aBuffer++ == 0x00) {
          
          aCount--;
          mState = DecodingFixHeader;
        } else {
          
          PostDataError();
          mState = DecodingFailed;
          return;
        }
        break;
      }

      case DecodingFixHeader:
      {
        if ((*aBuffer++ & 0x9F) == 0x00) {
          
          aCount--;
          
          mState = DecodingWidth;
        } else {
          
          PostDataError();
          mState = DecodingFailed;
          return;
        }
        break;
      }

      case DecodingWidth:
      {
        WbmpIntDecodeStatus widthReadResult = DecodeEncodedInt (mWidth, aBuffer, aCount);

        if (widthReadResult == IntParseSucceeded) {
          mState = DecodingHeight;
        } else if (widthReadResult == IntParseFailed) {
          
          PostDataError();
          mState = DecodingFailed;
          return;
        } else {
          
          NS_ABORT_IF_FALSE((widthReadResult == IntParseInProgress),
                            "nsWBMPDecoder got bad result from an encoded width field");
          return;
        }
        break;
      }

      case DecodingHeight:
      {
        WbmpIntDecodeStatus heightReadResult = DecodeEncodedInt (mHeight, aBuffer, aCount);

        if (heightReadResult == IntParseSucceeded) {
          
          const uint32_t k64KWidth = 0x0000FFFF;
          if (mWidth == 0 || mWidth > k64KWidth
              || mHeight == 0 || mHeight > k64KWidth) {
            
            
            PostDataError();
            mState = DecodingFailed;
            return;
          }

          
          PostSize(mWidth, mHeight);
          if (HasError()) {
            
            mState = DecodingFailed;
            return;
          }

          
          if (IsSizeDecode()) {
            mState = WbmpStateFinished;
            return;
          }

          if (!mImageData) {
            PostDecoderError(NS_ERROR_FAILURE);
            mState = DecodingFailed;
            return;
          }

          
          mRow = (uint8_t*)moz_malloc((mWidth + 7) / 8);
          if (!mRow) {
            PostDecoderError(NS_ERROR_OUT_OF_MEMORY);
            mState = DecodingFailed;
            return;
          }

          mState = DecodingImageData;

        } else if (heightReadResult == IntParseFailed) {
          
          PostDataError();
          mState = DecodingFailed;
          return;
        } else {
          
          NS_ABORT_IF_FALSE((heightReadResult == IntParseInProgress),
                            "nsWBMPDecoder got bad result from an encoded height field");
          return;
        }
        break;
      }

      case DecodingImageData:
      {
        uint32_t rowSize = (mWidth + 7) / 8; 
        uint32_t top = mCurLine;

        
        while ((aCount > 0) && (mCurLine < mHeight)) {
          
          uint32_t toCopy = rowSize - mRowBytes;

          
          if (toCopy) {
            if (toCopy > aCount)
              toCopy = aCount;
            memcpy(mRow + mRowBytes, aBuffer, toCopy);
            aCount -= toCopy;
            aBuffer += toCopy;
            mRowBytes += toCopy;
          }

          
          if (rowSize == mRowBytes) {
            uint8_t *p = mRow;
            uint32_t *d = reinterpret_cast<uint32_t*>(mImageData) + (mWidth * mCurLine); 
            uint32_t lpos = 0;

            while (lpos < mWidth) {
              for (int8_t bit = 7; bit >= 0; bit--) {
                if (lpos >= mWidth)
                  break;
                bool pixelWhite = (*p >> bit) & 1;
                SetPixel(d, pixelWhite);
                ++lpos;
              }
              ++p;
            }

            mCurLine++;
            mRowBytes = 0;
          }
        }

        nsIntRect r(0, top, mWidth, mCurLine - top);
        
        PostInvalidation(r);

        
        if (mCurLine == mHeight) {
          PostFrameStop();
          PostDecodeDone();
          mState = WbmpStateFinished;
        }
        break;
      }

      case WbmpStateFinished:
      {
        
        aCount = 0;
        break;
      }

      case DecodingFailed:
      {
        NS_ABORT_IF_FALSE(0, "Shouldn't process any data after decode failed!");
        return;
      }
    }
  }
}

} 
} 
