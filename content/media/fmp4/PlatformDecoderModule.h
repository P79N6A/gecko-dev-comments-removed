





#if !defined(PlatformDecoderModule_h_)
#define PlatformDecoderModule_h_

#include "MediaDecoderReader.h"
#include "mozilla/layers/LayersTypes.h"
#include "nsTArray.h"

namespace mp4_demuxer {
class VideoDecoderConfig;
class AudioDecoderConfig;
class MP4Sample;
}

namespace mozilla {

namespace layers {
class ImageContainer;
}

class MediaDataDecoder;
typedef int64_t Microseconds;















class PlatformDecoderModule {
public:
  
  
  static void Init();

  
  
  
  
  
  
  static PlatformDecoderModule* Create();

  
  
  
  
  
  virtual nsresult Shutdown() = 0;

  
  
  
  
  
  virtual MediaDataDecoder* CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                              layers::LayersBackend aLayersBackend,
                                              layers::ImageContainer* aImageContainer) = 0;

  
  
  
  
  
  
  virtual MediaDataDecoder* CreateAACDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig) = 0;

  
  virtual void OnDecodeThreadStart() {}

  
  virtual void OnDecodeThreadFinish() {}

  virtual ~PlatformDecoderModule() {}
protected:
  PlatformDecoderModule() {}
  
  static bool sUseBlankDecoder;
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

  
  
  
  
  
  
  virtual nsresult Init() = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual DecoderStatus Input(nsAutoPtr<mp4_demuxer::MP4Sample>& aSample) = 0;

  
  
  
  
  
  
  
  
  
  
  
  
  
  
  virtual DecoderStatus Output(nsAutoPtr<MediaData>& aOutData) = 0;

  
  
  
  
  
  virtual DecoderStatus Flush() = 0;

  
  
  
  
  virtual nsresult Shutdown() = 0;

};

} 

#endif
