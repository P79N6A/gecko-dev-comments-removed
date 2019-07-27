




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
GonkDecoderModule::CreateVideoDecoder(const VideoInfo& aConfig,
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
GonkDecoderModule::CreateAudioDecoder(const AudioInfo& aConfig,
                                      FlushableMediaTaskQueue* aAudioTaskQueue,
                                      MediaDataDecoderCallback* aCallback)
{
  nsRefPtr<MediaDataDecoder> decoder =
  new GonkMediaDataDecoder(new GonkAudioDecoderManager(aAudioTaskQueue, aConfig),
                           aAudioTaskQueue, aCallback);
  return decoder.forget();
}

PlatformDecoderModule::ConversionRequired
GonkDecoderModule::DecoderNeedsConversion(const TrackInfo& aConfig) const
{
  if (aConfig.IsVideo()) {
    return kNeedAnnexB;
  } else {
    return kNeedNone;
  }
}

bool
GonkDecoderModule::SupportsMimeType(const nsACString& aMimeType)
{
  return aMimeType.EqualsLiteral("audio/mp4a-latm") ||
    aMimeType.EqualsLiteral("audio/3gpp") ||
    aMimeType.EqualsLiteral("video/mp4") ||
    aMimeType.EqualsLiteral("video/mp4v-es") ||
    aMimeType.EqualsLiteral("video/avc");
}
} 
