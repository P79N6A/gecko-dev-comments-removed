





#ifndef nsIconDecoder_h__
#define nsIconDecoder_h__

#include "Decoder.h"

#include "nsCOMPtr.h"

namespace mozilla {
namespace image {
class RasterImage;




















class nsIconDecoder : public Decoder
{
public:

  explicit nsIconDecoder(RasterImage &aImage);
  virtual ~nsIconDecoder();

  virtual void WriteInternal(const char* aBuffer, uint32_t aCount, DecodeStrategy aStrategy);

  uint8_t mWidth;
  uint8_t mHeight;
  uint32_t mPixBytesRead;
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
