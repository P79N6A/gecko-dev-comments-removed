






































#ifndef nsDeviceCaptureProvider_h_
#define nsDeviceCaptureProvider_h_

#include "nsIInputStream.h"

struct nsCaptureParams {
  PRPackedBool captureAudio;
  PRPackedBool captureVideo;
  PRUint32 frameRate;
  PRUint32 frameLimit;
  PRUint32 timeLimit;
  PRUint32 width;
  PRUint32 height;
  PRUint32 bpp;
  PRUint32 camera;
};

class nsDeviceCaptureProvider : public nsISupports
{
public:
  virtual nsresult Init(nsACString& aContentType,
                        nsCaptureParams* aParams,
                        nsIInputStream** aStream) = 0;
};

#endif
