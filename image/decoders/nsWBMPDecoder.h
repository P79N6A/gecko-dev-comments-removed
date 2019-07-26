





#ifndef nsWBMPDecoder_h__
#define nsWBMPDecoder_h__

#include "Decoder.h"
#include "nsCOMPtr.h"
#include "imgDecoderObserver.h"
#include "gfxColor.h"

namespace mozilla {
namespace image {
class RasterImage;





typedef enum {
  WbmpStateStart,
  DecodingFixHeader,
  DecodingWidth,
  DecodingHeight,
  DecodingImageData,
  DecodingFailed,
  WbmpStateFinished
} WbmpDecodingState;

typedef enum {
  IntParseSucceeded,
  IntParseFailed,
  IntParseInProgress
} WbmpIntDecodeStatus;

class nsWBMPDecoder : public Decoder
{
public:

  nsWBMPDecoder(RasterImage &aImage, imgDecoderObserver* aObserver);
  virtual ~nsWBMPDecoder();

  virtual void WriteInternal(const char* aBuffer, uint32_t aCount);

private:
  uint32_t mWidth;
  uint32_t mHeight;

  uint32_t *mImageData;

  uint8_t* mRow;                    
  uint32_t mRowBytes;               
  uint32_t mCurLine;                

  WbmpDecodingState mState;         
};

} 
} 

#endif 
