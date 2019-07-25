







































#ifndef nsIconDecoder_h__
#define nsIconDecoder_h__

#include "imgIDecoder.h"

#include "nsCOMPtr.h"

#include "imgIContainer.h"
#include "imgIDecoderObserver.h"

namespace mozilla {
namespace imagelib {
class RasterImage;
} 
} 




















class nsIconDecoder : public imgIDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  nsIconDecoder();
  virtual ~nsIconDecoder();

  nsRefPtr<mozilla::imagelib::RasterImage> mImage;
  nsCOMPtr<imgIDecoderObserver> mObserver;
  PRUint32 mFlags;
  PRUint8 mWidth;
  PRUint8 mHeight;
  PRUint32 mPixBytesRead;
  PRUint32 mPixBytesTotal;
  PRUint8* mImageData;
  PRUint32 mState;

  PRBool mNotifiedDone;
  void NotifyDone(PRBool aSuccess);
};

enum {
  iconStateStart      = 0,
  iconStateHaveHeight = 1,
  iconStateReadPixels = 2,
  iconStateFinished   = 3,
  iconStateError      = 4
};


#endif 
