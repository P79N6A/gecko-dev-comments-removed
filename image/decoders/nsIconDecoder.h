







































#ifndef nsIconDecoder_h__
#define nsIconDecoder_h__

#include "Decoder.h"

#include "nsCOMPtr.h"

#include "imgIDecoderObserver.h"

namespace mozilla {
namespace imagelib {
class RasterImage;




















class nsIconDecoder : public Decoder
{
public:

  nsIconDecoder(RasterImage &aImage, imgIDecoderObserver* aObserver);
  virtual ~nsIconDecoder();

  virtual void WriteInternal(const char* aBuffer, PRUint32 aCount);

  PRUint8 mWidth;
  PRUint8 mHeight;
  PRUint32 mPixBytesRead;
  PRUint32 mPixBytesTotal;
  PRUint8* mImageData;
  PRUint32 mState;
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
