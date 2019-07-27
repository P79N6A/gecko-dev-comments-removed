




#if !defined(SoftwareWebMVideoDecoder_h_)
#define SoftwareWebMVideoDecoder_h_

#include <stdint.h>

#include "WebMReader.h"

namespace mozilla {

class SoftwareWebMVideoDecoder : public WebMVideoDecoder
{
public:
  static WebMVideoDecoder* Create(WebMReader* aReader);

  virtual nsresult Init(unsigned int aWidth, unsigned int aHeight) MOZ_OVERRIDE;

  virtual bool DecodeVideoFrame(bool &aKeyframeSkip,
                                int64_t aTimeThreshold) MOZ_OVERRIDE;

  virtual void Shutdown() MOZ_OVERRIDE;

  SoftwareWebMVideoDecoder(WebMReader* aReader);
  ~SoftwareWebMVideoDecoder();

private:
  nsRefPtr<WebMReader> mReader;

  
  vpx_codec_ctx_t mVPX;
};

} 

#endif
