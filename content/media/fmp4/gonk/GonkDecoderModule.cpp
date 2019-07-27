




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

nsresult
GonkDecoderModule::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");

  return NS_OK;
}

already_AddRefed<MediaDataDecoder>
GonkDecoderModule::CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                    mozilla::layers::LayersBackend aLayersBackend,
                                    mozilla::layers::ImageContainer* aImageContainer,
                                    MediaTaskQueue* aVideoTaskQueue,
                                    MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
  new GonkMediaDataDecoder(new GonkVideoDecoderManager(aImageContainer,aConfig),
                           aVideoTaskQueue, aCallback);
  return decoder.forget();
}

already_AddRefed<MediaDataDecoder>
GonkDecoderModule::CreateAACDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                   MediaTaskQueue* aAudioTaskQueue,
                                   MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
  new GonkMediaDataDecoder(new GonkAudioDecoderManager(aConfig), aAudioTaskQueue,
                           aCallback);
  return decoder.forget();
}

} 
