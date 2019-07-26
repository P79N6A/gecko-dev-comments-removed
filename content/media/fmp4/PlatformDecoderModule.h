





#if !defined(PlatformDecoderModule_h_)
#define PlatformDecoderModule_h_

#include "MediaDecoderReader.h"
#include "mozilla/layers/LayersTypes.h"

namespace mozilla {

namespace layers {
class ImageContainer;
}

class MediaDataDecoder;
typedef int64_t Microseconds;















class PlatformDecoderModule {
public:

  
  
  
  
  
  
  static PlatformDecoderModule* Create();

  
  
  
  
  
  virtual nsresult Shutdown() = 0;

  
  
  
  
  
  virtual MediaDataDecoder* CreateH264Decoder(layers::LayersBackend aLayersBackend,
                                              layers::ImageContainer* aImageContainer) = 0;

  
  
  
  
  
  
  virtual MediaDataDecoder* CreateAACDecoder(uint32_t aChannelCount,
                                             uint32_t aSampleRate,
                                             uint16_t aBitsPerSample,
                                             const uint8_t* aAACConfig,
                                             uint32_t aAACConfigLength) = 0;

  
  virtual void OnDecodeThreadStart() {}

  
  virtual void OnDecodeThreadFinish() {}

  virtual ~PlatformDecoderModule() {}
protected:
  PlatformDecoderModule() {}
};


enum DecoderStatus {
  DECODE_STATUS_NOT_ACCEPTING, 
  DECODE_STATUS_NEED_MORE_INPUT, 
  DECODE_STATUS_OK,
  DECODE_STATUS_ERROR
};









class MediaDataDecoder {
public:
  virtual ~MediaDataDecoder() {};

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual DecoderStatus Input(const uint8_t* aData,
                              uint32_t aLength,
                              Microseconds aDTS,
                              Microseconds aPTS,
                              int64_t aOffsetInStream) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual DecoderStatus Output(nsAutoPtr<MediaData>& aOutData) = 0;

  
  
  
  
  
  virtual DecoderStatus Flush() = 0;

  
  
  
  
  virtual nsresult Shutdown() = 0;

};

} 

#endif
