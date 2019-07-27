





#ifndef __FFmpegDecoderModule_h__
#define __FFmpegDecoderModule_h__

#include "PlatformDecoderModule.h"
#include "FFmpegAudioDecoder.h"
#include "FFmpegH264Decoder.h"

namespace mozilla
{

template <int V>
class FFmpegDecoderModule : public PlatformDecoderModule
{
public:
  static already_AddRefed<PlatformDecoderModule>
  Create()
  {
    nsRefPtr<PlatformDecoderModule> pdm = new FFmpegDecoderModule();
    return pdm.forget();
  }

  FFmpegDecoderModule() {}
  virtual ~FFmpegDecoderModule() {}

  virtual already_AddRefed<MediaDataDecoder>
  CreateVideoDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                     layers::LayersBackend aLayersBackend,
                     layers::ImageContainer* aImageContainer,
                     FlushableMediaTaskQueue* aVideoTaskQueue,
                     MediaDataDecoderCallback* aCallback) override
  {
    nsRefPtr<MediaDataDecoder> decoder =
      new FFmpegH264Decoder<V>(aVideoTaskQueue, aCallback, aConfig,
                               aImageContainer);
    return decoder.forget();
  }

  virtual already_AddRefed<MediaDataDecoder>
  CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override
  {
    nsRefPtr<MediaDataDecoder> decoder =
      new FFmpegAudioDecoder<V>(aAudioTaskQueue, aCallback, aConfig);
    return decoder.forget();
  }

  virtual bool SupportsAudioMimeType(const nsACString& aMimeType) override
  {
    return FFmpegAudioDecoder<V>::GetCodecId(aMimeType) != AV_CODEC_ID_NONE;
  }

  virtual bool SupportsVideoMimeType(const nsACString& aMimeType) override
  {
    return FFmpegH264Decoder<V>::GetCodecId(aMimeType) != AV_CODEC_ID_NONE;
  }

  virtual bool DecoderNeedsAVCC(const mp4_demuxer::VideoDecoderConfig& aConfig) override
  {
    return true;
  }

};

} 

#endif 
