





#include "nsIconDecoder.h"
#include "nsIInputStream.h"
#include "nspr.h"
#include "nsRect.h"
#include "nsError.h"
#include "RasterImage.h"
#include <algorithm>

namespace mozilla {
namespace image {

nsIconDecoder::nsIconDecoder(RasterImage* aImage)
 : Decoder(aImage),
   mWidth(-1),
   mHeight(-1),
   mPixBytesRead(0),
   mState(iconStateStart)
{
  
}

nsIconDecoder::~nsIconDecoder()
{ }

void
nsIconDecoder::WriteInternal(const char* aBuffer, uint32_t aCount)
{
  MOZ_ASSERT(!HasError(), "Shouldn't call WriteInternal after error!");

  
  
  uint32_t bytesToRead = 0;

  
  while (aCount > 0) {
    switch (mState) {
      case iconStateStart:

        
        mWidth = (uint8_t)*aBuffer;

        
        aBuffer++;
        aCount--;
        mState = iconStateHaveHeight;
        break;

      case iconStateHaveHeight:

        
        mHeight = (uint8_t)*aBuffer;

        
        PostSize(mWidth, mHeight);

        PostHasTransparency();

        if (HasError()) {
          
          mState = iconStateFinished;
          return;
        }

        
        if (IsSizeDecode()) {
          mState = iconStateFinished;
          break;
        }

        {
          MOZ_ASSERT(!mImageData, "Already have a buffer allocated?");
          nsresult rv = AllocateBasicFrame();
          if (NS_FAILED(rv)) {
            mState = iconStateFinished;
            return;
          }
        }

        MOZ_ASSERT(mImageData, "Should have a buffer now");

        
        aBuffer++;
        aCount--;
        mState = iconStateReadPixels;
        break;

      case iconStateReadPixels: {

        
        bytesToRead = std::min(aCount, mImageDataLength - mPixBytesRead);

        
        memcpy(mImageData + mPixBytesRead, aBuffer, bytesToRead);

        
        
        nsIntRect r(0, 0, mWidth, mHeight);

        
        PostInvalidation(r);

        
        aBuffer += bytesToRead;
        aCount -= bytesToRead;
        mPixBytesRead += bytesToRead;

        
        if (mPixBytesRead == mImageDataLength) {
          PostFrameStop();
          PostDecodeDone();
          mState = iconStateFinished;
        }
        break;
      }

      case iconStateFinished:

        
        aCount = 0;

        break;
    }
  }
}

} 
} 
