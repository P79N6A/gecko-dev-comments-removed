







































#ifndef nsIconDecoder_h__
#define nsIconDecoder_h__

#include "imgIDecoder.h"

#include "nsCOMPtr.h"

#include "imgIContainer.h"
#include "imgIDecoderObserver.h"

#define NS_ICONDECODER_CID                           \
{ /* FFC08380-256C-11d5-9905-001083010E9B */         \
     0xffc08380,                                     \
     0x256c,                                         \
     0x11d5,                                         \
    { 0x99, 0x5, 0x0, 0x10, 0x83, 0x1, 0xe, 0x9b }   \
}





















class nsIconDecoder : public imgIDecoder
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_IMGIDECODER

  nsIconDecoder();
  virtual ~nsIconDecoder();

  nsCOMPtr<imgIContainer> mImage;
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
