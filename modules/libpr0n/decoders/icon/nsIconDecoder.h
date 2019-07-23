






































#ifndef nsIconDecoder_h__
#define nsIconDecoder_h__

#include "imgIDecoder.h"

#include "nsCOMPtr.h"

#include "imgIContainer.h"
#include "imgIDecoderObserver.h"
#include "gfxIImageFrame.h"

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

private:
  nsCOMPtr<imgIContainer> mImage;
  nsCOMPtr<gfxIImageFrame> mFrame;
  nsCOMPtr<imgIDecoderObserver> mObserver; 
};

#endif 
