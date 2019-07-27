




#include "GonkDecoderModule.h"
#include "GonkVideoDecoderManager.h"
#include "GonkAudioDecoderManager.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"
#include "GonkMediaDataDecoder.h"

namespace mozilla {
GonkDecoderModule::GonkDecoderModule()
{
}

GonkDecoderModule::~GonkDecoderModule()
{
}


void
GonkDecoderModule::Init()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
}

already_AddRefed<MediaDataDecoder>
GonkDecoderModule::CreateVideoDecoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                     mozilla::layers::LayersBackend aLayersBackend,
                                     mozilla::layers::ImageContainer* aImageContainer,
                                     FlushableMediaTaskQueue* aVideoTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
  new GonkMediaDataDecoder(new GonkVideoDecoderManager(aVideoTaskQueue,
                                                       aImageContainer, aConfig),
                           aVideoTaskQueue, aCallback);
  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
GonkDecoderModule::CreateAudioDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                      FlushableMediaTaskQueue* aAudioTaskQueue,
                                      MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
  new GonkMediaDataDecoder(new GonkAudioDecoderManager(aAudioTaskQueue, aConfig),
                           aAudioTaskQueue, aCallback);
  return decoder.forget();
}

PlatformDecoderModule::ConversionRequired
GonkDecoderModule::DecoderNeedsConversion(const mp4_demuxer::TrackConfig& aConfig) const
{
  if (aConfig.IsVideoConfig()) {
    return kNeedAVCC;
  } else {
    return kNeedNone;
  }
}

} 
