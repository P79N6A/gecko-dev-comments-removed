





#include "nsIconDecoder.h"
#include "nsIInputStream.h"
#include "RasterImage.h"
#include "nspr.h"
#include "nsRect.h"

#include "nsError.h"
#include <algorithm>

namespace mozilla {
namespace image {

nsIconDecoder::nsIconDecoder(RasterImage &aImage)
 : Decoder(aImage),
   mWidth(-1),
   mHeight(-1),
   mPixBytesRead(0),
   mPixBytesTotal(0),
   mImageData(nullptr),
   mState(iconStateStart)
{
  
}

nsIconDecoder::~nsIconDecoder()
{ }

void
nsIconDecoder::WriteInternal(const char *aBuffer, uint32_t aCount)
{
  NS_ABORT_IF_FALSE(!HasError(), "Shouldn't call WriteInternal after error!");

  
  
  uint32_t bytesToRead = 0;
  nsresult rv;

  
  
  nsIntRect r(0, 0, mWidth, mHeight);

  
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
        if (HasError()) {
          
          mState = iconStateFinished;
          return;
        }

        
        if (IsSizeDecode()) {
          mState = iconStateFinished;
          break;
        }

        
        rv = mImage.EnsureFrame(0, 0, 0, mWidth, mHeight,
                                gfxASurface::ImageFormatARGB32,
                                &mImageData, &mPixBytesTotal);
        if (NS_FAILED(rv)) {
          PostDecoderError(rv);
          return;
        }

        
        PostFrameStart();

        
        aBuffer++;
        aCount--;
        mState = iconStateReadPixels;
        break;

      case iconStateReadPixels:

        
        bytesToRead = std::min(aCount, mPixBytesTotal - mPixBytesRead);

        
        memcpy(mImageData + mPixBytesRead, aBuffer, bytesToRead);

        
        PostInvalidation(r);

        
        aBuffer += bytesToRead;
        aCount -= bytesToRead;
        mPixBytesRead += bytesToRead;

        
        if (mPixBytesRead == mPixBytesTotal) {
          PostFrameStop();
          PostDecodeDone();
          mState = iconStateFinished;
        }
        break;

      case iconStateFinished:

        
        aCount = 0;

        break;
    }
  }
}

} 
} 
