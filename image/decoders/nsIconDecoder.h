





#ifndef nsIconDecoder_h__
#define nsIconDecoder_h__

#include "Decoder.h"

#include "nsCOMPtr.h"

#include "imgDecoderObserver.h"

namespace mozilla {
namespace image {
class RasterImage;




















class nsIconDecoder : public Decoder
{
public:

  nsIconDecoder(RasterImage &aImage, imgDecoderObserver* aObserver);
  virtual ~nsIconDecoder();

  virtual void WriteInternal(const char* aBuffer, uint32_t aCount);

  uint8_t mWidth;
  uint8_t mHeight;
  uint32_t mPixBytesRead;
  uint32_t mPixBytesTotal;
  uint8_t* mImageData;
  uint32_t mState;
};

enum {
  iconStateStart      = 0,
  iconStateHaveHeight = 1,
  iconStateReadPixels = 2,
  iconStateFinished   = 3
};

} 
} 

#endif 
