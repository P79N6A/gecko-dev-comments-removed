





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
  CreateVideoDecoder(const VideoInfo& aConfig,
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
  CreateAudioDecoder(const AudioInfo& aConfig,
                     FlushableMediaTaskQueue* aAudioTaskQueue,
                     MediaDataDecoderCallback* aCallback) override
  {
    nsRefPtr<MediaDataDecoder> decoder =
      new FFmpegAudioDecoder<V>(aAudioTaskQueue, aCallback, aConfig);
    return decoder.forget();
  }

  virtual bool SupportsMimeType(const nsACString& aMimeType) override
  {
    return FFmpegAudioDecoder<V>::GetCodecId(aMimeType) != AV_CODEC_ID_NONE ||
      FFmpegH264Decoder<V>::GetCodecId(aMimeType) != AV_CODEC_ID_NONE;
  }

  virtual ConversionRequired
  DecoderNeedsConversion(const TrackInfo& aConfig) const override
  {
    if (aConfig.IsVideo() &&
        (aConfig.mMimeType.EqualsLiteral("video/avc") ||
         aConfig.mMimeType.EqualsLiteral("video/mp4"))) {
      return PlatformDecoderModule::kNeedAVCC;
    } else {
      return kNeedNone;
    }
  }

};

} 

#endif 
