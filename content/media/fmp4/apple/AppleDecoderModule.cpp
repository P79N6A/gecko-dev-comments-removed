





#include "AppleATDecoder.h"
#include "AppleCMLinker.h"
#include "AppleDecoderModule.h"
#include "AppleVTDecoder.h"
#include "AppleVTLinker.h"
#include "mozilla/Preferences.h"
#include "mozilla/DebugOnly.h"

namespace mozilla {

bool AppleDecoderModule::sIsEnabled = false;

AppleDecoderModule::AppleDecoderModule()
{
}

AppleDecoderModule::~AppleDecoderModule()
{
}


void
AppleDecoderModule::Init()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  sIsEnabled = Preferences::GetBool("media.apple.mp4.enabled", false);
  if (!sIsEnabled) {
    return;
  }

  
  sIsEnabled = AppleCMLinker::Link();
  if (!sIsEnabled) {
    return;
  }

  
  sIsEnabled = AppleVTLinker::Link();
}

nsresult
AppleDecoderModule::Startup()
{
  
  
  
  if (!sIsEnabled) {
    return NS_ERROR_FAILURE;
  }
  return NS_OK;
}

nsresult
AppleDecoderModule::Shutdown()
{
  MOZ_ASSERT(NS_IsMainThread(), "Must be on main thread.");
  AppleVTLinker::Unlink();
  AppleCMLinker::Unlink();

  return NS_OK;
}

MediaDataDecoder*
AppleDecoderModule::CreateH264Decoder(const mp4_demuxer::VideoDecoderConfig& aConfig,
                                      layers::LayersBackend aLayersBackend,
                                      layers::ImageContainer* aImageContainer,
                                      MediaTaskQueue* aVideoTaskQueue,
                                      MediaDataDecoderCallback* aCallback)
{
  return new AppleVTDecoder(aConfig, aVideoTaskQueue, aCallback, aImageContainer);
}

MediaDataDecoder*
AppleDecoderModule::CreateAACDecoder(const mp4_demuxer::AudioDecoderConfig& aConfig,
                                     MediaTaskQueue* aAudioTaskQueue,
                                     MediaDataDecoderCallback* aCallback)
{
  return new AppleATDecoder(aConfig, aAudioTaskQueue, aCallback);
}

} 
