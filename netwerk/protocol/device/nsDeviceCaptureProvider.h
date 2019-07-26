




#ifndef nsDeviceCaptureProvider_h_
#define nsDeviceCaptureProvider_h_

#include "nsIInputStream.h"

struct nsCaptureParams {
  bool captureAudio;
  bool captureVideo;
  uint32_t frameRate;
  uint32_t frameLimit;
  uint32_t timeLimit;
  uint32_t width;
  uint32_t height;
  uint32_t bpp;
  uint32_t camera;
};

class nsDeviceCaptureProvider : public nsISupports
{
public:
  virtual nsresult Init(nsACString& aContentType,
                        nsCaptureParams* aParams,
                        nsIInputStream** aStream) = 0;
};

#endif
